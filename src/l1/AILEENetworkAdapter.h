#pragma once

#include <vector>
#include <string>

namespace ailee::l1 {

struct NetworkLatencySnapshot {
    double mean_rtt_ms;
    double p95_rtt_ms;
    double propagation_skew; // normalized [0,1]
};

class AILEENetworkAdapter {
public:
    explicit AILEENetworkAdapter(
        const std::vector<std::string>& peer_endpoints
    );

    NetworkLatencySnapshot snapshot() const;

private:
    std::vector<std::string> peers_;

    double computePropagationSkew(double mean,
                                  double p95) const;
};

} // namespace ailee::l1
