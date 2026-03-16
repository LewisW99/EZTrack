#pragma once
#include <thread>
#include <atomic>
class HttpServer
{
public:
	HttpServer(int port = 8080);
	~HttpServer();
	
	void Start();
	void Stop();
	
private:
	void Run();
	
	int port;
	std::thread serverThread;
	std::atomic<bool> running{false};
};
