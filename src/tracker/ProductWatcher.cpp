#include "ProductWatcher.h"
#include "Product.h"

#include <filesystem>
#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <fstream>
#include <vector>
#include "../http/HttpClient.h"
#include "../notifications/NotificationManager.h"

#include "../../external/json.hpp"

using json = nlohmann::json;


   void WriteStatus(const std::vector<Product>& products, const std::vector<bool>& states)
{
    using json = nlohmann::json;

    json status;
    status["products"] = json::array();

    auto now = std::time(nullptr);

    for (size_t i = 0; i < products.size(); i++)
    {
        json entry;

        entry["name"] = products[i].name;
        entry["inStock"] = states[i];
        entry["lastChecked"] = now;

        status["products"].push_back(entry);
    }

    std::ofstream file("status/status.json");
    file << status.dump(4);
}

void ProductWatcher::Start()
{

    std::filesystem::create_directories("status");
    std::filesystem::create_directories("logs");
    
    std::ofstream logFile("logs/tracker.log", std::ios::app);

    logFile  << "Tracker running..." << std::endl;

    
    /*
    Load product configuration
    */

    std::ifstream configFile("config/products.json");

    if (!configFile.is_open())
    {
        logFile << "Failed to open products.json" << std::endl;
        return;
    }

    json data;
    configFile >> data;

    std::vector<Product> products;

    for (auto& item : data["products"])
    {
        Product p;
        p.name = item["name"];
        p.url = item["url"];
        p.interval = item["checkInterval"];

        products.push_back(p);
    }

    logFile << "Loaded " << products.size() << " products" << std::endl;

    /*
    Track stock state per product
    */

    std::vector<bool> lastStockStates(products.size(), false);

    while (true)
    {
        auto now = std::time(nullptr);

        logFile << std::ctime(&now) << "Checking product stock..." << std::endl;

        for (size_t i = 0; i < products.size(); i++)
        {
            auto& product = products[i];

            logFile << "Checking: " << product.name << std::endl;

            bool inStock = false;

            std::string html = HttpClient::Get(product.url);

            logFile << "HTML size: " << html.size() << std::endl;

            /*
            Pokemon Center typically shows:
            "Out of stock"
            when unavailable.
            */

            if (html.find("Out of stock") == std::string::npos)
            {
                inStock = true;
            }

            /*
            Alert if stock changed
            */

            if (inStock && !lastStockStates[i])
            {
                NotificationManager::SendStockAlert(product.name);
            }

            lastStockStates[i] = inStock;
        }

        WriteStatus(products, lastStockStates);

        /*
        Wait before next check
        */

        std::this_thread::sleep_for(std::chrono::seconds(30));
    }


 
}
