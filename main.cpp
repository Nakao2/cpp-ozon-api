#include <fstream>
#include <string>
#include <iostream>
#include <chrono>

#include "log_duration2.h"
#include "print_func.h"
#include "process_stock.h"
#include "process_orders.h"
#include "date_processing.h"
#include "http_request.h"


using namespace std::string_literals;

int main() {
	std::cout << "Enter days offset"s << '\n';
	int days_offset = 0;
	std::cin >> days_offset;
	std::chrono::time_point<std::chrono::system_clock> time_now(std::chrono::system_clock::now());
	std::cout << "Sending request to the server"s << '\n';
	if (!requestStockData() || !requestOrdersData("resources/orders_data.txt"s, time_now - std::chrono::days(days_offset), time_now)) {
		throw std::runtime_error("Http request failure"s);
	}
	std::cout << "Checking server response for errors"s << '\n';
	std::wfstream file("resources/stock_data.txt"s);
	if (checkIllform(file)) {
		throw std::runtime_error("Stock file illformed"s);
	}
	file.close();
	file.open("resources/orders_data.txt"s);
	if (checkIllform(file)) {
		throw std::runtime_error("Orders file illformed"s);
	}
	std::cout << "Processing"s << '\n';
	OrdersProcessor p(days_offset);
	std::cout << "Printing result"s << '\n';
	printWarehouseData(p);
	printClusterData(p);
	std::cout << "Done!"s;
}