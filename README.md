A program that recieves remaining stock and recent orders data from Ozon servers.
Prints deliveries recommendation into an Excel-compatible .xls file based on recieved data.
Accounts for seasonality changes and other factors which are read from simple format .txt files.

External libraries used:
1) "nlohmann_json"(included in this project as json.hpp) https://github.com/nlohmann/json
2) "C++ Requests"(not included in this project) https://github.com/libcpr/cpr