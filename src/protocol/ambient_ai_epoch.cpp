#include "ambient_ai_epoch.hpp"
#include <openssl/sha.h>
#include <string.h>

namespace ailee {
namespace protocol {

Hash256 AmbientAIAnchoringEpoch::computeEpochCommitment() const {
    // The final payload to be serialized and committed into the Taproot tree.
    std::vector<uint8_t> out;

    auto push64 = [&out](uint64_t val) {
        for (int i = 7; i >= 0; --i) {
            out.push_back((val >> (i * 8)) & 0xFF);
        }
    };

    push64(epochId);
    push64(startBitcoinHeight);
    push64(endBitcoinHeight);

    out.insert(out.end(), l2StateRoot.begin(), l2StateRoot.end());
    out.insert(out.end(), energyLedgerRoot.begin(), energyLedgerRoot.end());
    out.insert(out.end(), participationRoot.begin(), participationRoot.end());
    out.insert(out.end(), ambientEventRoot.begin(), ambientEventRoot.end());
    out.insert(out.end(), protocolBuildMetadata.begin(), protocolBuildMetadata.end());

    Hash256 result;
    SHA256(out.data(), out.size(), result.data());
    return result;
}

}
}
