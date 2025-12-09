// BitcoinAdapter.cpp
#include "Global_Seven.h"
#include <thread>
#include <atomic>
#include <sstream>
#include <iostream>

namespace ailee {
namespace global_seven {

class BTCInternal {
public:
    // Replace with real RPC client (HTTP JSON‑RPC) and ZMQ subscriber
    bool connectRPC(const std::string& endpoint, const std::string& user, const std::string& pass) {
        rpcEndpoint_ = endpoint; rpcUser_ = user; rpcPass_ = pass;
        // TODO: validate credentials via getblockchaininfo
        return true;
    }
    bool connectZMQ(const std::string& endpoint) {
        zmqEndpoint_ = endpoint;
        // TODO: connect and subscribe to rawtx/rawblock
        return true;
    }
    bool broadcastRaw(const std::string& rawHex, std::string& outTxId) {
        // TODO: call sendrawtransaction via RPC, return txid
        outTxId = "btc_dummy_txid_" + std::to_string(++counter_);
        return true;
    }
    std::optional<NormalizedTx> fetchTx(const std::string& txid) {
        // TODO: call getrawtransaction + decode, normalize
        NormalizedTx nt;
        nt.chainTxId = txid;
        nt.normalizedId = txid;
        nt.chain = Chain::Bitcoin;
        nt.confirmed = false;
        nt.confirmations = 0;
        return nt;
    }
    std::optional<BlockHeader> fetchHeader(const std::string& hash) {
        // TODO: call getblockheader, normalize
        BlockHeader bh;
        bh.hash = hash;
        bh.height = 0;
        bh.parentHash = "";
        bh.timestamp = std::chrono::system_clock::now();
        bh.chain = Chain::Bitcoin;
        return bh;
    }
    std::optional<uint64_t> height() {
        // TODO: call getblockcount
        return 0ULL;
    }

private:
    std::string rpcEndpoint_, rpcUser_, rpcPass_;
    std::string zmqEndpoint_;
    uint64_t counter_{0};
};

struct BTCState {
    AdapterConfig cfg;
    ErrorCallback onError;
    TxCallback onTx;
    BlockCallback onBlock;
    EnergyCallback onEnergy;
    std::atomic<bool> running{false};
    std::thread eventThread;
    BTCInternal internal;
};

bool BitcoinAdapter::init(const AdapterConfig& cfg, ErrorCallback onError) {
    state_ = std::make_shared<BTCState>();
    state_->cfg = cfg;
    state_->onError = onError;

    // Connect RPC
    if (!state_->internal.connectRPC(cfg.nodeEndpoint, cfg.authUsername, cfg.authPassword)) {
        onError(AdapterError{Severity::Error, "BTC RPC connect failed", "RPC", -1});
        return false;
    }

    // Optional ZMQ endpoint in cfg.extra["zmq"]
    auto it = cfg.extra.find("zmq");
    if (it != cfg.extra.end()) {
        if (!state_->internal.connectZMQ(it->second)) {
            onError(AdapterError{Severity::Warn, "BTC ZMQ connect failed; falling back to poll", "Listener", -2});
        }
    }

    return true;
}

bool BitcoinAdapter::start(TxCallback onTx, BlockCallback onBlock, EnergyCallback onEnergy) {
    if (!state_) return false;
    state_->onTx = onTx;
    state_->onBlock = onBlock;
    state_->onEnergy = onEnergy;
    state_->running.store(true);

    // Event loop (polling placeholder if ZMQ not available)
    state_->eventThread = std::thread([s = state_]() {
        using namespace std::chrono_literals;
        auto lastEnergy = std::chrono::steady_clock::now();

        while (s->running.load()) {
            // Poll height as a heartbeat (placeholder)
            auto h = s->internal.height();
            if (h.has_value()) {
                BlockHeader bh;
                bh.hash = "btc_dummy_hash_" + std::to_string(h.value());
                bh.height = h.value();
                bh.parentHash = "btc_dummy_parent";
                bh.timestamp = std::chrono::system_clock::now();
                bh.chain = Chain::Bitcoin;
                if (s->onBlock) s->onBlock(bh);
            }

            // Emit energy telemetry periodically
            if (std::chrono::steady_clock::now() - lastEnergy > 5s && s->cfg.enableTelemetry) {
                EnergyTelemetry et;
                et.latencyMs = 10.0;
                et.nodeTempC = 45.0;
                et.energyEfficiencyScore = 88.0;
                if (s->onEnergy) s->onEnergy(et);
                lastEnergy = std::chrono::steady_clock::now();
            }

            std::this_thread::sleep_for(1s);
        }
    });

    return true;
}

void BitcoinAdapter::stop() {
    if (!state_) return;
    state_->running.store(false);
    if (state_->eventThread.joinable()) state_->eventThread.join();
}

bool BitcoinAdapter::broadcastTransaction(const std::vector<TxOut>& outputs,
                                          const std::unordered_map<std::string, std::string>& opts,
                                          std::string& outChainTxId) {
    if (!state_) return false;
    if (state_->cfg.readOnly) {
        state_->onError(AdapterError{Severity::Warn, "Read‑only mode; broadcast blocked", "Broadcast", -10});
        return false;
    }

    // Placeholder: build raw tx (PSBT) and sign via wallet/RPC; here we fake raw hex
    std::string rawHex = "01000000...";
    bool ok = state_->internal.broadcastRaw(rawHex, outChainTxId);
    if (!ok) {
        state_->onError(AdapterError{Severity::Error, "BTC broadcast failed", "Broadcast", -11});
        return false;
    }
    return true;
}

std::optional<NormalizedTx> BitcoinAdapter::getTransaction(const std::string& chainTxId) {
    if (!state_) return std::nullopt;
    return state_->internal.fetchTx(chainTxId);
}

std::optional<BlockHeader> BitcoinAdapter::getBlockHeader(const std::string& blockHash) {
    if (!state_) return std::nullopt;
    return state_->internal.fetchHeader(blockHash);
}

std::optional<uint64_t> BitcoinAdapter::getBlockHeight() {
    if (!state_) return std::nullopt;
    return state_->internal.height();
}

// Private member to hold state
std::shared_ptr<BTCState> BitcoinAdapter::state_;

} // namespace global_seven
} // namespace ailee
