# Wave Epoch and Phase State Machine

WNN utilizes an intricate state machine and epoch derivation strategy to provide continuous deterministic validation and protection against replay attacks without relying on traditional cryptographic handshakes.

## 1. Spiral-Time Phase Salting

To prevent delayed replays of continuous wave states, the network applies a deterministic micro-perturbation to the active forcing frequency (`omega`) of the Duffing oscillator. This concept is termed "Spiral-Time Phase Salting".

The `generate_phase_salt` function defines this logic:
```cpp
inline double generate_phase_salt(double ts, double omega) {
    uint64_t epoch = static_cast<uint64_t>(std::floor(ts / 2.5));
    uint64_t w_bits;
    double omega_d = omega;
    std::memcpy(&w_bits, &omega_d, sizeof(omega_d));
    uint64_t hash = std::hash<uint64_t>{}(epoch ^ w_bits);
    return (hash % 1000) * 1e-5;
}
```
**Hashing Rules:**
The epoch `E` is derived from the continuous time `ts` partitioned into 2.5-second windows.
The base frequency `omega` is bit-cast to an unsigned 64-bit integer.
The hash is an XOR of the epoch integer and the bit-cast frequency.
The resulting salt is constrained via modulo 1000 and scaled to `10^-5`, creating a subtle physical perturbation that valid nodes will track in their PLL, but replay attackers will miss, causing phase decoupling.

## 2. Schmitt Trigger Gatekeeping

Connections in the AILEE Trust Layer are managed by a Schmitt trigger state machine to prevent rapid flapping ("jitter") of connections when metrics oscillate near a boundary.

- **Lock-in Threshold:** A peer must achieve a structural lock-in score `>= 0.75` to be promoted to an active connection.
- **Drop-out Threshold:** An active connection is only severed if the score drops below `< 0.65`.

To further dampen false-positive drops, an Exponential Moving Average (EMA) filter is applied to the raw consistency scores before they feed the Schmitt trigger.

## 3. Replay Protection Integration

The `PllController` strictly monitors the phase error. If an incoming signal is a delayed replay, its phase salt will reflect an older epoch. The local PLL, operating in the current epoch, will immediately detect an anomalous phase divergence during the lock-in verification cycle.
An incorrect phase salt immediately triggers a quarantine event, zeroes the AILEE Determinism score, and flags `is_replay_detected_ = true` on the controller.
