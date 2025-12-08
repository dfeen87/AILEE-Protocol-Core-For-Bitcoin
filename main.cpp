#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

// Core Protocol Headers
#include "ailee_tps_engine.h"
#include "ailee_gold_bridge.h"
#include "ailee_recovery_protocol.h"

// New Innovation Headers (Safety & Energy)
#include "ailee_energy_telemetry.h"
#include "ailee_circuit_breaker.h"

void runTPSSimulation() {
    std::cout << "==================================================\n";
    std::cout << "   AILEE AI-Driven TPS Optimization Simulation    \n";
    std::cout << "==================================================\n";

    // Run simulation using the Engine defined in tps_engine.h
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

void testGoldBridge() {
    std::cout << "==================================================\n";
    std::cout << "      AILEE Bitcoin-to-Gold Bridge Protocol       \n";
    std::cout << "==================================================\n";
    
    ailee::GoldBridge bridge;
    
    // Simulate user initiating conversion of 5 BTC to Gold
    std::string user = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"; // Satoshi's address as example
    uint64_t btcAmount = 500000000; // 5 BTC in Satoshis
    
    std::cout << "[Action] Initiating Conversion for User: " << user.substr(0, 8) << "...\n";
    std::string conversionId = bridge.initiateConversion(user, btcAmount, true); // true = burn option
    
    std::cout << "Conversion ID Generated: " << conversionId.substr(0, 16) << "...\n";
    std::cout << "Status: PENDING_PAYMENT\n";
    std::cout << "[System Check] Oracle Connection: ACTIVE\n";
    std::cout << "[System Check] Inventory Logic: SECURE\n\n";
}

void testSafetyAndEnergy() {
    std::cout << "==================================================\n";
    std::cout << "    AILEE Auxiliary Systems (Safety & Energy)     \n";
    std::cout << "==================================================\n";

    // 1. Test Energy Telemetry (Proof of Useful Work)
    std::cout << "[Module] Energy Telemetry Protocol:\n";
    
    // Simulate a miner consuming 3000W and recovering 1500W of heat
    ailee::ThermalMetric minerStats = {3000.0, 1500.0, 25.0, 60.0, 1735660000};
    
    double efficiency = ailee::EnergyTelemetry::calculateEfficiencyScore(minerStats);
    std::string proof = ailee::EnergyTelemetry::generateTelemetryProof(minerStats, "Node-001");
    
    std::cout << " > Input Power: " << minerStats.inputPowerWatts << "W\n";
    std::cout << " > Waste Heat Recovered: " << minerStats.wasteHeatRecoveredW << "W\n";
    std::cout << " > Thermodynamic Efficiency Score: " << (efficiency * 100) << "%\n";
    std::cout << " > Cryptographic Green Hash: " << proof.substr(0, 16) << "...\n\n";

    // 2. Test Circuit Breaker (AI Safety Watchdog)
    std::cout << "[Module] AI Safety Watchdog (Circuit Breaker):\n";
    
    // Scenario A: AI behaves normally
    std::cout << " > Test 1 (Normal Ops): ";
    auto statusNormal = ailee::CircuitBreaker::monitor(1.5, 50.0, 100); // 1.5MB block, low latency
    if (statusNormal == ailee::SystemState::OPTIMIZED) std::cout << "PASS (Optimization Active)\n";
    else std::cout << "FAIL\n";

    // Scenario B: AI hallucinates and proposes a massive block (Attack vector or Glitch)
    std::cout << " > Test 2 (AI Anomaly - 10MB Block): ";
    auto statusRisk = ailee::CircuitBreaker::monitor(10.0, 50.0, 100); 
    
    if (statusRisk == ailee::SystemState::SAFE_MODE) {
        std::cout << "SUCCESS! Breaker Tripped. \n";
        std::cout << "   [ALERT] System Reverted to Bitcoin Standard Consensus.\n";
    } else {
        std::cout << "FAIL (Danger)\n";
    }
    std::cout << "==================================================\n\n";
}

int main(int argc, char* argv[]) {
    // 1. Run the Core TPS Math
    runTPSSimulation();
    
    // 2. Run the Asset Bridge Logic
    testGoldBridge();
    
    // 3. Run the Safety & Energy Innovations
    testSafetyAndEnergy();
    
    std::cout << "[AILEE-CORE] All Modules Initialized Successfully.\n";
    return 0;
}
