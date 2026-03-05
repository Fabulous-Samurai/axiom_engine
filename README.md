# AXIOM Engine v3.1.1

[![Version](https://img.shields.io/badge/version-3.1.1-blue.svg)](https://github.com/Fabulous-Samurai/axiom_engine/releases)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C++-20-blue.svg)](https://isocpp.org/)

AXIOM is a C++20-first mathematical engine with a hybrid FFI model.

- Core computation is in C++.
- Python is positioned mainly for GUI and visualization.
- `nanobind` is used as the C++/Python bridge.

## What This Project Provides

- Multi-mode computation engine (`algebraic`, `linear`, `statistics`, `symbolic`, `units`, `plot`).
- CLI execution for single-expression and interactive workflows.
- Python GUI frontend (`gui/python/axiom_gui.py`) with C++ engine integration.
- Dedicated benchmark binary with CSV and JSON report export.
- Large C++ test targets for integration and stress validation.

## Architecture Summary

### Core Runtime

- `DynamicCalc` routes requests by `CalculationMode` to parser implementations.
- Parser-backed path supports algebraic, linear system, statistics, and symbolic modes.
- Typed arithmetic fast-path (`EvaluateFast`) bypasses parser overhead for hot arithmetic operations.

### Dispatch and Performance Routing

- `SelectiveDispatcher` can route to native compute and optional backends.
- Native dispatch path reuses `thread_local DynamicCalc` to reduce per-request construction cost.
- Optional integration points exist for Eigen and nanobind backends when enabled.

### Daemon and IPC

- Daemon implementation exists in source with lock-free SPSC request queue and platform pipe handling.
- Current default build profile does not explicitly define `ENABLE_DAEMON_MODE` for `axiom` CLI.
- Treat daemon CLI commands as build-configuration-dependent features.

### Python Integration Model

- Hybrid FFI is the intended primary integration strategy.
- Embedded Python engine sources are optional (`AXIOM_ENABLE_EMBEDDED_PYTHON_ENGINE=OFF` by default).
- Optional GUI dependencies are intentionally minimal:
  - `numpy`
  - `matplotlib`
  - `PySide6` (Qt GUI runtime, when using CPython wheels)

## Build System and Presets

### Requirements

- C++20 compiler
- CMake 3.12+
- Ninja

### Presets

`CMakePresets.json` includes:

- `default-ninja`: release-oriented standard profile (`AXIOM_ENABLE_CXX20_MODULES=OFF`)
- `modules-ninja`: experimental profile with modules flag enabled
- `build-release`: build preset for default profile
- `build-modules`: builds `axiom_modules_pilot`

### Default Build

```bash
cmake --preset default-ninja
cmake --build build --config Release
```

### Optional Modules Pilot

```bash
cmake --preset modules-ninja
cmake --build --preset build-modules
```

Notes:

- C++20 modules are incremental/experimental in this repository.
- GNU/MinGW profiles intentionally skip module compilation and emit a configure-time warning.

## Main CMake Targets

- `axiom`: main CLI engine executable
- `run_tests`: baseline test executable
- `axiom_benchmark`: benchmark executable (`tests/benchmark_suite.cpp`)
- `giga_test_suite`: comprehensive monolithic C++ test executable
- `ast_drills`: AST-focused test executable
- `axiom_modules_pilot`: experimental modules target (or skip-stub depending on compiler)

## CLI Usage

### Interactive Mode

```bash
./build/axiom
```

### Single Expression

```bash
./build/axiom "2 + 3 * 4"
```

### Mode Selection

```bash
./build/axiom --mode=linear "solve [[2,1],[1,3]] [8,13]"
./build/axiom --mode=statistics "mean([1,2,3,4,5])"
./build/axiom --symbolic "x^2 + 2*x + 1"
```

### GUI Hint Mode

```bash
./build/axiom --gui
```

### Python GUI Launcher

```bash
python gui/python/axiom_gui.py
```

### Interactive Subprocess Mode (for GUI integrations)

```bash
./build/axiom --interactive
```

The interactive subprocess protocol supports mode switching via `:mode <name>` and delimits responses with `__END__`.

## Benchmarking

### Build and Run

```bash
cmake --build build --config Release --target axiom_benchmark
./build/axiom_benchmark
```

### What Is Measured

- Scalar parser throughput (`Evaluate("2+2")` loop)
- Typed fast-path throughput (`EvaluateFast`)
- Zero-copy style vector transfer behavior
- Lock-free queue IPC cycle behavior

### Generated Benchmark Artifacts

- `benchmark_results.csv`
- `benchmark_results.json`

## Formal Verification (TLA+)

AXIOM now includes an initial TLA+ model for IPC protocol checks.

- Spec location: `formal/tla/AxiomIpcProtocol.tla`
- TLC config: `formal/tla/AxiomIpcProtocol.cfg`
- Spec location: `formal/tla/AxiomDaemonQueueFairness.tla`
- TLC config: `formal/tla/AxiomDaemonQueueFairness.cfg`
- Guide: `docs/formal/TLA_PLUS_VERIFICATION.md`

Current model scope:

- Interactive protocol framing (`__END__`)
- Mode switch sequencing before command execution
- Request/response progression safety
- Daemon queue fairness (enqueued requests eventually processed and completed)

## Testing

Detailed test playbook is available at [tests/README.md](tests/README.md).

### C++ Test Binaries

```bash
cmake --build build --config Release --target run_tests giga_test_suite ast_drills
./build/run_tests
./build/giga_test_suite
./build/ast_drills
```

### Python/Integration Test Assets

Repository also includes Python-side tests and examples under `tests/` (unit/integration/example oriented files).

If you use `pytest`, run from repository root:

```bash
pytest
```

## Configuration Flags

Important CMake options:

- `AXIOM_AUTO_INSTALL_PYTHON_DEPS` (default `ON`)
- `AXIOM_ENABLE_EMBEDDED_PYTHON_ENGINE` (default `OFF`)
- `AXIOM_ENABLE_CXX20_MODULES` (default `OFF`)
- `BUILD_GIGA_TESTS` (default `ON`)

Behavior notes:

- Python dependency auto-install runs only when a virtual environment is detected.
- Embedded Python engine sources are excluded unless explicitly enabled.

## Runtime Guardrail Configuration

AXIOM now supports runtime tuning for expression policy and daemon resilience without recompilation.

### Expression Policy Environment Variables

- `AXIOM_POLICY_MAX_CHARS_DEFAULT` (default `8192`)
- `AXIOM_POLICY_MAX_CHARS_SYMBOLIC` (default `16384`)
- `AXIOM_POLICY_MAX_TOKENS` (default `2048`)
- `AXIOM_POLICY_MAX_DEPTH_DEFAULT` (default `128`)
- `AXIOM_POLICY_MAX_DEPTH_SYMBOLIC` (default `256`)
- `AXIOM_POLICY_MAX_CARET_OPS` (default `64`)
- `AXIOM_POLICY_MAX_MATRIX_ELEMENTS` (default `40000`)

### Daemon Resilience Environment Variables

- `AXIOM_DAEMON_CIRCUIT_FAILURE_THRESHOLD` (default `5`)
- `AXIOM_DAEMON_CIRCUIT_OPEN_MS` (default `2000`)
- `AXIOM_DAEMON_BACKPRESSURE_WAIT_MS` (default `5`)

### Suggested Presets

- `balanced`:
  - Keep defaults for most workloads.
- `strict`:
  - Reduce `AXIOM_POLICY_MAX_TOKENS` and `AXIOM_POLICY_MAX_DEPTH_DEFAULT` by 30-50%.
  - Reduce `AXIOM_DAEMON_CIRCUIT_FAILURE_THRESHOLD` (for example to `3`).
- `throughput`:
  - Increase `AXIOM_DAEMON_BACKPRESSURE_WAIT_MS` moderately (for example to `8-12`).
  - Keep circuit-open window conservative to avoid prolonged refusal windows.

## Repository Orientation

- `src/`: core engine and runtime implementation
- `include/`: public/core headers
- `core/dispatch/`: selective dispatcher and C API adapter
- `tests/`: C++ and Python test assets
- `gui/python/`: Python GUI frontend and helpers
- `modules/`: incremental C++20 modules pilot units

## Known Constraints

- Some CLI help output still references enterprise flags that depend on compile-time macros.
- Daemon runtime paths are platform-conditional and build-definition-dependent.
- Modules support is intentionally conservative per compiler profile.

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for release history and migration notes.

## Related Guides

- Test guide: [tests/README.md](tests/README.md)
- Optional Python dependencies: [requirements-optional.txt](requirements-optional.txt)
- TLA+ verification guide: [docs/formal/TLA_PLUS_VERIFICATION.md](docs/formal/TLA_PLUS_VERIFICATION.md)

## License

MIT. See [LICENSE](LICENSE).
