#pragma once
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <string>
#include "process_orders.h"
#include "json_processing.h"

using namespace std::string_literals;

const double VERY_SMALL_NUMBER = 0.0000001;

void printWarehouseData(OrdersProcessor& source) {
	std::wfstream output("RESULT_warehouse.csv"s, std::ios::out | std::ios::trunc);
	output << "\xEF\xBB\xBF"; // BOM to tell Excel to read it in UTF-8
	std::wfstream starting_columns("resources/excel_columns_w.txt", std::ios::in);
	std::wstring columns = getStringFromFile(starting_columns);
	output << columns << '\n';
	for (const auto& warehouse_to_item : source.getDataWarehouse()) {
		output << warehouse_to_item.first;
		for (const auto& item_to_data : warehouse_to_item.second) {
			output << ';' << item_to_data.first << ';' << item_to_data.second.in_stock << ';' <<
				             item_to_data.second.sold << ';' << item_to_data.second.seasonality << ';';
			double seasonality = (item_to_data.second.seasonality < VERY_SMALL_NUMBER) ? 1 : item_to_data.second.seasonality;
			double sales_before_delivery;
			if (source.getDeliveryTime(warehouse_to_item.first) != -1) {
				sales_before_delivery = double(item_to_data.second.sold) / source.getDaysOffset() *
					                    source.getDeliveryTime(warehouse_to_item.first);
			}
			else {
				sales_before_delivery = 0.0;
			}
			double expected_stock = (item_to_data.second.in_stock - sales_before_delivery -
				(item_to_data.second.sold * seasonality));
			output << std::round(expected_stock) << ';';
			if (expected_stock < 0.0) {
				output << std::round(expected_stock * -1.0) << '\n';
			}
			else {
				output << 0.0 << '\n';
			}
		}
	}
}

void printClusterData(OrdersProcessor& source) {
	std::wfstream output("RESULT_cluster.csv"s, std::ios::out | std::ios::trunc);
	output << "\xEF\xBB\xBF"; // BOM to tell Excel to read it in UTF-8
	std::wfstream starting_columns("resources/excel_columns_c.txt", std::ios::in);
	std::wstring columns = getStringFromFile(starting_columns);
	output << columns << '\n';
	for (const auto& cluster_to_item : source.getDataCluster()) {
		output << cluster_to_item.first;
		for (const auto& item_to_data : cluster_to_item.second) {
			output << ';' << item_to_data.first << ';' << item_to_data.second.in_stock << ';' <<
				             item_to_data.second.sold << ';' << item_to_data.second.seasonality << ';';
			double seasonality = (item_to_data.second.seasonality < VERY_SMALL_NUMBER) ? 1 : item_to_data.second.seasonality;
			double sales_before_delivery;
			if (source.getDeliveryTime(cluster_to_item.first) != -1) {
				sales_before_delivery = double(item_to_data.second.sold) / source.getDaysOffset() *
					source.getDeliveryTime(cluster_to_item.first);
			}
			else {
				sales_before_delivery = 0.0;
			}
			double expected_stock = (item_to_data.second.in_stock - sales_before_delivery -
				(item_to_data.second.sold * seasonality));
			output << std::round(expected_stock) << ';';
			if (expected_stock < 0.0) {
				output << std::round(expected_stock * -1.0) << '\n';
			}
			else {
				output << 0.0 << '\n';
			}
		}
	}
}
