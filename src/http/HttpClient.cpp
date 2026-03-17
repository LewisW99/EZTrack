#include "HttpClient.h"
#include <cstring>
#include <cstdio>
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
        "--compressed" 
        "-H \"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\" "
        "-H \"Accept-Language: en-GB,en-US;q=0.9,en;q=0.8\" "
        "-H \"Cache-Control: no-cache\" "
        "-H \"Pragma: no-cache\" "
        "-H \"Connection: keep-alive\" "
        "-H \"Upgrade-Insecure-Requests: 1\" "
        "-H \"Sec-Fetch-Dest: document\" "
        "-H \"Sec-Fetch-Mode: navigate\" "
        "-H \"Sec-Fetch-Site: none\" "
        "-H \"Sec-Fetch-User: ?1\" "
        "-A \"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36\" "
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
        response.errorMessage = "Failed to start curl";
        return response;
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        output += buffer.data();

#if defined(_WIN32)
    int exitCode = _pclose(pipe);
#else
    int exitCode = pclose(pipe);
#endif

    const size_t markerPos = output.rfind(kStatusMarker);

    if (markerPos != std::string::npos)
    {
        const size_t statusStart = markerPos + std::strlen(kStatusMarker);

        try
        {
            response.statusCode = std::stol(output.substr(statusStart));
        }
        catch (...)
        {
            response.statusCode = 0;
        }

        output.erase(markerPos);
    }

    response.body = std::move(output);

    response.success =
        (exitCode == 0 &&
         response.statusCode >= 200 &&
         response.statusCode < 400 &&
         !response.body.empty());

    if (!response.success)
    {
        if (exitCode != 0)
            response.errorMessage = "curl exited with code " + std::to_string(exitCode);
        else if (response.statusCode == 0)
            response.errorMessage = "No HTTP status returned";
        else if (response.body.empty())
            response.errorMessage = "Empty HTTP body";
        else
            response.errorMessage = "HTTP " + std::to_string(response.statusCode);
    }

    return response;
}