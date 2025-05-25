#pragma once

#include <cpr/cpr.h>
#include <string>
#include <fstream>
#include <chrono>

#include "json.hpp"
#include "date_processing.h"

int countElements(const std::string& source) {
    int output = 0;
    int unclosed_brackets = 0;
    for (char c : source) {
        if (c == '{') {
            unclosed_brackets += 1;
        }
        if (c == '}') {
            unclosed_brackets -= 1;
            if (unclosed_brackets == 1) {
                ++output;
            }
        }
    }
    return output;
}

bool requestStockData() {
    std::fstream stock_file("resources/stock_data.txt"s, std::ios::trunc | std::ios::out);
	std::string json_input;
    json_input += '{';
    json_input += "\"limit\": 1000,\"offset\": 0,\"warehouse_type\": \"NOT_EXPRESS_DARK_STORE\""s;
    json_input += '}';
    auto r = cpr::Post(cpr::Url{ "api-seller.ozon.ru/v2/analytics/stock_on_warehouses"s },
                       cpr::Header{ {"Client-Id"s, "Placeholder_Id"s} },
                       cpr::Header{ {"Api-Key"s, "Placeholder_Key"s} },
                       cpr::Body{ json_input });
    stock_file << r.text;
    return (r.status_code == 200);
}

// Add some throws for incorrect input
bool requestOrdersData(const std::string& file_name, const std::chrono::time_point<std::chrono::system_clock>& time_from, 
                       const std::chrono::time_point<std::chrono::system_clock>& time_to) {

    std::fstream orders_file(file_name, std::ios::trunc | std::ios::out);

    std::string from_date = formDateString(time_from);
    std::string to_date = formDateString(time_to);

    nlohmann::json json_obj;
    int offset = 0;
    json_obj["dir"s] = "ASC"s;
    json_obj["filter"s]["since"s] = from_date;
    json_obj["filter"s]["status"s] = "delivered"s;
    json_obj["filter"s]["to"s] = to_date;
    json_obj["limit"s] = 1000;
    json_obj["offset"s] = offset;
    json_obj["translit"s] = true;
    json_obj["with"s]["analytics_data"s] = true;
    json_obj["with"s]["financial_data"s] = false;

    cpr::Response r;
    do {
        r = cpr::Post(cpr::Url{ "api-seller.ozon.ru/v2/posting/fbo/list"s },
            cpr::Header{ {"Client-Id"s, "Placeholder_Id"s} },
            cpr::Header{ {"Api-Key"s, "Placeholder_Key"s} },
            cpr::Body{ nlohmann::to_string(json_obj) });
        orders_file << r.text;
        offset += 1000;
        json_obj["offset"s] = offset;
    } while (countElements(r.text) >= 1000);
    return (r.status_code == 200);
}