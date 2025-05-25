#pragma once
#include <fstream>
#include <string>
#include <unordered_map>

#include "json_processing.h"
#include "process_stock.h"
#include "date_processing.h"
#include "http_request.h"

using namespace std::string_literals;

class OrdersProcessor : public StockProcessor {
	using Database = std::unordered_map<std::wstring, std::unordered_map<std::wstring, ItemData>>;
public:
	OrdersProcessor(int days_offset) :StockProcessor(), days_offset_(days_offset) {
		// requestThisYearOrders(days_offset);
		// requestLastYearOrders(days_offset);
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

private:
	std::unordered_map<std::wstring, std::vector<int>> item_to_monthly_sales_;
	int days_offset_ = 0;

	void requestThisYearOrders(int days_offset) {
		if (!requestOrdersData("resources/orders_data.txt"s, this->getTime() - std::chrono::days(days_offset), this->getTime())) {
			throw std::runtime_error("This year orders request failure"s);
		}
	}

	void writeOrdersData() {
		auto& database_full = this->getDataFull();
		auto& database_cluster = this->getDataCluster();
		std::wfstream file("resources/orders_data.txt"s);
		while (setItemReader(file, L"result")) {
			do {
				std::wstring item_name = getItemName(getParameter(file, L"sku"));
				std::wstring warehouse_name = getParameter(file, L"warehouse_name");
				int sold = std::stoi(getParameter(file, L"quantity"));
				database_cluster[getClusterFromWarehouse(warehouse_name)][item_name].sold += sold;
				database_full[getClusterFromWarehouse(warehouse_name)][std::move(warehouse_name)][std::move(item_name)].sold += sold;
			} while (toNextItem(file));
		}
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
		std::wfstream file("resources/seasonality_data.txt"s);
		std::wstring file_contents = getStringFromFile(file);
		std::wstring item;
		std::vector<int>* element_ptr = nullptr;
		for (wchar_t c : file_contents) {
			if (c == '-') {
				element_ptr = &item_to_monthly_sales_[std::move(item)];
				element_ptr->reserve(12);
				item.clear();
			}
			else if (c == ';' || c == '\n') {
				element_ptr->push_back(std::stoi(item));
				item.clear();
			}
			else { 
				item += c;
			}
		}
		element_ptr->push_back(std::stoi(item));
	}
};