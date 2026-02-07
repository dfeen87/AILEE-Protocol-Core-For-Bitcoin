#include "Global_Seven.h"

#include "zk_proofs.h"

#include <iomanip>
#include <sstream>

namespace ailee::global_seven {

namespace {

std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    if (hex.size() % 2 != 0) {
        return bytes;
    }
    bytes.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        unsigned int value = 0;
        std::istringstream iss(hex.substr(i, 2));
        iss >> std::hex >> value;
        bytes.push_back(static_cast<uint8_t>(value));
    }
    return bytes;
}

std::vector<uint8_t> commitmentBytes(const std::string& payload) {
    if (payload.size() <= 80) {
        return std::vector<uint8_t>(payload.begin(), payload.end());
    }
    const std::string hashHex = ailee::zk::sha256Hex(payload);
    const std::vector<uint8_t> hashBytes = hexToBytes(hashHex);
    std::vector<uint8_t> tagged;
    static const char kTag[] = "AILEE";
    tagged.insert(tagged.end(), kTag, kTag + sizeof(kTag) - 1);
    tagged.insert(tagged.end(), hashBytes.begin(), hashBytes.end());
    return tagged;
}

std::vector<uint8_t> pushData(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> script;
    if (data.size() <= 75) {
        script.push_back(static_cast<uint8_t>(data.size()));
    } else {
        script.push_back(0x4c);
        script.push_back(static_cast<uint8_t>(data.size()));
    }
    script.insert(script.end(), data.begin(), data.end());
    return script;
}

std::string describePayload(const std::string& kind,
                            const std::string& l2StateRoot,
                            uint64_t timestampMs,
                            const std::string& recoveryMetadata,
                            const std::string& payload,
                            const std::vector<uint8_t>& commitment) {
    std::ostringstream oss;
    oss << "Anchor type=" << kind
        << " l2StateRoot=" << l2StateRoot
        << " timestampMs=" << timestampMs
        << " recoveryMetadata=" << recoveryMetadata
        << " payloadBytes=" << payload.size()
        << " commitmentBytes=" << commitment.size();
    return oss.str();
}

} // namespace

AnchorCommitment::AnchorPayload AnchorCommitment::buildOpReturnPayload() const {
    const auto commitment = commitmentBytes(payload);
    std::vector<uint8_t> script;
    script.push_back(0x6a);
    const auto pushed = pushData(commitment);
    script.insert(script.end(), pushed.begin(), pushed.end());

    AnchorPayload payloadResult;
    payloadResult.scriptBytes = script;
    payloadResult.description = describePayload("OP_RETURN", l2StateRoot, timestampMs,
                                               recoveryMetadata, payload, commitment);
    return payloadResult;
}

AnchorCommitment::AnchorPayload AnchorCommitment::buildTaprootCommitment() const {
    const auto commitment = commitmentBytes(payload);
    std::vector<uint8_t> script;
    script.push_back(0x00); // OP_FALSE
    script.push_back(0x63); // OP_IF
    const auto pushed = pushData(commitment);
    script.insert(script.end(), pushed.begin(), pushed.end());
    script.push_back(0x68); // OP_ENDIF

    AnchorPayload payloadResult;
    payloadResult.scriptBytes = script;
    payloadResult.description = describePayload("TAPSCRIPT", l2StateRoot, timestampMs,
                                               recoveryMetadata, payload, commitment);
    return payloadResult;
}

} // namespace ailee::global_seven

