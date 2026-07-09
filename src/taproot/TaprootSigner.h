#pragma once

#include "taproot/TaprootTxBuilder.h"
#include <array>
#include <string>
#include <vector>
#include <cstdint>

namespace ailee {
namespace taproot {

// Deterministic signature result
struct TaprootSignatureResult {
    bool success;
    std::array<uint8_t, 64> signature;   // BIP340 Schnorr signature
    std::string error;                   // empty on success
};

// Deterministic signing function
TaprootSignatureResult sign_taproot_input(
    const Tx& tx,
    size_t input_index,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& taproot_output_key,
    const std::array<uint8_t, 32>& privkey,
    uint64_t value_sats
);

TaprootSignatureResult sign_anchor_commit_input(
    const Tx& tx,
    size_t input_index,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& leaf_hash,
    uint64_t value_sats,
    const std::array<uint8_t, 32>& privkey
);

// Attaches the Taproot witness to the transaction
bool attach_taproot_witness(
    Tx& tx,
    size_t input_index,
    const std::array<uint8_t, 64>& signature
);

// Builds the final signed Taproot transaction
Tx build_signed_taproot_tx(
    const Tx& unsigned_tx,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& taproot_output_key,
    const std::array<uint8_t, 32>& privkey,
    uint64_t value_sats
);

// Deterministically signs an unsigned anchor-commit transaction
std::vector<uint8_t> sign_anchor_commit_tx(
    const std::vector<uint8_t>& unsigned_tx,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& leaf_hash,
    uint64_t value_sats,
    const std::array<uint8_t, 32>& privkey
);

} // namespace taproot
} // namespace ailee
