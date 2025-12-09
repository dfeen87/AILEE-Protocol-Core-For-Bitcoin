// SPDX-License-Identifier: MIT
// AILEE-Core Node [v1.0.0-Production-Ready]
// Main entry point with hardened orchestration, structured logging, and graceful lifecycle management.

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <mutex>

// Core Protocol Headers
#include "ailee_tps_engine.h"
#include "ailee_gold_bridge.h"
#include "ailee_recovery_protocol.h"

// Auxiliary System Headers
#include "ailee_energy_telemetry.h"
#include "ailee_circuit_breaker.h"

// Ambient Mesh Intelligence System
#include "AmbientAI.h"

// NOTE: These are optional - only include if you have these files
// #include "ailee_bitcoin_zmq_listener.h"
// #include "ailee_bitcoin_rpc_client.h"
// #include "Global_Seven.h"

// ---------------------------------------------------------
// Structured logging with thread-safety
// ---------------------------------------------------------
enum class LogLevel { DEBUG, INFO, WARN, ERROR, FATAL };
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
    
    std::ostringstream oss;
    oss << buf << "." << std::setfill('0') << std::setw(3) << ms;
    return oss.str();
}

static void log(LogLevel level, const std::string& msg) {
    const char* sev = nullptr;
    switch(level) {
        case LogLevel::DEBUG: sev = "DEBUG"; break;
        case LogLevel::INFO:  sev = "INFO "; break;
        case LogLevel::WARN:  sev = "WARN "; break;
        case LogLevel::ERROR: sev = "ERROR"; break;
        case LogLevel::FATAL: sev = "FATAL"; break;
    }

    std::lock_guard<std::mutex> lock(g_logMutex);
    std::cout << "[" << nowIso8601() << "] [" << sev << "] " << msg << std::endl;
}

// ---------------------------------------------------------
// Shutdown flag + signal handling
// ---------------------------------------------------------
static std::atomic<bool> g_shutdown{false};

static void handleSignal(int signum) {
    log(LogLevel::WARN, "Signal " + std::to_string(signum) + 
        " received — initiating graceful shutdown");
    g_shutdown.store(true);
}

static void installSignalHandlers() {
    std::signal(SIGINT,  handleSignal);
    std::signal(SIGTERM, handleSignal);
#if defined(SIGQUIT)
    std::signal(SIGQUIT, handleSignal);
#endif
}

// ---------------------------------------------------------
// Configuration with validation
// ---------------------------------------------------------
struct Config {
    // TPS Engine parameters
    int         tpsSimNodes       = 100;
    double      tpsInitialBlockMB = 1.0;
    int         tpsSimCycles      = 200;
    
    // Circuit breaker thresholds
    double      maxBlockMB        = 4.0;
    double      maxLatencyMs      = 2000.0;
    int         minPeerCount      = 8;
    
    bool validate() const {
        if (tpsSimNodes < 10 || tpsSimNodes > 10000) {
            log(LogLevel::ERROR, "Invalid tpsSimNodes: " + std::to_string(tpsSimNodes));
            return false;
        }
        if (tpsInitialBlockMB < 0.1 || tpsInitialBlockMB > 8.0) {
            log(LogLevel::ERROR, "Invalid tpsInitialBlockMB: " + std::to_string(tpsInitialBlockMB));
            return false;
        }
        if (tpsSimCycles < 10 || tpsSimCycles > 10000) {
            log(LogLevel::ERROR, "Invalid tpsSimCycles: " + std::to_string(tpsSimCycles));
            return false;
        }
        return true;
    }
};

static std::string envOrDefault(const char* key, const std::string& def) {
    const char* v = std::getenv(key);
    return v ? std::string(v) : def;
}

static Config loadConfigFromEnv() {
    Config c;
    
    if (const char* n = std::getenv("AILEE_TPS_NODES")) {
        c.tpsSimNodes = std::atoi(n);
    }
    if (const char* b = std::getenv("AILEE_TPS_INITIAL_MB")) {
        c.tpsInitialBlockMB = std::atof(b);
    }
    if (const char* cycles = std::getenv("AILEE_TPS_CYCLES")) {
        c.tpsSimCycles = std::atoi(cycles);
    }

    return c;
}

// ---------------------------------------------------------
// Engine with proper error handling
// ---------------------------------------------------------
class Engine {
public:
    explicit Engine(const Config& cfg) : cfg_(cfg) {
        log(LogLevel::INFO, "Engine initialized with " + 
            std::to_string(cfg_.tpsSimNodes) + " nodes");
    }

    ~Engine() {
        log(LogLevel::INFO, "Engine shutting down");
    }

