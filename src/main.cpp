#include <iostream>
#include "tracker/ProductWatcher.h"
#include "api/HttpServer.h"

int main()
{
    std::cout << "Stock Tracker Starting..." << std::endl;

	HttpServer server(8080);
    server.Start();
    
    ProductWatcher watcher;

    watcher.Start();
    
    

    return 0;
}
