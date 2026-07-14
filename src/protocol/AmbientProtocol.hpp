#pragma once

#include <string>
#include <json/json.h>

// Forward declaration of ProtocolFrame
struct ProtocolFrame;

// ---------------------------------------------------------
// AmbientProtocol
// ---------------------------------------------------------
// This class represents the high-level protocol interface
// for Ambient AI. It provides basic hooks for processing
// inbound protocol frames and generating outbound ones.
// ---------------------------------------------------------
class AmbientProtocol {
public:
    AmbientProtocol() = default;

    // Process an inbound protocol frame (activation, swarm, etc.)
    void processInboundFrame(const ProtocolFrame& frame);

    // Generate a simple protocol response (placeholder)
    Json::Value buildProtocolResponse(const std::string& type);
};
