#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <algorithm>

using namespace std::string_literals;


struct FontData {
	std::wstring font_name = L"Calibri";
	size_t size = 11; // Default font size for Excel spreadsheets
	std::wstring font_color = L"#000000"; // rrggbb Black
	bool bold = false;

	std::wstring interior_color; // If empty, default white will be used
};

struct CellData {

	CellData() = default;
	CellData(int source) : is_string(false), value(std::to_wstring(source)) {}
	CellData(double source) : is_string(false), value(std::to_wstring(source)) {}
	CellData(const std::wstring& source) : is_string(true), value(source) {}

	CellData(const std::pair<size_t, std::wstring>& source) :
	style_id(source.first), 
	is_string(true),
	value(source.second) {}

	CellData(const std::pair<size_t, int>& source) :
	style_id(source.first),
	is_string(false),
	value(std::to_wstring(source.second)) {}

	CellData(const std::pair<size_t, double>& source) :
	style_id(source.first),
	is_string(false),
	value(std::to_wstring(source.second)) {}

	CellData& operator=(int source) {
		style_id = 0;
		is_string = false;
		value = std::to_wstring(source);
		return *this;
	}
	CellData& operator=(double source) {
		style_id = 0;
		is_string = false;
		value = std::to_wstring(source);
		return *this;
	}
	CellData& operator=(const std::wstring& source) {
		style_id = 0;
		is_string = true;
		value = source;
		return *this;
	}
	CellData& operator=(const std::pair<size_t, int>& source) {
		style_id = source.first;
		is_string = false;
		value = std::to_wstring(source.second);
		return *this;
	}
	CellData& operator=(const std::pair<size_t, std::wstring>& source) {
		style_id = source.first;
		is_string = true;
		value = source.second;
		return *this;
	}

	size_t style_id = 0;
	bool is_string = true;
	std::wstring value;
};

struct TableRow {

	CellData& operator[](size_t column) {
		if (row.size() < column) {
			row.resize(column);
		}
		return row[column - 1];
	}

	std::vector<CellData> row;
};

struct Sheet {

	TableRow& operator[](size_t row) {
		if (table.size() < row) {
			table.resize(row);
		}
		return table[row - 1];
	}

	std::wstring name;
	std::vector<TableRow> table;
	std::pair<size_t, size_t> filters_range = { 0, 0 };
};

class XLSFormatter {
public:

	XLSFormatter() {
		styles_.push_back({ L"Calibri", 11, L"#000000", false, L"" });
	}

	bool setSheet(const std::wstring& sheet_name) {
		auto it = std::find_if(sheets_.begin(), sheets_.end(), [&sheet_name](const Sheet& sheet) {
			return (sheet_name == sheet.name);
			});
		if (it == sheets_.end()) {
			sheets_.insert(it, { sheet_name, {} });
			current_sheet_id_ = sheets_.size() - 1;
			return false;
		}
		else {
			current_sheet_id_ = std::distance(sheets_.begin(), it);
			return true;
		}
	}

	void addStyle(const FontData& style) {
		styles_.push_back(style);
	}

	void applyFilters(std::pair<size_t, size_t> range) {
		if (!sheets_.empty()) {
			sheets_[current_sheet_id_].filters_range = range;
		}
	}

	TableRow& operator[](size_t row) {
		return sheets_[current_sheet_id_][row];
	}

	std::wstring to_string() {
		std::wstring output;

		// Default xls data to make it readable with Excel
		output += L"<?xml version=\"1.0\"?>";
		output += L"<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\" ";
		output += L"xmlns:o=\"urn:schemas-microsoft-com:office:office\" ";
		output += L"xmlns:x=\"urn:schemas-microsoft-com:office:excel\" ";
		output += L"xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\" ";
		output += L"xmlns:html=\"http://www.w3.org/TR/REC-html40\">";


		output += L"<Styles>";
		for (int i = 0; i < styles_.size(); ++i) {
			output += L"<Style ss:ID=\"";
			output += std::to_wstring(i);
			output += L"\"><Font ss:FontName=\"";
			output += styles_[i].font_name;
			output += L"\" ss:Size=\"";
			output += std::to_wstring(styles_[i].size);
			output += L"\" ss:Color=\"";
			output += styles_[i].font_color;
			output += L"\" ss:Bold=\"";
			output += std::to_wstring(styles_[i].bold);
			output += L"\"/>";
			if (!styles_[i].interior_color.empty()) {
				output += L"<Interior ss:Color=\"";
				output += styles_[i].interior_color;
				output += L"\" ss:Pattern=\"Solid\"/>";
			}
			output += L"</Style>";
		}
		output += L"</Styles>";

		// Output for every sheet
		for (const Sheet& sheet : sheets_) {
			output += L"<Worksheet ss:Name=\"";
			output += sheet.name;
			output += L"\"><Table>";

			// Column formatting data preceds the actual cell data
			// Look for the largest word in a column and format it accordingly
			if (!sheet.table.empty()) {
				bool rhs_reached = false;
				for (size_t column_id = 0; !rhs_reached; ++column_id) {
					size_t max_size = 6; // Magic number to get (6 * 8) default Excel column size, see next comment
					rhs_reached = true;
					for (const TableRow& t_row : sheet.table) {
						if (column_id < t_row.row.size()) {
							max_size = max_size < t_row.row[column_id].value.size() / 2 ? // Russian locale shenanigans
								       t_row.row[column_id].value.size() / 2 : max_size;
							rhs_reached = false;
						}
					}
					output += L"<Column ss:AutoFitWidth=\"0\" ss:Width=\"";
					output += std::to_wstring(max_size * 8); // 8 per character assures word of any size will fit in a column(Calibri font)
					output += L"\"/>";
				}
			}

			// Cell data output
			for (const TableRow& t_row : sheet.table) {
				output += L"<Row>";
				for (const CellData& cell : t_row.row) {
					output += L"<Cell ss:StyleID=\"";
					output += std::to_wstring(cell.style_id);
					output += L"\"><Data ss:Type=\"";
					if (cell.is_string) {
						output += L"String";
					}
					else {
						output += L"Number";
					}
					output += L"\">";
					output += cell.value;
					output += L"</Data></Cell>";
				}
				output += L"</Row>";
			}
			output += L"</Table>";
			if (sheet.filters_range.first != 0) {
				output += L"<AutoFilter x:Range=\"R1C";
				output += std::to_wstring(sheet.filters_range.first);
				output += L":R1C";
				output += std::to_wstring(sheet.filters_range.second);
				output += L"\" xmlns=\"urn:schemas-microsoft-com:office:excel\">";
				output += L"</AutoFilter>";
			}
			output += L"</Worksheet>";
		}
		output += L"</Workbook>";
		return output;
	}

private:
	std::vector<Sheet> sheets_;
	size_t current_sheet_id_ = 0;
	std::vector<FontData> styles_;
};