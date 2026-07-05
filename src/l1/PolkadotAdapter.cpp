// PolkadotAdapter.cpp
// Deterministic fail-closed adapter for AILEE-Core Global_Seven.

#include "Global_Seven.h"
#include <iostream>

namespace ailee {
namespace global_seven {

bool PolkadotAdapter::init(const AdapterConfig&, ErrorCallback onError) {
    if (onError) {
        onError(AdapterError{Severity::Warn, "ADAPTER_NOT_IMPLEMENTED: PolkadotAdapter is disabled in deterministic fail-closed mode", "Init", -1});
    }
    return false; // Fail closed deterministically
}

bool PolkadotAdapter::start(TxCallback, BlockCallback, EnergyCallback) {
    return false; // Fail closed deterministically
}

void PolkadotAdapter::stop() {
    // Nothing to stop
}

bool PolkadotAdapter::broadcastTransaction(const std::vector<TxOut>&,
                                           const std::unordered_map<std::string, std::string>&,
                                           std::string& outTxHash) {
    outTxHash.clear();
    return false; // Fail closed deterministically
}

std::optional<NormalizedTx> PolkadotAdapter::getTransaction(const std::string&) {
    return std::nullopt;
}

std::optional<BlockHeader> PolkadotAdapter::getBlockHeader(const std::string&) {
    return std::nullopt;
}

std::optional<uint64_t> PolkadotAdapter::getBlockHeight() {
    return std::nullopt;
}

} // namespace global_seven
} // namespace ailee
