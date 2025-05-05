#pragma once

#include <fstream>
#include <string>
#include <unordered_map>
#include <chrono>

class ApiBase {
public:

	ApiBase() : time_now_(std::chrono::system_clock::now()) {
		initializeSkuToName();
	}

	std::wstring getItemName(const std::wstring& sku) const {
		auto it = sku_to_name_.find(sku);
		if (it != sku_to_name_.end()) {
			return it->second;
		}
		else {
			return L"unidentified_item";
		}
	}

	std::chrono::time_point<std::chrono::system_clock> getTime() {
		return time_now_;
	}

private:
	std::chrono::time_point<std::chrono::system_clock> time_now_;
	std::unordered_map<std::wstring, std::wstring> sku_to_name_;

	void initializeSkuToName() {
		std::wfstream file("resources/sku_to_name.txt");
		while (!file.eof()) {
			std::wstring sku;
			std::wstring item;
			while (file.peek() != '-') {
				sku += file.get();
			}
			file.get();
			while (!file.eof() && file.peek() != '\n') {
				item += file.get();
			}
			if (file.eof()) {
				item.pop_back();
			}
			file.get();
			sku_to_name_[std::move(sku)] = std::move(item);
		}
	}

};