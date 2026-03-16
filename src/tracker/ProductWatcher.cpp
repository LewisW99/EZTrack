#include "ProductWatcher.h"
#include "Product.h"
#include "StockDetector.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <vector>

#include "../core/Logger.h"
#include "../http/HttpClient.h"
#include "../notifications/NotificationManager.h"
#include "../../external/json.hpp"

using json = nlohmann::json;

namespace
{
    std::atomic<bool> g_shouldStop = false;
    std::filesystem::path g_logsPath = "logs";
    std::filesystem::path g_statusPath = "status";
    std::filesystem::path g_debugPath = "debug";

    void OnSignal(int)
    {
        g_shouldStop = true;
    }

    std::string IsoTime(const std::time_t value)
    {
        if (value == 0)
            return "";

        std::tm localTime{};
#if defined(_WIN32)
        localtime_s(&localTime, &value);
#else
        localtime_r(&value, &localTime);
#endif

        std::ostringstream stream;
        stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
        return stream.str();
    }

    std::string Slugify(const std::string& text)
    {
        std::string result;
        result.reserve(text.size());

        for (char c : text)
        {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            else if (c == ' ' || c == '-' || c == '_')
                result += '-';
        }

        if (result.empty())
            result = "product";

        return result;
    }

    bool LoadProducts(const std::string& path, std::vector<Product>& products, Logger& logger)
    {
        std::ifstream configFile(path);
        if (!configFile.is_open())
        {
            logger.Error("Failed to open " + path);
            return false;
        }

        json data;
        try
        {
            configFile >> data;
        }
        catch (const std::exception& ex)
        {
            logger.Error(std::string("Invalid JSON in ") + path + ": " + ex.what());
            return false;
        }

        if (!data.contains("products") || !data["products"].is_array())
        {
            logger.Error("config/products.json must contain a 'products' array");
            return false;
        }

        products.clear();
        products.reserve(data["products"].size());

        for (const auto& item : data["products"])
        {
            if (!item.contains("name") || !item["name"].is_string() ||
                !item.contains("url") || !item["url"].is_string())
            {
                logger.Warn("Skipping product with missing name/url");
                continue;
            }

            Product product;
            product.name = item["name"].get<std::string>();
            product.url = item["url"].get<std::string>();
            product.checkIntervalSeconds = item.value("checkInterval", 30);

            if (product.checkIntervalSeconds < 5)
            {
                logger.Warn("Product '" + product.name + "' had checkInterval < 5, clamping to 5 seconds");
                product.checkIntervalSeconds = 5;
            }

            product.nextCheckAt = std::chrono::steady_clock::now();
            products.push_back(std::move(product));
        }

        if (products.empty())
        {
            logger.Error("No valid products were loaded from config/products.json");
            return false;
        }

        return true;
    }

    void DumpDebugBody(const Product& product, const HttpResponse& response)
    {
        const std::filesystem::path filePath = g_debugPath / (Slugify(product.name) + "-last.html");
        std::ofstream file(filePath, std::ios::trunc);
        if (!file.is_open())
            return;

        file << "<!-- URL: " << product.url << " -->\n";
        file << "<!-- HTTP: " << response.statusCode << " -->\n";
        file << "<!-- success: " << (response.success ? "true" : "false") << " -->\n";
        if (!response.errorMessage.empty())
            file << "<!-- error: " << response.errorMessage << " -->\n";
        file << response.body;
    }

    void WriteStatus(const std::vector<Product>& products)
    {
        json status;
        status["updatedAt"] = IsoTime(std::time(nullptr));
        status["products"] = json::array();

        const auto now = std::chrono::steady_clock::now();

        for (const auto& product : products)
        {
            const auto remaining = std::chrono::duration_cast<std::chrono::seconds>(product.nextCheckAt - now).count();

            status["products"].push_back(
                {
                    {"name", product.name},
                    {"url", product.url},
                    {"checkInterval", product.checkIntervalSeconds},
                    {"hasKnownState", product.hasKnownState},
                    {"inStock", product.inStock},
                    {"status", product.lastStatus},
                    {"lastChecked", IsoTime(product.lastChecked)},
                    {"lastHttpStatus", product.lastHttpStatus},
                    {"lastError", product.lastError},
                    {"nextCheckInSeconds", std::max<long long>(0, remaining)}
                });
        }

        std::ofstream file(g_statusPath / "status.json");
        file << status.dump(4);
    }

    void CheckProduct(Product& product, Logger& logger)
    {
        logger.Info("Checking '" + product.name + "'");

        const HttpResponse response = HttpClient::Get(product.url);

        product.lastChecked = std::time(nullptr);
        product.lastHttpStatus = response.statusCode;
        product.lastError.clear();

        DumpDebugBody(product, response);

        const DetectionResult detection = StockDetector::Detect(product.url, response);

        product.lastStatus = detection.statusText;
        product.lastError = detection.reason;

        logger.Info("HTTP " + std::to_string(response.statusCode) + ", body bytes=" + std::to_string(response.body.size()));
        logger.Info("Detection: " + detection.statusText + " (" + detection.reason + ")");

        if (detection.state == StockState::InStock)
        {
            const bool wasKnown = product.hasKnownState;
            const bool previousState = product.inStock;

            product.inStock = true;
            product.hasKnownState = true;

            if (wasKnown && !previousState)
                NotificationManager::SendStockAlert(product.name);
        }
        else if (detection.state == StockState::OutOfStock)
        {
            product.inStock = false;
            product.hasKnownState = true;
        }
    }
}

void ProductWatcher::Start()
{
    std::signal(SIGINT, OnSignal);
#if defined(SIGTERM)
    std::signal(SIGTERM, OnSignal);
#endif

    std::filesystem::create_directories(g_logsPath);
    std::filesystem::create_directories(g_statusPath);
    std::filesystem::create_directories(g_debugPath);

    Logger logger((g_logsPath / "tracker.log").string());
    logger.Info("Tracker starting");

    std::vector<Product> products;
    if (!LoadProducts("config/products.json", products, logger))
        return;

    logger.Info("Loaded " + std::to_string(products.size()) + " products");
    WriteStatus(products);

    while (!g_shouldStop)
    {
        const auto now = std::chrono::steady_clock::now();
        bool checkedAnything = false;

        for (auto& product : products)
        {
            if (now < product.nextCheckAt)
                continue;

            CheckProduct(product, logger);
            product.nextCheckAt = std::chrono::steady_clock::now() + std::chrono::seconds(product.checkIntervalSeconds);
            checkedAnything = true;
        }

        if (checkedAnything)
            WriteStatus(products);

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    logger.Info("Tracker stopping cleanly");
    WriteStatus(products);
}