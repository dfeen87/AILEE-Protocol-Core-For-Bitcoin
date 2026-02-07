# Security Policy

## Project Status

**AILEE-Core is an actively developed Bitcoin Layer-2 infrastructure project.**

The codebase implements real security-relevant mechanisms, including deterministic
state snapshots, Bitcoin-consumable anchor commitments, federated exit authorization,
and offline third-party verification tooling.

However:

- The system uses an **explicit federated trust model** (not trustless exits).
- No on-chain fraud-proof or validity-proof system is implemented.
- The project has **not undergone a formal third-party security audit**.
- Operational security (key management, signer availability, deployment hardening)
  is the responsibility of operators and downstream forks.

**AILEE-Core should not be considered production-hardened financial infrastructure
without independent security review, operational controls, and risk assessment.**

---

## Reporting a Vulnerability

If you discover a security issue, **please do not open a public issue**.

Instead, report it privately so it can be responsibly assessed and addressed.

### How to Report

Email the maintainers at:

**security@ailee.example**

Please include:
- A clear description of the vulnerability
- Steps to reproduce (if applicable)
- Affected files, modules, or components
- Potential impact (funds at risk, correctness, availability, etc.)
- Any mitigation ideas, if known

### Response Expectations

- We will acknowledge receipt within **5 business days**
- We will assess severity and coordinate a fix
- We will work with you on a responsible disclosure timeline where appropriate

---

## Security Model & Scope

This policy applies to security-relevant behavior in:

- All code under `src/`
- State snapshotting, anchoring, and verification logic
- Bridge / peg-in / peg-out authorization logic
- Cryptographic commitment and verification utilities
- Build tooling and configuration that could affect correctness or safety
- Documentation that could materially misrepresent security guarantees

### Out of Scope

The following are **explicitly out of scope**:

- Trustless exit guarantees
- L1-enforced fraud or validity proofs
- Economic guarantees beyond the documented federated model
- Operator key management, signer availability, or deployment security
- Third-party dependencies and infrastructure (e.g., Bitcoin Core, OS, HSMs)

---

## Supported Versions

AILEE-Core is under **active development**.

There are currently **no long-term supported (LTS) or production-certified releases**.

Security fixes are applied to the current development branch. Downstream forks and
deployments are responsible for tracking updates and applying patches as appropriate.

---

## Responsible Use Notice

Running or deploying AILEE-Core implies **real operational and custodial responsibility**.

Operators and fork maintainers must:
- Clearly disclose trust assumptions
- Protect signing keys and infrastructure
- Monitor system behavior and verification outputs
- Be prepared to respond to incidents and halt operations if required

Security emerges from correct operation as much as from correct code.
