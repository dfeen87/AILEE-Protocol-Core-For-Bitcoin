#include "taproot/TaprootSigner.h"
#include "taproot/TaprootScript.h"
#include "taproot/TaprootUtils.h"
#include <openssl/sha.h>
#include <secp256k1.h>
#include <secp256k1_schnorrsig.h>
#include <secp256k1_extrakeys.h>
#include <cstring>
#include <vector>
#include <algorithm>

namespace ailee {
namespace taproot {

namespace {

void tagged_hash(const char* tag, const std::vector<uint8_t>& msg, std::array<uint8_t, 32>& out_hash) {
    std::array<uint8_t, 32> tag_hash;
    SHA256(reinterpret_cast<const unsigned char*>(tag), std::strlen(tag), tag_hash.data());

    std::vector<uint8_t> buffer;
    buffer.reserve(32 + 32 + msg.size());
    buffer.insert(buffer.end(), tag_hash.begin(), tag_hash.end());
    buffer.insert(buffer.end(), tag_hash.begin(), tag_hash.end());
    buffer.insert(buffer.end(), msg.begin(), msg.end());

    SHA256(buffer.data(), buffer.size(), out_hash.data());
}

} // anonymous namespace

TaprootSignatureResult sign_taproot_input(
    const Tx& tx,
    size_t input_index,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& taproot_output_key,
    const std::array<uint8_t, 32>& privkey,
    uint64_t value_sats
) {
    TaprootSignatureResult result;
    result.success = false;
    std::fill(result.signature.begin(), result.signature.end(), 0);

    if (input_index >= tx.vin.size()) {
        result.error = "invalid-input-index";
        return result;
    }

    // --- 1. Compute BIP341 sighash ---

    // We only have one input according to scope
    std::vector<uint8_t> prevouts_data;
    for (const auto& in : tx.vin) {
        prevouts_data.insert(prevouts_data.end(), in.prev_txid.begin(), in.prev_txid.end());
        write_uint32_le(prevouts_data, in.prev_vout);
    }
    std::array<uint8_t, 32> sha_prevouts;
    SHA256(prevouts_data.data(), prevouts_data.size(), sha_prevouts.data());

    std::vector<uint8_t> amounts_data;
    // Assuming single input logic as specified, we only know the value_sats for this input.
    write_uint64_le(amounts_data, value_sats);
    std::array<uint8_t, 32> sha_amounts;
    SHA256(amounts_data.data(), amounts_data.size(), sha_amounts.data());

    std::vector<uint8_t> spks_data;
    std::vector<uint8_t> script_pubkey = build_p2tr_script_pubkey(taproot_output_key);
    write_compact_size(spks_data, script_pubkey.size());
    spks_data.insert(spks_data.end(), script_pubkey.begin(), script_pubkey.end());
    std::array<uint8_t, 32> sha_scriptpubkeys;
    SHA256(spks_data.data(), spks_data.size(), sha_scriptpubkeys.data());

    std::vector<uint8_t> sequences_data;
    for (const auto& in : tx.vin) {
        write_uint32_le(sequences_data, in.sequence);
    }
    std::array<uint8_t, 32> sha_sequences;
    SHA256(sequences_data.data(), sequences_data.size(), sha_sequences.data());

    std::vector<uint8_t> outputs_data;
    for (const auto& out : tx.vout) {
        write_uint64_le(outputs_data, out.value_sats);
        write_compact_size(outputs_data, out.script_pubkey.size());
        outputs_data.insert(outputs_data.end(), out.script_pubkey.begin(), out.script_pubkey.end());
    }
    std::array<uint8_t, 32> sha_outputs;
    SHA256(outputs_data.data(), outputs_data.size(), sha_outputs.data());

    // Assemble sighash message
    std::vector<uint8_t> sighash_msg;
    // Epoch: 0x00
    sighash_msg.push_back(0x00);
    // hash_type (SIGHASH_DEFAULT)
    sighash_msg.push_back(0x00);
    // nVersion
    write_uint32_le(sighash_msg, tx.version);
    // nLockTime
    write_uint32_le(sighash_msg, tx.locktime);

    // hashes
    sighash_msg.insert(sighash_msg.end(), sha_prevouts.begin(), sha_prevouts.end());
    sighash_msg.insert(sighash_msg.end(), sha_amounts.begin(), sha_amounts.end());
    sighash_msg.insert(sighash_msg.end(), sha_scriptpubkeys.begin(), sha_scriptpubkeys.end());
    sighash_msg.insert(sighash_msg.end(), sha_sequences.begin(), sha_sequences.end());

    if (!(0x00 & 0x01) && !(0x00 & 0x02)) { // SIGHASH_ALL (SIGHASH_DEFAULT maps to SIGHASH_ALL semantics for outputs)
        sighash_msg.insert(sighash_msg.end(), sha_outputs.begin(), sha_outputs.end());
    }

    // spend_type = 0 (no annex, no script path)
    sighash_msg.push_back(0x00);

    // input index
    write_uint32_le(sighash_msg, static_cast<uint32_t>(input_index));

    std::array<uint8_t, 32> sighash;
    tagged_hash("TapSighash", sighash_msg, sighash);

    // --- 2. Produce Schnorr signature ---

    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    if (!ctx) {
        result.error = "secp256k1-context-failed";
        return result;
    }

    secp256k1_keypair keypair;
    if (secp256k1_keypair_create(ctx, &keypair, privkey.data()) != 1) {
        result.error = "invalid-private-key";
        secp256k1_context_destroy(ctx);
        return result;
    }

    // Tweak the private key with TapTweak
    std::vector<uint8_t> internal_key_vec(internal_key.begin(), internal_key.end());
    std::array<uint8_t, 32> tweak;
    tagged_hash("TapTweak", internal_key_vec, tweak);

    if (secp256k1_keypair_xonly_tweak_add(ctx, &keypair, tweak.data()) != 1) {
        result.error = "tweak-failed";
        std::fill(result.signature.begin(), result.signature.end(), 0);
        secp256k1_context_destroy(ctx);
        return result;
    }

    if (secp256k1_schnorrsig_sign32(ctx, result.signature.data(), sighash.data(), &keypair, nullptr) != 1) {
        result.error = "schnorr-sign-failed";
        std::fill(result.signature.begin(), result.signature.end(), 0);
        secp256k1_context_destroy(ctx);
        return result;
    }

    secp256k1_context_destroy(ctx);
    result.success = true;
    return result;
}

TaprootSignatureResult sign_anchor_commit_input(
    const Tx& tx,
    size_t input_index,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& leaf_hash,
    uint64_t value_sats,
    const std::array<uint8_t, 32>& privkey
) {
    TaprootSignatureResult result;
    result.success = false;
    std::fill(result.signature.begin(), result.signature.end(), 0);

    if (input_index >= tx.vin.size()) {
        result.error = "invalid-input-index";
        return result;
    }

    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    if (!ctx) {
        result.error = "secp256k1-context-failed";
        return result;
    }

    // Compute TapTweak(internal_key || leaf_hash)
    std::vector<uint8_t> tweak_preimage;
    tweak_preimage.reserve(64);
    tweak_preimage.insert(tweak_preimage.end(), internal_key.begin(), internal_key.end());
    tweak_preimage.insert(tweak_preimage.end(), leaf_hash.begin(), leaf_hash.end());

    std::array<uint8_t, 32> tweak;
    tagged_hash("TapTweak", tweak_preimage, tweak);

    // Derive tweaked pubkey
    secp256k1_xonly_pubkey xonly_internal;
    if (secp256k1_xonly_pubkey_parse(ctx, &xonly_internal, internal_key.data()) != 1) {
        result.error = "invalid-internal-key";
        secp256k1_context_destroy(ctx);
        return result;
    }

    secp256k1_pubkey output_pubkey;
    if (secp256k1_xonly_pubkey_tweak_add(ctx, &output_pubkey, &xonly_internal, tweak.data()) != 1) {
        result.error = "pubkey-tweak-failed";
        secp256k1_context_destroy(ctx);
        return result;
    }

    secp256k1_xonly_pubkey out_xonly;
    if (secp256k1_xonly_pubkey_from_pubkey(ctx, &out_xonly, nullptr, &output_pubkey) != 1) {
        result.error = "xonly-from-pubkey-failed";
        secp256k1_context_destroy(ctx);
        return result;
    }

    std::array<uint8_t, 32> taproot_output_key;
    secp256k1_xonly_pubkey_serialize(ctx, taproot_output_key.data(), &out_xonly);

    // --- Compute Sighash ---

    std::vector<uint8_t> prevouts_data;
    for (const auto& in : tx.vin) {
        prevouts_data.insert(prevouts_data.end(), in.prev_txid.begin(), in.prev_txid.end());
        write_uint32_le(prevouts_data, in.prev_vout);
    }
    std::array<uint8_t, 32> sha_prevouts;
    SHA256(prevouts_data.data(), prevouts_data.size(), sha_prevouts.data());

    std::vector<uint8_t> amounts_data;
    write_uint64_le(amounts_data, value_sats);
    std::array<uint8_t, 32> sha_amounts;
    SHA256(amounts_data.data(), amounts_data.size(), sha_amounts.data());

    std::vector<uint8_t> spks_data;
    // Crucial: Use the newly derived taproot_output_key for the prevout scriptPubKey
    std::vector<uint8_t> script_pubkey = build_p2tr_script_pubkey(taproot_output_key);
    write_compact_size(spks_data, script_pubkey.size());
    spks_data.insert(spks_data.end(), script_pubkey.begin(), script_pubkey.end());
    std::array<uint8_t, 32> sha_scriptpubkeys;
    SHA256(spks_data.data(), spks_data.size(), sha_scriptpubkeys.data());

    std::vector<uint8_t> sequences_data;
    for (const auto& in : tx.vin) {
        write_uint32_le(sequences_data, in.sequence);
    }
    std::array<uint8_t, 32> sha_sequences;
    SHA256(sequences_data.data(), sequences_data.size(), sha_sequences.data());

    std::vector<uint8_t> outputs_data;
    for (const auto& out : tx.vout) {
        write_uint64_le(outputs_data, out.value_sats);
        write_compact_size(outputs_data, out.script_pubkey.size());
        outputs_data.insert(outputs_data.end(), out.script_pubkey.begin(), out.script_pubkey.end());
    }
    std::array<uint8_t, 32> sha_outputs;
    SHA256(outputs_data.data(), outputs_data.size(), sha_outputs.data());

    std::vector<uint8_t> sighash_msg;
    sighash_msg.push_back(0x00); // Epoch: 0x00
    sighash_msg.push_back(0x00); // hash_type (SIGHASH_DEFAULT)
    write_uint32_le(sighash_msg, tx.version);
    write_uint32_le(sighash_msg, tx.locktime);

    sighash_msg.insert(sighash_msg.end(), sha_prevouts.begin(), sha_prevouts.end());
    sighash_msg.insert(sighash_msg.end(), sha_amounts.begin(), sha_amounts.end());
    sighash_msg.insert(sighash_msg.end(), sha_scriptpubkeys.begin(), sha_scriptpubkeys.end());
    sighash_msg.insert(sighash_msg.end(), sha_sequences.begin(), sha_sequences.end());

    if (!(0x00 & 0x01) && !(0x00 & 0x02)) { // SIGHASH_ALL
        sighash_msg.insert(sighash_msg.end(), sha_outputs.begin(), sha_outputs.end());
    }

    sighash_msg.push_back(0x00); // spend_type = 0 (no annex, no script path)
    write_uint32_le(sighash_msg, static_cast<uint32_t>(input_index));

    std::array<uint8_t, 32> sighash;
    tagged_hash("TapSighash", sighash_msg, sighash);

    // --- Derive tweaked private key and sign ---

    secp256k1_keypair keypair;
    if (secp256k1_keypair_create(ctx, &keypair, privkey.data()) != 1) {
        result.error = "invalid-private-key";
        secp256k1_context_destroy(ctx);
        return result;
    }

    if (secp256k1_keypair_xonly_tweak_add(ctx, &keypair, tweak.data()) != 1) {
        result.error = "tweak-add-failed";
        secp256k1_context_destroy(ctx);
        return result;
    }

    if (secp256k1_schnorrsig_sign32(ctx, result.signature.data(), sighash.data(), &keypair, nullptr) != 1) {
        result.error = "schnorr-sign-failed";
        std::fill(result.signature.begin(), result.signature.end(), 0);
        secp256k1_context_destroy(ctx);
        return result;
    }

    secp256k1_context_destroy(ctx);
    result.success = true;
    return result;
}

bool attach_taproot_witness(
    Tx& tx,
    size_t input_index,
    const std::array<uint8_t, 64>& signature
) {
    if (input_index >= tx.vin.size()) {
        return false;
    }

    tx.vin[input_index].script_sig.clear();
    tx.vin[input_index].witness.clear();

    std::vector<uint8_t> sig_vec(signature.begin(), signature.end());
    tx.vin[input_index].witness.push_back(sig_vec);

    return true;
}

Tx build_signed_taproot_tx(
    const Tx& unsigned_tx,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& taproot_output_key,
    const std::array<uint8_t, 32>& privkey,
    uint64_t value_sats
) {
    Tx tx = unsigned_tx;

    TaprootSignatureResult sig_result = sign_taproot_input(tx, 0, internal_key, taproot_output_key, privkey, value_sats);
    if (sig_result.success) {
        attach_taproot_witness(tx, 0, sig_result.signature);
    }

    return tx;
}

std::vector<uint8_t> sign_anchor_commit_tx(
    const std::vector<uint8_t>& unsigned_tx,
    const std::array<uint8_t, 32>& internal_key,
    const std::array<uint8_t, 32>& leaf_hash,
    uint64_t value_sats,
    const std::array<uint8_t, 32>& privkey
) {
    std::vector<uint8_t> result;
    if (unsigned_tx.size() < 60) {
        return result;
    }

    // Assert structure matches Builder (version=2, marker=0, flag=1)
    if (unsigned_tx[0] != 2 || unsigned_tx[4] != 0 || unsigned_tx[5] != 1) {
        return result;
    }

    size_t offset = 7; // skip version(4), marker(1), flag(1), input_count(1)

    std::array<uint8_t, 32> in_txid;
    std::memcpy(in_txid.data(), &unsigned_tx[offset], 32);
    offset += 32;

    uint32_t in_vout = unsigned_tx[offset] | (unsigned_tx[offset+1] << 8) | (unsigned_tx[offset+2] << 16) | (unsigned_tx[offset+3] << 24);
    offset += 4;

    offset += 1; // scriptsig_len

    uint32_t in_seq = unsigned_tx[offset] | (unsigned_tx[offset+1] << 8) | (unsigned_tx[offset+2] << 16) | (unsigned_tx[offset+3] << 24);
    offset += 4;

    offset += 1; // output_count

    uint64_t out_val = 0;
    std::memcpy(&out_val, &unsigned_tx[offset], 8);
    offset += 8;

    uint8_t spk_len = unsigned_tx[offset];
    offset += 1;

    std::vector<uint8_t> spk;
    spk.insert(spk.end(), &unsigned_tx[offset], &unsigned_tx[offset + spk_len]);
    offset += spk_len;

    Tx tx;
    tx.version = unsigned_tx[0] | (unsigned_tx[1] << 8) | (unsigned_tx[2] << 16) | (unsigned_tx[3] << 24);

    TxIn in;
    in.prev_txid = in_txid;
    in.prev_vout = in_vout;
    in.sequence = in_seq;
    tx.vin.push_back(in);

    TxOut out;
    out.value_sats = out_val;
    out.script_pubkey = spk; // Important: This is the output being created, NOT the prevout being spent
    tx.vout.push_back(out);

    tx.locktime = unsigned_tx[unsigned_tx.size() - 4] | (unsigned_tx[unsigned_tx.size() - 3] << 8) | (unsigned_tx[unsigned_tx.size() - 2] << 16) | (unsigned_tx[unsigned_tx.size() - 1] << 24);

    // Sign using the specialized anchor-commit helper which computes the correct prevout scriptPubKey
    // based on internal_key || leaf_hash, independent of tx.vout[0].script_pubkey
    TaprootSignatureResult sig_result = sign_anchor_commit_input(
        tx, 0, internal_key, leaf_hash, value_sats, privkey);

    if (sig_result.success) {
        attach_taproot_witness(tx, 0, sig_result.signature);
    } else {
        return result; // return empty on failure
    }

    return tx.to_bytes();
}

} // namespace taproot
} // namespace ailee
