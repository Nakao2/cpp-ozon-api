#pragma once
#include <fstream>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <chrono>

#include "json_processing.h"
#include "process_stock.h"
#include "date_processing.h"
#include "http_request.h"

using namespace std::string_literals;

class OrdersProcessor : public StockProcessor {
	using Database = std::unordered_map<std::wstring, std::unordered_map<std::wstring, ItemData>>;
public:
	OrdersProcessor(std::string_view account_name, int days_offset) :StockProcessor(account_name), days_offset_(days_offset) {
		sendOrdersRequest();
		setupUtilities();
		writeOrdersData();
		calculateSeasonality(days_offset);
	}

	const std::unordered_map<std::wstring, std::vector<int>>& getMonthlySeasonality() const {
		return item_to_monthly_sales_;
	}

	int getDaysOffset() {
		return days_offset_;
	}

	std::filesystem::path printInXLS() override {
		const double VERY_SMALL_NUMBER = 0.0000001;

		std::chrono::time_point<std::chrono::system_clock> now = getTime();
		std::string date = formDateString(now); // 2024-09-25T09:01:05.000Z (example)
		date.resize(10); // 2024-09-25 (example)
		std::string output_path_string = "RESULT/"s;
		output_path_string += getAccountName();
		output_path_string += "/RESULT-"s + date + '(' + std::to_string(getDaysOffset()) + " days)"s;
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
		while (*it2 != L'\n') {
			it2 = std::find_if(it1, input_helper.end(), [](wchar_t c) {
				return c == L';' || c == L'\n';
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
					table[row][7] = item_to_data.second.sold;
					double seasonality = (item_to_data.second.seasonality < VERY_SMALL_NUMBER) ? 1 : item_to_data.second.seasonality;
					seasonality = std::round(seasonality * 100) / 100; // round to hundredths
					table[row][8] = seasonality;

					double sales_before_delivery;
					if (getDeliveryTime(warehouse_to_items.first) != -1) {
						sales_before_delivery = double(item_to_data.second.sold) / getDaysOffset() *
							                    getDeliveryTime(warehouse_to_items.first);
					}
					else {
						sales_before_delivery = 0.0;
					}
					double expected_stock = double(item_to_data.second.in_stock) - sales_before_delivery -
						                    (item_to_data.second.sold * seasonality) + item_to_data.second.en_route;
					table[row][9] = std::round(expected_stock);
					if (expected_stock < 0.0) {
						table[row][10] = std::round(expected_stock * -1.0);
					}
					else {
						table[row][10] = 0.0;
					}

					if (expected_stock < -4.499999) { // if expected stock rounds to -5 and below
						for (int column = 1; column <= 10; ++column) {
							table[row][column].style_id = 2;
						}
					}

					++row;
				}
			}
		}
		table.applyFilters({ 1, 10 });

		it2 = std::find(it1, input_helper.end(), L';');
		table.setSheet(std::wstring(it1, it2));
		it1 = it2;
		++it1;
		column_id = 1;
		while (it2 != input_helper.end()) {
			it2 = std::find_if(it1, input_helper.end(), [](wchar_t c) {
				return c == L';' || c == L'\n';
				});
			table[1][column_id] = { 1, std::wstring(it1, it2) };
			++column_id;
			it1 = it2;
			if (it2 != input_helper.end()) {
				++it1;
			}
		}

		row = 2;
		for (const auto& cluster_to_items : getDataCluster()) {
			for (const auto& item_to_data : cluster_to_items.second) {
				table[row][1] = cluster_to_items.first;
				table[row][2] = getItemArticle(item_to_data.first);
				table[row][3] = item_to_data.first;
				table[row][4] = item_to_data.second.in_stock;
				table[row][5] = item_to_data.second.en_route;
				table[row][6] = item_to_data.second.sold;
				double seasonality = (item_to_data.second.seasonality < VERY_SMALL_NUMBER) ? 1 : item_to_data.second.seasonality;
				seasonality = std::round(seasonality * 100) / 100;
				table[row][7] = seasonality;

				double sales_before_delivery;
				if (getDeliveryTime(cluster_to_items.first) != -1) {
					sales_before_delivery = double(item_to_data.second.sold) / getDaysOffset() *
						                    getDeliveryTime(cluster_to_items.first);
				}
				else {
					sales_before_delivery = 0.0;
				}
				double expected_stock = double(item_to_data.second.in_stock) - sales_before_delivery -
					(item_to_data.second.sold * seasonality) + item_to_data.second.en_route;
				table[row][8] = std::round(expected_stock);
				if (expected_stock < 0.0) {
					table[row][9] = std::round(expected_stock * -1.0);
				}
				else {
					table[row][9] = 0.0;
				}

				if (expected_stock < -4.499999) { // if expected stock rounds to -5 and below
					for (int column = 1; column <= 9; ++column) {
						table[row][column].style_id = 2;
					}
				}

				++row;
			}
		}
		table.applyFilters({ 1, 9 });

		output << table.to_string();
		return output_path;
	}

private:
	std::unordered_map<std::wstring, std::vector<int>> item_to_monthly_sales_;
	int days_offset_ = 0;

#if SENDREQUESTS
	void sendOrdersRequest() {
		auto time_now = getTime();
		size_t attempts = 0;
		while (attempts != 3 && !requestOrdersData(getCredentials(), time_now - std::chrono::days(days_offset_), time_now)) {
			++attempts;
			std::this_thread::sleep_for(1s);
		}
		if (attempts == 3) {
			throw std::runtime_error("orders_request_failure");
		}
	}
#else 
	void sendOrdersRequest() {}
#endif

