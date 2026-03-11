---
description: How to fix SonarQube code smells in the AXIOM Engine
---

# Fix SonarQube Code Smells

## Prerequisites
- Read `output/parsed_issues.txt` for the full list of Sonar issues grouped by file.
- Read `output/files_with_issues.json` for the structured JSON source.

## Completed Phases (DO NOT REDO)
The following phases have been completed in a prior session:

- **Phase 1** ✅ — `advanced_3d_visualization.py`, `algebraic_parser.cpp`
  - Extracted duplicate string literals into constants
  - `snprintf` → `std::format`, `string_view` temp fixes
- **Phase 2** ✅ — `arena_allocator.cpp/h`
  - `char*` → `std::byte*` pointer arithmetic
  - Rule of 5 applied, explicit constructors
- **Phase 3** ✅ — `daemon_engine.cpp/h`, `eigen_engine.cpp/h`, `dynamic_calc.cpp`
  - `std::thread` → `std::jthread`
  - In-class member initializers
  - Specific exception catches (`std::invalid_argument`)
- **Phase 4** ✅ — Test files (`giga_test_suite.cpp`, `edge_case_suite.cpp`, `tests.cpp`)
  - Octal `\033` → hex `\x1b` escape sequences
  - Specific catch blocks before generic `std::exception`
  - In-class initializers for test runner classes
- **Phase 5** ✅ — HPC/HFT Performance Optimizations
  - Centralized `AXIOM_FORCE_INLINE`/`AXIOM_YIELD_PROCESSOR` macros in `cpu_optimization.h`
  - `TryEvaluateFast` moved to header for zero-overhead inlining
  - CPU pause intrinsics in spin-loops
- **Phase 6 & 7** ✅ — Core Remediations
  - `std::map` -> Transparent StringMap `std::less<>` mappings.
  - Implemented thread safety, `jthread`, and `enum` bounds.
- **Phase 8** ✅ — Engine Entry Points
  - Replaced `<print>` formatting with `<format>`.
  - Added structural code constraint guarantees (`const` purity in parsers).
- **Phase 9, 10, 11 & 12** ✅ — Final Audits & Format Alignments
  - Fixed 114 critical cognition loops across Algebraic Parsers.
  - Developed "The Abyss" Adversarial Tests.
  - Model-Checked `MantisFixedMinHeap` with TLA+ Formal Verification.
  - Eliminated hardcoded M_PI (`S6164`) and explicit lambdas (`S3608`).
  - Flattened `daemon_loop` structural depth using isolated pipeline scopes (S134).
  - Adopted strict `std::format` allocations bypassing expensive concatenations (`S6495`).
  - System Weakness Audit: Eliminated concurrency bottlenecks (Spinlock) and optimized AST RTTI/Map allocations.

## Remaining Work
*(All critical priority targets have been cleared from Phase 1 to Phase 12!)*
**Transitioning to Phase 13**: Awaiting next set of architectural objectives.

## Workflow Steps
1. Pick the next priority file from `output/parsed_issues.txt`
2. Read the file and identify the specific Sonar rule violations
3. Apply fixes in batches (max ~10 edits at a time to avoid regressions)
4. Build using the `build` workflow (cross-platform)
5. Run tests to verify no regressions (use `ctest` in CI where possible)
6. Repeat for the next file

## Protocol conformance

This workflow must comply with `.agents/global_agent_process_protocol.md`.
- Document `Inputs`: `output/parsed_issues.txt`, `output/files_with_issues.json`
- Document `Outputs`: patched files (committed in small batches), `output/repair_report.json`
- Validation: build + tests must pass after each batch; Sonar scan (or local rule checks) should show the target issues resolved.

## Purpose

Systematic remediation of SonarQube issues prioritized by severity and impact.

## Preconditions

- `output/files_with_issues.json` and `output/parsed_issues.txt` must be present
- Developer must run local build environment and have test harness available

## Inputs

- `output/files_with_issues.json` — file-to-issues mapping
- `output/parsed_issues.txt` — human-readable prioritized list

## Outputs

- Patched source files (committed in focused PRs)
- `output/repair_report.json` summarizing fixed issues and residuals

## Steps

1. Select top priority file from `output/parsed_issues.txt`
2. Create a short branch: `fix/sonar/<file>-<shortdesc>`
3. Make up to 10 focused edits and run `cmake --build build --config Release`
4. Run tests via `ctest --test-dir build --output-on-failure` (or run specific test binaries locally)
5. Commit and open a PR with `repair_report.json` attached

## Validation

- PR must pass CI build and test jobs
- Sonar re-analysis (or local rule runner) must show decreased issue count for target rules

## Rollback

- Revert the branch if the change causes test regressions or new critical issues.
