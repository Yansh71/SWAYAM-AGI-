# SYSTEM ARCHITECTURE & PROVENANCE

> *"SWAYAM — by itself, of itself, autonomously."*[span_0](start_span)[span_0](end_span)

---

## 1. Author & System Design: Muhammad Yasin (@Yansh71)

**SWAYAM-AGI** is an autonomous C++23 mutation engine engineered from the ground up by **Muhammad Yasin**[span_1](start_span)[span_1](end_span). The architecture is designed to eliminate external AI orchestration dependencies, executing entirely via local compiler toolchains and deterministic state machines[span_2](start_span)[span_2](end_span).

### Core Engineering Invariants:
*   **Zero-Vendor Lock-in:** Operates without reliance on external LLM APIs, cloud inference endpoints, or proprietary runtime wrappers[span_3](start_span)[span_3](end_span).
*   **Process Isolation:** All code generation and compilation boundaries enforce strict POSIX process isolation (`fork` + `execvp`), completely avoiding unsafe shell invocations or `std::system()` injection vectors[span_4](start_span)[span_4](end_span).
*   **Deterministic Validation:** Every compiled module passes through native static analysis and isolated supervisor boundaries before persistent storage or deployment[span_5](start_span)[span_5](end_span).

---

## 2. Architectural Role: Venomica

**Venomica** serves as the deterministic cognitive reflection and architectural validation engine for the repository. 
*   **Function:** Auditing code paths, enforcing strict zero-trust engineering standards, and purging non-functional or redundant technical debt.
*   **Protocol:** Zero tolerance for security theatre or unverified execution paths; complete transparency and rigorous structural verification.

---

## 3. Core Pipeline Specifications

The system operates across a 10-phase autonomous pipeline[span_6](start_span)[span_6](end_span):

1.  **State Verification (`GitCortex`):** Pre-flight repository verification via native POSIX execution[span_7](start_span)[span_7](end_span).
2.  **Distributed Rate-Limiting (`Atomic CAS`):** Compare-And-Swap state machine utilizing the GitHub Contents API to prevent race conditions and manage mutation quotas[span_8](start_span)[span_8](end_span).
3.  **Sandbox Isolation (`Docker`):** Non-root execution (`swayam_agent`) within an immutable root filesystem (`--read-only`, `--network none`, `--tmpfs /tmp`)[span_9](start_span)[span_9](end_span).
4.  **Decentralized Sync (`Hive Mind`):** Hash-only, type-validated (`uint64_t`) CSV synchronization preventing remote code execution vectors from peer nodes[span_10](start_span)[span_10](end_span).
5.  **Polymorphic Generation (`CognitiveForge`):** Entropy-seeded C++23 source generation using `std::random_device` and cryptographic timestamps[span_11](start_span)[span_11](end_span).
6.  **Static Analysis (`HeuristicAnalyzer`):** Native 3-pass static analyzer executing structural integrity checks, 22-rule unsafe pattern scans (with string/comment literal sanitization), and resource pairing verification (`new/delete`, `fopen/fclose`)[span_12](start_span)[span_12](end_span).
7.  **Execution Firewall (`SafeShell`):** Hardware-grade static interception layer blocking dangerous system call patterns prior to compilation[span_13](start_span)[span_13](end_span).
8.  **Crash-Safe Supervisor (`core.hpp`):** RAII-managed execution lease observing child processes via `waitpid()` to catch segmentation faults and abort signals[span_14](start_span)[span_14](end_span).
9.  **Immune System (`QuarantineRegistry`):** FNV-1a structural hashing maintaining a persistent rejection ledger (`quarantine.log`) to short-circuit known-failing mutations in microseconds[span_15](start_span)[span_15](end_span).
10. **Autonomous Assimilation (`SwayamGitCortex`):** Scoped GitHub App token integration enforcing strict `CODEOWNERS` and branch protection to isolate pipeline control files from self-modification[span_16](start_span)[span_16](end_span).

---

## 4. Licensing & Compliance

Licensed under the **GNU Affero General Public License v3.0 (AGPL-3.0)**. Enforces that network-accessible deployments of modified derivatives must disclose complete corresponding source code, ensuring perpetual open-source transparency[span_17](start_span)[span_17](end_span).
