#include "ambient_ai_mesh_routing_engine.hpp"
#include "ambient_ai_mesh_protocol.hpp"

namespace ailee {
namespace ambient_mesh {

std::vector<std::string> AmbientMeshRoutingEngine::selectNextHops(
    const AmbientMeshMessage& message,
    const std::set<NeighborEntry>& activeNeighbors
) const {
    (void)message;
    std::vector<std::string> nextHops;
    // Set is already sorted by energy descending then by peerId, so we just take the top MAX_FAN_OUT_DEGREE
    for (const auto& neighbor : activeNeighbors) {
        if (nextHops.size() >= AmbientMeshLimits::MAX_FAN_OUT_DEGREE) {
            break;
        }
        nextHops.push_back(neighbor.peerId);
    }
    return nextHops;
}

bool AmbientMeshRoutingEngine::validateRouteConstraints(
    const AmbientMeshMessage& message,
    uint32_t currentMessagesPerSecond
) const {
    if (message.header.hopCount > AmbientMeshLimits::MAX_HOP_COUNT) return false;
    if (message.payload.size() > AmbientMeshLimits::MAX_PAYLOAD_SIZE_BYTES) return false;
    if (currentMessagesPerSecond >= AmbientMeshLimits::MAX_MESSAGES_PER_SECOND) return false;
    return true;
}

}
}
