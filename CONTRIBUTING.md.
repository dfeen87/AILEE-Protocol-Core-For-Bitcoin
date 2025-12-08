# Contributing to AILEE-Core

First off, thank you for considering contributing to AILEE (AI-Load Energy Efficiency). 

This project is not just code; it is a **call to action** to evolve Bitcoin into a dynamic global financial powerhouse. We recognize that the claims made in our White Paper (46k+ TPS, Trustless Recovery) are ambitious. As such, we actively welcome skeptics, cryptographers, mathematicians, and systems engineers to challenge, test, and refine this framework.

## 🤝 Our Philosophy
Faith is what drives this project. We operate on principles of **truth, respect, and constructive collaboration**.

*   **Skepticism is Welcome:** You are encouraged to challenge the mathematics behind the TPS formula.
*   **Respect is Mandatory:** We attack ideas, not people. 
*   **Open Source is Vital:** Innovation thrives when shared. We are building this in the public domain to ensure transparency.

## 🎯 How You Can Help
We are currently in the **Simulation & Prototype Phase**. We need help in the following specific areas:

### 1. Mathematical Verification
*   Review `ailee_tps_engine.h`.
*   Validate the **Closed-Loop Feedback** logic.
*   Challenge the `Latency & Propagation Model` derivation.
*   **Goal:** Ensure the $\epsilon(n, b, l)$ error term accurately models real-world entropy.

### 2. Cryptography (Zero-Knowledge & VDFs)
*   Review `ailee_recovery_protocol.h`.
*   We need auditing on the **Verifiable Delay Function (VDF)** implementation to ensure time-locks cannot be bypassed via parallel processing.
*   We need Circom or Halo2 engineers to move the ZK-proof logic from C++ simulation to actual circuit constraints.

### 3. Systems Engineering (C++ / Rust)
*   The current `main.cpp` is a simulation. We need to begin porting the logic into a full node implementation.
*   Optimize the **Gold Bridge** Oracle aggregation logic in `ailee_gold_bridge.h` for fault tolerance.

## 🛠 Development Workflow

### Prerequisite
Ensure you have the following installed:
*   C++17 Compiler
*   CMake 3.10+
*   OpenSSL Development Libraries

### Pull Request Process
1.  **Fork the Repository** to your own GitHub account.
2.  **Create a Branch** for your feature or fix:
    ```bash
    git checkout -b feature/optimize-mempool-sorting
    ```
3.  **Commit your changes**. Please provide clear, descriptive commit messages.
    *   *Bad:* "Fixed code"
    *   *Good:* "Implemented M/M/1 queuing theory in mempool logic"
4.  **Push to the Branch**:
    ```bash
    git push origin feature/optimize-mempool-sorting
    ```
5.  **Open a Pull Request (PR)** against the `main` branch.
    *   In the PR description, explain **why** you made the change.
    *   If you are fixing an open Issue, reference it (e.g., `Fixes #42`).

## 🐛 Reporting Issues
If you find a bug or a mathematical inconsistency:
1.  Check the **Issues** tab to see if it has already been reported.
2.  Open a new Issue.
3.  Tag it appropriately:
    *   `[BUG]` for code errors.
    *   `[MATH]` for theoretical challenges to the TPS formula.
    *   `[SECURITY]` for potential vulnerabilities in the recovery protocol.

## 📜 License
By contributing to AILEE-Core, you agree that your contributions will be licensed under the project's **MIT License**.

---

**"Innovation thrives when shared. Let’s build the future together."**
– Don Michael Feeney Jr.
