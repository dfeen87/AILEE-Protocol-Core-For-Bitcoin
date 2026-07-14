#pragma once

#include <string>
#include <json/json.h>

class JsonLoader {
public:
    static Json::Value loadFromFile(const std::string& path);
    static Json::Value loadFromString(const std::string& jsonStr);
};

