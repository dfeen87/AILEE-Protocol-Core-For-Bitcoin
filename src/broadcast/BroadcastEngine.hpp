#pragma once

#include <string>
#include <json/json.h>

#include "network/MainnetDiscovery.hpp"
#include "network/NetworkBinding.hpp"

// ---------------------------------------------------------
// BroadcastEngine
// ---------------------------------------------------------
// Now supports:
//   - normal broadcast
//   - broadcast to verified mainnet peers
//   - discovery binding
//   - network binding
// ---------------------------------------------------------
class BroadcastEngine {
public:
    // Emit a signed protocol frame (unchanged signature)
    static void emit(const std::string& type,
                     const std::string& version,
                     const Json::Value& payload);

    // Bind network layer (existing behavior)
    static void bind(NetworkBinding* binding);

    // Bind mainnet discovery subsystem (new)
    static void bindDiscovery(MainnetDiscovery* discovery);

private:
    static NetworkBinding* net;
    static MainnetDiscovery* discovery;
};
