# AXIOM Agent Process

This file defines a practical workflow to manage iterative work with coding agents.

## Goals

- Keep changes small, verifiable, and reversible.
- Separate stabilization work from optimization work.
- Make each phase measurable with explicit exit criteria.

## Operating Rules

- One primary objective per phase.
- Always run a build validation after code changes.
- Prefer root-cause fixes over temporary patches.
- Do not start the next phase before gate criteria are met.

## Phase Plan

## Phase 0: Baseline and Safety Net

### Phase 0 Objective

Capture current status and lock a reproducible baseline.

### Phase 0 Actions

- Record `git status` and changed files.
- Run release build:
  - `cmake --build build --config Release`
- Record test/benchmark quick snapshot if available.

### Phase 0 Exit Criteria

- Build passes.
- Baseline metrics and notes are written in session log.

## Phase 1: Nanobind Stability Gate

### Phase 1 Objective

Ensure nanobind path compiles cleanly and does not block release builds.

### Phase 1 Actions

- Keep `nanobind_interface` implementation aligned with current nanobind APIs.
- Avoid internal/unstable nanobind detail APIs.
- Validate with release build.

### Phase 1 Exit Criteria

- `cmake --build build --config Release` succeeds.
- No active compile error in `src/nanobind_interface.cpp`.

## Phase 2: Concepts Adoption (Selective)

### Phase 2 Objective

Improve template API clarity with C++20 concepts at public/generic boundaries.

### Phase 2 Actions

- Add concepts to allocator-facing generic entry points.
- Keep hot-path internals simple and benchmarked.
- Avoid broad refactors in one step.

### Phase 2 Exit Criteria

- Build passes.
- No regression in key benchmarks.
- Readability/error-message quality improves in reviewed APIs.

## Phase 3: Opaque Pointer Integration (Scoped)

### Phase 3 Objective

Introduce opaque-pointer boundaries where ABI/build isolation is beneficial.

### Phase 3 Actions

- Apply opaque handle at FFI/C API boundaries first.
- Keep allocator core direct and low-overhead.
- Measure p95 latency and throughput before/after.

### Phase 3 Exit Criteria

- Build/test pass.
- Measured overhead remains within agreed threshold.
- No regressions in allocator hot path.

## Phase 4: Consolidation

### Phase 4 Objective

Unify docs, remove drift, and finalize rollout.

### Phase 4 Actions

- Update `README.md` and test docs.
- Remove obsolete comments/toggles introduced during transition.
- Produce short completion report.

### Phase 4 Exit Criteria

- Documentation reflects real behavior.
- Remaining TODO items are either closed or explicitly deferred.

## Phase 5: Runtime Guardrails and Daemon Resilience

### Phase 5 Objective

Move stability controls from compile-time constants to runtime-tunable policy.

### Phase 5 Actions

- Add expression policy limits configurable via environment variables.
- Add daemon circuit-breaker and backpressure thresholds configurable via environment variables.
- Keep safe defaults aligned with prior behavior to avoid surprise regressions.
- Validate with release build and full `run_tests` binary.

### Phase 5 Exit Criteria

- Runtime knobs are documented in `README.md`.
- Release build succeeds for `axiom`, `run_tests`, and `axiom_benchmark`.
- `run_tests` passes with zero failures.

## Agent Task Template

Use this template when assigning work to an agent.

## Task

- Title:
- Scope (files/modules):
- Non-goals:

## Constraints

- Build profile:
- Performance constraints:
- Compatibility constraints:

## Deliverables

- Code changes:
- Validation commands:
- Expected evidence:

## Acceptance

- Build:
- Tests:
- Benchmarks:
- Docs updated:

## Progress Table Template

| Phase | Focus | Status | Evidence | Risk | Next Action |
| --- | --- | --- | --- | --- | --- |
| 0 | Baseline | TODO | - | Low | Capture baseline |
| 1 | Nanobind stability | TODO | - | Medium | Compile + fix |
| 2 | Concepts rollout | TODO | - | Medium | Apply selective concepts |
| 3 | Opaque ptr scope | TODO | - | Medium | Integrate boundary handles |
| 4 | Consolidation | TODO | - | Low | Final docs/report |

## Current Phase Gain Table (2026-03-05)

| Item | Focus | Status | Evidence | Impact |
| --- | --- | --- | --- | --- |
| P0 | Fast-path API design | DONE | `DynamicCalc::EvaluateFast` + `TryEvaluateFast` API active in core | Typed arithmetic path avoids parser/vtable overhead |
| P1 | DynamicCalc fast-path implementation | DONE | `src/dynamic_calc.cpp` fast operation switch path in release build | Deterministic O(1) arithmetic dispatch |
| P2 | Benchmark fast-path coverage | DONE | `tests/benchmark_suite.cpp` includes `typed_fast_path` scenario + CSV/JSON export | Parser path vs typed path now measured in same harness |
| P3 | Build validation | DONE | `axiom`, `run_tests`, `axiom_benchmark` build targets passed | End-to-end compile confidence |
| P4 | Aşama kazanım tablosu | DONE | Benchmark snapshot below | Quantified speedup for rollout planning |

