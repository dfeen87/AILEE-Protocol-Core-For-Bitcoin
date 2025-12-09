// SolanaAdapter.cpp
#include "Global_Seven.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <iostream>
#include <cmath>

namespace ailee {
namespace global_seven {

// Internal client stub — replace with JSON-RPC/WebSocket (Solana)
class SOLInternal {
public:
    bool connectRPC(const std::string& endpoint) {
        rpcEndpoint_ = endpoint;
        connectedRPC_ = true;
        return true;
    }
    bool connectWS(const std::string& endpoint) {
        wsEndpoint_ = endpoint;
        connectedWS_ = true;
        return true;
    }
    bool sendRawTx(const std::string& rawBase64, std::string& outSig) {
        if (!connectedRPC_) return false;
        outSig = "sol_dummy_sig_" + std::to_string(++counter_);
        return true;
    }
    std::optional<NormalizedTx> getTx(const std::string& sig) {
        if (!connectedRPC_) return std::nullopt;
        NormalizedTx nt;
        nt.chainTxId = sig;
        nt.normalizedId = sig;
        nt.chain = Chain::Solana;
        nt.confirmed = false;
        nt.confirmations = 0;
        return nt;
    }
    std::optional<BlockHeader> getHeader(const std::string& slotHash) {
        if (!connectedRPC_) return std::nullopt;
        BlockHeader bh;
        bh.hash = slotHash;
        bh.height = 0;
        bh.parentHash = "";
        bh.timestamp = std::chrono::system_clock::now();
        bh.chain = Chain::Solana;
        return bh;
    }
    std::optional<uint64_t> height() {
        if (!connectedRPC_) return std::nullopt;
        return ++heartbeatSlot_;
    }

private:
    std::string rpcEndpoint_, wsEndpoint_;
    bool connectedRPC_{false}, connectedWS_{false};
    uint64_t counter_{0};
    uint64_t heartbeatSlot_{0};
};

struct SOLState {
    AdapterConfig cfg;
    ErrorCallback onError;
    TxCallback onTx;
    BlockCallback onBlock;
    EnergyCallback onEnergy;
    std::atomic<bool> running{false};
    std::thread eventThread;
    SOLInternal internal;
};

static std::mutex g_solStateMutex;
static std::unordered_map<const SolanaAdapter*, std::shared_ptr<SOLState>> g_solStates;

static std::shared_ptr<SOLState> getState(const SolanaAdapter* self) {
    std::lock_guard<std::mutex> lock(g_solStateMutex);
    auto it = g_solStates.find(self);
    return (it != g_solStates.end()) ? it->second : nullptr;
}
static void setState(const SolanaAdapter* self, std::shared_ptr<SOLState> st) {
    std::lock_guard<std::mutex> lock(g_solStateMutex);
    g_solStates[self] = std::move(st);
}
static void clearState(const SolanaAdapter* self) {
    std::lock_guard<std::mutex> lock(g_solStateMutex);
    g_solStates.erase(self);
}

// IChainAdapter impl
bool SolanaAdapter::init(const AdapterConfig& cfg, ErrorCallback onError) {
    auto st = std::make_shared<SOLState>();
    st->cfg = cfg;
    st->onError = onError;

    if (!st->internal.connectRPC(cfg.nodeEndpoint)) {
        if (onError) onError(AdapterError{Severity::Error, "SOL RPC connect failed", "RPC", -1});
        return false;
    }
    auto it = cfg.extra.find("ws");
    if (it != cfg.extra.end() && !st->internal.connectWS(it->second)) {
        if (onError) onError(AdapterError{Severity::Warn, "SOL WS connect failed; polling", "Listener", -2});
    }

    setState(this, st);
    return true;
}

bool SolanaAdapter::start(TxCallback onTx, BlockCallback onBlock, EnergyCallback onEnergy) {
    auto st = getState(this);
    if (!st) return false;
    st->onTx = onTx;
    st->onBlock = onBlock;
    st->onEnergy = onEnergy;
    st->running.store(true);

    st->eventThread = std::thread([st]() {
        using namespace std::chrono_literals;
        auto lastEnergy = std::chrono::steady_clock::now();
        while (st->running.load()) {
            auto h = st->internal.height();
            if (h.has_value() && st->onBlock) {
                BlockHeader bh;
                bh.hash = "sol_dummy_slot_" + std::to_string(h.value());
                bh.height = h.value();
                bh.parentHash = "sol_dummy_parent";
                bh.timestamp = std::chrono::system_clock::now();
                bh.chain = Chain::Solana;
                st->onBlock(bh);
            }
            if (st->cfg.enableTelemetry &&
                (std::chrono::steady_clock::now() - lastEnergy > 5s) &&
                st->onEnergy) {
                EnergyTelemetry et;
                et.latencyMs = 8.0;
                et.nodeTempC = 44.0;
                et.energyEfficiencyScore = 86.0;
                st->onEnergy(et);
                lastEnergy = std::chrono::steady_clock::now();
            }
            std::this_thread::sleep_for(1s);
        }
    });

    return true;
}

void SolanaAdapter::stop() {
    auto st = getState(this);
    if (!st) return;
    st->running.store(false);
    if (st->eventThread.joinable()) st->eventThread.join();
    clearState(this);
}

bool SolanaAdapter::broadcastTransaction(const std::vector<TxOut>& outputs,
                                         const std::unordered_map<std::string, std::string>& opts,
                                         std::string& outChainTxId) {
    auto st = getState(this);
    if (!st) return false;
    if (st->cfg.readOnly) {
        if (st->onError) st->onError(AdapterError{Severity::Warn, "Read-only; broadcast blocked", "Broadcast", -10});
        return false;
    }
    // Stub base64-encoded Solana transaction
    std::string rawBase64 = "AQIDBA==";
    bool ok = st->internal.sendRawTx(rawBase64, outChainTxId);
    if (!ok) {
        if (st->onError) st->onError(AdapterError{Severity::Error, "SOL broadcast failed", "Broadcast", -11});
        return false;
    }
    return true;
}

std::optional<NormalizedTx> SolanaAdapter::getTransaction(const std::string& chainTxId) {
    auto st = getState(this);
    if (!st) return std::nullopt;
    return st->internal.getTx(chainTxId);
}

std::optional<BlockHeader> SolanaAdapter::getBlockHeader(const std::string& blockHash) {
    auto st = getState(this);
    if (!st) return std::nullopt;
    return st->internal.getHeader(blockHash);
}

std::optional<uint64_t> SolanaAdapter::getBlockHeight() {
    auto st = getState(this);
    if (!st) return std::nullopt;
    return st->internal.height();
}

} // namespace global_seven
} // namespace ailee