	void writeOrdersData() {
		auto& database_full = this->getDataFull();
		auto& database_cluster = this->getDataCluster();
		std::wstring file_contents(getWStringFromFile("resources/runtime/orders_data.txt"));
		JsonReader doc(file_contents);
		do {
			doc.setReader(L"result");
			while (doc.toNextItem()) {
				if (doc.getValue(L"status") != L"cancelled") {
					doc.setReader(L"analytics_data");
					std::wstring warehouse_name(doc.getValue(L"warehouse_name"));
					auto& cluster_data = database_cluster[getClusterFromWarehouse(warehouse_name)];
					auto& cluster_to_warehouse_data = database_full[getClusterFromWarehouse(warehouse_name)][std::move(warehouse_name)];
					doc.goUp();
					doc.setReader(L"products");
					while (doc.toNextItem()) {
						const std::wstring& item_name = getItemName(std::wstring(doc.getValue(L"sku")));
						int sold = SViewToInt(doc.getValue(L"quantity"));
						cluster_data[item_name].sold += sold;
						cluster_to_warehouse_data[item_name].sold += sold;
					}
					doc.goUp();
				}
			}
		} while (doc.toNextJson());
	}

	// Seasonality coefficient is calculated from data provided by graph made of broken lines
	// For example, in March sales were 100 units and in April sales were 200 units
	// We set value of 100 to 15th of March and 200 to 15th of April (that's why you will see magic number 15 in here)
	// We assume sales rise linearly from thoso two dates
	// We calcuate a simple integral value of the graph in range of time periods
	// Then we find ratio between integral values of time periods to find the coefficient
	void calculateSeasonality(int days_offset) {
		std::vector<Database*> location_to_item_ptrs;
		location_to_item_ptrs.push_back(&this->getDataCluster());
		for (auto& cluster_to_warehouses : this->getDataFull()) {
			location_to_item_ptrs.push_back(&cluster_to_warehouses.second);
		}
		for (Database* database : location_to_item_ptrs) {
			for (auto& warehouse_to_item : *database) {
				if (this->getDeliveryTime(warehouse_to_item.first) == -1) {
					continue;
				}
				for (auto& item_to_data : warehouse_to_item.second) {
					if (item_to_monthly_sales_.count(item_to_data.first) == 0) {
						continue;
					}

					auto time_rhs = this->getTime();
					auto time_lhs = time_rhs - std::chrono::days(days_offset);
					double integral_lhs = 0.0;
					double integral_rhs = 0.0;
					double* integral_ptr = &integral_lhs;
					int days_between;
					unsigned month, prev_month;
					double middle_line_pos, delta, gain_per_day, middle_line_value;
					for (int i = 0; i < 2; ++i) {
						int day_counter = 0;
						while (time_lhs != time_rhs) {
							time_lhs += std::chrono::days(1);
							++day_counter;
							if (getDayFromDate(time_lhs) == 15) {
								month = getMonthFromDate(time_lhs);
								prev_month = month - 1;
								if (prev_month == 0) { // Set month to december
									prev_month = 12;
								}
								days_between = getLastDayOfMonth(time_lhs - std::chrono::days(16));
								middle_line_pos = double(days_between + days_between - day_counter) / 2;
								delta = item_to_monthly_sales_[item_to_data.first][month - 1] -
									    item_to_monthly_sales_[item_to_data.first][prev_month - 1];
								gain_per_day = delta / days_between;
								middle_line_value = gain_per_day * middle_line_pos + item_to_monthly_sales_[item_to_data.first][prev_month - 1];
								*integral_ptr += middle_line_value * day_counter;
								day_counter = 0;
							}
						}
						if (getDayFromDate(time_rhs) > 15) {
							prev_month = getMonthFromDate(time_rhs);
							month = (prev_month == 12) ? 1 : prev_month + 1;
							days_between = getLastDayOfMonth(time_rhs);
							middle_line_pos = double(getDayFromDate(time_rhs) - 15 + getDayFromDate(time_rhs) - day_counter - 15) / 2;
						}
						else {
							month = getMonthFromDate(time_rhs);
							prev_month = (month == 1) ? 12 : month - 1;
							days_between = getLastDayOfMonth(time_rhs - std::chrono::days(16));
							middle_line_pos = double(getDayFromDate(time_rhs) + days_between - 15 +
								                     getDayFromDate(time_rhs) + days_between - 15 - day_counter) / 2;
						}
						delta = item_to_monthly_sales_[item_to_data.first][month - 1] -
							    item_to_monthly_sales_[item_to_data.first][prev_month - 1];
						gain_per_day = delta / days_between;
						middle_line_value = gain_per_day * middle_line_pos + item_to_monthly_sales_[item_to_data.first][prev_month - 1];
						*integral_ptr += middle_line_value * day_counter;

						time_lhs = time_rhs + std::chrono::days(getDeliveryTime(warehouse_to_item.first));
						time_rhs = time_lhs + std::chrono::days(days_offset);
						integral_ptr = &integral_rhs;
					}
					item_to_data.second.seasonality = integral_rhs / integral_lhs;
				}
			}
		}
	}

	void setupUtilities() {
		std::wstring file_contents(getWStringFromFile("resources/common/seasonality_data.txt"s));
		JsonReader doc(file_contents);
		doc.setReader(L"items");
		while (doc.toNextItem()) {
			std::vector<int>& monthly_sales = item_to_monthly_sales_[std::wstring(doc.getValue(L"name"))];
			doc.setReader(L"monthly_sales");
			std::vector<std::wstring_view> monthly_sales_json(doc.getValues());
			monthly_sales.reserve(monthly_sales_json.size());
			for (std::wstring_view sale : monthly_sales_json) {
				monthly_sales.push_back(SViewToInt(sale));
			}
			doc.goUp();
		}
	}
};