Fast-path gain snapshot (`build/axiom_benchmark.exe`, 2026-03-05):

| Metric | Scalar Parser Path | Typed Fast-Path | Gain |
| --- | --- | --- | --- |
| Throughput (ops/sec) | `100074` | `4.16632e+08` | `~4163x` |
| Latency (us/op) | `9.99264` | `0.0024002` | `~4163x lower` |

Validation snapshot for P4:

- Build targets validated: `axiom`, `run_tests`, `axiom_benchmark`
- Test result: `run_tests` passed (`86 passed, 0 failed`)

## Daily/Session Checklist

- Pull latest and check `git status`.
- Confirm active phase and acceptance criteria.
- Implement only phase-scoped changes.
- Run validation commands.
- Update progress table and next action.

## Suggested Validation Commands

- Build: `cmake --build build --config Release`
- Target build: `cmake --build build --config Release --target axiom`
- Benchmark target: `cmake --build build --config Release --target axiom_benchmark`
- Benchmark run: `./build/axiom_benchmark`

## Sequential Incompatibility Test (src + include)

This test is the required compatibility gate before starting optimization phases.

### Purpose

- Detect API mismatch errors (missing members, wrong signatures, stale calls).
- Detect macro/configuration drift between implementation and headers.
- Ensure all files in `src/` and `include/` are covered in deterministic order.

### Execution Order

1. List files under `src/` and `include/`.
2. Run diagnostics (`Problems`) for both folders.
3. Run mismatch grep for known broken patterns (for example `EvaluateWithContext`).
4. Build release profile.
5. Log findings and blocked files in session notes.

### Standard Commands

- List source files: `file_search("src/**/*.{cpp,h,hpp}")`
- List include files: `file_search("include/**/*.{h,hpp,cpp}")`
- Diagnostics: `get_errors([src, include])`
- Mismatch scan: `grep_search("EvaluateWithContext\\(", regex=true)`
- Build gate: `cmake --build build --config Release`

### Pass/Fail Criteria

- PASS:
  - `get_errors` returns no errors for `src/` and `include/`.
  - No stale API calls found in mismatch scan.
  - Release build succeeds.
- FAIL:
  - Any missing-member or signature mismatch remains.
  - Build breaks on any translation unit in `src/`.

### Current Run Record (2026-03-04)

Status: PASS

- Diagnostics (`src` + `include`): no errors.
- Mismatch scan (`EvaluateWithContext`): no matches in `include/`, fixed in `core/dispatch/selective_dispatcher.cpp`.
- Build state: release build green.

Files covered in run order:

- `src/algebraic_parser.cpp`
- `src/symbolic_parser.cpp`
- `src/symbolic_engine.cpp`
- `src/string_helpers.cpp`
- `src/statistics_parser.cpp`
- `src/statistics_engine.cpp`
- `src/unit_manager.cpp`
- `src/unit_parser.cpp`
- `core/dispatch/selective_dispatcher.cpp`
- `src/python_repl.cpp`
- `src/python_parser.cpp`
- `src/python_engine.cpp`
- `src/plot_engine.cpp`
- `src/nanobind_interface.cpp`
- `src/main.cpp`
- `src/linear_system_parser.cpp`
- `src/eigen_engine.cpp`
- `src/dynamic_calc.cpp`
- `src/daemon_engine.cpp`
- `src/cpu_optimization.cpp`
- `src/arena_allocator.cpp`
- `include/algebraic_parser.h`
- `include/dynamic_calc_types.h`
- `include/statistics_engine.h`
- `core/dispatch/selective_dispatcher.h`
- `include/statistics_parser.h`
- `include/python_repl.h`
- `include/symbolic_parser.h`
- `include/symengine_integration.h`
- `include/symbolic_engine.h`
- `include/string_helpers.h`
- `include/python_parser.h`
- `include/python_engine.h`
- `include/unit_manager.h`
- `include/plot_engine.h`
- `include/nanobind_interface.h`
- `include/unit_parser.h`
- `include/linear_system_parser.h`
- `include/iParser.h`
- `include/extended_types.h`
- `include/eigen_engine.h`
- `include/dynamic_calc.h`
- `include/daemon_engine.h`
- `include/cpu_optimization.h`
- `include/arena_allocator.h`

## Decision Log (Lightweight)

For each major decision, capture:

- Decision:
- Why:
- Alternatives considered:
- Measured impact:
- Follow-up action:

## Wrapper Readiness Backlog (3 Phases)

This backlog is the execution plan before building commercial wrappers on top of the core.

## Phase A: Core Contract Stabilization

### Phase A Objective

Eliminate behavioral drift and expose one canonical dispatch contract.

### Phase A Scope

- Canonicalize dispatcher implementation and remove duplicate/legacy behavior paths.
- Normalize core error semantics for wrapper-safe mapping.
- Ensure symbolic capability surface is explicitly declared as supported/unsupported.

### Phase A Deliverables

