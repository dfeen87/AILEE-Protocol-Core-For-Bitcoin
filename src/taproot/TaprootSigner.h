#pragma once

#include "taproot/TaprootTxBuilder.h"
#include <array>
#include <string>
#include <vector>
#include <cstdint>

namespace ailee {
namespace taproot {

// ============================================================================
// Deterministic signature result
// ============================================================================
struct TaprootSignatureResult {
    bool success;
    std::array<uint8_t, 64> signature;   // BIP340 Schnorr signature
    std::string error;                   // empty on success
};

// ============================================================================
// Key‑path Taproot signing (NO script tree)
// Used for generic P2TR outputs without leaf_hash.
// ============================================================================
TaprootSignatureResult sign_taproot_input(
    const Tx& tx,
    size_t input_index,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& taproot_output_key,
    const std::array<uint8_t, 32>& privkey,
    uint64_t value_sats
);

// Builds a fully signed key‑path Taproot transaction.
// (Not used for anchor‑commit.)
Tx build_signed_taproot_tx(
    const Tx& unsigned_tx,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& taproot_output_key,
    const std::array<uint8_t, 32>& privkey,
    uint64_t value_sats
);

// ============================================================================
// Anchor‑commit Taproot signing (script tree present)
// Uses TapTweak(internal_key || leaf_hash).
// ============================================================================
TaprootSignatureResult sign_anchor_commit_input(
    const Tx& tx,
    size_t input_index,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& leaf_hash,
    uint64_t value_sats,
    const std::array<uint8_t, 32>& privkey
);

// Deterministically signs an unsigned anchor‑commit transaction.
// Produces final signed transaction bytes.
std::vector<uint8_t> sign_anchor_commit_tx(
    const std::vector<uint8_t>& unsigned_tx,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& leaf_hash,
    uint64_t value_sats,
    const std::array<uint8_t, 32>& privkey
);

// ============================================================================
// Witness attachment (shared by both key‑path and anchor‑commit)
// ============================================================================
bool attach_taproot_witness(
    Tx& tx,
    size_t input_index,
    const std::array<uint8_t, 64>& signature
);

} // namespace taproot
} // namespace ailee
