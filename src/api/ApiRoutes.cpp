#include "ApiRoutes.h"
#include "../database/Database.h"
#include "../tracker/Product.h"


#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

void RegisterApiRoutes(httplib::Server& server)
{
	
	//Health check 
	server.Get("/ping", [](const httplib::Request&, httplib::Response& res)
	{
		res.set_content("pong!", "text/plain");
	});
	
	
	//Status file 
	server.Get("/status", [](const httplib::Request&, httplib::Response& res)
	{
		std::ifstream file("status/status.json");
		
		if(!file.is_open())
		{
			res.status = 500;
			res.set_content("status.json not found", "text/plain");
			return;
		}
		
		std::stringstream buffer;
		buffer << file.rdbuf();
		res.set_content(buffer.str(), "application/json");
	});
	
	
	//product file
	server.Get("/products", [](const httplib::Request&, httplib::Response& res)
	{
		Database db("tracker.db");
		
		auto products = db.GetProducts();
		
		std::stringstream json;
		json << "{ \"products\": [";
		
		for(size_t i = 0; - < products.size(); i++)
		{
			const auto& p = products[i];
			
			json << "{";
			json << "\"id\":" << p.id << "," ;
			json << "\"name\":\"" << p.name << "\",";
			json << "\"url\":\"" << p.url << "\"";
			json << "}";
			
			if(i != products.size() - 1)
			{
				json << ",";
			}
		}
		
		json << "]}";
		
		res.set_content(json.str(), "application/json");
		
	});
	
	//history 
	server.Get("/history", [](const httplib::Request&, httplib::Response& res)
	{
		if(request.has_param("product_id");
		{
			res.status =- 400;
			res.set_content("Missing ID");
			return;
		}
		
		int productId = std:stoi(req.get_param_value("product_id");
		
		Database db("tracker.db");
		
		auto history = db.GetProductHistoryu(productId);
		
		std::stringstream json;
		json << "{ \"checks\": [";
		
		for(size_t i = 0; - < history.size(); i++)
		{
			const auto& h = history[i];
			
			json << "{";
			json << "\"timestamp\":\"" << h.timestamp << "\"," ;
			json << "\"status\":\"" << h.status << "\",";
			json << "\"http_status\":" << h.httpStatus;
			json << "}";
			
			if(i != history.size() - 1)
			{
				json << ",";
			}
		}
		
		json << "]}";
		
		res.set_content(json.str(), "application/json");
		
	});
	
	//metrics
	server.Get("/metrics", [](const httplib::Request&, httplib::Response& res)
	{
		std::stringstream json;
		
		json << "{";
		json <<"/"service/":/"EZTrack/",";
		json << "/"status/":/"running/"";
		json << "}";
		
		res.set_content(json.str(), "application/json");
		
	});
}
