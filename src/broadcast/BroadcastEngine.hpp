#pragma once

#include <string>
#include <json/json.h>

#include "network/MainnetDiscovery.hpp"
#include "l3/NetworkBinding.h"

class BroadcastEngine {
public:
    static void emit(const std::string& type,
                     const std::string& version,
                     const Json::Value& payload);

    static void bind(ailee::l3::NetworkBinding* binding);
    static void bindDiscovery(MainnetDiscovery* discovery);

private:
    static ailee::l3::NetworkBinding* net;
    static MainnetDiscovery* discovery;
};

