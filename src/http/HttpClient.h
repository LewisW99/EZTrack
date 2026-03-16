#pragma once
#include <string>


struct HttpResponse
{
    bool success = false;
    long statusCode = 0;
    std::string body;
    std::string errorMessage;
};

class HttpClient
{
public:
    static HttpResponse Get(const std::string& url);
};