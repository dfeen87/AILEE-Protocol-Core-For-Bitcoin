// AmbientAI.cpp
// Supplemental helpers for AmbientAI.h (serialization, debug print)

#include "AmbientAI.h"
#include <sstream>

namespace ambient {

std::string to_string(const NodeId& n) {
    std::ostringstream ss;
    ss << "NodeId(pubkey=" << n.pubkey
       << ", region=" << n.region
       << ", deviceClass=" << n.deviceClass << ")";
    return ss.str();
}

std::string to_string(const EnergyProfile& e) {
    std::ostringstream ss;
    ss << "Energy(inputPowerW=" << e.inputPowerW
       << ", wasteHeatRecoveredW=" << e.wasteHeatRecoveredW
       << ", temperatureC=" << e.temperatureC
       << ", ambientTempC=" << e.ambientTempC
       << ", carbon_gCO2/kWh=" << e.carbonIntensity_gCO2_kWh << ")";
    return ss.str();
}

std::string to_string(const ComputeProfile& c) {
    std::ostringstream ss;
    ss << "Compute(cpu%=" << c.cpuUtilization
       << ", npu%=" << c.npuUtilization
       << ", gpu%=" << c.gpuUtilization
       << ", memMB=" << c.availableMemMB
       << ", bwMbps=" << c.bandwidthMbps
       << ", latMs=" << c.latencyMs << ")";
    return ss.str();
}

std::string to_string(const TelemetrySample& s) {
    std::ostringstream ss;
    ss << "{ node=" << to_string(s.node)
       << ", energy=" << to_string(s.energy)
       << ", compute=" << to_string(s.compute)
       << ", privacy(eps=" << s.privacy.epsilon << ", delta=" << s.privacy.delta << ") }";
    return ss.str();
}

} // namespace ambient
