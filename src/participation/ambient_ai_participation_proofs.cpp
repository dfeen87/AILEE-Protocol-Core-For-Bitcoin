#include <openssl/sha.h>
#include "ambient_ai_participation_proofs.hpp"

namespace ailee {
namespace participation {

std::vector<uint8_t> AmbientParticipationProofHeader::serialize() const {
    std::vector<uint8_t> out;
    out.insert(out.end(), publicKeyHex.begin(), publicKeyHex.end());

    auto push64 = [&out](uint64_t val) {
        for (int i = 7; i >= 0; --i) {
            out.push_back((val >> (i * 8)) & 0xFF);
        }
    };
    push64(epochId);

    out.insert(out.end(), summaryHash.begin(), summaryHash.end());

    // cast to uint64 for serialization to avoid negatives issues
    push64(static_cast<uint64_t>(derivedEnergyScore));
    return out;
}

Hash256 AmbientParticipationProofHeader::hash() const {
    auto bytes = serialize();
    Hash256 result;
    SHA256(bytes.data(), bytes.size(), result.data());
    return result;
}

bool AmbientParticipationProof::verify() const {
    // 1. Verifies ECDSA signature - stubbed
    // 2. Asserts summary.hash() == header.summaryHash
    if (summary.hash() != header.summaryHash) return false;
    // 3. Asserts derivedEnergyScore >= 0
    if (header.derivedEnergyScore < 0) return false;
    return true;
}

}
}
