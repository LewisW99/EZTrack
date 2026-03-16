#include "HttpClient.h"
#include <cstdio>
#include <iostream>
#include <array>

namespace
{
    constexpr const char* kStatusMarker = "EZTRACK_HTTP_STATUS:";
}


HttpResponse HttpClient::Get(const std::string& url)
{
    HttpResponse response;

    const std::string command =
        "curl -L -s --max-time 20 --connect-timeout 10 "
        "-A \"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36\" "
        "-w \"\\n" + std::string(kStatusMarker) + "%{http_code}\" \"" + url + "\"";

    std::array<char, 4096> buffer{};
    std::string output;

#if defined(_WIN32)
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif

    if (!pipe)
    {
        response.errorMessage = "Failed to start curl process";
        return response;
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
        output += buffer.data();

#if defined(_WIN32)
    const int exitCode = _pclose(pipe);
#else
    const int exitCode = pclose(pipe);
#endif

    const std::size_t markerPos = output.rfind(kStatusMarker);
    if (markerPos != std::string::npos)
    {
        const std::size_t statusStart = markerPos + std::string(kStatusMarker).size();
        const std::string statusText = output.substr(statusStart);

        try
        {
            response.statusCode = std::stol(statusText);
        }
        catch (...)
        {
            response.statusCode = 0;
        }

        output.erase(markerPos);

        while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
            output.pop_back();
    }

    response.body = std::move(output);
    response.success = (exitCode == 0 && response.statusCode >= 200 && response.statusCode < 400 && !response.body.empty());

    if (!response.success)
    {
        if (exitCode != 0)
            response.errorMessage = "curl exited with code " + std::to_string(exitCode);
        else if (response.statusCode == 0)
            response.errorMessage = "No HTTP status returned";
        else if (response.body.empty())
            response.errorMessage = "Empty HTTP body";
        else
            response.errorMessage = "HTTP status " + std::to_string(response.statusCode);
    }

    return response;
}
