# Ambient Participation Proofs for Bitcoin L2

* **AmbientParticipationProof**: The root structure binding contributions to identity.
* **AmbientContributionSummary**: The deterministic aggregation of contributions for an epoch.
* **AmbientContributionEvent**: The foundational unit of contribution (routing, sensing, uptime).
* **AmbientGiveBackHint**: The non-monetary L1 coordination benefit derived from participation.

## 1. Proof Generation and Validation

```cpp
#ifndef AMBIENT_AI_PARTICIPATION_PROOFS_HPP
#define AMBIENT_AI_PARTICIPATION_PROOFS_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include "ambient_participation_measurement.hpp"

namespace ailee {
namespace participation {

using Hash256 = std::array<uint8_t, 32>;

struct AmbientParticipationProofHeader {
    std::string publicKeyHex;         // Binds directly to IdentityPayload::publicKeyHex
    uint64_t epochId;                 // Monotonically increasing L2 epoch counter
    Hash256 summaryHash;              // Hash of the AmbientContributionSummary
    int64_t derivedEnergyScore;       // Sourced from energy::EnergyProfile::calculateEpochEnergy()

    // Deterministic binary serialization
    std::vector<uint8_t> serialize() const;
    Hash256 hash() const;
};

struct AmbientParticipationProof {
    AmbientParticipationProofHeader header;
    AmbientContributionSummary summary;
    std::vector<uint8_t> ecdsaSignature; // Strict DER-encoded secp256k1 signature over header.hash()

    // Validation rules:
    // 1. Verifies the ECDSA signature matches publicKeyHex.
    // 2. Asserts summary.hash() == header.summaryHash.
    // 3. Asserts derivedEnergyScore >= 0.
    // 4. Asserts limits via ParticipationLimits.
    bool verify() const;
};

} // namespace participation
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_PROOFS_HPP
```

### Invariants & Integration Points
* `AmbientParticipationProof` MUST map one-to-one with `ailee::identity::IdentityPayload::publicKeyHex` (defined in `ambient_ai_primitives_spec.md`).
* `derivedEnergyScore` MUST be sourced deterministically from `ailee::energy::EnergyProfile::calculateEpochEnergy()` (`ambient_ai_primitives_spec.md`).
* `AmbientParticipationProof::verify()` MUST use deterministic `libsecp256k1` primitives and validate the signature against `header.hash()`.
* verified proofs MUST be sorted lexicographically by `publicKeyHex` and hashed into a Merkle root to populate `participationRoot` within `ailee::protocol::AmbientAIAnchoringEpoch` (`bitcoin_anchoring_layer_spec.md`).

## 2. Deterministic Measurement and Anti-Cheating Rules

```cpp
#ifndef AMBIENT_AI_PARTICIPATION_MEASUREMENT_HPP
#define AMBIENT_AI_PARTICIPATION_MEASUREMENT_HPP

#include <cstdint>
#include <vector>
#include <set>
#include <array>
#include "ambient_participation_contributions.hpp"

namespace ailee {
namespace participation {

// Canonical limits to prevent overflow and spam attacks
struct ParticipationLimits {
    static constexpr uint32_t MAX_EVENTS_PER_EPOCH = 5000;
    static constexpr uint32_t MAX_SENSING_PER_TICK = 1;
};

// Summarizes a full epoch's worth of contribution events deterministically
struct AmbientContributionSummary {
    uint32_t totalRoutingSuccesses;
    uint32_t totalSensingSnapshots;
    uint32_t totalUptimeSegments;

    // Serializes into a canonical length-prefixed binary array
    std::vector<uint8_t> serialize() const;
    Hash256 hash() const;
};

// Deterministic event logger enforcing rate-limits and deduplication
class AmbientContributionLog {
public:
    // Attempts to log an event. Deterministically drops events that exceed:
    // - MAX_EVENTS_PER_EPOCH
    // - Duplicate eventId (replay protection)
    // - Rate limits (e.g., MAX_SENSING_PER_TICK)
    bool logEvent(const AmbientContributionEvent& event);

    // Finalizes the epoch's valid events into a bounded summary.
    // Clears the internal log state for the next epoch.
    AmbientContributionSummary finalizeEpochSummary();

private:
    std::vector<AmbientContributionEvent> validEvents;

    // Custom comparator for deterministic ordering of Hashes
    struct HashCompare {
        bool operator()(const Hash256& a, const Hash256& b) const;
    };
    std::set<Hash256, HashCompare> seenEventIds;
};

} // namespace participation
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_MEASUREMENT_HPP
```

