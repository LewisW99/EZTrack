#include "BaseAdapter.h"

class GenericAdapter : public BaseAdapter
{
public:
    DetectionResult Detect(const HttpResponse& response) override
    {
        const std::string& body = response.body;

        if (body.find("Out of stock") != std::string::npos ||
            body.find("Sold out") != std::string::npos)
        {
            return { StockState::OutOfStock, "out_of_stock", "Matched generic stock text" };
        }

        if (body.find("Add to cart") != std::string::npos)
        {
            return { StockState::InStock, "in_stock", "Matched generic purchase text" };
        }

        return { StockState::Unknown, "unknown", "Generic detection failed" };
    }
};