# SWAYAM-AGI - Master Blueprint

This is the single document to read top-to-bottom to understand the whole pipeline: what phase it's in, what each component does, what trust boundary it sits on, and what has to be true before the next phase opens up. It indexes the code by path—it does not re-embed it. The scripts themselves are the source of truth for their own behavior; this document is the source of truth for how they fit together.

## Standing Invariants (true at every phase, including Phase 5)
*   **A human can always halt the pipeline immediately**, regardless of how much autonomy later phases add. `emergency-halt.yml` stays independent of the mutation pipeline's own credentials.
*   **Every mutation's full lifecycle is logged and attributable:** what was generated, its structural hash, accept/reject decision, which PR (if any) it produced, who/what merged it.
*   **Novelty is not safety.** The AST duplicate check (Phase 2) proves a mutation is structurally new. It proves nothing about whether it's correct, secure, or behaves the way its rationale claims. Nothing in this pipeline is allowed to treat "passed the duplicate check" as "safe to ship".

## Phase 0 - Foundational Infrastructure & Base Security
**Status:** Mostly in place; two gaps worth closing before Phase 1 hardening is "done."
*   ISO C++23 core (`include/swayam_core.hpp`, `src/main.cpp`), CMake build (`CMakeLists.txt`).
*   **Gap:** No build-verification CI job is visible anywhere in this pipeline.
*   **Gap:** Branch protection on `main` isn't described (required PR review, required status checks).

## Phases 1-3 - Zero-Trust Mutation Pipeline (current work)
*   **Phase 1 - Workflow hardening:** Least-privilege `permissions:` blocks per workflow, safe artifact extraction (`scripts/safe_extract.py`).
*   **Phase 2 - Structural integrity:** AST-level duplicate detection (`scripts/structural_hash.py`) backed by a CAS-protected manifest (`scripts/cas_commit.sh` + `scripts/transforms/manifest_transform.sh`). Verified end-to-end in `scripts/master_deployment_matrix.py`.
*   **Phase 3 - Verified shadow run:** `master_deployment_matrix.py` as a required pre-flight check before `sandboxed-mutation.yml` is allowed to run against real credentials. Add structured audit logging of every mutation attempt. Define the rollback procedure.

## Phase 4 - Correctness & Safety Gates (the actual next milestone)
Everything before this proves a mutation is novel. Nothing yet proves it's good.
*   **Build verification:** The mutation must actually compile under the real `CMakeLists.txt` config.
*   **Test execution:** Unit/integration tests scoped to whatever the mutation touches must pass.
*   **Static analysis:** (clang-tidy/cppcheck / equivalent) on the mutation itself.
*   **Behavioral bounds:** Does the mutated code stay within resource/behavior limits?

## Phase 5 - Governed Scale (the honest version of "autonomous production")
Not "zero humans, all mutations auto-ship." It is risk-routed:
*   **Risk-based routing:** Narrowly scoped, passing mutations become eligible for auto-merge. Anything touching auth, crypto, or failing any gate routes to a human.
*   **Progressive rollout with automatic rollback:** Canary the merged change, roll back automatically on failed health checks.
*   **External auditing:** Periodic third-party review of the pipeline itself.
*   **Continuous adversarial testing:** Scheduled, automated runs of malicious fixtures.

## Component Index

| Path | Purpose | Runs with |
| :--- | :--- | :--- |
| `include/swayam_core.hpp`, `src/main.cpp` | C++23 core | n/a (compiled artifact) |
| `.github/workflows/sandboxed-mutation.yml` | Generates candidate mutations | `contents: read`, network-disabled/resource-capped container |
| `scripts/safe_extract.py` | Validates & extracts the mutation artifact | Same job as above |
| `scripts/structural_hash.py` | Computes AST hash, registers via CAS | Runs on already-extracted, validated content |
| `scripts/cas_commit.sh` + `manifest_transform.sh` | CAS-protected manifest update; duplicate rejection | Invoked by `structural_hash.py` |
| `.github/workflows/sign-and-open-pr.yml` | Commits mutation to staging branch, opens PR | `contents: write`, `pull-requests: write`, GitHub App token |
| `.github/workflows/emergency-halt.yml` | Kill switch | `actions: write`, environment-protected |
| `scripts/master_deployment_matrix.py` | CI-runnable smoke test | Local only, no secrets needed |
| `scripts/testing/mock_cas_commit.sh` | CAS stand-in for testing | Test/CI only |
