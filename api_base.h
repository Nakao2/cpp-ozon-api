#pragma once

#define SENDREQUESTS true

#include <fstream>
#include <string>
#include <unordered_map>
#include <chrono>
#include <filesystem>
#include "json_processing.h"

class ApiBase {
public:

	ApiBase(std::string_view account_name) : time_now_(std::chrono::system_clock::now()) {
		initializeAccountDataPath(account_name);
		initializeAccountCredentials();
		initializeItemIdentifiers();
	}

	const std::wstring& getItemName(const std::wstring& sku) {
		auto it = sku_to_name_.find(sku);
		if (it != sku_to_name_.end()) {
			return it->second;
		}
		else {
			return UNIDENTIFIED_NAME_TEXT;
		}
	}
	const std::wstring& getItemArticle(const std::wstring& name) {
		auto it = name_to_article_.find(name);
		if (it != name_to_article_.end()) {
			return it->second;
		}
		else {
			return UNIDENTIFIED_ARTICLE_TEXT;
		}
	}

	std::chrono::time_point<std::chrono::system_clock> getTime() {
		return time_now_;
	}

	std::filesystem::path getAccountDataPath() {
		return path_to_account_data_;
	}

	const std::string& getAccountName() {
		return account_name_;
	}

	const std::pair<std::string, std::string>& getCredentials() {
		return account_credentials_;
	}

private:
	std::chrono::time_point<std::chrono::system_clock> time_now_;
	std::unordered_map<std::wstring, std::wstring> sku_to_name_; // sku : name
	std::unordered_map<std::wstring, std::wstring> name_to_article_; // item_name : article
	std::string account_name_;
	std::filesystem::path path_to_account_data_;
	std::pair<std::string, std::string> account_credentials_; // (id, key)

	const std::wstring UNIDENTIFIED_NAME_TEXT = L"unidentified item";
	const std::wstring UNIDENTIFIED_ARTICLE_TEXT = L"unidentified article";

	void initializeAccountDataPath(std::string_view account_name) {
		std::string path_string = "resources/accounts/"s;
		path_string += std::string(account_name);
		std::filesystem::path path = path_string;
		if (!std::filesystem::is_directory(path) || std::filesystem::is_empty(path)) {
			// createAccount();
			throw std::invalid_argument("missing_account"s);
		}
		else {
			account_name_ = account_name;
			path_to_account_data_ = path;
		}
	}

	void initializeAccountCredentials() {
		auto path = getAccountDataPath();
		path.append("ozon_credentials.txt"s);
		std::string file_contents(getStringFromFile(path));
		JsonReader doc(file_contents);
		account_credentials_.first = std::string(doc.getValue("client_id"s));
		account_credentials_.second = std::string(doc.getValue("key"s));
	}

	void initializeItemIdentifiers() {
		auto path = getAccountDataPath();
		path.append("sku_to_name.txt"s);
		std::wstring file_contents(getWStringFromFile(path));
		JsonReader doc(file_contents);
		doc.setReader(L"items");
		while (doc.toNextItem()) {
			std::wstring item_name(doc.getValue(L"name"));
			sku_to_name_[std::wstring(doc.getValue(L"sku"))] = item_name;
			name_to_article_[std::move(item_name)] = std::wstring(doc.getValue(L"article"));
		}
	}

};