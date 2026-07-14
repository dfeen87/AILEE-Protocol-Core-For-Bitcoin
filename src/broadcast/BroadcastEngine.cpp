#include "BroadcastEngine.hpp"
#include "NetworkBinding.hpp"
#include <iostream>

// Static network binding pointer
NetworkBinding* BroadcastEngine::net = nullptr;

void BroadcastEngine::bind(NetworkBinding* binding)
{
    net = binding;

    std::cout << "[BroadcastEngine] Network binding attached" << std::endl;
}

void BroadcastEngine::emit(const std::string& type,
                           const std::string& version,
                           const Json::Value& payload)
{
    // If no network binding is attached, fall back to stdout
    if (!net) {
        std::cout << "[BroadcastEngine] No network binding available" << std::endl;
        std::cout << "  Type: " << type << std::endl;
        std::cout << "  Version: " << version << std::endl;
        std::cout << "  Payload: " << payload.toStyledString() << std::endl;
        return;
    }

    // Build a protocol broadcast frame
    Json::Value frame;
    frame["type"] = type;
    frame["version"] = version;
    frame["payload"] = payload;

    // Forward to the network layer
    net->broadcast(frame);
}
