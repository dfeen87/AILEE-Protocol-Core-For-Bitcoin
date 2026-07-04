#include <nlohmann/json.hpp>
#include <string>
#include <iostream>

int main() {
    std::string data = "{\"score\": 42.5}";
    auto j = nlohmann::json::parse(data);
    std::cout << j["score"].get<double>() << std::endl;
    return 0;
}
