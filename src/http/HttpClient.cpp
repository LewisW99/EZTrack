#include "HttpClient.h"
#include <cstdio>
#include <iostream>
#include <array>

std::string HttpClient::Get(const std::string& url)
{
    std::string command = "curl -L -s \"" + url + "\"";

    std::array<char, 4096> buffer;
    std::string result;

    FILE* pipe = _popen(command.c_str(), "r");

    if (!pipe)
    {
        std::cerr << "Failed to run curl command\n";
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        result += buffer.data();
    }

    _pclose(pipe);

    return result;
}