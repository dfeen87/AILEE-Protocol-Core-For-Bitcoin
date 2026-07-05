# Wave Native Routing Layer

The routing logic in WNN bypasses traditional payload address logic, instead electing pathways based on phase stability and physical synchronization capabilities.

## 1. Adaptive Routing Architecture

The centralized `wavefrontd` orchestrator evaluates global network health (sync drift, density, average trust) and dynamically selects an overarching Routing Mode. This mode informs the `RoutingEngine` on how to score downstream paths.

**Modes:**
- `LOW_LATENCY`: Prioritizes raw propagation speed. Selected when sync drift is very low (<10ppm), average trust is high (>0.7), and mesh density is high (>0.6).
- `HIGH_STABILITY`: Prioritizes signal coherence and penalizes phase misalignment heavily. Selected when drift is high (>50ppm) or mesh density drops.
- `BALANCED`: The default baseline state.

## 2. Hybrid Wavefront Routing Metrics

The `RoutingEngine` evaluates prospective hops (candidate vectors) by calculating a composite "Hybrid Distance". This distance is not geospatial; rather, it is a phase-space divergence metric.

The hybrid distance `d_WNN` is defined by the function:
`d_WNN = w_L * L + w_J * J + w_phi * |Delta_phi| + w_omega * |Delta_omega| + w_DPhi * Delta_Phi`

**Weights (Standard Balanced Mode):**
- `w_L` (Latency): 0.15
- `w_J` (Jitter): 0.15
- `w_phi` (Phase Mismatch): 0.30
- `w_omega` (Frequency Mismatch): 0.20
- `w_DPhi` (Stability Divergence): 0.20

This formulation strongly favors peers who are phase-locked over peers who simply have low ping times.

## 3. Propagation Decisions and Execution

The routing layer constructs a `RouteDecision` object:
```cpp
struct RouteDecision {
    TransportVector selected_vector; // Vector A or Vector B
    std::optional<std::vector<uint8_t>> destination_peer;
    RoutingMode mode_used;
    double estimated_latency_ms;
    double estimated_stability_score;
    bool is_valid;
};
```

1. The `RoutingEngine` ranks the `KnownPeer` map via the hybrid distance.
2. Depending on the `RoutingMode`, it selects the most viable candidate.
3. It determines the `TransportVector` based on whether the local physical medium can directly transmit continuous phase (`Vector A`) or must resort to network temporal modulation (`Vector B`).
4. If Vector B is required, Transduction (IAT modulation) is executed.

## 4. Integration with PLL
If the global network Distributed Phase-Locked Loop (PLL) is unstable or broken, the `RoutingEngine` automatically applies stability-based penalizations to the metrics, artificially inflating the distances of unproven nodes to force the signal to route through highly trusted Phase Coherence Anchors.
