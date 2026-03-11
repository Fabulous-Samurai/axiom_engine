---
description: How to build the AXIOM Engine project
---

# Build AXIOM Engine

// turbo-all

1. Configure the build (only needed once or after CMakeLists.txt changes):
```bash
cmake --preset default-ninja
```

For AVX-VNNI support (Intel Alder Lake+/Zen4+):
```bash
cmake --preset default-ninja -DAXIOM_ENABLE_SIMD_AVX_VNNI=ON
```

2. Build all targets:
```bash
cmake --build build
```

3. Verify the build succeeded by checking for zero errors in the output. Expected targets:
   - `axiom.exe` — main CLI engine
   - `run_tests.exe` — baseline test suite (86 tests)
   - `giga_test_suite.exe` — comprehensive monolithic test suite (56 tests)
   - `axiom_benchmark.exe` — benchmark binary with Mantis BEFORE/AFTER tests

4. Run the test suites:
```bash
.\build\run_tests.exe
.\build\giga_test_suite.exe
```

5. Run benchmarks:
```bash
.\build\axiom_benchmark.exe
```

## Protocol conformance

This workflow follows the `global_agent_process_protocol` rules. See `.agents/global_agent_process_protocol.md` for required metadata, preconditions, inputs/outputs, validation and rollback guidance. Ensure any change to the build steps documents `Outputs` and expected artifact names under `output/`.

## Purpose

Build and verify AXIOM Engine C++ binaries, tests and benchmarks.

## Preconditions

- CMake >= 3.12 installed
- Ninja available in PATH
- Optional: Python dev headers if `AXIOM_ENABLE_NANOBIND` is on

## Inputs

- Source tree (repo root)
- `CMakeLists.txt`, `requirements-optional.txt` (for Python deps)

## Outputs

- `build/` directory with binaries: `axiom.exe`, `run_tests.exe`, `giga_test_suite.exe`, `axiom_benchmark.exe`
- `output/build_report.txt` (optional, produced by CI)

## Steps

1. Configure: `cmake --preset default-ninja`
2. Build: `cmake --build build`
3. Run unit tests: `build/run_tests.exe`
4. Run benchmarks: `build/axiom_benchmark.exe`

## Validation

- Build exits with zero. Unit tests pass.
- Benchmark binary produces `output/benchmarks/*.json` when run with `--benchmark_out`.

## Rollback

- If build fails after a source change, revert the last commit(s) that touched CMake or the failing files and re-run the build.
