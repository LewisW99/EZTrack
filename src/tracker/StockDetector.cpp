#include "StockDetector.h"
#include "../http/HttpClient.h"

namespace
{
    bool Contains(const std::string& body, const std::string& text)
    {
        return body.find(text) != std::string::npos;
    }
}

DetectionResult StockDetector::Detect(const std::string& url, const HttpResponse& response)
{

    if (response.statusCode != 200)
    {
    return { StockState::Error, "error", "HTTP status: " + std::to_string(response.statusCode) };
    }

    const std::string& body = response.body;

    if (body.empty())
        return { StockState::Error, "error", "Empty response body" };

    /*
        Detect anti-bot / challenge pages first
    */

    if (Contains(body, "Pardon Our Interruption") ||
        Contains(body, "made us think you were a bot") ||
        Contains(body, "Access denied") ||
        Contains(body, "cf-chl") ||
        Contains(body, "captcha") ||
        Contains(body, "__cf_bm"))
    {
        return { StockState::Blocked, "blocked", "Anti-bot protection detected" };
    }

    /*
        Pokémon Center specific detection
    */

    if (url.find("pokemoncenter.com") != std::string::npos)
    {
        if (Contains(body, "Out of stock") ||
            Contains(body, "Sold out") ||
            Contains(body, "Currently unavailable"))
        {
            return { StockState::OutOfStock, "out_of_stock", "Pokémon Center unavailable text" };
        }

        if (Contains(body, "Add to cart") ||
            Contains(body, "Add to Cart") ||
            Contains(body, "Add to bag") ||
            Contains(body, "Add to Basket"))
        {
            return { StockState::InStock, "in_stock", "Pokémon Center purchase button detected" };
        }

        return { StockState::Unknown, "unknown", "Pokémon Center state unclear" };
    }

    /*
        Generic fallback detection
    */

    if (Contains(body, "Out of stock") ||
        Contains(body, "Sold out") ||
        Contains(body, "Currently unavailable"))
    {
        return { StockState::OutOfStock, "out_of_stock", "Generic unavailable text" };
    }

    if (Contains(body, "Add to cart") ||
        Contains(body, "Add to Cart") ||
        Contains(body, "Add to bag") ||
        Contains(body, "Add to Basket"))
    {
        return { StockState::InStock, "in_stock", "Generic purchase button detected" };
    }

    return { StockState::Unknown, "unknown", "Could not determine stock state" };
}