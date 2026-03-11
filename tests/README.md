# AXIOM Test Guide

This directory contains C++ test binaries, Python integration tests, and benchmark assets for AXIOM Engine.

## Scope

- Validate parser and engine correctness across supported modes.
- Verify integration between CLI and Python-side workflows.
- Measure hot-path and IPC performance with repeatable benchmark runs.

## Test Layout

Current repository structure includes:

- Root-level C++ test programs (for example `giga_test_suite.cpp`, `benchmark_suite.cpp`, `ast_drills.cpp`).
- Python-oriented tests under folders such as `unit/` and `integration/`.
- Example scenarios under `examples/`.

## Build Test Targets

From repository root:

```bash
cmake --preset default-ninja
cmake --build build --config Release --target run_tests giga_test_suite ast_drills axiom_benchmark
```

Primary targets:

- `run_tests`: baseline C++ test executable.
- `giga_test_suite`: broad integration and subsystem coverage.
- `ast_drills`: AST-focused validation.
- `axiom_benchmark`: benchmark executable used for performance reports.

## Run C++ Tests

```bash
./build/run_tests
./build/giga_test_suite
./build/ast_drills
```

On Windows, use executable names with `.exe` as needed.

## Run Python Tests

If using `pytest` from repository root:

```bash
pytest
```

You can also run specific files directly from `tests/` when needed.

## Benchmark Execution

Build and run:

```bash
cmake --build build --config Release --target axiom_benchmark
./build/axiom_benchmark
```

Generated artifacts are written to project root:

- `benchmark_results.csv`
- `benchmark_results.json`

Measured benchmark sections currently include:

- scalar parser throughput
- typed fast-path throughput
- zero-copy vector transfer behavior
- lock-free queue IPC cycles

## Recommended Validation Flow

1. Build release profile with Ninja preset.
2. Run `run_tests` for quick correctness check.
3. Run `giga_test_suite` for broader subsystem confidence.
4. Run `axiom_benchmark` and inspect CSV/JSON outputs.
5. Optionally run `pytest` for Python-side and integration checks.

## Troubleshooting

- If a binary is missing, rebuild target explicitly with `--target <name>`.
- If Python tests fail on imports, verify virtual environment activation and dependency installation.
- If benchmark output files are missing, confirm benchmark process completed successfully in current working directory.

## Related Files

- Project overview: [../README.md](../README.md)
- Main usage/build guide: [../README.md#build-system-and-presets](../README.md#build-system-and-presets)
- Main benchmarking section: [../README.md#benchmarking](../README.md#benchmarking)
- Release history: [../CHANGELOG.md](../CHANGELOG.md)
- Optional Python deps: [../requirements-optional.txt](../requirements-optional.txt)


