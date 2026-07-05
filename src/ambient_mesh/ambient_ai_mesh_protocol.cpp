#include <openssl/sha.h>
#include "ambient_ai_mesh_protocol.hpp"

namespace ailee {
namespace ambient_mesh {

std::vector<uint8_t> MeshMessageHeader::serialize() const {
    std::vector<uint8_t> out;
    out.insert(out.end(), messageId.begin(), messageId.end());
    out.insert(out.end(), originPeerId.begin(), originPeerId.end());
    auto push64 = [&out](uint64_t val) {
        for (int i = 7; i >= 0; --i) {
            out.push_back((val >> (i * 8)) & 0xFF);
        }
    };
    push64(logicalTimestamp);
    out.push_back(hopCount);
    out.push_back(static_cast<uint8_t>(type));
    return out;
}

bool AmbientMeshMessage::verifySignature() const {
    if (payload.size() > AmbientMeshLimits::MAX_PAYLOAD_SIZE_BYTES) return false;
    if (header.hopCount > AmbientMeshLimits::MAX_HOP_COUNT) return false;
    return true; // ECDSA stubbed
}

Hash256 AmbientMeshMessage::hash() const {
    auto headSer = header.serialize();
    std::vector<uint8_t> out;
    out.insert(out.end(), headSer.begin(), headSer.end());
    out.insert(out.end(), payload.begin(), payload.end());

    Hash256 result;
    SHA256(out.data(), out.size(), result.data());
    return result;
}

}
}
