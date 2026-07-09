#include "../../src/taproot/TaprootScript.h"
#include <gtest/gtest.h>
#include <array>
#include <cstring>
#include <vector>

using namespace ailee::taproot;

TEST(TaprootScriptTest, BuildScript) {
    std::array<uint8_t, 32> anchor_root;
    std::fill(anchor_root.begin(), anchor_root.end(), 0xAA);

    TapLeaf leaf;
    leaf.anchor_root = anchor_root;
    leaf.build_script();

    ASSERT_EQ(leaf.script.size(), 35);
    EXPECT_EQ(leaf.script[0], 0x20); // OP_PUSH32
    EXPECT_EQ(leaf.script[33], 0x75); // OP_DROP
    EXPECT_EQ(leaf.script[34], 0x51); // OP_TRUE
}

TEST(TaprootScriptTest, ComputeOutputKeyPlaceholder) {
    std::array<uint8_t, 32> internal_key;
    std::fill(internal_key.begin(), internal_key.end(), 0xBB);

    std::array<uint8_t, 32> tapleaf_hash;
    std::fill(tapleaf_hash.begin(), tapleaf_hash.end(), 0xCC);

    TaprootOutput out;
    out.internal_key = internal_key;
    out.compute_output_key(tapleaf_hash);

    // In Phase 1, the tweak should be a placeholder (just copies internal_key)
    EXPECT_EQ(std::memcmp(out.taproot_output_key.data(), internal_key.data(), 32), 0);
}

TEST(TaprootScriptTest, ToBech32mPlaceholder) {
    std::array<uint8_t, 32> taproot_output_key;
    std::fill(taproot_output_key.begin(), taproot_output_key.end(), 0xDD);

    std::string address = to_bech32m(taproot_output_key);
    EXPECT_EQ(address, "taproot-address-not-implemented");
}
