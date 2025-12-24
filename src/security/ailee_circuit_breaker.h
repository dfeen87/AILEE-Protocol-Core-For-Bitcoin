/**
 * ailee_circuit_breaker.cpp
 *
 * Implements the AILEE Autonomous Circuit Breaker (canonical v1.4)
 *
 * License: MIT
 * Author: Don Michael Feeney Jr
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <mutex>
#include <sstream>

// Optional: record incidents (header may exist in your tree)
#include "ailee_recovery_protocol.h"
#include "ailee_energy_telemetry.h"

namespace ailee {

enum class SystemState {
    OPTIMIZED,
    SOFT_TRIP,
    SAFE_MODE,
    CRITICAL
};

struct BreakerReport {
    SystemState state = SystemState::OPTIMIZED;
    std::string reason;
    double eis = 0.0;
    double entropyDelta = 0.0;
    double driftScore = 0.0;
};

class CircuitBreaker {
public:
    static constexpr double MAX_SAFE_BLOCK_SIZE_MB = 4.0;
    static constexpr double MAX_LATENCY_TOLERANCE_MS = 2000.0;
    static constexpr int MIN_PEER_COUNT = 8;
    static constexpr double MAX_ENTROPY_SURGE_DELTA = 0.25;
    static constexpr double MIN_EIS_FOR_OPTIMIZATION = 0.5;
    static constexpr double MAX_AI_DRIFT_SCORE = 0.5;

    static double computeAIDrift(double targetBlockSize, double proposedBlockSize);

    static BreakerReport monitor(
        double proposedBlockSize,
        double currentLatency,
        int peerCount,
        double targetBlockSize,
        const EnergyAnalysis& energy,
        double previousEIS
    );
};

// Internal module state to support hysteresis and rate-limited transitions
namespace {
    std::mutex g_stateMutex;
    SystemState g_lastState = SystemState::OPTIMIZED;
    std::chrono::system_clock::time_point g_lastTransitionTime = std::chrono::system_clock::now();
    // Minimum time between state transitions to prevent oscillation
    const std::chrono::seconds TRANSITION_HYSTERESIS = std::chrono::seconds(10);

    // Helper logger (replaceable with your structured logger)
    void localLog(const std::string& level, const std::string& msg) {
        std::ostringstream ss;
        auto now = std::chrono::system_clock::now();
        auto s = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        ss << "[" << s << "] [" << level << "] " << msg;
        std::cerr << ss.str() << std::endl;
    }

    // Rate-limited transition helper
    bool allowTransition() {
        auto now = std::chrono::system_clock::now();
        std::lock_guard<std::mutex> lock(g_stateMutex);
        if ((now - g_lastTransitionTime) < TRANSITION_HYSTERESIS) return false;
        g_lastTransitionTime = now;
        return true;
    }

    // Safe wrapper to attempt RecoveryProtocol recording (if available)
    void recordIncidentSafe(const std::string& key, const std::string& detail) {
        try {
            ailee::RecoveryProtocol::recordIncident(key, detail);
        } catch (...) {
            // Swallow exceptions to avoid cascading failures in breaker logic
            localLog("WARN", std::string("RecoveryProtocol record failed for: ") + key);
        }
    }
} // namespace (internal)

inline double CircuitBreaker::computeAIDrift(double targetBlockSize, double proposedBlockSize) {
    if (targetBlockSize <= 0.0) return 0.0;
    return std::fabs(proposedBlockSize - targetBlockSize) / targetBlockSize;
}

inline BreakerReport CircuitBreaker::monitor(
    double proposedBlockSize,
    double currentLatency,
    int peerCount,
    double targetBlockSize,
    const EnergyAnalysis& energy,
    double previousEIS
) {
    BreakerReport report;
    report.state = SystemState::OPTIMIZED;
    report.reason = "Unknown";
    report.eis = energy.energyIntegrityScore;
    report.entropyDelta = std::fabs(energy.energyIntegrityScore - previousEIS);
    report.driftScore = computeAIDrift(targetBlockSize, proposedBlockSize);

    // --------------- Hard Red-lines ----------------
    if (proposedBlockSize > MAX_SAFE_BLOCK_SIZE_MB) {
        report.state = SystemState::SAFE_MODE;
        report.reason = "Unsafe block size proposal — exceeds MAX_SAFE_BLOCK_SIZE_MB.";
        localLog("ALERT", report.reason + " proposed=" + std::to_string(proposedBlockSize));
        recordIncidentSafe("CircuitBreaker_HardBlockSize", report.reason);
        // Update last state
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            g_lastState = report.state;
        }
        return report;
    }

    if (currentLatency > MAX_LATENCY_TOLERANCE_MS) {
        report.state = SystemState::SAFE_MODE;
        report.reason = "Network latency exceeds tolerance.";
        localLog("ALERT", report.reason + " latency=" + std::to_string(currentLatency));
        recordIncidentSafe("CircuitBreaker_HighLatency", report.reason);
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            g_lastState = report.state;
        }
        return report;
    }

    if (peerCount < MIN_PEER_COUNT) {
        report.state = SystemState::SAFE_MODE;
        report.reason = "Peer count below safe minimum.";
        localLog("ALERT", report.reason + " peers=" + std::to_string(peerCount));
        recordIncidentSafe("CircuitBreaker_LowPeers", report.reason);
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            g_lastState = report.state;
        }
        return report;
    }

    // --------------- Entropy Surge Detection ----------------
    if (report.entropyDelta > MAX_ENTROPY_SURGE_DELTA) {
        // Soft trip: throttle AI but do not fully disable it
        report.state = SystemState::SOFT_TRIP;
        report.reason = "Entropy surge detected — EIS changed rapidly.";
        localLog("WARN", report.reason + " delta=" + std::to_string(report.entropyDelta));
        recordIncidentSafe("CircuitBreaker_EntropySurge", "delta=" + std::to_string(report.entropyDelta));

        // Apply hysteresis: only transition if allowed
        if (!allowTransition()) {
            localLog("DEBUG", "Entropy surge detected but transition suppressed by hysteresis.");
            // keep previous state if transition suppressed
            std::lock_guard<std::mutex> lock(g_stateMutex);
            report.state = g_lastState;
            report.reason += " (transition suppressed)";
        } else {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            g_lastState = report.state;
        }
        return report;
    }

    // --------------- EIS Floor Check ----------------
    if (energy.energyIntegrityScore < MIN_EIS_FOR_OPTIMIZATION) {
        // Low EIS: soft trip first. If repeatedly low, escalate to SAFE_MODE.
        report.state = SystemState::SOFT_TRIP;
        report.reason = "Energy Integrity Score below MIN_EIS_FOR_OPTIMIZATION.";
        localLog("WARN", report.reason + " EIS=" + std::to_string(energy.energyIntegrityScore));
        recordIncidentSafe("CircuitBreaker_LowEIS", "eis=" + std::to_string(energy.energyIntegrityScore));

        // If previous state was already SOFT_TRIP and transition allowed -> escalate
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            if (g_lastState == SystemState::SOFT_TRIP && allowTransition()) {
                report.state = SystemState::SAFE_MODE;
                report.reason = "Persistent low EIS — escalating to SAFE_MODE.";
                localLog("ALERT", report.reason);
                recordIncidentSafe("CircuitBreaker_EscalateLowEIS", report.reason);
                g_lastState = report.state;
            } else {
                g_lastState = report.state;
            }
        }

        return report;
    }

    // --------------- AI Drift Monitoring ----------------
    if (report.driftScore > MAX_AI_DRIFT_SCORE) {
        report.state = SystemState::SAFE_MODE;
        report.reason = "AI drift exceeds MAX_AI_DRIFT_SCORE — reverting to safe defaults.";
        localLog("ALERT", report.reason + " drift=" + std::to_string(report.driftScore));
        recordIncidentSafe("CircuitBreaker_AIDrift", "drift=" + std::to_string(report.driftScore));
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            g_lastState = report.state;
        }
        return report;
    }

    // --------------- Critical checks (aggregate risk) ----------------
    // If multiple soft signals present, escalate to SAFE_MODE
    int softSignals = 0;
    if (report.entropyDelta > (MAX_ENTROPY_SURGE_DELTA * 0.6)) softSignals++;
    if (energy.sensorConfidence < 0.25) softSignals++;
    if (currentLatency > (MAX_LATENCY_TOLERANCE_MS * 0.75)) softSignals++;
    if (softSignals >= 3) {
        report.state = SystemState::SAFE_MODE;
        report.reason = "Multiple concurrent soft signals — escalating to SAFE_MODE.";
        localLog("ALERT", report.reason);
        recordIncidentSafe("CircuitBreaker_MultiSoftSignals", report.reason);
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            g_lastState = report.state;
        }
        return report;
    }

    // --------------- All checks passed: OPTIMIZED ----------------
    report.state = SystemState::OPTIMIZED;
    report.reason = "All safety checks passed — AI optimization permitted.";
    localLog("INFO", report.reason);
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        g_lastState = report.state;
    }

    return report;
}

} // namespace ailee
