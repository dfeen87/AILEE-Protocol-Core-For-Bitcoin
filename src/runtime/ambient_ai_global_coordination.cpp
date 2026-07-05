#include "ambient_ai_global_coordination.hpp"
#include <openssl/sha.h>

namespace ailee {
namespace runtime {

std::vector<uint8_t> AmbientCoordinationHint::serialize() const {
    std::vector<uint8_t> out;
    out.insert(out.end(), peerId.begin(), peerId.end());

    auto push64 = [&out](uint64_t val) {
        for (int i = 7; i >= 0; --i) {
            out.push_back((val >> (i * 8)) & 0xFF);
        }
    };

    auto push32 = [&out](uint32_t val) {
        for (int i = 3; i >= 0; --i) {
            out.push_back((val >> (i * 8)) & 0xFF);
        }
    };

    push64(static_cast<uint64_t>(energyScore)); // avoid negatives
    push32(routingPriorityMultiplier);
    push32(relayPriorityMultiplier);

    return out;
}

Hash256 AmbientCoordinationState::stateHash() const {
    std::vector<uint8_t> out;
    for (const auto& hint : networkHints) {
        auto ser = hint.serialize();
        out.insert(out.end(), ser.begin(), ser.end());
    }

    Hash256 result;
    SHA256(out.data(), out.size(), result.data());
    return result;
}

std::vector<AmbientCoordinationHint> AmbientCoordinationFabric::generateHintsFromEnergy(
    const std::vector<AmbientCoordinationHint>& previousHints
) const {
    // Stub implementation returning input
    return previousHints;
}

AmbientCoordinationState AmbientCoordinationFabric::updateCoordinationState(
    const std::vector<AmbientCoordinationHint>& newHints,
    uint64_t logicalTimestamp
) {
    AmbientCoordinationState state;
    state.currentLogicalTimestamp = logicalTimestamp;
    state.networkHints = newHints;
    return state;
}

}
}
