#include "NotificationManager.h"
#include <iostream>

void NotificationManager::SendStockAlert(const std::string& productName)
{
    /*
    CURRENT BEHAVIOUR
    -----------------
    Just prints a console message.

    LATER (after iOS app exists):
    -----------------------------
    Replace this with:
    - Firebase Cloud Messaging
    OR
    - Apple Push Notification Service
    */

    std::cout << "[ALERT] " << productName << " is IN STOCK!" << std::endl;
}