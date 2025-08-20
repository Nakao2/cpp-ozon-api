#pragma once
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <chrono>
#include "process_orders.h"
#include "json_processing.h"
#include "xls_formatter.h"
#include "date_processing.h"

using namespace std::string_literals;

const double VERY_SMALL_NUMBER = 0.0000001;

void printXLS(OrdersProcessor& source) {
	std::chrono::time_point<std::chrono::system_clock> now = source.getTime();
	std::string date = formDateString(now); // 2024-09-25T09:01:05.000Z (example)
	date.resize(10); // 2024-09-25 (example)
	std::wfstream output("RESULT/RESULT-"s + date +
						'(' + std::to_string(source.getDaysOffset()) + " days)"s + ".xls"s, std::ios::out | std::ios::trunc);
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
	for (const auto& cluster_to_warehouses : source.getDataFull()) {
		for (const auto& warehouse_to_items : cluster_to_warehouses.second) {
			for (const auto& item_to_data : warehouse_to_items.second) {
				table[row][1] = cluster_to_warehouses.first;
				table[row][2] = warehouse_to_items.first;
				table[row][3] = item_to_data.first;
				table[row][4] = item_to_data.second.in_stock;
				table[row][5] = item_to_data.second.en_route;
				table[row][6] = item_to_data.second.sold;
				double seasonality = (item_to_data.second.seasonality < VERY_SMALL_NUMBER) ? 1 : item_to_data.second.seasonality;
				seasonality = std::round(seasonality * 100) / 100; // round to hundredths
				table[row][7] = seasonality;

				double sales_before_delivery;
				if (source.getDeliveryTime(warehouse_to_items.first) != -1) {
					sales_before_delivery = double(item_to_data.second.sold) / source.getDaysOffset() *
						                    source.getDeliveryTime(warehouse_to_items.first);
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
	}
	table.applyFilters({ 1, 9 });

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
	for (const auto& cluster_to_items : source.getDataCluster()) {
		for (const auto& item_to_data : cluster_to_items.second) {
			table[row][1] = cluster_to_items.first;
			table[row][2] = item_to_data.first;
			table[row][3] = item_to_data.second.in_stock;
			table[row][4] = item_to_data.second.en_route;
			table[row][5] = item_to_data.second.sold;
			double seasonality = (item_to_data.second.seasonality < VERY_SMALL_NUMBER) ? 1 : item_to_data.second.seasonality;
			seasonality = std::round(seasonality * 100) / 100;
			table[row][6] = seasonality;

			double sales_before_delivery;
			if (source.getDeliveryTime(cluster_to_items.first) != -1) {
				sales_before_delivery = double(item_to_data.second.sold) / source.getDaysOffset() *
					                    source.getDeliveryTime(cluster_to_items.first);
			}
			else {
				sales_before_delivery = 0.0;
			}
			double expected_stock = double(item_to_data.second.in_stock) - sales_before_delivery -
				                    (item_to_data.second.sold * seasonality) + item_to_data.second.en_route;
			table[row][7] = std::round(expected_stock);
			if (expected_stock < 0.0) {
				table[row][8] = std::round(expected_stock * -1.0);
			}
			else {
				table[row][8] = 0.0;
			}

			if (expected_stock < -4.499999) { // if expected stock rounds to -5 and below
				for (int column = 1; column <= 8; ++column) {
					table[row][column].style_id = 2;
				}
			}

			++row;
		}
	}
	table.applyFilters({ 1, 8 });

	output << table.to_string();
}
