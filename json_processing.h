#pragma once
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <forward_list>


std::wstring getWStringFromFile(std::wfstream& file) {
	std::wstring output;
	while (!file.eof()) {
		output += file.get();
	}
	output.pop_back();
	return output;
}
std::string getStringFromFile(std::fstream& file) {
	std::string output;
	while (!file.eof()) {
		output += file.get();
	}
	output.pop_back();
	return output;
}
std::string getStringFromFile(std::filesystem::path path_to_file) {
	int file_size = std::filesystem::file_size(path_to_file);
	std::string output;
	output.reserve(file_size);
	std::fstream file(path_to_file, std::ios::in);

	wchar_t c = file.get();
	while (!file.eof()) {
		output += c;
		c = file.get();
	}

	return output;
}
std::wstring getWStringFromFile(std::filesystem::path path_to_file) {
	int file_size = std::filesystem::file_size(path_to_file);
	std::wstring output;
	output.reserve(file_size);
	std::wfstream file(path_to_file, std::ios::in);

	wchar_t c = file.get();
	while (!file.eof()) {
		output += c;
		c = file.get();
	}

	return output;
}

template <typename Char>
class JsonReader {
public:

	JsonReader() = delete;
	JsonReader(const std::basic_string<Char>& text) : json_text_(&text) {
		pos_stack_.push_back(json_text_->cbegin());
		if (!checkJsonSimple()) {
			throw std::invalid_argument("json malformed");
		}
	}

	bool setJson(const std::basic_string<Char>& text) {
		const std::basic_string<Char>* json_text_tmp = json_text_;
		size_t jsons_num = documents_.size();
		json_text_ = &text;
		if (checkJsonSimple()) {
			std::move(documents_.begin() + jsons_num, documents_.end(), documents_.begin());
			documents_.resize(documents_.size() - jsons_num);
			current_document_ = 0;
			pos_stack_.clear();
			pos_stack_.push_back(json_text_->cbegin());
			return true;
		}
		else {
			json_text_ = json_text_tmp;
			documents_.resize(jsons_num);
			return false;
		}
	}

	bool setReader(const std::basic_string<Char>& parameter) {
		auto it_c = pos_stack_.back();
		if (*it_c != '{') {
			return false;
		}
		int unclosed_brackets = 0;
		auto it_p = parameter.cbegin();
		do {
			if (unclosed_brackets == 1 && *it_c == *it_p) {
				++it_p;
			}
			else {
				it_p = parameter.cbegin();
			}
			if (*it_c == '{') {
				unclosed_brackets += 1;
			}
			if (*it_c == '}') {
				unclosed_brackets -= 1;
			}
			++it_c;
		} while (unclosed_brackets != 0 && it_p != parameter.cend());

		if (it_p == parameter.cend()) {
			it_c = std::find(it_c, json_text_->cend(), ':');
			++it_c;
			it_c = std::find_if(it_c, json_text_->cend(), [](Char c) {
				return (c != ' ') && (c != '\n');
				});
			if (*it_c == '{' || *it_c == '[') {
				pos_stack_.push_back(it_c);
				return true;
			}
		}
		return false;
	}

	std::basic_string_view<Char> getValue(const std::basic_string<Char>& parameter) {
		auto it_c = pos_stack_.back();
		if (*it_c != '{') {
			return {};
		}
		int unclosed_brackets = 0;
		auto it_p = parameter.cbegin();
		do {
			if (unclosed_brackets == 1 && *it_c == *it_p) {
				++it_p;
			}
			else {
				it_p = parameter.cbegin();
			}
			if (*it_c == '{') {
				unclosed_brackets += 1;
			}
			if (*it_c == '}') {
				unclosed_brackets -= 1;
			}
			++it_c;
		} while (unclosed_brackets != 0 && it_p != parameter.cend());

		if (it_p == parameter.cend()) {
			it_c = std::find(it_c, json_text_->cend(), ':');
			++it_c;
			it_c = std::find_if(it_c, json_text_->cend(), [](Char c) {
				return (c != ' ') && (c != '\n');
				});
			if (*it_c != '{' && *it_c != '[') {
				auto it_c2 = it_c;
				if (*it_c == '\"') {
					++it_c;
					it_c2 = std::find(it_c, json_text_->cend(), '\"');
				}
				else {
					it_c2 = std::find_if(it_c, json_text_->cend(), [](Char c) {
						return (c == ',') || (c == ' ') || (c == '}') || (c == '\n');
						});
				}
				return std::basic_string_view<Char>(it_c, it_c2);
			}
		}
		return {};
	}