- Single canonical dispatcher path with documented ownership.
- Error taxonomy map for wrapper integration.
- Symbolic support matrix (implemented, partial, not implemented).

### Phase A Success Metrics

- Zero API mismatch findings in `src/` and `include/` compatibility scan.
- No unresolved TODO routes in matrix dispatch critical path.
- Deterministic error category mapping for top-level operations.

### Phase A Rollback Trigger

- Build instability or behavior regressions in baseline calculation commands.
- Increased fallback rate without explicit design intent.

## Phase B: Integration Reliability and Test Realignment

### Phase B Objective

Align test and dependency model with product strategy (`numpy` + `matplotlib` baseline).

### Phase B Scope

- Split optional heavy scientific package tests from default product gate.
- Add explicit default profile for production-like wrapper compatibility checks.
- Keep nanobind and GUI integration compile-safe and runtime-safe.

### Phase B Deliverables

- Default test profile matching minimal supported dependency set.
- Optional extended profile for SciPy/Pandas/SymPy experiments.
- Updated compatibility checklist in process docs.

### Phase B Success Metrics

- Default profile passes with only required dependencies.
- Wrapper smoke tests (CLI + Python bridge path) pass in clean environment.
- No import-time crashes for GUI modules when optional packages are absent.

### Phase B Rollback Trigger

- Test pass rate drops in default profile.
- Critical user flows require optional dependencies unexpectedly.

## Phase C: Performance and Productization Gate

### Phase C Objective

Lock performance baselines and operational gates for revenue-facing wrapper releases.

### Phase C Scope

- Freeze benchmark schema and acceptance thresholds.
- Define release gate: build, compatibility scan, test profile, benchmark delta.
- Add regression alert rules for dispatcher and fast-path performance.

### Phase C Deliverables

- Wrapper release gate checklist with hard pass/fail rules.
- Baseline benchmark reference (CSV/JSON) with threshold annotations.
- Session template for go/no-go decision records.

### Phase C Success Metrics

- Release build + compatibility scan + default tests all green in one run.
- No material regression beyond agreed benchmark tolerance.
- Traceable release decision logs for each wrapper shipment.

### Phase C Rollback Trigger

- Benchmark regression above tolerance.
- Compatibility scan detects new API mismatch.
- Non-deterministic behavior in core dispatch path.

## Execution Rhythm

- Work strictly phase-by-phase (A -> B -> C).
- Do not open a new phase before previous phase exit criteria are satisfied.
- After each phase, record:
  - changed files,
  - validation evidence,
  - accepted risks,
  - deferred items.

## Sonar API Remediation Log (2026-03-06)

Source inputs:

- `docs/reports/sonar_api/issues_p1.json`
- `docs/reports/sonar_api/quality_gate.json`
- `docs/reports/sonar_api/measures.json`

Iteration objective:

- Reduce high-signal Sonar findings with low regression risk in dispatch and benchmark paths.

Applied improvements:

- `core/dispatch/selective_dispatcher.cpp`
  - Replaced map lookup pattern `count()+at()` with `contains()+at()` in engine resolution path.
  - Updated helper interfaces to prefer `std::string_view` where ownership is not needed.
  - Kept behavior equivalent while reducing temporary string/copy overhead in helper boundaries.
- `tests/benchmark_suite.cpp`
  - Removed `volatile`-qualified benchmark guards and replaced with explicit result accounting.
  - Resolved moved-from loop risk by avoiding repeated `std::move(req)` retries in queue push loop.
  - Added `std::this_thread::yield()` in spin loops to avoid empty-loop smell and improve cooperative scheduling.

Validation evidence:

- Diagnostics: no active IDE errors in modified files after patch.
- Build: `cmake --build build --config Release` passed (result code `0`).

Accepted risk / defer:

- Large Sonar backlog (`issues_p1.json` total 500 issues) remains; this iteration targets only safe/high-value slices.
- Remaining reliability/duplication/security-hotspot gate items require additional focused batches.

## Sonar API Remediation Log (2026-03-06, Batch 2)

Source inputs:

- `docs/reports/sonar_api/issues_p1.json`

Iteration objective:

- Reduce high-frequency style/perf findings in CLI/main path while preserving behavior.

Applied improvements:

- `src/main.cpp`
  - Replaced multiple `std::find(begin,end,...)` checks with `std::ranges::find(args, ...)`.
  - Replaced `push_back` with `emplace_back` in benchmark and plot vector assembly paths.
  - Replaced `printf` result formatting with iostream + `std::setprecision(15)` output.
- `include/algebraic_parser.h` and `src/algebraic_parser.cpp`
  - Migrated parser variable context path from ordered map to hash table (`std::unordered_map`) for faster lookup on `ans`/variable-heavy paths.
  - Added compatibility overloads so external callers using `std::map` continue to work without breakage.

Validation evidence:

- Diagnostics: no active IDE errors in modified files.
- Build: `cmake --build build --config Release` passed (result code `0`).

Accepted risk / defer:

- Cognitive complexity (`S3776`) and deep nesting (`S134`) in `src/main.cpp` are still pending; these require a dedicated refactor batch to avoid functional drift.

