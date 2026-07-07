#pragma once

#include "l3/NetworkReflection.h"
#include "ReflectionLayer.h"
#include "SettlementIngestion.h"
#include "MeshCoherence.h"

namespace ailee {
namespace l3 {

// Converts L3 NetworkBlockSnapshot to L2 ReflectionSnapshot
reflection::ReflectionSnapshot bind_network_block(const NetworkBlockSnapshot& net_block);

// Converts L3 NetworkMempoolSnapshot to L1 SettlementIngestion (used in L2 engine)
l1::SettlementIngestion bind_network_mempool(const NetworkMempoolSnapshot& net_mempool, const NetworkBlockSnapshot& net_block);

// Converts L3 NetworkPeerSnapshot to MeshCoherenceResult
mesh::MeshCoherenceResult bind_network_peer(const NetworkPeerSnapshot& net_peer);

} // namespace l3
} // namespace ailee
