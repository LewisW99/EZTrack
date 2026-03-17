#include "ApiRoutes.h"
#include "../database/Database.h"

#include <sstream>
#include <fstream>

// -------------------------
// REGISTER ROUTES
// -------------------------
void RegisterApiRoutes(httplib::Server& server)
{
    // -------------------------
    // CORE
    // -------------------------
    server.Get("/ping", [](auto&, auto& res)
    {
        res.set_content("pong", "text/plain");
    });

    server.Get("/metrics", [](auto&, auto& res)
    {
        res.set_content("{\"service\":\"EZTrack\",\"status\":\"running\"}", "application/json");
    });

    server.Get("/status", [](auto&, auto& res)
    {
        std::ifstream file("status/status.json");

        if (!file.is_open())
        {
            res.status = 500;
            res.set_content("Missing status file", "text/plain");
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        res.set_content(buffer.str(), "application/json");
    });

    // -------------------------
    // PRODUCTS
    // -------------------------
    server.Get("/products", [](auto&, auto& res)
    {
        Database db("tracker.db");
        auto products = db.GetProducts();

        std::stringstream json;
        json << "{ \"products\":[";

        for (size_t i = 0; i < products.size(); i++)
        {
            auto& p = products[i];

            json << "{";
            json << "\"id\":" << p.id << ",";
            json << "\"name\":\"" << p.name << "\",";
            json << "\"url\":\"" << p.url << "\"";
            json << "}";

            if (i != products.size() - 1) json << ",";
        }

        json << "]}";

        res.set_content(json.str(), "application/json");
    });

    // GET product by id
    server.Get(R"(/product/(\d+))", [](const auto& req, auto& res)
    {
        int id = std::stoi(req.matches[1]);

        Database db("tracker.db");
        auto products = db.GetProducts();

        for (auto& p : products)
        {
            if (p.id == id)
            {
                std::stringstream json;
                json << "{";
                json << "\"id\":" << p.id << ",";
                json << "\"name\":\"" << p.name << "\",";
                json << "\"url\":\"" << p.url << "\"";
                json << "}";

                res.set_content(json.str(), "application/json");
                return;
            }
        }

        res.status = 404;
        res.set_content("Not found", "text/plain");
    });

    // CREATE
    server.Post("/products", [](const httplib::Request& req, httplib::Response& res)
{
    if (!req.has_param("name") || !req.has_param("url"))
    {
        res.status = 400;
        res.set_content("Missing name or url", "text/plain");
        return;
    }

    std::string name = req.get_param_value("name");
    std::string url  = req.get_param_value("url");

    Database db("tracker.db");

    int id = db.GetOrCreateProduct(name, url);

    if (id == -1)
    {
        res.status = 500;
        res.set_content("Failed to create product", "text/plain");
        return;
    }

    std::stringstream json;
    json << "{ \"id\": " << id << " }";

    res.set_content(json.str(), "application/json");
});

    // DELETE
    server.Delete(R"(/product/(\d+))", [](const auto& req, auto& res)
    {
        int id = std::stoi(req.matches[1]);

        Database db("tracker.db");

        if (!db.DeleteProduct(id))
        {
            res.status = 500;
            res.set_content("Delete failed", "text/plain");
            return;
        }

        res.set_content("Deleted", "text/plain");
    });

    // PATCH
    server.Patch(R"(/product/(\d+))", [](const auto& req, auto& res)
    {
        int id = std::stoi(req.matches[1]);

        auto name = req.get_param_value("name");
        auto url  = req.get_param_value("url");

        Database db("tracker.db");

        if (!db.UpdateProduct(id, name, url))
        {
            res.status = 500;
            res.set_content("Update failed", "text/plain");
            return;
        }

        res.set_content("Updated", "text/plain");
    });

    // ENABLE
    server.Post(R"(/product/(\d+)/enable)", [](const auto& req, auto& res)
    {
        int id = std::stoi(req.matches[1]);
        Database db("tracker.db");
        db.SetProductEnabled(id, true);
        res.set_content("Enabled", "text/plain");
    });

    // DISABLE
    server.Post(R"(/product/(\d+)/disable)", [](const auto& req, auto& res)
    {
        int id = std::stoi(req.matches[1]);
        Database db("tracker.db");
        db.SetProductEnabled(id, false);
        res.set_content("Disabled", "text/plain");
    });

    // -------------------------
    // HISTORY
    // -------------------------
    server.Get("/history", [](const auto& req, auto& res)
    {
        if (!req.has_param("product_id"))
        {
            res.status = 400;
            res.set_content("Missing product_id", "text/plain");
            return;
        }

        int id = std::stoi(req.get_param_value("product_id"));

        Database db("tracker.db");
        auto history = db.GetProductHistory(id);

        std::stringstream json;
        json << "{ \"checks\":[";

        for (size_t i = 0; i < history.size(); i++)
        {
            auto& h = history[i];

            json << "{";
            json << "\"timestamp\":\"" << h.timestamp << "\",";
            json << "\"status\":\"" << h.status << "\",";
            json << "\"http_status\":" << h.httpStatus;
            json << "}";

            if (i != history.size() - 1) json << ",";
        }

        json << "]}";

        res.set_content(json.str(), "application/json");
    });

    // latest
    server.Get("/history/latest", [](auto&, auto& res)
    {
        Database db("tracker.db");
        auto history = db.GetLatestChecks();

        std::stringstream json;
        json << "{ \"checks\":[";

        for (size_t i = 0; i < history.size(); i++)
        {
            auto& h = history[i];

            json << "{";
            json << "\"timestamp\":\"" << h.timestamp << "\",";
            json << "\"status\":\"" << h.status << "\"";
            json << "}";

            if (i != history.size() - 1) json << ",";
        }

        json << "]}";

        res.set_content(json.str(), "application/json");
    });
}