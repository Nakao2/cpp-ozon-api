#include <fstream>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <filesystem>

#include "log_duration2.h"
#include "print_func.h"
#include "process_stock.h"
#include "process_orders.h"
#include "date_processing.h"
#include "http_request.h"


using namespace std::string_literals;
using namespace std::chrono_literals;

std::string InputHelper() {
	std::cout << "Available accounts:\n";
	std::filesystem::path accounts_path = "resources/accounts"s;
	if (std::filesystem::is_empty(accounts_path)) {
		throw std::runtime_error("no_accounts_present");
	}
	for (const auto& dir_entry : std::filesystem::directory_iterator(accounts_path)) {
		std::cout << dir_entry.path().filename() << '\n';
	}
	std::cout << "Enter account name(without double quotes)\n";
	std::string account_name;
	std::cin >> account_name;
	accounts_path.append(account_name);
	while (!std::filesystem::is_directory(accounts_path)) {
		std::cout << "Enter account name\n";
		std::cin >> account_name;
		accounts_path.replace_filename(account_name);
	}
	return account_name;
}

int main() {
	std::string account_name = InputHelper();
	std::cout << "Enter days offset\n"s;
	int days_offset;
	std::cin >> days_offset;
	OrdersProcessor op(account_name, days_offset);
	op.printInXLS();
}