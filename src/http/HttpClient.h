#pragma once
#include <string>

class HttpClient
{
public:
    static std::string Get(const std::string& url);
};