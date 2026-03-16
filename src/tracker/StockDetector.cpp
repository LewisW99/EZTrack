#include "StockDetector.h"

#include "../http/HttpClient.h"

namespace
{
    bool Contains(const std::string& haystack, const std::string& needle)
    {
        return haystack.find(needle) != std::string::npos;
    }
}

DetectionResult StockDetector::Detect(const std::string& url, const HttpResponse& response)
{
    if (!response.success)
        return { StockState::Error, "error", response.errorMessage };

    if (Contains(url, "pokemoncenter.com"))
        return DetectPokemonCenter(response);

    return DetectGeneric(response);
}

DetectionResult StockDetector::DetectPokemonCenter(const HttpResponse& response)
{
    const std::string& body = response.body;

    if (body.empty())
        return { StockState::Error, "error", "Empty body" };

    if (Contains(body, "cf-chl") ||
        Contains(body, "Attention Required") ||
        Contains(body, "captcha") ||
        Contains(body, "Access denied") ||
        Contains(body, "__cf_bm"))
    {
        return { StockState::Blocked, "blocked", "Likely Cloudflare/challenge page" };
    }

    if (body.size() < 3000)
        return { StockState::Unknown, "unknown", "Body too small to confidently classify as product page" };

    if (Contains(body, "Out of stock") ||
        Contains(body, "Sold out") ||
        Contains(body, "Currently unavailable"))
    {
        return { StockState::OutOfStock, "out_of_stock", "Matched Pokémon Center unavailable text" };
    }

    if (Contains(body, "Add to cart") ||
        Contains(body, "Add to Cart") ||
        Contains(body, "Add to bag") ||
        Contains(body, "Add to Basket"))
    {
        return { StockState::InStock, "in_stock", "Matched Pokémon Center purchase button text" };
    }

    if (!Contains(body, "Pokemon") && !Contains(body, "Pokémon"))
        return { StockState::Unknown, "unknown", "Response does not strongly resemble a product page" };

    return { StockState::Unknown, "unknown", "Could not confidently determine stock state" };
}

DetectionResult StockDetector::DetectGeneric(const HttpResponse& response)
{
    const std::string& body = response.body;

    if (body.empty())
        return { StockState::Error, "error", "Empty body" };

    if (Contains(body, "captcha") ||
        Contains(body, "Attention Required") ||
        Contains(body, "Access denied") ||
        Contains(body, "cf-chl"))
    {
        return { StockState::Blocked, "blocked", "Likely anti-bot/challenge page" };
    }

    if (Contains(body, "Out of stock") ||
        Contains(body, "Sold out") ||
        Contains(body, "Currently unavailable"))
    {
        return { StockState::OutOfStock, "out_of_stock", "Matched generic unavailable text" };
    }

    if (Contains(body, "Add to cart") ||
        Contains(body, "Add to Cart") ||
        Contains(body, "Add to bag") ||
        Contains(body, "Add to Basket"))
    {
        return { StockState::InStock, "in_stock", "Matched generic purchase button text" };
    }

    return { StockState::Unknown, "unknown", "Could not confidently determine stock state" };
}