	// Gets values from an array, if they are not in {}
	std::vector<std::basic_string_view<Char>> getValues() {
		std::vector<std::basic_string_view<Char>> output;
		auto it_c = pos_stack_.back();
		if (*it_c == '[') {
			++it_c;
			it_c = std::find_if(it_c, json_text_->cend(), [](Char c) {
				return (c != ' ') && (c != '\n');
				});
			if (*it_c != '{') {
				if (*it_c == '\"') {
					++it_c;
					auto it_c2 = it_c;
					while (*it_c != ']') {
						it_c2 = std::find(it_c, json_text_->cend(), '\"');
						output.push_back(std::basic_string_view<Char>(it_c, it_c2));
						it_c = it_c2;
						++it_c;
						it_c = std::find_if(it_c, json_text_->cend(), [](Char c) {
							return (c != ',') && (c != ' ') && (c != '\"') && (c != '\n');
							});
					}
				}
				else {
					auto it_c2 = it_c;
					while (*it_c != ']') {
						it_c2 = std::find_if(it_c, json_text_->cend(), [](Char c) {
							return (c == ',') || (c == ' ') || (c == ']');
							});
						output.push_back(std::basic_string_view<Char>(it_c, it_c2));
						it_c = it_c2;
						it_c = std::find_if(it_c, json_text_->cend(), [](Char c) {
							return (c != ',') && (c != ' ') && (c != '\n');
							});
					}
				}
			}
		}
		return output;
	}

	// Only supports array elements in {}
	bool toNextItem() {
		auto it_c = pos_stack_.back();
		if (*it_c == '[') {
			++it_c;
			it_c = std::find_if(it_c, json_text_->cend(), [](Char c) {
				return (c != ' ') && (c != '\n');
				});
			if (*it_c == '{') {
				pos_stack_.push_back(it_c);
				return true;
			}
			else {
				return false;
			}
		}
		else {
			if (pos_stack_.size() > 1 && *pos_stack_[pos_stack_.size() - 2] == '[') {
				int unclosed_brackets = 0;
				do {
					if (*it_c == '{') {
						unclosed_brackets += 1;
					}
					if (*it_c == '}') {
						unclosed_brackets -= 1;
					}
					++it_c;
				} while (unclosed_brackets != 0);

				it_c = std::find_if(it_c, json_text_->cend(), [](Char c) {
					return (c != ' ') && (c != '\n');
					});
				if (*it_c == ',') { // Next item in array found
					it_c = std::find(it_c, json_text_->cend(), '{');
					pos_stack_.back() = it_c;
					return true;
				}
				else {
					// Goes up in scope(back to array)
					pos_stack_.pop_back();
				}
			}
			return false;
		}
	}

	void goUp() {
		if (pos_stack_.size() > 1) {
			pos_stack_.pop_back();
		}
	}
	void goBegin() {
		pos_stack_.resize(1);
	}

	bool toNextJson() {
		if (current_document_ < documents_.size() - 1) {
			pos_stack_.resize(1);
			pos_stack_[0] = documents_[current_document_ + 1];
			current_document_ += 1;
			return true;
		}
		else {
			return false;
		}
	}
	bool toPrevJson() {
		if (current_document_ != 0) {
			pos_stack_.resize(1);
			pos_stack_[0] = documents_[current_document_ - 1];
			current_document_ -= 1;
			return true;
		}
		else {
			return false;
		}
	}

private:
	const std::basic_string<Char>* json_text_ = nullptr; // Pointer to json string
	std::vector<typename std::basic_string<Char>::const_iterator> pos_stack_;
	std::vector<typename std::basic_string<Char>::const_iterator> documents_; // Multiple jsons in one string
	size_t current_document_ = 0;

	// Fast but primitive check, some malformed jsons might pass this test
	bool checkJsonSimple() {
		if (json_text_->empty()) {
			return false;
		}
		int unclosed_sqbrackets = 0;
		int unclosed_brackets = 0;
		int dquotes = 0;
		auto it = std::find(json_text_->cbegin(), json_text_->cend(), '{');
		while (it != json_text_->cend()) {
			switch (*it) {
			case '[':
				unclosed_sqbrackets += 1;
				break;
			case '{':
				if (unclosed_brackets == 0) {
					documents_.push_back(it);
				}
				unclosed_brackets += 1;
				break;
			case '\"':
				dquotes += 1;
				break;
			case ']':
				unclosed_sqbrackets -= 1;
				break;
			case '}':
				unclosed_brackets -= 1;
				break;
			}
			++it;
		}
		if (unclosed_sqbrackets != 0 || unclosed_brackets != 0 ||
			(dquotes % 2) == 1 || documents_.empty()) {
			return false;
		}
		else {
			return true;
		}
	}
};