#include "BaseAdapter.h"

class PokemonCenterAdapter : public BaseAdapter
{
public:
    DetectionResult Detect(const HttpResponse& response) override
    {
        const std::string& body = response.body;

        if (body.find("Pardon Our Interruption") != std::string::npos ||
            body.find("made us think you were a bot") != std::string::npos)
        {
            return { StockState::Blocked, "blocked", "Imperva bot protection page" };
        }

        if (body.find("Out of stock") != std::string::npos ||
            body.find("Sold out") != std::string::npos)
        {
            return { StockState::OutOfStock, "out_of_stock", "Product unavailable" };
        }

        if (body.find("Add to cart") != std::string::npos ||
            body.find("Add to Cart") != std::string::npos ||
            body.find("Add to bag") != std::string::npos)
        {
            return { StockState::InStock, "in_stock", "Purchase button detected" };
        }

        return { StockState::Unknown, "unknown", "Could not determine stock state" };
    }
};