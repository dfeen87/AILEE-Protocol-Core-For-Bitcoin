#pragma once

#include <cstdint>
#include <string>

namespace ailee {
namespace l3 {

struct alignas(64) NetworkBlockSnapshot {
    uint8_t block_hash[32];
    uint8_t previous_block_hash[32];
    uint8_t merkle_root[32];
    uint64_t height;
    uint64_t timestamp;
    uint64_t nonce;
    uint32_t difficulty_target;
    uint32_t transaction_count;
};
static_assert(sizeof(NetworkBlockSnapshot) == 128, "NetworkBlockSnapshot must be exactly 128 bytes");
static_assert(alignof(NetworkBlockSnapshot) == 64, "NetworkBlockSnapshot must be 64-byte aligned");

struct alignas(64) NetworkMempoolSnapshot {
    uint64_t transaction_count;
    uint64_t total_vsize;
    uint64_t total_fee_sats;
    uint32_t min_fee_rate;
    uint32_t max_fee_rate;
    uint8_t padding[32]; // 8*3=24, 4*2=8 -> 32. 32+32 = 64
};
static_assert(sizeof(NetworkMempoolSnapshot) == 64, "NetworkMempoolSnapshot must be exactly 64 bytes");
static_assert(alignof(NetworkMempoolSnapshot) == 64, "NetworkMempoolSnapshot must be 64-byte aligned");

struct alignas(64) NetworkPeerSnapshot {
    uint64_t peer_count;
    uint64_t total_bytes_sent;
    uint64_t total_bytes_recv;
    uint32_t active_connections;
    uint32_t banned_peers;
    uint8_t padding[32]; // 8*3=24, 4*2=8 -> 32. 32+32 = 64
};
static_assert(sizeof(NetworkPeerSnapshot) == 64, "NetworkPeerSnapshot must be exactly 64 bytes");
static_assert(alignof(NetworkPeerSnapshot) == 64, "NetworkPeerSnapshot must be 64-byte aligned");

struct alignas(64) NetworkSnapshot {
    NetworkBlockSnapshot block;     // 128
    NetworkMempoolSnapshot mempool; // 64
    NetworkPeerSnapshot peer;       // 64
};
static_assert(sizeof(NetworkSnapshot) == 256, "NetworkSnapshot must be exactly 256 bytes");
static_assert(alignof(NetworkSnapshot) == 64, "NetworkSnapshot must be 64-byte aligned");

// Parses a JSON string (e.g. from Bitcoin Core RPC `getblock` + `getblockheader`)
// into a deterministic NetworkBlockSnapshot struct.
NetworkBlockSnapshot parse_network_block_snapshot(const std::string& json_data);

// Parses a JSON string (e.g. from Bitcoin Core RPC `getmempoolinfo`)
// into a deterministic NetworkMempoolSnapshot struct.
NetworkMempoolSnapshot parse_network_mempool_snapshot(const std::string& json_data);

// Parses a JSON string (e.g. from Bitcoin Core RPC `getpeerinfo`)
// into a deterministic NetworkPeerSnapshot struct.
NetworkPeerSnapshot parse_network_peer_snapshot(const std::string& json_data);

} // namespace l3
} // namespace ailee
