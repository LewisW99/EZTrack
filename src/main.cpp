#include <iostream>
#include "tracker/ProductWatcher.h"

int main()
{
    std::cout << "Stock Tracker Starting..." << std::endl;

    ProductWatcher watcher;

    watcher.Start();

    return 0;
}