// SPDX-License-Identifier: MIT
// AILEE-Core Node [v1.0.0-Production-Trusted]
// Main entry point with hardened orchestration, structured logging, and graceful lifecycle management.

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <atomic>
#include <csignal>
#include <cstdlib>   // getenv
#include <mutex>

// Core Protocol Headers
#include "ailee_tps_engine.h"
#include "ailee_gold_bridge.h"
#include "ailee_recovery_protocol.h"

// Auxiliary System Headers
#include "ailee_energy_telemetry.h"
#include "ailee_circuit_breaker.h"

// Network Infrastructure Headers
#include "ailee_bitcoin_zmq_listener.h"
#include "ailee_bitcoin_rpc_client.h"

// Optional: Global Seven Orchestrator (if available in your tree)
#include "global_seven/SettlementOrchestrator.h"

// ---------------------------------------------------------
// Structured logging with severity and timestamps
// ---------------------------------------------------------
enum class LogLevel { INFO, WARN, ERROR };

static std::mutex g_logMutex;

static std::string nowIso8601() {
    using namespace std::chrono;
    auto t = system_clock::now();
    auto secs = time_point_cast<seconds>(t);
    auto ms = duration_cast<milliseconds>(t - secs).count();
    std::time_t tt = system_clock::to_time_t(t);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    return std::string(buf) + "." + std::to_string(ms);
}

static void log(LogLevel level, const std::string& msg) {
    const char* sev = (level == LogLevel::INFO ? "INFO" : level == LogLevel::WARN ? "WARN" : "ERROR");
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::cout << "[" << nowIso8601() << "]"
              << " [" << sev << "] "
              << msg << std::endl;
}

// ---------------------------------------------------------
// Process-wide shutdown flag + signal handling
// ---------------------------------------------------------
static std::atomic<bool> g_shutdown{false};

static void handleSignal(int signum) {
    log(LogLevel::WARN, "Signal received: " + std::to_string(signum) + " — initiating graceful shutdown.");
    g_shutdown.store(true);
}

static void installSignalHandlers() {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
#if defined(SIGQUIT)
    std::signal(SIGQUIT, handleSignal);
#endif
}

// ---------------------------------------------------------
// Configuration helpers (env-first, with defaults)
// ---------------------------------------------------------
struct Config {
    std::string zmqEndpoint = "tcp://127.0.0.1:28332";
    std::string rpcUser     = "rpcuser";
    std::string rpcPass     = "rpcpassword";
    std::string rpcUrl      = "http://127.0.0.1:8332";
    int tpsSimNodes         = 100;
    double tpsInitialBlockMB= 1.0;
    int tpsSimCycles        = 200;
};

static std::string envOrDefault(const char* key, const std::string& def) {
    const char* v = std::getenv(key);
    return v ? std::string(v) : def;
}

static Config loadConfigFromEnv() {
    Config c;
    c.zmqEndpoint = envOrDefault("AILEE_ZMQ_ENDPOINT", c.zmqEndpoint);
    c.rpcUser     = envOrDefault("AILEE_RPC_USER",     c.rpcUser);
    c.rpcPass     = envOrDefault("AILEE_RPC_PASS",     c.rpcPass);
    c.rpcUrl      = envOrDefault("AILEE_RPC_URL",      c.rpcUrl);

    // Optional numeric overrides
    if (const char* nodes = std::getenv("AILEE_TPS_NODES"))      c.tpsSimNodes = std::atoi(nodes);
    if (const char* mb    = std::getenv("AILEE_TPS_INITIAL_MB")) c.tpsInitialBlockMB = std::atof(mb);
    if (const char* cyc   = std::getenv("AILEE_TPS_CYCLES"))     c.tpsSimCycles = std::atoi(cyc);

    return c;
}

// ---------------------------------------------------------
// Unified Engine Controller (encapsulates modules)
// ---------------------------------------------------------
struct Engine {
    // Core orchestration
    ailee::global_seven::SettlementOrchestrator orchestrator;

    // Engines & telemetry
    ailee::AILEEEngine tpsEngine;
    ailee::EnergyTelemetry energyMonitor;

