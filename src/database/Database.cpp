#include "Database.h"
#include <iostream>

Database::Database(const std::string& path)
{
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK)
    {
        std::cerr << "SQLite failed to open database\n";
        db = nullptr;
    }
}

Database::~Database()
{
    if (db)
        sqlite3_close(db);
}

bool Database::Init()
{
    if (!db)
        return false;

    if (!CreateTables())
        return false;

    if (!CreateIndexes())
        return false;

    return true;
}

bool Database::CreateTables()
{
    const char* createProducts =
        "CREATE TABLE IF NOT EXISTS products ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT UNIQUE,"
        "url TEXT,"
        "enabled INTEGER DEFAULT 1"
        ");";

    const char* createChecks =
        "CREATE TABLE IF NOT EXISTS checks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "product_id INTEGER,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "status TEXT,"
        "http_status INTEGER,"
        "reason TEXT,"
        "FOREIGN KEY(product_id) REFERENCES products(id)"
        ");";

    char* err = nullptr;

    if (sqlite3_exec(db, createProducts, nullptr, nullptr, &err) != SQLITE_OK)
    {
        sqlite3_free(err);
        return false;
    }

    if (sqlite3_exec(db, createChecks, nullptr, nullptr, &err) != SQLITE_OK)
    {
        sqlite3_free(err);
        return false;
    }

    return true;
}

bool Database::CreateIndexes()
{
    const char* indexChecks =
        "CREATE INDEX IF NOT EXISTS idx_checks_product_time "
        "ON checks(product_id, timestamp);";

    char* err = nullptr;

    if (sqlite3_exec(db, indexChecks, nullptr, nullptr, &err) != SQLITE_OK)
    {
        sqlite3_free(err);
        return false;
    }

    return true;
}

int Database::GetOrCreateProduct(const std::string& name, const std::string& url)
{
    sqlite3_stmt* stmt;

    const char* insert =
        "INSERT OR IGNORE INTO products (name, url) VALUES (?, ?);";

    sqlite3_prepare_v2(db, insert, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, url.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    const char* query =
        "SELECT id FROM products WHERE name=?;";

    sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    int id = -1;

    if (sqlite3_step(stmt) == SQLITE_ROW)
        id = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);

    return id;
}

void Database::RecordCheck(
    const std::string& productName,
    const std::string& url,
    const std::string& status,
    long httpStatus,
    const std::string& reason)
{
    std::lock_guard<std::mutex> lock(dbMutex);

    int productId = GetOrCreateProduct(productName, url);

    if (productId < 0)
        return;

    const char* sql =
        "INSERT INTO checks (product_id, status, http_status, reason) "
        "VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_int(stmt, 1, productId);
    sqlite3_bind_text(stmt, 2, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, httpStatus);
    sqlite3_bind_text(stmt, 4, reason.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<Product> Database::GetProducts()
{
    std::vector<Product> products;

    const char* query = "SELECT id, name, url, enabled FROM products;";

    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK)
        return products;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Product p;

        p.id = sqlite3_column_int(stmt, 0);
        p.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        p.url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        p.enabled = sqlite3_column_int(stmt, 3); // ✅ FIX

        products.push_back(p);
    }

    sqlite3_finalize(stmt);

    return products;
}

std::vector<ProductCheck> Database::GetProductHistory(int productId)
{
    std::vector<ProductCheck> results;

    const char* query =
        "SELECT timestamp, status, http_status, reason "
        "FROM checks WHERE product_id=? "
        "ORDER BY timestamp DESC LIMIT 50;";

    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK)
        return results;

    sqlite3_bind_int(stmt, 1, productId);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ProductCheck c;

        c.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        c.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        c.httpStatus = sqlite3_column_int(stmt, 2);
        c.reason = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

        results.push_back(c);
    }

    sqlite3_finalize(stmt);

    return results;
}

bool Database::UpdateProduct(int id, const std::string& name, const std::string& url)
{
    const char* query = "UPDATE products SET name=?, url=? WHERE id=?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, url.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);

    sqlite3_finalize(stmt);
    return ok;
}

bool Database::SetProductEnabled(int id, bool enabled)
{
    const char* query = "UPDATE products SET enabled=? WHERE id=?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 2, id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);

    sqlite3_finalize(stmt);
    return ok;
}

std::vector<ProductCheck> Database::GetLatestChecks(int limit)
{
    std::vector<ProductCheck> results;

    const char* query =
        "SELECT timestamp, status, http_status, reason "
        "FROM checks ORDER BY timestamp DESC LIMIT ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK)
        return results;

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ProductCheck c;
        c.timestamp = (const char*)sqlite3_column_text(stmt, 0);
        c.status = (const char*)sqlite3_column_text(stmt, 1);
        c.httpStatus = sqlite3_column_int(stmt, 2);
        c.reason = (const char*)sqlite3_column_text(stmt, 3);

        results.push_back(c);
    }

    sqlite3_finalize(stmt);
    return results;
}

bool Database::DeleteProduct(int id)
{
    std::lock_guard<std::mutex> lock(dbMutex);

    const char* query = "DELETE FROM products WHERE id=?;";

    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);

    sqlite3_finalize(stmt);

    return success;
}