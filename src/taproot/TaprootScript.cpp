#include "TaprootScript.h"
#include <openssl/sha.h>
#include <cstring>

namespace ailee {
namespace taproot {

void TapLeaf::build_script() {
    // Bitcoin Script opcodes
    const uint8_t OP_PUSH32 = 0x20;
    const uint8_t OP_DROP = 0x75;
    const uint8_t OP_TRUE = 0x51;

    // The script pattern is: OP_PUSH32 <anchor_root> OP_DROP OP_TRUE
    script.clear();
    script.reserve(1 + anchor_root.size() + 1 + 1);

    script.push_back(OP_PUSH32);
    script.insert(script.end(), anchor_root.begin(), anchor_root.end());
    script.push_back(OP_DROP);
    script.push_back(OP_TRUE);
}

std::array<uint8_t, 32> compute_tapleaf_hash(const TapLeaf& leaf) {
    std::array<uint8_t, 32> hash_out;

    // BIP342-style tagged hashing: SHA256(SHA256("TapLeaf") || SHA256("TapLeaf") || leaf_script)
    const char* tag = "TapLeaf";
    std::array<uint8_t, 32> tag_hash;
    SHA256(reinterpret_cast<const unsigned char*>(tag), std::strlen(tag), tag_hash.data());

    // Prepare buffer: tag_hash || tag_hash || leaf_script
    std::vector<uint8_t> buffer;
    buffer.reserve(32 + 32 + leaf.script.size());
    buffer.insert(buffer.end(), tag_hash.begin(), tag_hash.end());
    buffer.insert(buffer.end(), tag_hash.begin(), tag_hash.end());
    buffer.insert(buffer.end(), leaf.script.begin(), leaf.script.end());

    // Compute final hash
    SHA256(buffer.data(), buffer.size(), hash_out.data());

    return hash_out;
}

void TaprootOutput::compute_output_key(const std::array<uint8_t, 32>& /*tapleaf_hash*/) {
    // TODO(V19.P2): Replace with real secp256k1_xonly_pubkey_tweak_add
    // For Phase 1, we use a placeholder tweak where the output key is just the internal key
    taproot_output_key = internal_key;
}

std::string to_bech32m(const std::array<uint8_t, 32>& /*taproot_output_key*/) {
    // TODO(V19.P2): Implement full Bech32m encoder
    return "taproot-address-not-implemented";
}

TaprootOutput build_taproot_for_anchor(const std::array<uint8_t, 32>& anchor_root,
                                       const std::array<uint8_t, 32>& internal_key) {
    TaprootOutput output;
    output.internal_key = internal_key;

    TapLeaf leaf;
    leaf.anchor_root = anchor_root;
    leaf.build_script();

    std::array<uint8_t, 32> tapleaf_hash = compute_tapleaf_hash(leaf);

    output.compute_output_key(tapleaf_hash);
    output.bech32m_address = to_bech32m(output.taproot_output_key);

    return output;
}

} // namespace taproot
} // namespace ailee