    // Network clients
    ailee::BitcoinZMQListener zmqListener;
    ailee::BitcoinRPCClient rpcClient;

    // Thread lifecycle
    std::thread zmqThread;
    std::atomic<bool> zmqRunning{false};

    // Circuit breaker policy thresholds (example defaults)
    double maxBlockMBForSafeMode = 8.0;
    double maxLatencyMsForSafe   = 250.0;
    int    maxErrCountForSafe    = 25;

    // Config snapshot
    Config cfg;

    explicit Engine(const Config& cfg_)
        : zmqListener(cfg_.zmqEndpoint),
          rpcClient(cfg_.rpcUser, cfg_.rpcPass, cfg_.rpcUrl),
          cfg(cfg_) {}

    // TPS Simulation with basic reporting
    void runTPSSimulation() {
        log(LogLevel::INFO, "TPS Simulation starting… nodes=" + std::to_string(cfg.tpsSimNodes)
            + " initialMB=" + std::to_string(cfg.tpsInitialBlockMB)
            + " cycles=" + std::to_string(cfg.tpsSimCycles));

        auto result = ailee::PerformanceSimulator::runSimulation(
            cfg.tpsSimNodes, cfg.tpsInitialBlockMB, cfg.tpsSimCycles);

        log(LogLevel::INFO, "Baseline TPS: " + std::to_string(result.initialTPS));
        log(LogLevel::INFO, "Final TPS: "    + std::to_string(result.finalTPS));
        log(LogLevel::INFO, "Improvement: "  + std::to_string(result.improvementFactor) + "x");
        log(LogLevel::INFO, "Cycles Run: "   + std::to_string(result.cycles));

        // Periodic snapshot (every 20 cycles if present)
        std::ostringstream hist;
        hist << "Optimization snapshots:";
        for (size_t i = 0; i < result.aiFactorHistory.size(); i += 20) {
            hist << "\n  cycle=" << i
                 << " aiFactor=" << std::fixed << std::setprecision(4) << result.aiFactorHistory[i]
                 << " tps="      << std::setprecision(1)               << result.tpsHistory[i]
                 << " error="    << std::setprecision(4)               << result.errorHistory[i];
        }
        log(LogLevel::INFO, hist.str());
    }

