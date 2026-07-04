#include "Global_Seven.h"

#include "zk_proofs.h"

#include <iomanip>
#include <sstream>
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <openssl/sha.h>

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

std::vector<uint8_t> createCommitmentBytes(const std::string& payload) {
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
    const auto commitment = createCommitmentBytes(payload);
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
    std::vector<uint8_t> commitment;
    if (!this->commitmentBytes.empty()) {
        commitment = this->commitmentBytes; // Use 96 byte raw commitment if available
    } else {
        commitment = createCommitmentBytes(payload);
    }
    std::vector<uint8_t> script;

    if (!this->tweakedTaprootKey.empty()) {
        // Build Taproot output using the computed tweaked key (Key Path)
        // scriptPubKey for Taproot (SegWit v1): OP_1 <32-byte tweaked key>
        script.push_back(0x51); // OP_1
        script.push_back(0x20); // Push 32 bytes
        script.insert(script.end(), this->tweakedTaprootKey.begin(), this->tweakedTaprootKey.end());

        AnchorPayload payloadResult;
        payloadResult.scriptBytes = script;
        payloadResult.description = describePayload("TAPROOT_KEY_PATH", l2StateRoot, timestampMs,
                                                   recoveryMetadata, payload, commitment);
        return payloadResult;
    }

    // Fallback: Script Path
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

bool AnchorCommitment::computeTweakedKey(const std::vector<uint8_t>& internalPubkey) {
    if (internalPubkey.size() != 32 || this->commitmentBytes.empty()) {
        return false;
    }

    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    if (!ctx) return false;

    secp256k1_xonly_pubkey pubkey;
    if (secp256k1_xonly_pubkey_parse(ctx, &pubkey, internalPubkey.data()) != 1) {
        secp256k1_context_destroy(ctx);
        return false;
    }

    // Hash H(P || commitment)
    std::vector<uint8_t> preimage;
    preimage.insert(preimage.end(), internalPubkey.begin(), internalPubkey.end());
    preimage.insert(preimage.end(), this->commitmentBytes.begin(), this->commitmentBytes.end());

    unsigned char tweak[SHA256_DIGEST_LENGTH];
    SHA256(preimage.data(), preimage.size(), tweak);

    secp256k1_pubkey output_pubkey;
    if (secp256k1_xonly_pubkey_tweak_add(ctx, &output_pubkey, &pubkey, tweak) != 1) {
        secp256k1_context_destroy(ctx);
        return false;
    }

    secp256k1_xonly_pubkey out_xonly;
    if (secp256k1_xonly_pubkey_from_pubkey(ctx, &out_xonly, nullptr, &output_pubkey) != 1) {
        secp256k1_context_destroy(ctx);
        return false;
    }

    unsigned char serialized_xonly[32];
    secp256k1_xonly_pubkey_serialize(ctx, serialized_xonly, &out_xonly);

    this->tweakedTaprootKey = std::vector<uint8_t>(serialized_xonly, serialized_xonly + 32);

    secp256k1_context_destroy(ctx);
    return true;
}

bool AnchorCommitment::validateTaprootCommitment(const std::vector<uint8_t>& tweakedKey,
                                                 const std::vector<uint8_t>& internalPubkey,
                                                 const std::vector<uint8_t>& commitment) {
    if (tweakedKey.size() != 32 || internalPubkey.size() != 32 || commitment.empty()) {
        return false;
    }

    AnchorCommitment temp;
    temp.commitmentBytes = commitment;
    if (!temp.computeTweakedKey(internalPubkey)) {
        return false;
    }

    return temp.tweakedTaprootKey == tweakedKey;
}

} // namespace ailee::global_seven


namespace ailee::global_seven {

std::string TapLeaf::getTapLeafHash() const {
    // In a full implementation, this uses tagged SHA256(TapLeaf)
    // Tag = "TapLeaf"
    // H = SHA256(SHA256(Tag) || SHA256(Tag) || leafVersion || scriptLength || script)
    // For now we mock it deterministically using existing sha256Hex
    std::string toHash = std::to_string(leafVersion) + scriptHex;
    return ailee::zk::sha256Hex(toHash);
}

void TapTree::computeRoot() {
    if (leaves.empty()) {
        rootHash = "";
        return;
    }

    // Simulate MAST construction. We just hash all leaf hashes together for the mock.
    std::string combined = "";
    for (const auto& leaf : leaves) {
        combined += leaf.getTapLeafHash();
    }
    rootHash = ailee::zk::sha256Hex(combined);
}

TapTree AnchorCommitment::buildChallengeResponseTree(const std::string& zkProofHash) const {
    TapTree tree;
    TapLeaf happyLeaf; happyLeaf.leafVersion = 0xc0;
    const std::string happyPayload = l2StateRoot + ":" + zkProofHash;
    const auto happyBytes = createCommitmentBytes(happyPayload);
    std::vector<uint8_t> script1;
    script1.push_back(0x00); script1.push_back(0x63);
    const auto pushedHappy = pushData(happyBytes);
    script1.insert(script1.end(), pushedHappy.begin(), pushedHappy.end());
    script1.push_back(0x68);
    happyLeaf.script = script1; happyLeaf.scriptHex = "happy_path_" + happyPayload;
    tree.leaves.push_back(happyLeaf);
    TapLeaf disputeLeaf; disputeLeaf.leafVersion = 0xc0;
    const std::string disputePayload = "DISPUTE_EXEC:" + l2StateRoot;
    const auto disputeBytes = createCommitmentBytes(disputePayload);
    std::vector<uint8_t> script2;
    script2.push_back(0x00); script2.push_back(0x63);
    const auto pushedDispute = pushData(disputeBytes);
    script2.insert(script2.end(), pushedDispute.begin(), pushedDispute.end());
    script2.push_back(0xa8); script2.push_back(0x68);
    disputeLeaf.script = script2; disputeLeaf.scriptHex = "dispute_path_" + disputePayload;
    tree.leaves.push_back(disputeLeaf);
    tree.computeRoot(); return tree;
}

} // namespace ailee::global_seven
