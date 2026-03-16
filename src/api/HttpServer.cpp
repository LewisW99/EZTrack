#include "HttpServer.h"
#include "ApiRoutes.h"
#include "httplib.h"
#include <sstream>
#include <fstream>
#include <iostream>


HttpServer::HttpServer(int p) : port(p) {}

HttpServer::~HttpServer()
{
	Stop();
}

void HttpServer::Start()
{
	running = true;
	serverThread = std::thread(&HttpServer::Run, this);
}

void HttpServer::Stop()
{
	running = false;
	if(serverThread.joinable())
		serverThread.join();
}

void HttpServer::Run()
{
	httplib::Server server;
	
	RegisterApiRoutes(server);
	std::cout << "HTTP server listening on port" << port << std::endl;
	
	server.listen("0.0.0.0", port);
}
