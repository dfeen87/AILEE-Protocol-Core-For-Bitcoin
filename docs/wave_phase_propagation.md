# Wave Phase Propagation

WNN propagates its phase state through the network utilizing a dual-mode approach managed by the `RoutingEngine`. Propagation is driven by physical phase matching rather than address-based routing.

## 1. Propagation Logic (Vectors)

### Vector A: Resonant Direct
The primary, high-performance transport mechanism. It represents direct, physical phase transmission across contiguous physical mediums (e.g., dedicated wireless or fiber links where physical properties can be measured and synchronized).

### Vector B: Transduction
A fallback mechanism for traversing standard IP networks where continuous physical phase cannot be directly measured. It converts continuous phase data into discrete network packet timing events.

## 2. Modulation into Inter-Arrival Times (IAT)

In Vector B, continuous phase shifts (Delta Theta) are modulated into discrete network packet delays known as Inter-Arrival Times (IAT).
This is managed by the `PhyListener` which injects packets asynchronously.

The mathematical transduction applies a delay constant:
`Delay (ns) = Delta Theta * K_iat`
(where `K_iat = 1,000,000.0 ns/rad` as per the core definition)

**Carrier Opacity Constraints:**
To maintain temporal precision and reduce noise introduced by the OS scheduler (Carrier Opacity), transduction delays shorter than 50,000 ns use high-precision spin-wait loops rather than thread sleeping.

## 3. Refraction and Multi-Hop Rules

As the wave state propagates across multiple nodes, it is "refracted." Refraction is the process of updating an incoming `WaveState` to produce an outgoing `WaveState` based on the internal resonance and physical synchronization limits of the intermediary node.

```cpp
void refract_wavefront(const WaveState& state_in, WaveState& state_out);
```

During multi-hop traversal via Transduction, if baseline entropy degrades beyond the `s_crit_` threshold (e.g., due to Deep Packet Inspection interference), the phase transduction logic can "snap-back," rejecting the hop and deferring to another route.

## 4. Spectral Awareness and Promiscuous Listening

Network traffic is captured natively at the hardware level using `libpcap` running in promiscuous mode. Standard TCP/UDP spectral leakage is filtered. A Hann Window function must be applied to incoming amplitude buffers before non-linear feature extraction to isolate true wave signatures.
