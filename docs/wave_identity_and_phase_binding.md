# Wave Identity and Phase Binding

WNN does not utilize standard IP addresses for peer identification at the physical/phase layer. Instead, it binds identities directly to the observable phase and deterministic properties of a node.

## 1. Signatures as Identities

Peers are identified strictly by a raw byte array:
`using PeerId = std::vector<uint8_t>;`

This signature is preserved to maintain compatibility with legacy routing and KnownPeer logic but deliberately avoids creating a redundant discrete identity system. An identity only has value when bound to a verifiable physical phase state.

## 2. Zero-Trust Evaluation

WNN operates on a strict zero-trust network model, backed by the AILEE Trust Layer integration.
- Identity is not assumed via a handshake; it is continuously evaluated.
- A peer's physical phase state is continuously monitored. If its synchronization drifts, or if its transduced timing packets violate the established entropy baseline (indicating interference or replay), its `AileeTrustScore` degrades.

## 3. Phase Coherence Anchors

The network relies on specific reference nodes known as Phase Coherence Anchors.
These are high-stability nodes (e.g., orbital or high-infrastructure edge nodes) that serve as foundational reference points for the entire network's synchronization.

Anchors have a numeric `AnchorId` (a 32-bit unsigned integer), which can be mapped down into the `PeerId` byte signature format (e.g., extracting the pseudo-anchor ID from the first 4 bytes of a vector).

**Trust Scaling:**
Trust is highly non-linear. The `CoherenceEngine` groups anchors into `CoherenceCluster` structs. If an anchor falls out of phase with its cluster (verified by Orthogonal Handshakes), its individual trust score collapses, preventing it from influencing the `DistributedPllController`.

## 4. State Representation

Peers are stored in the routing subsystem as `KnownPeer` structures:
```cpp
struct KnownPeer {
    std::vector<uint8_t> signature;
    double spectral_frequency;
    mesh_legacy::AileeTrustScore trust_score;
    std::atomic<double> latency_ema;
    std::atomic<double> jitter_ema;
    long double phase_theta;
};
```
Identity and Phase are bound mathematically in this structure. The node's reputation (`trust_score`) is directly correlated with its historical ability to maintain a matching `phase_theta` and `spectral_frequency` within the bounds established by the Anchors.
