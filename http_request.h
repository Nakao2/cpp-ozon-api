#pragma once

#include <cpr/cpr.h>
#include <string>
#include <fstream>
#include <chrono>
#include <thread>

#include "json.hpp"
#include "date_processing.h"
#include "json_processing.h"

using namespace std::string_literals;
using namespace std::chrono_literals;

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

// Overflows are possible with too small/large numbers for int64
int64_t SViewToInt(std::string_view source) {
    int64_t output = 0;
    if (source.size() == 0) {
        throw std::invalid_argument("");
    }
    int8_t pos_neg_mult = 1;
    size_t distance_from_new_end = 0;
    if (*source.begin() == '-' && source.size() > 1) {
        pos_neg_mult = -1;
        ++distance_from_new_end;
    }
    int64_t multiplier = 1;
    for (auto pos = source.rbegin(); std::distance(pos, source.rend()) != distance_from_new_end; ++pos) {
        if (*pos < '0' || *pos > '9') {
            throw std::invalid_argument("");
        }
        else {
            output += int64_t(*pos - '0') * multiplier;
            multiplier *= 10;
        }
    }
    return output * pos_neg_mult;
}
// Overflows are possible with too small/large numbers for int64
int64_t SViewToInt(std::wstring_view source) {
    int64_t output = 0;
    if (source.size() == 0) {
        throw std::invalid_argument("");
    }
    int8_t pos_neg_mult = 1;
    size_t distance_from_new_end = 0;
    if (*source.begin() == L'-' && source.size() > 1) {
        pos_neg_mult = -1;
        ++distance_from_new_end;
    }
    int64_t multiplier = 1;
    for (auto pos = source.rbegin(); std::distance(pos, source.rend()) != distance_from_new_end; ++pos) {
        if (*pos < L'0' || *pos > L'9') {
            throw std::invalid_argument("");
        }
        else {
            output += int64_t(*pos - L'0') * multiplier;
            multiplier *= 10;
        }
    }
    return output * pos_neg_mult;
}

bool requestStockData(const std::pair<std::string, std::string>& credentials) {
    std::fstream stock_file("resources/runtime/stock_data.txt"s, std::ios::trunc | std::ios::out);
	std::string json_input;
    json_input += '{';
    json_input += "\"limit\": 1000,\"offset\": 0,\"warehouse_type\": \"NOT_EXPRESS_DARK_STORE\""s;
    json_input += '}';
    auto r = cpr::Post(cpr::Url{ "api-seller.ozon.ru/v2/analytics/stock_on_warehouses"s },
                       cpr::Header{ {"Client-Id"s, credentials.first} },
                       cpr::Header{ {"Api-Key"s, credentials.second} },
                       cpr::Body{ json_input });
    stock_file << r.text;
    return (r.status_code == 200);
}

bool requestOrdersData(const std::pair<std::string, std::string>& credentials,
                       const std::chrono::time_point<std::chrono::system_clock>& time_from, 
                       const std::chrono::time_point<std::chrono::system_clock>& time_to) {

    std::fstream orders_file("resources/runtime/orders_data.txt"s, std::ios::trunc | std::ios::out);

    std::string from_date = formDateString(time_from);
    std::string to_date = formDateString(time_to);

    nlohmann::json json_obj;
    int offset = 0;
    json_obj["dir"s] = "ASC"s;
    json_obj["filter"s]["since"s] = from_date;
    json_obj["filter"s]["to"s] = to_date;
    json_obj["limit"s] = 1000;
    json_obj["offset"s] = offset;
    json_obj["translit"s] = true;
    json_obj["with"s]["analytics_data"s] = true;
    json_obj["with"s]["financial_data"s] = false;

    cpr::Response r;
    do {
        r = cpr::Post(cpr::Url{ "api-seller.ozon.ru/v2/posting/fbo/list"s },
                      cpr::Header{ {"Client-Id"s, credentials.first} },
                      cpr::Header{ {"Api-Key"s, credentials.second} },
                      cpr::Body{ nlohmann::to_string(json_obj) });
        orders_file << r.text;
        offset += 1000;
        json_obj["offset"s] = offset;
    } while (countElements(r.text) >= 1000);
    return (r.status_code == 200);
}

