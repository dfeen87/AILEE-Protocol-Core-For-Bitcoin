# Wave Resonance Profiles

The core Duffing oscillator governing a WNN node's phase state is defined by a specific set of physics-based parameters. These parameters define the hardware's optimal driving conditions and expected ambient state.

## 1. Parameters

The Duffing equation relies on the following constants:
- **`omega` (ω):** The optimal forcing frequency of the node.
- **`alpha` (α):** The linear stiffness parameter (typically negative, creating a double-well potential).
- **`beta` (β):** The non-linear stiffness parameter (typically positive).
- **`delta` (δ):** The damping coefficient, representing energy loss or network resistance.
- **`gamma` (γ):** The forcing amplitude. In ambient states, this scales down dramatically.

## 2. Resonance Logic and Hardware Calibration

Each hardware edge node requires calibration to find its most stable optimal forcing frequency and resonance state. During a calibration sweep (`CalibrationMode::run_sweep`), the system scans frequencies and non-linear interactions to determine the stable bounds of the phase space for that specific physical machine.

### Go-Dark Ambient Mode
During "Go-Dark" protocol scenarios triggered by the AILEE Guardrails (e.g., when interference is detected), the `gamma` forcing amplitude is dropped to near-zero (e.g., multiplied by 0.01). The oscillator enters a low-energy ambient decay mode, reducing measurable spectral leakage while preserving internal phase integrity for swift recovery.

## 3. Persistence Rules

The exact parameters calculated during calibration represent the physical "fingerprint" of the node's resonance capabilities. These are serialized explicitly to disk to ensure immediate lock-on after a restart.

**File Format:**
A flat binary format stored in `wnp_hw_profile.bin`.
Serialization rules utilize direct memory copies of IEEE-754 `double` primitives without additional metadata.

```cpp
// Persistence mapping
double omega;
double alpha;
double beta;
double delta;
```
