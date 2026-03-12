#include "ProductWatcher.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>

#include"../http/HttpClient.h"
#include "../notifications/NotificationManager.h"

void ProductWatcher::Start()
{
    std::cout << "Tracker running..." << std::endl;

    bool lastStockState = false;

    while (true)
    {
        auto now = std::time(nullptr);

        std::cout << std::ctime(&now) << "Checking product stock..." << std::endl;

        bool inStock = false;

        std::string html = HttpClient::Get("https://www.pokemoncenter.com/en-gb/product/10-10315-108/pokemon-tcg-mega-evolution-ascended-heroes-pokemon-center-elite-trainer-box");
        std::cout << "HTML size: " << html.size() << std::endl;
        /*
        Simple detection logic.

        Pokemon Center typically shows:
        "Out of stock"
        when unavailable.
        */

        if(html.find("Out of stock") == std::string::npos)
        {
            inStock = true;
        }
        /*
        TODO (NEXT STEP):
        Replace this with real HTTP request + HTML parsing.
        */

        if (inStock && !lastStockState)
        {
            NotificationManager::SendStockAlert("Test Alert");
        }

        lastStockState = inStock;

        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}