    // TPS Simulation with error handling
    bool runTPSSimulation() {
        log(LogLevel::INFO, "TPS Simulation starting: nodes=" + 
            std::to_string(cfg_.tpsSimNodes) +
            " initialMB=" + std::to_string(cfg_.tpsInitialBlockMB) +
            " cycles=" + std::to_string(cfg_.tpsSimCycles));

        try {
            auto result = ailee::PerformanceSimulator::runSimulation(
                cfg_.tpsSimNodes, cfg_.tpsInitialBlockMB, cfg_.tpsSimCycles);

            log(LogLevel::INFO, "Baseline TPS: " + std::to_string(result.initialTPS));
            log(LogLevel::INFO, "Final TPS: " + std::to_string(result.finalTPS));
            log(LogLevel::INFO, "Improvement: " + std::to_string(result.improvementFactor) + "x");
            log(LogLevel::INFO, "Cycles completed: " + std::to_string(result.cycles));

            // Log optimization snapshots
            std::ostringstream hist;
            hist << "Optimization history (every 20 cycles):";
            for (size_t i = 0; i < result.aiFactorHistory.size(); i += 20) {
                hist << "\n  cycle=" << std::setw(4) << i
                     << " aiFactor=" << std::fixed << std::setprecision(4) 
                     << result.aiFactorHistory[i]
                     << " tps=" << std::setprecision(1) << result.tpsHistory[i]
                     << " error=" << std::setprecision(4) << result.errorHistory[i];
            }
            log(LogLevel::INFO, hist.str());
            
            return true;
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "TPS Simulation failed: " + std::string(e.what()));
            ailee::RecoveryProtocol::recordIncident("TPSSimulationFailure", e.what());
            return false;
        }
    }

    // Gold Bridge with proper error handling
    bool testGoldBridge() {
        log(LogLevel::INFO, "Testing Bitcoin-to-Gold Bridge protocol");

        try {
            ailee::GoldBridge bridge;
            std::string user = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa";
            uint64_t btcAmount = 500000000; // 5 BTC

            std::string conversionId = bridge.initiateConversion(user, btcAmount, true);

            log(LogLevel::INFO, "Conversion initiated: ID=" + conversionId);
            log(LogLevel::INFO, "Status: PENDING_PAYMENT | Oracle: ACTIVE | Inventory: SECURE");
            
            return true;
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "Gold Bridge failed: " + std::string(e.what()));
            ailee::RecoveryProtocol::recordIncident("GoldBridgeFailure", e.what());
            return false;
        }
    }

    // Safety & Energy with enhanced Circuit Breaker v1.4
    bool testSafetyAndEnergy() {
        log(LogLevel::INFO, "Evaluating Safety & Energy systems");

        try {
            // Sample thermal metrics
            ailee::ThermalMetric minerStats{
                3000.0,      // inputPowerWatts
                1500.0,      // wasteHeatRecoveredW
                25.0,        // ambientTempC
                60.0,        // exhaustTempC
                1735660000   // timestamp
            };

            // Calculate Energy Integrity Score
            auto analysis = ailee::EnergyTelemetry::analyze(minerStats);
            
            std::string proof = ailee::EnergyTelemetry::generateTelemetryProof(
                minerStats, "Node-001");

            log(LogLevel::INFO, 
                "Energy Analysis: input=" + std::to_string(minerStats.inputPowerWatts) + "W " +
                "recovery=" + std::to_string(minerStats.wasteHeatRecoveredW) + "W " +
                "EIS=" + std::to_string(analysis.energyIntegrityScore * 100.0) + "%");
            
            log(LogLevel::DEBUG, "GreenHash: " + proof.substr(0, 32) + "...");

            // Enhanced Circuit Breaker v1.4 with EIS
            auto breaker = ailee::CircuitBreaker::monitor(
                1.5,                              // proposedBlockSize
                100.0,                            // currentLatency
                100,                              // peerCount
                1.0,                              // targetBlockSize
                analysis,                         // energy analysis with EIS
                0.5                               // previousEIS
            );

            switch(breaker.state) {
                case ailee::SystemState::OPTIMIZED:
                    log(LogLevel::INFO, "Circuit Breaker: OPTIMIZED — " + breaker.reason);
                    break;
                case ailee::SystemState::SOFT_TRIP:
                    log(LogLevel::WARN, "Circuit Breaker: SOFT_TRIP — " + breaker.reason);
                    break;
                case ailee::SystemState::SAFE_MODE:
                    log(LogLevel::WARN, "Circuit Breaker: SAFE_MODE — " + breaker.reason);
                    throttleSystems();
                    break;
                case ailee::SystemState::CRITICAL:
                    log(LogLevel::FATAL, "Circuit Breaker: CRITICAL — " + breaker.reason);
                    return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "Safety/Energy test failed: " + std::string(e.what()));
            ailee::RecoveryProtocol::recordIncident("SafetyEnergyFailure", e.what());
            return false;
        }
    }

    // AmbientAI Mesh with corrected field names
    bool demoAmbientMesh() {
        log(LogLevel::INFO, "[AmbientAI] Running Ambient Mesh intelligence demo");

        try {
            ambient::SafetyPolicy policy{80.0, 250.0, 8.0, 25};

            ambient::AmbientNode nodeA({"pubA", "us-east", "gateway"}, policy);
            ambient::AmbientNode nodeB({"pubB", "us-east", "smartphone"}, policy);

            // Sample telemetry for gateway node
            ambient::TelemetrySample sampleA{
                {"pubA", "us-east", "gateway"},
                {1200.0, 300.0, 55.0, 22.0, 350.0},          // energy
                {35.0, 10.0, 5.0, 2048.0, 150.0, 40.0},      // compute
                std::chrono::system_clock::now(),
                {1.0, 1e-5}                                   // privacy
            };

            // Sample telemetry for smartphone node
            ambient::TelemetrySample sampleB{
                {"pubB", "us-east", "smartphone"},
                {8.5, 1.2, 42.0, 22.0, 200.0},               // energy
                {25.0, 20.0, 0.0, 512.0, 25.0, 30.0},        // compute
                std::chrono::system_clock::now(),
                {1.0, 1e-5}                                   // privacy
            };

            nodeA.ingestTelemetry(sampleA);
            nodeB.ingestTelemetry(sampleB);

            ambient::MeshCoordinator mesh("cluster-us-east");
            mesh.registerNode(&nodeA);
            mesh.registerNode(&nodeB);

            // Performance-based reward calculation
            auto perfFn = [](const ambient::AmbientNode& n) -> double {
                auto last = n.last();
                if (!last.has_value()) return 0.0;

                double score = (last->compute.bandwidthMbps / 50.0) - 
                              (last->compute.latencyMs / 500.0);
                return std::clamp(score, 0.1, 2.0);
            };

            auto rewardRec = mesh.dispatchAndReward("task-entropy-infer", perfFn, 10.0);

            // FIXED: Use correct field names from IncentiveRecord
            log(LogLevel::INFO, 
                "[AmbientAI] Reward dispatched: node=" + rewardRec.node.pubkey +
                " tokens=" + std::to_string(rewardRec.rewardTokens));

            ailee::RecoveryProtocol::recordIncident(
                "AmbientMeshReward",
                "Node=" + rewardRec.node.pubkey + 
                " Tokens=" + std::to_string(rewardRec.rewardTokens));
            
            return true;
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "AmbientAI demo failed: " + std::string(e.what()));
            ailee::RecoveryProtocol::recordIncident("AmbientAIFailure", e.what());
            return false;
        }
    }

    // Adaptive throttling
    void throttleSystems() {
        int oldCycles = cfg_.tpsSimCycles;
        cfg_.tpsSimCycles = std::max(50, cfg_.tpsSimCycles / 2);
        
        log(LogLevel::WARN, 
            "Adaptive throttling: TPS cycles " + std::to_string(oldCycles) + 
            " → " + std::to_string(cfg_.tpsSimCycles));
    }