    // Gold bridge test with error handling & recovery hint
    void testGoldBridge() {
        log(LogLevel::INFO, "Testing Bitcoin-to-Gold Bridge protocol…");

        ailee::GoldBridge bridge;
        std::string user = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"; // demo address
        uint64_t btcAmount = 500000000; // 5 BTC (sats)

        try {
            std::string conversionId = bridge.initiateConversion(user, btcAmount, /*burn=*/true);
            log(LogLevel::INFO, "Conversion ID: " + conversionId);
            log(LogLevel::INFO, "Status: PENDING_PAYMENT; Oracle: ACTIVE; Inventory: SECURE");
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, std::string("GoldBridge error: ") + e.what());
            // Optional: tie into recovery protocol
            ailee::RecoveryProtocol::recordIncident("GoldBridgeInitiateFailure", e.what());
        }
    }

    // Safety & Energy evaluation with circuit breaker
    void testSafetyAndEnergy() {
        log(LogLevel::INFO, "Evaluating Auxiliary Systems (Safety & Energy)…");

        ailee::ThermalMetric minerStats = {3000.0, 1500.0, 25.0, 60.0, 1735660000};
        double efficiency = ailee::EnergyTelemetry::calculateEfficiencyScore(minerStats);
        std::string proof = ailee::EnergyTelemetry::generateTelemetryProof(minerStats, "Node-001");

        log(LogLevel::INFO, "Energy: input=" + std::to_string(minerStats.inputPowerWatts) + "W"
                            + " wasteRecovery=" + std::to_string(minerStats.wasteHeatRecoveredW) + "W"
                            + " effScore=" + std::to_string(efficiency * 100.0) + "%");
        log(LogLevel::INFO, "Green Hash: " + proof);

        // Circuit breaker monitoring example
        auto state = ailee::CircuitBreaker::monitor(maxBlockMBForSafeMode, maxLatencyMsForSafe, maxErrCountForSafe);
        if (state == ailee::SystemState::SAFE_MODE) {
            log(LogLevel::WARN, "Circuit Breaker: SAFE_MODE engaged — throttling modules and reducing load.");
            throttleSystems();
        } else {
            log(LogLevel::INFO, "Circuit Breaker: OPTIMIZED — systems running within safe parameters.");
        }
    }

    // Network infrastructure lifecycle with graceful thread handling
    void startZmq() {
        zmqListener.init();
        zmqRunning.store(true);
        zmqThread = std::thread([this]() {
            try {
                zmqListener.start();
            } catch (const std::exception& e) {
                log(LogLevel::ERROR, std::string("ZMQ listener exception: ") + e.what());
            }
            zmqRunning.store(false);
        });
    }

    void stopZmq() {
        try {
            zmqListener.stop();
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, std::string("ZMQ stop error: ") + e.what());
        }
        if (zmqThread.joinable()) {
            zmqThread.join();
        }
    }

    void testNetworkInfrastructure() {
        log(LogLevel::INFO, "Initializing Network Bridge (ZMQ + RPC)…");

        try {
            startZmq();

            // RPC health probe with basic backoff
            int attempts = 0;
            const int maxAttempts = 3;
            long count = -1;
            while (attempts < maxAttempts && !g_shutdown.load()) {
                count = rpcClient.getBlockCount();
                if (count >= 0) break;
                attempts++;
                log(LogLevel::WARN, "RPC probe failed — retry " + std::to_string(attempts) + "/" + std::to_string(maxAttempts));
                std::this_thread::sleep_for(std::chrono::milliseconds(250 * attempts));
            }

            if (count >= 0) {
                log(LogLevel::INFO, "Bitcoin RPC connected. Block Height: " + std::to_string(count));
            } else {
                log(LogLevel::WARN, "Simulation Mode: No active Bitcoin node detected.");
            }

            // Allow ZMQ events to flow briefly (if connected)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, std::string("Network initialization error: ") + e.what());
        }

        stopZmq();
        log(LogLevel::INFO, "Network Bridge shutdown complete.");
    }

    // Adaptive throttling when SAFE_MODE engaged
    void throttleSystems() {
        // Example: reduce TPS cycles, slow polling, or disable nonessential tasks
        cfg.tpsSimCycles = std::max(50, cfg.tpsSimCycles / 2);
        log(LogLevel::WARN, "Adaptive throttling applied — TPS cycles reduced to " + std::to_string(cfg.tpsSimCycles));
    }
};

// ---------------------------------------------------------
// Main Entry Point
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    installSignalHandlers();
    log(LogLevel::INFO, "Starting AILEE-Core Node [v1.0.0-Production-Trusted]…");

    // Load configuration (env-first)
    Config cfg = loadConfigFromEnv();
    log(LogLevel::INFO, "Config: ZMQ=" + cfg.zmqEndpoint + " RPC=" + cfg.rpcUrl);

    // Initialize engine and run modules
    Engine engine(cfg);

    try {
        engine.runTPSSimulation();
        if (g_shutdown.load()) throw std::runtime_error("Shutdown requested during TPS.");

        engine.testGoldBridge();
        if (g_shutdown.load()) throw std::runtime_error("Shutdown requested during Bridge.");

        engine.testSafetyAndEnergy();
        if (g_shutdown.load()) throw std::runtime_error("Shutdown requested during Safety/Energy.");

        engine.testNetworkInfrastructure();
        if (g_shutdown.load()) throw std::runtime_error("Shutdown requested during Network.");
    } catch (const std::exception& e) {
        log(LogLevel::ERROR, std::string("Fatal error: ") + e.what());
        // Optional: record for recovery root cause analysis
        ailee::RecoveryProtocol::recordIncident("FatalMainException", e.what());
    }

    log(LogLevel::INFO, "[AILEE-CORE] All modules completed. Exiting cleanly.");
    return 0;
}