// Assumes a single reply contains all the information
bool requestEnRouteStock(const std::pair<std::string, std::string>& credentials) {
    std::fstream output_file("resources/runtime/en_route_data.txt"s, std::ios::out | std::ios::trunc);
    // First request - All order ids
    nlohmann::json json_obj;
    json_obj["filter"s]["states"s] += "ORDER_STATE_DATA_FILLING"s;
    json_obj["filter"s]["states"s] += "ORDER_STATE_READY_TO_SUPPLY"s;
    json_obj["filter"s]["states"s] += "ORDER_STATE_ACCEPTED_AT_SUPPLY_WAREHOUSE"s;
    json_obj["filter"s]["states"s] += "ORDER_STATE_IN_TRANSIT"s;
    json_obj["filter"s]["states"s] += "ORDER_STATE_ACCEPTANCE_AT_STORAGE_WAREHOUSE"s;
    json_obj["filter"s]["states"s] += "ORDER_STATE_REPORTS_CONFIRMATION_AWAITING"s;
    json_obj["paging"s]["from_supply_order_id"s] = 0;
    json_obj["paging"s]["limit"s] = 100;
    cpr::Response r = cpr::Post(cpr::Url{ "api-seller.ozon.ru/v2/supply-order/list"s },
                                cpr::Header{ {"Client-Id"s, credentials.first} },
                                cpr::Header{ {"Api-Key"s, credentials.second} },
                                cpr::Body{ nlohmann::to_string(json_obj) });;

    // Second request - All orders data
    JsonReader doc(r.text);
    doc.setReader("supply_order_id"s);
    std::vector<std::string_view> order_ids = doc.getValues();
    if (order_ids.empty()) {
        output_file << '-'; // Put 'empty' flag in the output file
        return true;
    }
    json_obj.clear();
    for (std::string_view order_id : order_ids) {
        json_obj["order_ids"s] += SViewToInt(order_id);
    }
     r = cpr::Post(cpr::Url{ "api-seller.ozon.ru/v2/supply-order/get"s },
                   cpr::Header{ {"Client-Id"s, credentials.first} },
                   cpr::Header{ {"Api-Key"s, credentials.second} },
                   cpr::Body{ nlohmann::to_string(json_obj) });;

     doc.setJson(r.text);
     doc.setReader("warehouses"s);
     std::unordered_map<std::string_view, std::string_view> warehouseid_to_name;
     while (doc.toNextItem()) {
         warehouseid_to_name[doc.getValue("warehouse_id"s)] = doc.getValue("name"s);
     }
     doc.goBegin();
     doc.setReader("orders"s);
     std::unordered_map<std::string, std::string> bundles_to_warehouse;
     while (doc.toNextItem()) {
         doc.setReader("supplies"s);
         while (doc.toNextItem()) {
             bundles_to_warehouse[std::string(doc.getValue("bundle_id"s))] = std::string(warehouseid_to_name[doc.getValue("storage_warehouse_id"s)]);
         }
         doc.goUp();
     }

     // Third series of requests - Get item data from EACH bundle ids, which are recieved in order data
     std::unordered_map<std::string, std::unordered_map<std::string, size_t>> warehouse_to_sku_to_num;
     size_t sent_requests = 2;
     for (const auto& b_to_w : bundles_to_warehouse) {
         if (sent_requests % 3 == 0) {
             std::this_thread::sleep_for(1s); // Ozon server allows for no more than 3 requests per second
         }
         ++sent_requests;
         json_obj.clear();
         json_obj["bundle_ids"s] += b_to_w.first;
         json_obj["limit"s] = 100;
         r = cpr::Post(cpr::Url{ "api-seller.ozon.ru/v1/supply-order/bundle"s },
                       cpr::Header{ {"Client-Id"s, credentials.first} },
                       cpr::Header{ {"Api-Key"s, credentials.second} },
                       cpr::Body{ nlohmann::to_string(json_obj) });;
         doc.setJson(r.text);
         doc.setReader("items"s);
         auto& sku_to_num = warehouse_to_sku_to_num[b_to_w.second];
         while (doc.toNextItem()) {
             auto [it, _] = sku_to_num.insert({ std::string(doc.getValue("sku"s)), 0 });
             it->second += SViewToInt(doc.getValue("quantity"s));
         }
     }

     // Output into a file
     json_obj.clear();
     for (const auto& warehouse_to_sku : warehouse_to_sku_to_num) {
         for (const auto& sku_to_num : warehouse_to_sku.second) {
             nlohmann::json json_tmp;
             json_tmp["warehouse"s] = warehouse_to_sku.first;
             json_tmp["sku"s] = sku_to_num.first;
             json_tmp["quantity"s] = sku_to_num.second;
             json_obj["result"s] += json_tmp;
         }
     }
     output_file << nlohmann::to_string(json_obj);
     return true; 
}