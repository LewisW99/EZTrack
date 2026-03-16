#pragma once
#include "../tracker/Product.h"
#include <string>
#include <sqlite3.h>
#include <mutex>
#include <vector>
class Database
{
public:

    Database(const std::string& path);
    ~Database();

    bool Init();

    void RecordCheck(
        const std::string& productName,
        const std::string& url,
        const std::string& status,
        long httpStatus,
        const std::string& reason
    );
    
    std::vector<Product> GetProducts();

private:

    sqlite3* db = nullptr;

    bool CreateTables();
    bool CreateIndexes();

    int GetOrCreateProduct(
        const std::string& name,
        const std::string& url
    );

    std::mutex dbMutex;
};
