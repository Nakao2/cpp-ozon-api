#pragma once
#include <fstream>
#include <string>
#include <string_view>

bool setItemReader(std::wfstream& file, std::wstring_view key) {
	bool found = false;
	auto it = key.begin();
	while (!file.eof() && it != key.end()) {
		if (file.get() == *it) {
			++it;
		}
		else {
			it = key.begin();
		}
	}
	if (it == key.end()) {
		while (file.get() != '[');
		found = true;
	}
	return found;
}

bool toNextItem(std::wfstream& file) {
	bool found = false;
	int bracket_counter = 0;
	wchar_t element;
	while (!file.eof()) {
		element = file.get();
		if (element == '{') {
			++bracket_counter;
		}
		if (element == '}') {
			--bracket_counter;
			if (bracket_counter == 0) {
				if (file.get() == ',') {
					found = true;
				}
				break;
			}
		}
	}
	return found;
}

// Gets parameter for key.
// Resets file reader pointer if found, otherwise returns empty string and sets pointer to end of file.
std::wstring getParameter(std::wfstream& file, std::wstring_view key) {
	auto initial_pos = file.tellg();
	auto it = key.begin();
	// Find exact match with key
	while (!file.eof() && it != key.end()) {
		if (file.get() == *it) {
			++it;
		}
		else {
			it = key.begin();
		}
	}
	std::wstring output;
	// Key found
	if (it == key.end()) {
		wchar_t c = file.get();
		while (c == '\"' || c == ':') {
			c = file.get();
		}
		while (c != '\"' && c != ',' && c != '}') {
			output += c;
			c = file.get();
		}
	}
	file.seekg(initial_pos);
	return output;
}

bool checkIllform(std::wfstream& file) {
	auto start_pos = file.tellg();
	int bracket_pairs = 0;
	while (!file.eof()) {
		wchar_t c = file.get();
		if (c == L'{') {
			bracket_pairs += 1;
		}
		if (c == L'}') {
			bracket_pairs -= 1;
		}
	}
	file.seekg(start_pos);
	return (bracket_pairs != 0);
}

std::wstring getStringFromFile(std::wfstream& file) {
	std::wstring output;
	while (!file.eof()) {
		output += file.get();
	}
	output.pop_back();
	return output;
}