#include "AILEEMempoolAdapter.h"

namespace ailee::l1 {

AILEEMempoolAdapter::AILEEMempoolAdapter(
    const std::string& node_endpoint
) : node_endpoint_(node_endpoint) {}

MempoolSnapshot AILEEMempoolAdapter::snapshot() const {
    // NOTE: This adapter is observational only.
    // Real data sources may include Bitcoin RPC or ZMQ feeds.

    uint64_t tx_count = 0;        // placeholder
    double avg_fee = 0.0;         // sat/vB
    double arrival_rate = 0.0;    // tx/sec

    return {
        tx_count,
        avg_fee,
        arrival_rate,
        computeCongestionIndex(tx_count, arrival_rate)
    };
}

double AILEEMempoolAdapter::computeCongestionIndex(
    uint64_t tx_count,
    double arrival_rate
) const {
    // Bounded, monotonic congestion signal
    // Explicitly avoids hard-coded Bitcoin limits
    double load = static_cast<double>(tx_count) * arrival_rate;
    return load > 0.0 ? load / (load + 1.0) : 0.0;
}

} // namespace ailee::l1
