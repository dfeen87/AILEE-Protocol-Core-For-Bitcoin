/**
 * AILEE Energy Telemetry Protocol
 * 
 * Cryptographic verification of thermodynamic efficiency.
 * Connects physical waste-heat recovery sensors to the blockchain.
 * 
 * License: MIT
 * Author: Don Michael Feeney Jr
 */

#ifndef AILEE_ENERGY_TELEMETRY_H
#define AILEE_ENERGY_TELEMETRY_H

#include <string>
#include <vector>
#include <cmath>
#include <openssl/sha.h>

namespace ailee {

struct ThermalMetric {
    double inputPowerWatts;      // e.g., 3000W ASIC
    double wasteHeatRecoveredW;  // e.g., 1500W recycled into grid
    double ambientTempC;
    double exhaustTempC;
    uint64_t timestamp;
};

class EnergyTelemetry {
public:
    // Calculate the TE (Thermodynamic Efficiency) Score (0.0 to 1.0)
    static double calculateEfficiencyScore(const ThermalMetric& metric) {
        if (metric.inputPowerWatts <= 0) return 0.0;
        
        // Efficiency = Recovered Heat / Total Input Energy
        double efficiency = metric.wasteHeatRecoveredW / metric.inputPowerWatts;
        
        // Cap at 1.0 (Laws of Thermodynamics)
        return (efficiency > 1.0) ? 1.0 : efficiency;
    }

    // Generate a "Green Hash" proof of this energy data
    static std::string generateTelemetryProof(const ThermalMetric& metric, const std::string& nodeId) {
        double score = calculateEfficiencyScore(metric);
        
        std::string rawData = nodeId + 
                              std::to_string(metric.inputPowerWatts) + 
                              std::to_string(metric.wasteHeatRecoveredW) +
                              std::to_string(score) + 
                              std::to_string(metric.timestamp);

        // SHA256 Hash
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256((unsigned char*)rawData.c_str(), rawData.size(), hash);

        char hexString[SHA256_DIGEST_LENGTH * 2 + 1];
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            sprintf(hexString + (i * 2), "%02x", hash[i]);
        }
        return std::string(hexString);
    }
};

} // namespace ailee
#endif // AILEE_ENERGY_TELEMETRY_H
