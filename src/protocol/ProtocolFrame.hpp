#pragma once

#include <string>
#include <json/json.h>

// ---------------------------------------------------------
// Protocol Frame (V33.2.x)
// ---------------------------------------------------------
struct ProtocolFrame {
    std::string frame_id;       // unique ID for dedup + gossip loop prevention
    std::string type;           // "activation", "swarm", "handshake", etc.
    std::string version;        // protocol version
    std::string node_id;        // sender identity
    uint64_t timestamp;         // unix epoch
    std::string payload;        // serialized JSON payload
    std::string signature;      // optional (added later)
};

// Serialize ProtocolFrame → JSON string
std::string serialize_frame(const ProtocolFrame& pf);
