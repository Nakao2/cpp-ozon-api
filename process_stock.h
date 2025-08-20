#pragma once
#include <unordered_map>
#include <string>
#include <fstream>
#include <tuple>
#include <algorithm>
#include <filesystem>
#include <chrono>

#include "http_request.h"
#include "json_processing.h"
#include "api_base.h"
#include "date_processing.h"
#include "xls_formatter.h"

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
	StockProcessor(std::string_view account_name) : ApiBase(account_name) {
		sendStockRequest();
		sendEnRouteRequest();
		setupUtilities();
		initializeDatabase();
		writeEnRouteData();
	}

	std::unordered_map<std::wstring, Database>& getDataFull() {
		return database_full_;
	}
	Database& getDataCluster() {
		return database_cluster_;
	}
	
	const std::wstring& getClusterFromWarehouse(const std::wstring& warehouse) {
		auto it = warehouse_to_cluster_.find(warehouse);
		if (it != warehouse_to_cluster_.end()) {
			return it->second;
		}
		else {
			return UNIDENTIFIED_CLUSTER;
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

	virtual std::filesystem::path printInXLS() {
		std::chrono::time_point<std::chrono::system_clock> now = getTime();
		std::string date = formDateString(now); // 2024-09-25T09:01:05.000Z (example)
		date.resize(10); // 2024-09-25 (example)
		std::string output_path_string = "RESULT/"s;
		output_path_string += getAccountName();
		output_path_string += "/RESULT-"s + date + "(STOCK_ONLY)"s;
		output_path_string += '_' + getAccountName() + ".xls"s;
		std::filesystem::path output_path = output_path_string;
		std::wfstream output(output_path, std::ios::out | std::ios::trunc);
		std::wfstream input_helper_file("resources/common/xls_input.txt"s, std::ios::in);
		std::wstring input_helper = getWStringFromFile(input_helper_file);
		auto it1 = input_helper.begin();
		auto it2 = it1;

		XLSFormatter table;
		table.addStyle({ L"Calibri", 12, L"#000000", true, L"" }); // id = 1. Bold
		table.addStyle({ L"Calibri", 11, L"#000000", false, L"#FFFF00" }); // id = 2. Yellow fill

		it2 = std::find(it1, input_helper.end(), L';');
		table.setSheet(std::wstring(it1, it2));
		it1 = it2;
		++it1;
		size_t column_id = 1;
		while (column_id <= 6) {
			it2 = std::find_if(it1, input_helper.end(), [](wchar_t c) {
				return c == L';';
				});
			table[1][column_id] = { 1, std::wstring(it1, it2) };
			++column_id;
			it1 = it2;
			++it1;
		}

		size_t row = 2;
		for (const auto& cluster_to_warehouses : getDataFull()) {
			for (const auto& warehouse_to_items : cluster_to_warehouses.second) {
				for (const auto& item_to_data : warehouse_to_items.second) {
					table[row][1] = cluster_to_warehouses.first;
					table[row][2] = warehouse_to_items.first;
					table[row][3] = getItemArticle(item_to_data.first);
					table[row][4] = item_to_data.first;
					table[row][5] = item_to_data.second.in_stock;
					table[row][6] = item_to_data.second.en_route;

					if (item_to_data.second.in_stock + item_to_data.second.en_route <= 5) {
						for (int column = 1; column <= 6; ++column) {
							table[row][column].style_id = 2;
						}
					}

					++row;
				}
			}
		}
		table.applyFilters({ 1, 6 });

		it1 = std::find(it1, input_helper.end(), L'\n');
		++it1;
		it2 = std::find(it1, input_helper.end(), L';');
		table.setSheet(std::wstring(it1, it2));
		it1 = it2;
		++it1;
		column_id = 1;
		while (column_id <= 5) {
			it2 = std::find_if(it1, input_helper.end(), [](wchar_t c) {
				return c == L';' || c == L'\n';
				});
			table[1][column_id] = { 1, std::wstring(it1, it2) };
			++column_id;
			it1 = it2;
			++it1;
		}

		row = 2;
		for (const auto& cluster_to_items : getDataCluster()) {
			for (const auto& item_to_data : cluster_to_items.second) {
				table[row][1] = cluster_to_items.first;
				table[row][2] = getItemArticle(item_to_data.first);
				table[row][3] = item_to_data.first;
				table[row][4] = item_to_data.second.in_stock;
				table[row][5] = item_to_data.second.en_route;
				
				if (item_to_data.second.in_stock + item_to_data.second.en_route <= 5) {
					for (int column = 1; column <= 5; ++column) {
						table[row][column].style_id = 2;
					}
				}

				++row;
			}
		}
		table.applyFilters({ 1, 5 });

		output << table.to_string();
		return output_path;
	}

private:
	Database database_cluster_; // Cluster to item data
	std::unordered_map<std::wstring, Database> database_full_; // Cluster to warehouses to item data

	std::unordered_map<std::wstring, std::wstring> warehouse_to_capitalized_; // To fix discrepancy between names in Ozon's stock and orders data
	std::unordered_map<std::wstring, std::wstring> warehouse_to_cluster_;
	std::unordered_map<std::wstring, int> warehouse_to_deltime_; // Includes cluster data as well

	const std::wstring UNIDENTIFIED_CLUSTER = L"unidentified cluster";

#if SENDREQUESTS
	void sendStockRequest() {
		size_t attempts = 0;
		while (attempts != 3 && !requestStockData(getCredentials())) {
			++attempts;
			std::this_thread::sleep_for(1s);
		}
		if (attempts == 3) {
			throw std::runtime_error("stock_request_failure");
		}
	}
	void sendEnRouteRequest() {
		size_t attempts = 0;
		while (attempts != 3 && !requestEnRouteStock(getCredentials())) {
			++attempts;
			std::this_thread::sleep_for(1s);
		}
		if (attempts == 3) {
			throw std::runtime_error("enroute_request_failure");
		}
	}
#else 
	void sendStockRequest() {}
	void sendEnRouteRequest() {}
#endif

	void initializeDatabase() {
		std::wstring file_contents(getWStringFromFile("resources/runtime/stock_data.txt"s));
		JsonReader doc(file_contents);
		doc.setReader(L"result");
		doc.setReader(L"rows");
		while (doc.toNextItem()) {
			std::wstring warehouse_name(doc.getValue(L"warehouse_name"));
			auto it1 = warehouse_to_capitalized_.find(warehouse_name);
			if (it1 != warehouse_to_capitalized_.end()) {
				warehouse_name = it1->second;
			}
			const std::wstring& item_name = getItemName(std::wstring(doc.getValue(L"sku")));
			int stock = SViewToInt(doc.getValue(L"free_to_sell_amount"));

			auto& cluster_item_data = database_cluster_[getClusterFromWarehouse(warehouse_name)][item_name];
			cluster_item_data.in_stock += stock;

			auto& warehouse_item_data = database_full_[getClusterFromWarehouse(warehouse_name)][std::move(warehouse_name)][item_name];
			warehouse_item_data.in_stock += stock;
		}
	}

	void writeEnRouteData() {
		std::wstring file_contents(getWStringFromFile("resources/runtime/en_route_data.txt"s));
		if (!file_contents.empty() && file_contents[0] == '{') {
			JsonReader doc(file_contents);
			doc.setReader(L"result");
			while (doc.toNextItem()) {
				std::wstring warehouse_name(doc.getValue(L"warehouse"));
				auto it = warehouse_to_capitalized_.find(warehouse_name);
				if (it != warehouse_to_capitalized_.end()) {
					warehouse_name = it->second;
				}
				const std::wstring& item_name = getItemName(std::wstring(doc.getValue(L"sku")));
				int quantity = SViewToInt(doc.getValue(L"quantity"));
				database_cluster_[getClusterFromWarehouse(warehouse_name)][item_name].en_route += quantity;
				database_full_[getClusterFromWarehouse(warehouse_name)][std::move(warehouse_name)][item_name].en_route += quantity;
			}
		}
	}

	void setupUtilities() {
		std::wstring file_contents(getWStringFromFile("resources/common/cap_warehouse.txt"s));
		JsonReader doc(file_contents);
		doc.setReader(L"warehouses");
		while (doc.toNextItem()) {
			warehouse_to_capitalized_[std::wstring(doc.getValue(L"warehouse"))] = std::wstring(doc.getValue(L"warehouse_cap"));
		}

		file_contents = std::move(getWStringFromFile("resources/common/warehouse_delivery_time.txt"s));
		doc.setJson(file_contents);
		doc.setReader(L"warehouses");
		while (doc.toNextItem()) {
			warehouse_to_deltime_[std::wstring(doc.getValue(L"warehouse"))] = SViewToInt(doc.getValue(L"deltime"));
		}

		file_contents = std::move(getWStringFromFile("resources/common/warehouse_to_cluster.txt"s));
		doc.setJson(file_contents);
		doc.setReader(L"clusters");
		while (doc.toNextItem()) {
			std::wstring cluster(doc.getValue(L"cluster"));
			doc.setReader(L"warehouses");
			auto warehouses = doc.getValues();
			for (std::wstring_view warehouse : warehouses) {
				warehouse_to_cluster_[std::wstring(warehouse)] = cluster;
			}
			doc.goUp();
		}
	}
};