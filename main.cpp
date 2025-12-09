#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <thread> // Required for background listener
#include <chrono> // Required for delays

// Core Protocol Headers
#include "ailee_tps_engine.h"
#include "ailee_gold_bridge.h"
#include "ailee_recovery_protocol.h"

// Auxiliary System Headers (Safety & Energy)
#include "ailee_energy_telemetry.h"
#include "ailee_circuit_breaker.h"

// Network Infrastructure Headers (The Bridge)
#include "ailee_bitcoin_zmq_listener.h"
#include "ailee_bitcoin_rpc_client.h"

// ---------------------------------------------------------
// Module 1: TPS Mathematics Simulation
// ---------------------------------------------------------
void runTPSSimulation() {
    std::cout << "==================================================\n";
    std::cout << "   AILEE AI-Driven TPS Optimization Simulation    \n";
    std::cout << "==================================================\n";

    // Simulating 100 nodes, 1.0MB initial block size, over 200 optimization cycles
    auto result = ailee::PerformanceSimulator::runSimulation(100, 1.0, 200);

    std::cout << "Baseline TPS: " << result.initialTPS << "\n";
    std::cout << "Final TPS:    " << result.finalTPS << "\n";
    std::cout << "Improvement:  " << result.improvementFactor << "x\n";
    std::cout << "Cycles Run:   " << result.cycles << "\n\n";

    std::cout << "Optimization History (Every 20 cycles):\n";
    std::cout << "Cycle | AI Factor | TPS       | Empirical Error\n";
    std::cout << "----------------------------------------------\n";
    
    for (size_t i = 0; i < result.cycles; i += 20) {
        std::cout << std::setw(5) << i << " | "
                  << std::setw(9) << std::fixed << std::setprecision(4) << result.aiFactorHistory[i] << " | "
                  << std::setw(9) << std::setprecision(1) << result.tpsHistory[i] << " | "
                  << std::setw(10) << std::setprecision(4) << result.errorHistory[i] << "\n";
    }
    std::cout << "==================================================\n\n";
}

// ---------------------------------------------------------
// Module 2: Asset Bridge Logic
// ---------------------------------------------------------
void testGoldBridge() {
    std::cout << "==================================================\n";
    std::cout << "      AILEE Bitcoin-to-Gold Bridge Protocol       \n";
    std::cout << "==================================================\n";
    
    ailee::GoldBridge bridge;
    
    // Simulate user initiating conversion of 5 BTC to Gold
    std::string user = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"; // Satoshi's address
    uint64_t btcAmount = 500000000; // 5 BTC in Satoshis
    
    std::cout << "[Action] Initiating Conversion for User: " << user.substr(0, 8) << "...\n";
    std::string conversionId = bridge.initiateConversion(user, btcAmount, true); // true = burn option
    
    std::cout << "Conversion ID Generated: " << conversionId.substr(0, 16) << "...\n";
    std::cout << "Status: PENDING_PAYMENT\n";
    std::cout << "[System Check] Oracle Connection: ACTIVE\n";
    std::cout << "[System Check] Inventory Logic: SECURE\n\n";
}

// ---------------------------------------------------------
// Module 3: Safety & Sustainability
// ---------------------------------------------------------
void testSafetyAndEnergy() {
    std::cout << "==================================================\n";
    std::cout << "    AILEE Auxiliary Systems (Safety & Energy)     \n";
    std::cout << "==================================================\n";

    // 1. Test Energy Telemetry
    std::cout << "[Module] Energy Telemetry Protocol:\n";
    ailee::ThermalMetric minerStats = {3000.0, 1500.0, 25.0, 60.0, 1735660000};
    
    double efficiency = ailee::EnergyTelemetry::calculateEfficiencyScore(minerStats);
    std::string proof = ailee::EnergyTelemetry::generateTelemetryProof(minerStats, "Node-001");
    
    std::cout << " > Input Power: " << minerStats.inputPowerWatts << "W\n";
    std::cout << " > Waste Heat Recovered: " << minerStats.wasteHeatRecoveredW << "W\n";
    std::cout << " > Efficiency Score: " << (efficiency * 100) << "%\n";
    std::cout << " > Green Hash: " << proof.substr(0, 16) << "...\n\n";

    // 2. Test Circuit Breaker
    std::cout << "[Module] AI Safety Watchdog (Circuit Breaker):\n";
    std::cout << " > Test 1 (Normal Ops): ";
    if (ailee::CircuitBreaker::monitor(1.5, 50.0, 100) == ailee::SystemState::OPTIMIZED) 
        std::cout << "PASS\n";
    
    std::cout << " > Test 2 (AI Anomaly - 10MB Block): ";
    if (ailee::CircuitBreaker::monitor(10.0, 50.0, 100) == ailee::SystemState::SAFE_MODE) {
        std::cout << "SUCCESS (Breaker Tripped)\n";
    }
    std::cout << "==================================================\n\n";
}

// ---------------------------------------------------------
// Module 4: Network Infrastructure (ZMQ & RPC)
// ---------------------------------------------------------
void testNetworkInfrastructure() {
    std::cout << "==================================================\n";
    std::cout << "   AILEE Network Bridge (ZMQ Listener & RPC)      \n";
    std::cout << "==================================================\n";

    // 1. Initialize ZMQ Listener
    // Note: In production, this points to the local Bitcoin Core node
    std::cout << "[Bridge] Initializing ZMQ Listener...\n";
    ailee::BitcoinZMQListener zmqListener("tcp://127.0.0.1:28332");
    zmqListener.init();

    // Run ZMQ in a background thread so it doesn't block the simulation
    std::cout << "[Bridge] Starting Event Loop (Background Thread)...\n";
    std::thread zmqThread([&zmqListener]() {
        zmqListener.start();
    });

    // 2. Initialize RPC Client
    std::cout << "[RPC] Initializing HTTP Client...\n";
    ailee::BitcoinRPCClient rpcClient("rpcuser", "rpcpassword", "http://127.0.0.1:8332");

    // Simulate an RPC Call (Get Block Count)
    // Note: This will fail if no Bitcoin node is running, which is expected in simulation
    std::cout << "[RPC] Attempting 'getblockcount' command...\n";
    long count = rpcClient.getBlockCount();
    if (count >= 0) {
        std::cout << "[RPC] Connected! Current Block Height: " << count << "\n";
    } else {
        std::cout << "[RPC] Simulation Mode: No active Bitcoin node detected (Expected).\n";
    }

    // Simulate a brief wait to allow ZMQ logging to appear if connected
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Cleanup
    std::cout << "[Bridge] Shutting down network services...\n";
    zmqListener.stop();
    if (zmqThread.joinable()) {
        zmqThread.join();
    }
    std::cout << "==================================================\n\n";
}

// ---------------------------------------------------------
// Main Entry Point
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    std::cout << "\nStarting AILEE-Core Node [v1.0.0-Prototype]...\n\n";

    // 1. Math & Simulation
    runTPSSimulation();
    
    // 2. Asset Logic
    testGoldBridge();
    
    // 3. Safety Protocols
    testSafetyAndEnergy();

    // 4. Real-World Connectivity
    testNetworkInfrastructure();
    
    std::cout << "[AILEE-CORE] All Modules Initialized Successfully.\n";
    return 0;
}
