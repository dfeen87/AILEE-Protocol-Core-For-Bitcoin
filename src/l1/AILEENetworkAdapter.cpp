#include "AILEENetworkAdapter.h"

namespace ailee::l1 {

AILEENetworkAdapter::AILEENetworkAdapter(
    const std::vector<std::string>& peer_endpoints
) : peers_(peer_endpoints) {}

NetworkLatencySnapshot AILEENetworkAdapter::snapshot() const {
    // Observational adapter — no routing or peer selection.
    double mean_rtt = 0.0;
    double p95_rtt = 0.0;

    return {
        mean_rtt,
        p95_rtt,
        computePropagationSkew(mean_rtt, p95_rtt)
    };
}

double AILEENetworkAdapter::computePropagationSkew(
    double mean,
    double p95
) const {
    if (mean <= 0.0) return 0.0;
    double skew = (p95 - mean) / mean;
    return skew < 0.0 ? 0.0 : (skew > 1.0 ? 1.0 : skew);
}

} // namespace ailee::l1
