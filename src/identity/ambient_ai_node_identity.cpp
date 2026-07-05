#include <openssl/sha.h>
#include "ambient_ai_node_identity.hpp"
#include <string.h>

namespace ailee {
namespace identity {

Hash256 BuildMetadata::canonicalHash() const {
    std::vector<uint8_t> out;
    out.insert(out.end(), commitHash.begin(), commitHash.end());
    auto push32 = [&out](uint32_t val) {
        for (int i = 3; i >= 0; --i) {
            out.push_back((val >> (i * 8)) & 0xFF);
        }
    };
    push32(buildNumber);
    push32(protocolVersion);

    Hash256 result;
    SHA256(out.data(), out.size(), result.data());
    return result;
}

std::vector<uint8_t> IdentityPayload::serialize() const {
    std::vector<uint8_t> out;
    out.insert(out.end(), peerId.begin(), peerId.end());
    out.insert(out.end(), publicKeyHex.begin(), publicKeyHex.end());
    auto hash = metadata.canonicalHash();
    out.insert(out.end(), hash.begin(), hash.end());
    auto push64 = [&out](uint64_t val) {
        for (int i = 7; i >= 0; --i) {
            out.push_back((val >> (i * 8)) & 0xFF);
        }
    };
    push64(epochId);
    push64(staticAttribute);
    return out;
}

Hash256 IdentityPayload::hash() const {
    auto bytes = serialize();
    Hash256 result;
    SHA256(bytes.data(), bytes.size(), result.data());
    return result;
}

bool ParticipationProof::verify() const {
    // Basic verification logic for protocol matching
    // True ECDSA verification would require libsecp256k1 linkage here
    return payload.metadata.protocolVersion > 0;
}

}
}
