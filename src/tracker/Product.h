#pragma once
#include <string>
#include <ctime>
#include <chrono>

struct Product
{
	int id = -1;
    std::string name;
    std::string url;
    int checkIntervalSeconds = 30;
    int enabled = 1;

    bool hasKnownState = false;
    bool inStock = false;
    std::time_t lastChecked = 0;
    std::string lastStatus = "unknown";
    std::string lastError;
    long lastHttpStatus = 0;
    std::chrono::steady_clock::time_point nextCheckAt = std::chrono::steady_clock::now();
};