### Invariants & Integration Points
* `AmbientContributionSummary` strictly merges inputs from `ailee::ambient_node::AmbientParticipationEpochSummary` (`ambient_node_mode_design.md`) and `ailee::ambient_mesh::AmbientMeshParticipationSummary` (`ambient_mesh_networking_layer.md`).
* `logEvent` MUST return `false` deterministically if bounds like `MAX_EVENTS_PER_EPOCH` are exceeded, ensuring consensus safety.
* Replay protection MUST be guaranteed across epochs via `seenEventIds`, ensuring no duplicate event is accepted per epoch.
* Overflows in `totalRoutingSuccesses`, `totalSensingSnapshots`, or `totalUptimeSegments` MUST be strictly guarded with fixed limits rather than hardware maximums.

## 3. Valid Ambient Contributions

```cpp
#ifndef AMBIENT_AI_PARTICIPATION_CONTRIBUTIONS_HPP
#define AMBIENT_AI_PARTICIPATION_CONTRIBUTIONS_HPP

#include <cstdint>
#include <vector>
#include <array>
#include <string>

namespace ailee {
namespace participation {

using Hash256 = std::array<uint8_t, 32>;

// Deterministic categories of ambient contributions
enum class ContributionType : uint8_t {
    ROUTING_SUCCESS = 0,    // Successful micro-route propagation
    SENSING_SNAPSHOT = 1,   // Valid privacy-preserving signal snapshot
    UPTIME_SEGMENT = 2      // Verified low-power node availability segment
};

// Privacy-preserving discrete buckets for sensing events
struct AmbientContributionBucket {
    uint8_t powerStateClass;     // e.g., 0=Battery, 1=Charging, 2=Mains
    uint8_t connectivityBand;    // e.g., 0=LowBandwidth, 1=HighBandwidth
    uint8_t activityLevel;       // Deterministically discretized activity index

    // Serializes to a strictly bounded byte array
    std::vector<uint8_t> serialize() const;
};

// Represents a single, deterministic contribution event logged by a node
struct AmbientContributionEvent {
    Hash256 eventId;                 // Hash of local context (must not contain sensitive data)
    uint64_t logicalTimestamp;       // Protocol-defined logical tick (NOT wall-clock)
    ContributionType type;
    AmbientContributionBucket bucket;

    // Binary serialization for deterministic aggregation
    std::vector<uint8_t> serialize() const;
    Hash256 hash() const;
};

} // namespace participation
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_CONTRIBUTIONS_HPP
```

### Invariants & Integration Points
* `AmbientContributionBucket` MUST map to discrete states (e.g., `ailee::ambient_node::PowerState`, `ailee::ambient_node::ConnectivityBand`) defined in `ambient_node_mode_design.md` to guarantee non-raw privacy preservation.
* `logicalTimestamp` MUST align with deterministic L2 sub-ticks; `std::chrono::system_clock` and wall-clock times are strictly forbidden.
* Floating-point data is explicitly prohibited.

## 4. Anchoring Proofs into Bitcoin L2

This section defines how participation proofs are aggregated, merkleized, and committed into `AmbientAIAnchoringEpoch`, including data availability (DA) routing rules.

### Aggregation and Merkleization
* **Aggregation**: During epoch transition, the `BlockProducer` gathers all validated `AmbientParticipationProof` structures from mesh nodes.
* **Deterministic Merkleization**: The `header.hash()` of each verified proof forms the leaves of a Merkle tree. Proofs are strictly sorted lexicographically by `header.publicKeyHex` before hashing.
* **Commitment**: The resulting Merkle root becomes the `participationRoot` within the `protocol::AmbientAIAnchoringEpoch` struct defined in `bitcoin_anchoring_layer_spec.md`.

### Batching Rules and Cost/Benefit Tradeoffs
* **Batching**: Raw proof payloads are decoupled from the Taproot footprint. The anchor transaction on L1 commits only 32 bytes (the epoch root, which encapsulates the participation root).
* **Cost/Benefit**: By offloading the raw `AmbientParticipationProof` arrays and bounding event inputs via `ParticipationLimits`, L1 anchoring fees remain fixed regardless of network size, while L2 DA storage is utilized for reconstruction and verification.
* **DA Integration**: The raw `AmbientParticipationProof` serialized bytes are appended to the Data Availability payload, alongside `L2StateDiff`.

