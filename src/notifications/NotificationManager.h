#pragma once
#include <string>

class NotificationManager
{
public:
    static void SendStockAlert(const std::string& productName);
};