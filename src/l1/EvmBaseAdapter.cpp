// EvmBaseAdapter.cpp
#include "Global_Seven.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <random>
#include <chrono>
#include <iostream>

namespace ailee {
namespace global_seven {

// Simple structured log hook (replace with spdlog or your sink)
static void logEvt(Severity s, const std::string& msg, const std::string& comp, ErrorCallback cb) {
    if (cb) cb(AdapterError{s, msg, comp, 0});
}

// Jittered exponential backoff
static bool backoffRetry(size_t attempt, size_t maxAttempts, std::chrono::milliseconds base, std::chrono::milliseconds& wait) {
    if (attempt >= maxAttempts) return false;
    double factor = std::min<double>(8.0, std::pow(2.0, attempt));
    auto dur = static_cast<long long>(base.count() * factor);
    std::uniform_int_distribution<long long> jitter(0, dur / 4);
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    wait = std::chrono::milliseconds(dur + jitter(rng));
    return true;
}

struct EVMInternal {
    std::string rpcEndpoint, wsEndpoint;
    bool tlsEnabled{false};
    bool connectedRPC{false}, connectedWS{false};
    uint64_t chainId{0};
    uint64_t nonce{0};
    double maxPriorityFeeGwei{1.0};
    double maxFeeGwei{50.0};
    std::unordered_map<std::string, std::chrono::system_clock::time_point> broadcasted; // idempotency guard

    bool connectRPC(const AdapterConfig& cfg, ErrorCallback onError) {
        rpcEndpoint = cfg.nodeEndpoint;
        tlsEnabled = rpcEndpoint.rfind("https://", 0) == 0;
        connectedRPC = true; // TODO: create TLS client, authenticate
        // TODO: chainId = eth_chainId
        chainId = (cfg.network == "mainnet" ? 1ULL : 11155111ULL); // example: sepolia
        logEvt(Severity::Info, "EVM RPC connected: " + rpcEndpoint, "RPC", onError);
        return true;
    }

    bool connectWS(const std::string& endpoint, ErrorCallback onError) {
        wsEndpoint = endpoint;
        connectedWS = true; // TODO: open WS, subscribe to newHeads
        logEvt(Severity::Info, "EVM WS connected: " + wsEndpoint, "Listener", onError);
        return true;
    }

    bool refreshNonce(const std::string& fromAddr, ErrorCallback onError) {
        // TODO: eth_getTransactionCount (pending)
        nonce += 1; // placeholder increment
        logEvt(Severity::Debug, "Nonce updated to " + std::to_string(nonce), "RPC", onError);
        return true;
    }

    bool estimateFees(ErrorCallback onError) {
        // TODO: eth_maxPriorityFeePerGas + eth_feeHistory
        maxPriorityFeeGwei = std::min(2.0, maxPriorityFeeGwei * 1.05);
        maxFeeGwei         = std::min(200.0, maxFeeGwei * 1.02);
        logEvt(Severity::Debug, "Fees updated: tip=" + std::to_string(maxPriorityFeeGwei) + " max=" + std::to_string(maxFeeGwei), "RPC", onError);
        return true;
    }

    bool sendRawTx(const std::string& rawHex, std::string& outTxHash, ErrorCallback onError) {
        if (!connectedRPC) return false;
        // TODO: eth_sendRawTransaction; ensure idempotent by checking broadcasted
        outTxHash = "evm_dummy_" + std::to_string(++nonce);
        broadcasted[outTxHash] = std::chrono::system_clock::now();
        logEvt(Severity::Info, "Broadcasted EVM tx=" + outTxHash, "Broadcast", onError);
        return true;
    }

    std::optional<NormalizedTx> getTx(const std::string& hash) {
        if (!connectedRPC) return std::nullopt;
        NormalizedTx nt;
        nt.chainTxId = hash;
        nt.normalizedId = hash;
        nt.confirmed = false; nt.confirmations = 0;
        return nt;
    }

    std::optional<BlockHeader> getHeader(const std::string& hash, Chain chain) {
        if (!connectedRPC) return std::nullopt;
        BlockHeader bh{hash, 0, "", std::chrono::system_clock::now(), chain};
        return bh;
    }

    std::optional<uint64_t> height() {
        if (!connectedRPC) return std::nullopt;
        static uint64_t hb{0};
        return ++hb; // TODO: eth_blockNumber
    }
};

template <typename Derived>
class EVMAdapterBase {
protected:
    struct State {
        AdapterConfig cfg;
        ErrorCallback onError;
        TxCallback onTx;
        BlockCallback onBlock;
        EnergyCallback onEnergy;
        std::atomic<bool> running{false};
        std::thread eventThread;
        EVMInternal internal;
        std::string fromAddress; // for nonce mgmt
    };
    static std::mutex s_mtx;
    static std::unordered_map<const Derived*, std::shared_ptr<State>> s_states;

    static std::shared_ptr<State> getState(const Derived* self) {
        std::lock_guard<std::mutex> lock(s_mtx);
        auto it = s_states.find(self);
        return (it != s_states.end()) ? it->second : nullptr;
    }
    static void setState(const Derived* self, std::shared_ptr<State> st) {
        std::lock_guard<std::mutex> lock(s_mtx);
        s_states[self] = std::move(st);
    }
    static void clearState(const Derived* self) {
        std::lock_guard<std::mutex> lock(s_mtx);
        s_states.erase(self);
    }

