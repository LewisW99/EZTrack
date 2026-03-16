#pragma once

#include <string>

struct HttpResponse;

enum class StockState
{
    InStock,
    OutOfStock,
    Blocked,
    Unknown,
    Error
};

struct DetectionResult
{
    StockState state = StockState::Unknown;
    std::string statusText = "unknown";
    std::string reason;
};

class StockDetector
{
public:
    static DetectionResult Detect(const std::string& url, const HttpResponse& response);

private:
    static DetectionResult DetectPokemonCenter(const HttpResponse& response);
    static DetectionResult DetectGeneric(const HttpResponse& response);
};