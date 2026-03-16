#pragma once

#include "../../http/HttpClient.h"
#include "../StockDetector.h"

class BaseAdapter
{
public:
    virtual ~BaseAdapter() = default;

    virtual DetectionResult Detect(const HttpResponse& response) = 0;
};