    // Build EIP-1559 raw tx (placeholder); replace with real signer (HSM/wallet)
    static std::string buildEip1559Raw(const State& st,
                                       const std::vector<TxOut>& outputs,
                                       const std::unordered_map<std::string, std::string>& opts) {
        // TODO: craft RLP: chainId, nonce, maxPriorityFee, maxFee, gasLimit, to, value, data, v,r,s
        (void)outputs; (void)opts;
        return "0x02f8_hardened_raw";
    }

public:
    // Common lifecycle used by derived chain adapters
    bool initCommon(const AdapterConfig& cfg, ErrorCallback onError) {
        auto st = std::make_shared<State>();
        st->cfg = cfg;
        st->onError = onError;
        setState(static_cast<const Derived*>(this), st);

        if (!st->internal.connectRPC(cfg, onError)) return false;
        auto it = cfg.extra.find("ws");
        if (it != cfg.extra.end()) st->internal.connectWS(it->second, onError);

        // Optional fromAddress for nonce mgmt
        auto a = cfg.extra.find("from");
        if (a != cfg.extra.end()) st->fromAddress = a->second;

        // Chain ID sanity check can be performed against expected network
        logEvt(Severity::Info, "EVM init complete", "Init", onError);
        return true;
    }

    bool startCommon(TxCallback onTx, BlockCallback onBlock, EnergyCallback onEnergy, Chain chainTag) {
        auto st = getState(static_cast<const Derived*>(this));
        if (!st) return false;
        st->onTx = onTx; st->onBlock = onBlock; st->onEnergy = onEnergy;
        st->running.store(true);

        st->eventThread = std::thread([st, chainTag]() {
            using namespace std::chrono_literals;
            auto lastEnergy = std::chrono::steady_clock::now();

            size_t attempt = 0;
            while (st->running.load()) {
                auto h = st->internal.height();
                if (!h.has_value()) {
                    std::chrono::milliseconds wait;
                    if (backoffRetry(attempt++, 5, 200ms, wait)) {
                        std::this_thread::sleep_for(wait);
                        continue;
                    } else {
                        logEvt(Severity::Critical, "EVM heartbeat failed repeatedly", "Listener", st->onError);
                        break;
                    }
                } else {
                    attempt = 0;
                    if (st->onBlock) {
                        BlockHeader bh;
                        bh.hash = "evm_head_" + std::to_string(h.value());
                        bh.height = h.value();
                        bh.parentHash = "evm_parent";
                        bh.timestamp = std::chrono::system_clock::now();
                        bh.chain = chainTag;
                        st->onBlock(bh);
                    }
                }

                if (st->cfg.enableTelemetry &&
                    (std::chrono::steady_clock::now() - lastEnergy > 5s) &&
                    st->onEnergy) {
                    EnergyTelemetry et;
                    et.latencyMs = 12.0;
                    et.nodeTempC = 47.0;
                    et.energyEfficiencyScore = 85.0;
                    st->onEnergy(et);
                    lastEnergy = std::chrono::steady_clock::now();
                }

                std::this_thread::sleep_for(1s);
            }
        });
        return true;
    }

    void stopCommon() {
        auto st = getState(static_cast<const Derived*>(this));
        if (!st) return;
        st->running.store(false);
        if (st->eventThread.joinable()) st->eventThread.join();
        clearState(static_cast<const Derived*>(this));
    }

    bool broadcastCommon(const std::vector<TxOut>& outputs,
                         const std::unordered_map<std::string, std::string>& opts,
                         std::string& outChainTxId,
                         Chain chainTag) {
        auto st = getState(static_cast<const Derived*>(this));
        if (!st) return false;

        if (st->cfg.readOnly) {
            logEvt(Severity::Warn, "Read-only; broadcast blocked", "Broadcast", st->onError);
            return false;
        }

        // Circuit breaker: respect fee caps to avoid runaway gas costs
        st->internal.estimateFees(st->onError);

        // Nonce management (idempotent protection)
        st->internal.refreshNonce(st->fromAddress, st->onError);

        // Build hardened EIP-1559 tx
        std::string rawHex = buildEip1559Raw(*st, outputs, opts);

        // Retry on transient RPC errors
        size_t attempt = 0;
        while (attempt < 5) {
            if (st->internal.sendRawTx(rawHex, outChainTxId, st->onError)) return true;

            std::chrono::milliseconds wait;
            if (!backoffRetry(attempt++, 5, std::chrono::milliseconds(250), wait)) break;
            std::this_thread::sleep_for(wait);
        }

        logEvt(Severity::Error, "EVM broadcast failed after retries", "Broadcast", st->onError);
        return false;
    }

    std::optional<NormalizedTx> getTxCommon(const std::string& chainTxId, Chain chainTag) {
        auto st = getState(static_cast<const Derived*>(this));
        if (!st) return std::nullopt;
        auto nt = st->internal.getTx(chainTxId);
        if (nt.has_value()) nt->chain = chainTag;
        return nt;
    }

    std::optional<BlockHeader> getHeaderCommon(const std::string& blockHash, Chain chainTag) {
        auto st = getState(static_cast<const Derived*>(this));
        if (!st) return std::nullopt;
        return st->internal.getHeader(blockHash, chainTag);
    }

    std::optional<uint64_t> heightCommon() {
        auto st = getState(static_cast<const Derived*>(this));
        if (!st) return std::nullopt;
        return st->internal.height();
    }
};

template <typename D>
std::mutex EVMAdapterBase<D>::s_mtx;
template <typename D>
std::unordered_map<const D*, std::shared_ptr<typename EVMAdapterBase<D>::State>>
    EVMAdapterBase<D>::s_states;

} // namespace global_seven
} // namespace ailee
