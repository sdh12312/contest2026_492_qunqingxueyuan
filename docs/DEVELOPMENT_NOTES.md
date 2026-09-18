# Development notes

## Milestone 0 — environment and rules

FILES: no source modifications.

TEST: inspected WSL workspace, manifest, branch targets, current goldfish ARM64 artifact and official contest documentation.

RESULT: existing working environment located in WSL Ubuntu-D; official manifest branch dev-ai-contest-2026; existing 233 MB firmware found.

KNOWN ISSUES: organizer-created team repository was not present in the workspace and could not be uniquely identified from public GitHub data.

NEXT: use a migration-ready local repository without changing public openvela repos.

RISK: manual migration and push remain required.

## Milestone 1 — scientific evidence

FILES: tools/connectome, data/connectome, data/engineering.

TEST: live neuPrint male-cns:v1.0 query and independent artifact verifier.

RESULT: 311 direct edges; LC4 → DNp01 6,362 synapses; LPLC2 → DNp01 4,862 synapses.

KNOWN ISSUES: model collapses left/right populations to one aggregate node per type.

NEXT: implement transparent fixed-point mapping.

RISK: low; full raw API result and query are checked in.

## Milestone 2 — core and tests

FILES: app/flyreflex, tests, root CMakeLists.txt.

TEST: Release host build with warnings-as-errors, CTest, all deterministic scenarios and 1,000-iteration benchmark.

RESULT: all tests passed; danger scenario outputs REFLEX_OVERRIDE.

KNOWN ISSUES: host benchmark is not the contest platform measurement.

NEXT: cross-compile and execute in openvela simulator.

RISK: LVGL/openvela API compatibility must be validated by target build.

## Milestone 3 — openvela integration

FILES: app/flyreflex Kconfig CMake Make UI and docs/BENCHMARK.md.

TEST: goldfish ARM64 cross-build builtin registration System.map inspection NSH danger run 1,000-iteration benchmark and full LVGL scenario cycle.

RESULT: target build passed; danger frame 6 produced ESCAPE and REFLEX_OVERRIDE; simulator end-to-end median 2,144 ns and P95 3,904 ns.

KNOWN ISSUES: simulator measurements are not MCU measurements; the emulator wrapper reports a segmentation fault only after the deliberate QEMU monitor termination.

NEXT: migrate to the organizer-created contest repository and record the submission video.

RISK: repository invitation push PR merge AI log export and portal submission require the participant account.
