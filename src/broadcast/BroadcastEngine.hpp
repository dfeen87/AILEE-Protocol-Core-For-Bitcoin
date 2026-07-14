#pragma once

#include <string>
#include <json/json.h>

#include "network/MainnetDiscovery.hpp"
#include "network/NetworkBinding.hpp"

class BroadcastEngine {
public:
    static void emit(const std::string& type,
                     const std::string& version,
                     const Json::Value& payload);

    static void bind(NetworkBinding* binding);
    static void bindDiscovery(MainnetDiscovery* discovery);

private:
    static NetworkBinding* net;
    static MainnetDiscovery* discovery;
};