### Integration Points
* Must coordinate with `anchoring::TaprootAnchor` (`bitcoin_anchoring_layer_spec.md`) which requires the fully resolved `AmbientAIAnchoringEpoch`.
* Must align with `ailee::ambient::AmbientEventAggregator` (`ambient_ai_primitives_spec.md` / `bitcoin_anchoring_layer_spec.md`). The final Merkle root for ambient events is computed separately, but is committed to the same `AmbientAIAnchoringEpochRoot` structure in parallel to the `participationRoot`.

## 5. Bitcoin Give-Back Mechanism (Non-Monetary)

```cpp
#ifndef AMBIENT_AI_PARTICIPATION_GIVE_BACK_HPP
#define AMBIENT_AI_PARTICIPATION_GIVE_BACK_HPP

#include <cstdint>
#include <string>

namespace ailee {
namespace participation {

// Encapsulates a coordination and routing advantage signal
struct AmbientReputationSignal {
    std::string publicKeyHex;
    uint32_t reliabilityTier;    // Deterministically bucketed (e.g., 1 to 5)
    uint64_t cumulativeUptime;   // Sourced from multi-epoch participation records
};

// A hint used by L2 coordination nodes to bias L1 relay and L2 mesh routing
struct AmbientGiveBackHint {
    AmbientReputationSignal signal;

    // Deterministic routing boost applied to ReputationRateLimiter.
    // e.g., higher reputation yields a larger forward rate multiplier.
    int32_t calculateRoutingMultiplier() const;

    // Generates a priority score used to prefer this node as a relay candidate
    // for anchoring Bitcoin L1 transactions.
    uint32_t calculateL1RelayPriority() const;
};

} // namespace participation
} // namespace ailee

#endif // AMBIENT_AI_PARTICIPATION_GIVE_BACK_HPP
```

### Invariants & Integration Points
* NO tokens, NO inflation, NO synthetic monetary rewards. The give-back mechanism is strictly routing and prioritization-based.
* `calculateRoutingMultiplier()` MUST feed directly into `ailee::ambient_mesh::NeighborEntry::cachedEnergyScore` (`ambient_mesh_networking_layer.md`).
* `calculateL1RelayPriority()` MUST be consumed by L2 coordination nodes when gossiping the `TaprootAnchor` transaction to the Bitcoin Mempool (`bitcoin_anchoring_layer_spec.md`), biasing toward high-participation nodes.

## 6. Execution Plan

### Phase 1: Contribution Definitions & Measurement Logistics
* Implement `AmbientContributionEvent`, `AmbientContributionBucket`, and `AmbientContributionSummary`.
* Connect log inputs to `AmbientMeshRoutingEngine` and `AmbientNodeMode` sensing logic, enforcing `ParticipationLimits`.

### Phase 2: Proof Generation & Cryptographic Binding
* Implement `AmbientParticipationProof` and `AmbientParticipationProofHeader`.
* Bind proof generation strictly to the epoch boundary transition within the L2 BlockProducer, utilizing `libsecp256k1` and energy derivation from `EnergyProfile`.

### Phase 3: Bitcoin L2 Anchoring & Merkleization
* Implement deterministic lexicographical sorting and Merkle root calculation for all verified proofs.
* Inject the resulting hash into `AmbientAIAnchoringEpoch::participationRoot`.
* Plumb the full raw proofs into the DA payload serialization alongside `L2StateDiff`.

### Phase 4: Non-Monetary Give-Back Mechanics
* Define `AmbientGiveBackHint` and `AmbientReputationSignal`.
* Update mesh layer's rate-limiting to ingest `calculateRoutingMultiplier()`.
* Update the L1 relay logic to bias transaction propagation based on `calculateL1RelayPriority()`.

### Phase 5: Resource-Usage Validation & Test Scenarios
* Test Adversarial Node Spamming: verify `AmbientContributionLog` truncates at `MAX_EVENTS_PER_EPOCH` and `MAX_SENSING_PER_TICK` deterministically without consensus disruption.
* Test Replay Protection: verify `verify()` rejects proofs with mismatching `epochId` and `seenEventIds` blocks duplication within an epoch.
* Test Cross-Platform Reproducibility: verify ARM mobile and x86_64 desktop nodes yield byte-for-byte identical `AmbientContributionSummary` serialization.
* Test L1 Relay Prioritization: verify nodes with superior `derivedEnergyScore` across multiple epochs are consistently selected as Taproot anchor relays.