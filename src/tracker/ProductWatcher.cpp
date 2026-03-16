#include "ProductWatcher.h"
#include "Product.h"
#include "StockDetector.h"
#include "../database/Database.h"
#include "../core/WorkerPool.h"

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

        for (char c : text)
        {
            if (isalnum((unsigned char)c))
                result += std::tolower(c);
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
        configFile >> data;

        products.clear();

        for (auto& item : data["products"])
        {
            Product product;

            product.name = item["name"];
            product.url = item["url"];
            product.checkIntervalSeconds = item.value("checkInterval", 30);

            product.nextCheckAt = std::chrono::steady_clock::now();

            products.push_back(product);
        }

        return !products.empty();
    }

    void DumpDebugBody(const Product& product, const HttpResponse& response)
    {
        std::ofstream file(g_debugPath / (Slugify(product.name) + "-last.html"));

        if (!file.is_open())
            return;

        file << response.body;
    }

    void WriteStatus(const std::vector<Product>& products)
    {
        json status;

        status["updatedAt"] = IsoTime(std::time(nullptr));
        status["products"] = json::array();

        auto now = std::chrono::steady_clock::now();

        for (const auto& product : products)
        {
            auto remaining =
                std::chrono::duration_cast<std::chrono::seconds>(product.nextCheckAt - now).count();

            status["products"].push_back({
                {"name", product.name},
                {"url", product.url},
                {"status", product.lastStatus},
                {"lastChecked", IsoTime(product.lastChecked)},
                {"nextCheckInSeconds", std::max<long long>(0, remaining)}
            });
        }

        std::ofstream file(g_statusPath / "status.json");
        file << status.dump(4);
    }

    DetectionResult CheckProduct(Product& product, Logger& logger, Database& db)
    {
        logger.Info("Checking '" + product.name + "'");

        HttpResponse response = HttpClient::Get(product.url);

        product.lastChecked = std::time(nullptr);

        DumpDebugBody(product, response);

        DetectionResult detection = StockDetector::Detect(product.url, response);

        product.lastStatus = detection.statusText;

        logger.Info("HTTP " + std::to_string(response.statusCode) +
            ", body bytes=" + std::to_string(response.body.size()));

        logger.Info("Detection: " + detection.statusText + " (" + detection.reason + ")");

        if (detection.state == StockState::InStock)
        {
            bool wasKnown = product.hasKnownState;
            bool previousState = product.inStock;

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

        db.RecordCheck(
            product.name,
            product.url,
            detection.statusText,
            response.statusCode,
            detection.reason
        );

        return detection;
    }
}

void ProductWatcher::Start()
{
    std::signal(SIGINT, OnSignal);

    std::filesystem::create_directories(g_logsPath);
    std::filesystem::create_directories(g_statusPath);
    std::filesystem::create_directories(g_debugPath);

    Logger logger((g_logsPath / "tracker.log").string());

    logger.Info("Tracker starting");

    Database db("tracker.db");

    if (!db.Init())
    {
        logger.Error("Failed to initialise SQLite database");
    }

    WorkerPool pool(4);

    std::vector<Product> products;

    if (!LoadProducts("config/products.json", products, logger))
        return;

    logger.Info("Loaded " + std::to_string(products.size()) + " products");

    WriteStatus(products);

    while (!g_shouldStop)
    {
        auto now = std::chrono::steady_clock::now();
        bool scheduledAnything = false;

        for (auto& product : products)
        {
            if (now < product.nextCheckAt)
                continue;

            scheduledAnything = true;

            pool.Enqueue([&product, &logger, &db]()
            {
                DetectionResult detection = CheckProduct(product, logger, db);

                auto nowTime = std::chrono::steady_clock::now();

                if (detection.state == StockState::Blocked)
                {
                    logger.Warn("Site blocked request for '" + product.name + "' - backing off for 10 minutes");

                    product.nextCheckAt = nowTime + std::chrono::minutes(10);
                }
                else
                {
                    product.nextCheckAt =
                        nowTime + std::chrono::seconds(product.checkIntervalSeconds);
                }
            });
        }

        if (scheduledAnything)
            WriteStatus(products);

        auto nextWake = std::chrono::steady_clock::time_point::max();

        for (const auto& product : products)
        {
            if (product.nextCheckAt < nextWake)
                nextWake = product.nextCheckAt;
        }

        auto nowTime = std::chrono::steady_clock::now();

        if (nextWake > nowTime)
            std::this_thread::sleep_for(nextWake - nowTime);
    }

    logger.Info("Tracker stopping cleanly");

    WriteStatus(products);
}