# FlyReflex development log

This engineering log supplements but does not replace the official exported AI Coding conversation required by the contest.

## 2026-09-18 — workspace and rules

- Located the existing openvela workspace in WSL Ubuntu-D.
- Confirmed official manifest uses dev-ai-contest-2026.
- Preserved the existing cmake_out/vela_goldfish-arm64-v8a-ap build artifact.
- Read official overview, code submission, AI Coding log and AI hardware guidance.
- Noted mandatory deliverables: contest repository PR, AI log export, at least one Skill, work-introduction document and ≤5 minute video.
- The organizer-created repository was not present locally and could not be uniquely found publicly for GitHub user sdh12312, so implementation continued in a migration-ready local repository.

## 2026-09-18 — biological evidence

- Queried Janelia neuPrint dataset male-cns:v1.0 directly.
- Verified DNp01/GF body IDs 10001 and 10010.
- Retrieved all direct LPLC2/LC4 → DNp01 cell-pair edges.
- Stored exact Cypher, raw JSON response, 311-row edge CSV and aggregate CSV.
- Adopted 433:567 relative weights from normalized aggregate counts.
- Classified leak, thresholds, encoders and action mapping as engineering parameters.

## 2026-09-18 — core implementation

- Implemented deterministic synthetic looming scenarios.
- Implemented fixed-point encoder and leaky DNp01/GF node.
- Implemented deterministic Safety Arbiter and fail-safe STOP.
- Added machine-readable logs, CSV mode and CLOCK_MONOTONIC timing.
- Added 1,000-iteration percentile benchmark.
- Added host tests for safe, slow, danger, override, boundary, recovery, decay, determinism, invalid input and noise.
- Host Release build and CTest passed.

## 2026-09-18 — openvela integration

- Linked app/flyreflex into packages/demos/flyreflex without editing upstream source repositories.
- Added Kconfig, NuttX CMake/Make build files and an LVGL dashboard.
- Enabled CONFIG_FLYREFLEX in the existing goldfish ARM64 CMake configuration.
- Cross-compiled successfully; the builtin table and System.map both contain flyreflex.
- Ran danger in NSH and observed AI FORWARD → ESCAPE with REFLEX_OVERRIDE at frame 6.
- Ran the full LVGL SAFE → SLOW → DANGER → RECOVERY cycle in the simulator.
- Measured 1,000 iterations: reflex median 1,200 ns and P95 2,144 ns; end-to-end median 2,144 ns and P95 3,904 ns.

## 2026-09-18 — delivery validation

- Validated the reusable flyreflex-provenance Skill with the official quick validator.
- Rendered and visually inspected all five pages of the competition introduction document.
- Kept simulator and host benchmarks separate and made no MCU or real-LLM latency claim.
