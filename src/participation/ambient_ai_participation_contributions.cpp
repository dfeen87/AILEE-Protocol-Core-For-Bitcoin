#include <openssl/sha.h>
#include "ambient_ai_participation_contributions.hpp"

namespace ailee {
namespace participation {

std::vector<uint8_t> AmbientContributionBucket::serialize() const {
    std::vector<uint8_t> out;
    out.push_back(powerStateClass);
    out.push_back(connectivityBand);
    out.push_back(activityLevel);
    return out;
}

std::vector<uint8_t> AmbientContributionEvent::serialize() const {
    std::vector<uint8_t> out;
    out.insert(out.end(), eventId.begin(), eventId.end());

    auto push64 = [&out](uint64_t val) {
        for (int i = 7; i >= 0; --i) {
            out.push_back((val >> (i * 8)) & 0xFF);
        }
    };
    push64(logicalTimestamp);

    out.push_back(static_cast<uint8_t>(type));

    auto bSer = bucket.serialize();
    out.insert(out.end(), bSer.begin(), bSer.end());

    return out;
}

Hash256 AmbientContributionEvent::hash() const {
    auto bytes = serialize();
    Hash256 result;
    SHA256(bytes.data(), bytes.size(), result.data());
    return result;
}

}
}
