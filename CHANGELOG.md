# Changelog

All notable changes to this project are documented here.

Note: v1.1.0 was intentionally skipped to align versioning with capability boundaries introduced in v1.2.0.

## [Unreleased]

### Changed
- None.

### Rationale
This change improves alignment between documentation and current implementation status.

---

## v1.2.0 — AILEE Observational Adapter Layer
- Added optional, read-only AILEE adapters:
  - Mempool state
  - Network topology
  - Energy efficiency (AILEE η)
- No changes to consensus behavior
- No transaction routing or submission changes
- Bitcoin adapter remains authoritative
- Telemetry enrichment only

---

## v1.2.1 — Build & Test Hardening
- Fixed unit test source paths for CMake discovery
- Added FetchContent fallback for GoogleTest when not installed
- Added CI workflow to build and run tests on Ubuntu

---

## v1.1.0 — Verification & Systems Integrity
- Added formal verification and falsifiability boundaries
- Clarified system assumptions and non-goals
- No code changes

## v1.0.0 — Initial Public Release
- Core AILEE framework
- Multi-chain adapter architecture
- Orchestration and telemetry foundations
