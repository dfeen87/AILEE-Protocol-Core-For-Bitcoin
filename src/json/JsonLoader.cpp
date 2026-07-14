#include "JsonLoader.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

Json::Value JsonLoader::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("JsonLoader: Failed to open file: " + path);
    }

    Json::Value root;
    file >> root;
    return root;
}

Json::Value JsonLoader::loadFromString(const std::string& jsonStr) {
    std::stringstream ss(jsonStr);
    Json::Value root;
    ss >> root;
    return root;
}

