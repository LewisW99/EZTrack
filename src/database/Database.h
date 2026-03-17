#pragma once
#include "../tracker/Product.h"
#include <string>
#include <sqlite3.h>
#include <mutex>
#include <vector>

struct ProductCheck
{
    std::string timestamp;
    std::string status;
    long httpStatus;
    std::string reason;
};

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
    
    int GetOrCreateProduct(const std::string& name, const std::string& url);
    bool DeleteProduct(int id);
    bool UpdateProduct(int id, const std::string& name, const std::string& url);
    bool SetProductEnabled(int id, bool enabled);

    std::vector<ProductCheck> GetProductHistory(int productId);
    std::vector<ProductCheck> GetLatestChecks(int limit = 50);

private:

    sqlite3* db = nullptr;

    bool CreateTables();
    bool CreateIndexes();

    

    std::mutex dbMutex;
};
