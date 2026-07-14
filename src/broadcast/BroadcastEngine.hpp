#pragma once

#include <string>
#include <json/json.h>

class BroadcastEngine {
public:
    static void emit(const std::string& type,
                     const std::string& version,
                     const Json::Value& payload);
};