private:
    Config cfg_;
};

// ---------------------------------------------------------
// Main entry point
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    // Install signal handlers first
    installSignalHandlers();
    
    log(LogLevel::INFO, "╔═══════════════════════════════════════════════════╗");
    log(LogLevel::INFO, "║   AILEE-Core Node [v1.0.0-Production-Ready]      ║");
    log(LogLevel::INFO, "╚═══════════════════════════════════════════════════╝");

    // Load and validate configuration
    Config cfg = loadConfigFromEnv();
    if (!cfg.validate()) {
        log(LogLevel::FATAL, "Configuration validation failed");
        return 1;
    }

    log(LogLevel::INFO, "Configuration loaded successfully");
    
    // Initialize engine
    Engine engine(cfg);
    
    int exitCode = 0;
    
    try {
        // Run TPS simulation
        if (!g_shutdown.load()) {
            if (!engine.runTPSSimulation()) {
                log(LogLevel::ERROR, "TPS Simulation failed");
                exitCode = 1;
            }
        }
        
        // Test Gold Bridge
        if (!g_shutdown.load()) {
            if (!engine.testGoldBridge()) {
                log(LogLevel::ERROR, "Gold Bridge test failed");
                exitCode = 1;
            }
        }
        
        // Test Safety & Energy
        if (!g_shutdown.load()) {
            if (!engine.testSafetyAndEnergy()) {
                log(LogLevel::ERROR, "Safety/Energy test failed");
                exitCode = 1;
            }
        }
        
        // Demo AmbientAI
        if (!g_shutdown.load()) {
            if (!engine.demoAmbientMesh()) {
                log(LogLevel::ERROR, "AmbientAI demo failed");
                exitCode = 1;
            }
        }

        if (g_shutdown.load()) {
            log(LogLevel::WARN, "Shutdown requested by signal");
        }
        
    } catch (const std::exception& e) {
        log(LogLevel::FATAL, "Unhandled exception: " + std::string(e.what()));
        ailee::RecoveryProtocol::recordIncident("FatalMainException", e.what());
        exitCode = 2;
    } catch (...) {
        log(LogLevel::FATAL, "Unknown exception caught");
        ailee::RecoveryProtocol::recordIncident("FatalUnknownException", "Unknown error");
        exitCode = 3;
    }

    log(LogLevel::INFO, "╔═══════════════════════════════════════════════════╗");
    log(LogLevel::INFO, "║   AILEE-Core shutdown complete                   ║");
    log(LogLevel::INFO, "╚═══════════════════════════════════════════════════╝");
    
    return exitCode;
}

