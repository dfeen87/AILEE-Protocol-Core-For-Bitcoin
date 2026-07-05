#include <openssl/sha.h>
#include "ambient_ai_sensing_engine.hpp"

namespace ailee {
namespace ambient_node {

std::vector<uint8_t> AmbientSignalBucket::serialize() const {
    std::vector<uint8_t> out;
    out.push_back(static_cast<uint8_t>(power));
    out.push_back(static_cast<uint8_t>(location));
    out.push_back(static_cast<uint8_t>(connectivity));
    out.push_back(static_cast<uint8_t>(timeOfDay));
    return out;
}

std::vector<uint8_t> AmbientSignalSnapshot::hash() const {
    auto bSer = bucket.serialize();
    std::vector<uint8_t> out;
    auto push64 = [&out](uint64_t val) {
        for (int i = 7; i >= 0; --i) {
            out.push_back((val >> (i * 8)) & 0xFF);
        }
    };
    push64(logicalTimestamp);
    out.insert(out.end(), bSer.begin(), bSer.end());

    std::vector<uint8_t> result(SHA256_DIGEST_LENGTH);
    SHA256(out.data(), out.size(), result.data());
    return result;
}

AmbientSignalSnapshot AmbientSensingEngine::sample() const {
    return {0, {PowerState::LOW, LocationClass::UNKNOWN, ConnectivityBand::POOR, TimeOfDayBand::MORNING}};
}

std::vector<AmbientSignalSnapshot> AmbientSensingEngine::getAggregatedWindow() const {
    return {};
}

void AmbientSensingEngine::pruneOldSnapshots(uint64_t currentLogicalTimestamp) {
    (void)currentLogicalTimestamp;
}

}
}
