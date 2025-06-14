#pragma once
#include <unordered_map>
#include <string>
#include <fstream>
#include <tuple>
#include <algorithm>
#include "http_request.h"
#include "json_processing.h"
#include "api_base.h"

using namespace std::string_literals;

struct ItemData {
	int in_stock = 0;
	int en_route = 0;
	int sold = 0;
	double seasonality = 0.0;
};

class StockProcessor : public ApiBase {
	using Database = std::unordered_map<std::wstring, std::unordered_map<std::wstring, ItemData>>;
public:
	StockProcessor() :ApiBase() {
		// sendStockRequest();
		setupUtilities();
		initializeDatabase();
	}

	std::unordered_map<std::wstring, Database>& getDataFull() {
		return database_full_;
	}
	Database& getDataCluster() {
		return database_cluster_;
	}
	
	std::wstring getClusterFromWarehouse(const std::wstring& warehouse) {
		auto it = warehouse_to_cluster_.find(warehouse);
		if (it != warehouse_to_cluster_.end()) {
			return it->second;
		}
		else {
			return L"unidentified_cluster";
		}
	}

	int getDeliveryTime(const std::wstring& destination) {
		auto it = warehouse_to_deltime_.find(destination);
		if (it != warehouse_to_deltime_.end()) {
			return it->second;
		}
		else {
			return -1;
		}
	}

private:
	Database database_cluster_; // Cluster to item data
	std::unordered_map<std::wstring, Database> database_full_; // Cluster to warehouses to item data

	std::unordered_map<std::wstring, std::wstring> warehouse_to_capitalized_; // To fix discrepancy between names in Ozon's stock and orders data
	std::unordered_map<std::wstring, std::wstring> warehouse_to_cluster_;
	std::unordered_map<std::wstring, int> warehouse_to_deltime_; // Includes cluster data as well

	void initializeDatabase() {
		std::wfstream file("resources/stock_data.txt"s);
		while (setItemReader(file, L"rows")) {
			do {
				std::wstring warehouse_name = getParameter(file, L"warehouse_name");
				auto it1 = warehouse_to_capitalized_.find(warehouse_name);
				if (it1 != warehouse_to_capitalized_.end()) {
					warehouse_name = it1->second;
				}
				std::wstring item_name = getItemName(getParameter(file, L"sku"));
				int stock = std::stoi(getParameter(file, L"free_to_sell_amount"));
				int en_route = std::stoi(getParameter(file, L"promised_amount"));

				auto& cluster_item_data = database_cluster_[getClusterFromWarehouse(warehouse_name)][item_name];
				cluster_item_data.in_stock += stock;
				cluster_item_data.en_route += en_route;

				auto& warehouse_item_data = database_full_[getClusterFromWarehouse(warehouse_name)][std::move(warehouse_name)][std::move(item_name)];
				warehouse_item_data.in_stock += stock;
				warehouse_item_data.en_route += en_route;
			} while (toNextItem(file));
		}
	}

	// Catch the throw?
	void sendStockRequest() {
		if (!requestStockData()) {
			throw std::runtime_error("Stock server request failure"s);
		}
	}

	void setupUtilities() {
		std::wfstream file("resources/cap_warehouse.txt"s);
		while (!file.eof()) {
			std::wstring no_cap;
			std::wstring cap;
			while (file.peek() != ';') {
				no_cap += file.get();
			}
			file.get();
			while (!file.eof() && file.peek() != '\n') {
				cap += file.get();
			}
			if (file.eof()) {
				cap.pop_back();
			}
			file.get();
			warehouse_to_capitalized_[std::move(no_cap)] = std::move(cap);
		}
		file.close();

		file.open("resources/warehouse_delivery_time.txt"s);
		setItemReader(file, L"warehouses");
		do {
			warehouse_to_deltime_[std::move(getParameter(file, L"warehouse"))] = std::stoi(getParameter(file, L"deltime"));
		} while (toNextItem(file));
		file.close();

		file.open("resources/warehouse_to_cluster.txt"s);
		std::wstring data = getStringFromFile(file);
		auto it1 = data.begin();
		while (it1 != data.end()) {
			auto it2 = std::find(it1, data.end(), ';');
			auto it3 = it2;
			++it3;
			auto it4 = std::find(it3, data.end(), '\n');
			warehouse_to_cluster_[std::wstring(it1, it2)] = std::wstring(it3, it4);
			it1 = std::find_if(it4, data.end(), [](wchar_t c) {
				return c != '\n';
				});
		}
	}
};