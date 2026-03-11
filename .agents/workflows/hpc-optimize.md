---
description: How to apply HPC/HFT performance optimizations to the AXIOM Engine
---

# HPC/HFT Performance Optimization Workflow

// turbo-all

## Project Performance Goals
AXIOM Engine targets HPC/HFT-grade latency standards:
- Sub-microsecond arithmetic dispatch via `EvaluateFast` hot path
- Lock-free memory allocation via `HarmonicArena`
- Lock-free SPSC request queue in `DaemonEngine`
- SIMD acceleration via Eigen (AVX2/SSE4.1)
- **Mantis Heuristic**: FMA3 + AVX-VNNI vectorized A* heuristic kernel

## Key Performance Files
| File | Role |
|------|------|
| `include/cpu_optimization.h` | `AXIOM_FORCE_INLINE`, `AXIOM_YIELD_PROCESSOR`, `AXIOM_ALIGN_AVX`, `CpuFeatureGuard` |
| `include/dynamic_calc.h` | `TryEvaluateFast` — inlined zero-overhead arithmetic dispatch |
| `include/mantis_heuristic.h` | FMA3 + VNNI vectorized dot-product kernels |
| `include/mantis_solver.h` | Zero-allocation A* solver with `thread_local` scratch |
| `src/arena_allocator.cpp` | `HarmonicArena` lock-free block allocator |
| `src/daemon_engine.cpp` | SPSC queue with CPU pause intrinsics |
| `tests/benchmark_suite.cpp` | Full before/after benchmark with Mantis SIMD tests |
| `formal/tla/MantisAStarCorrectness.tla` | TLA+ proof: closed-set monotonicity, state-skip prevention |
| `formal/tla/MantisHeuristicDispatch.tla` | TLA+ proof: SIMD path ordering equivalence |
| `formal/tla/MantisDogThreshold.tla` | TLA+ proof: Dog-threshold branch consistency |
| `tests/unit/test_tla_specs.py` | Structural + TLC integration tests for all three models |

## CMake SIMD Flags
Build with VNNI enabled:
```bash
cmake --preset default-ninja -DAXIOM_ENABLE_SIMD_AVX_VNNI=ON
cmake --build build
```

Available flags:
- `AXIOM_ENABLE_SIMD_AVX2` (default ON) — AVX2 256-bit operations
- `AXIOM_ENABLE_SIMD_FMA` (default ON) — Fused multiply-add
- `AXIOM_ENABLE_SIMD_AVX_VNNI` (default OFF) — INT8 dot-product (Alder Lake+/Zen4+)
- `AXIOM_ENABLE_SIMD_AVX512` (default OFF) — AVX-512

## Completed Optimizations
- [x] Centralized `AXIOM_FORCE_INLINE` / `AXIOM_YIELD_PROCESSOR` macros
- [x] Inlined `TryEvaluateFast` in header
- [x] CPU pause intrinsics in arena and daemon spin-loops
- [x] `CpuFeatureGuard` + `CheckCXXCompilerFlag` probes in CMake
- [x] FMA3 `_mm256_fmadd_ps` heuristic kernel
- [x] AVX-VNNI `_mm256_dpbusd_epi32` heuristic kernel
- [x] Zero-allocation A* solver with `alignas(64)` nodes
- [x] Before/after benchmark suite
- [x] TLA+ formal verification: `MantisAStarCorrectness` (monotone closed-set, state-skip prevention)
- [x] TLA+ formal verification: `MantisHeuristicDispatch` (SIMD ordering equivalence, no silent drop)
- [x] TLA+ formal verification: `MantisDogThreshold` (Dog-threshold branch consistency + formula contract)

## Benchmark Results (Intel Core Ultra 7 255U)
| Test | Latency | Throughput |
|------|---------|-----------|
| Scalar FP32 dot | 0.09 ns/op | 11 Gops/sec |
| FMA3 SIMD FP32 dot | 0.40 ns/op | 2.5 Gops/sec |
| Scalar INT8 dot | 1.45 ns/op | 700 Mops/sec |
| VNNI SIMD INT8 dot | 1.38 ns/op | 730 Mops/sec |
| A* solver (64 nodes) | 15.68 ns/node | — |

> Note: With `-O3 -march=native -flto`, the compiler auto-vectorizes scalar loops.
> The explicit SIMD path guarantees this performance without `-march=native`.

## Future Optimization Opportunities
- [ ] Apply `AXIOM_HOT` attribute to `Evaluate()`, `EvaluateFast()`, `allocate()`
- [ ] Profile-guided optimization (PGO) build preset
- [ ] Cache-line padding (`alignas(64)`) on contended atomics
- [ ] Prefetch hints (`__builtin_prefetch`) in parser token scanning
- [ ] Batch evaluation API for vectorized expression throughput
- [ ] Wider SIMD kernels for multi-row feature matrices

## Running Benchmarks
```bash
.\build\axiom_benchmark.exe
```

## Protocol conformance

This workflow follows the `global_agent_process_protocol`. Documents required:
- `Inputs`: benchmark fixtures under `tests/benchmark_fixtures/` and `benchmarks/` expected values
- `Outputs`: `output/benchmarks/*.json`, performance regression alerts
- Validation: benchmark results must be compared to baseline and any >5% regressions require rollback or performance investigation.

## Purpose

Drive and validate performance improvements that meet AXIOM's HPC/HFT targets.

## Preconditions

- Baseline benchmark data available in `benchmarks/baseline/`
- Compiler toolchain supporting AVX/FMA as required

## Inputs

- Source code
- Benchmark fixtures under `tests/benchmark_fixtures/`

## Outputs

- `output/benchmarks/<run>.json`
- `output/benchmarks/summary.txt`

## Steps

1. Enable the desired SIMD flags via CMake options
2. Build with `cmake --preset default-ninja`
3. Run `build/axiom_benchmark.exe --benchmark_out=output/benchmarks/run.json`
4. Compare `run.json` to the baseline and create `output/benchmarks/summary.txt`

## Validation

- No regressions beyond tolerated thresholds (e.g., 5%)
- All benchmark runs complete successfully

## Rollback

- If regressions exceed threshold, revert perf-related commits and bisect to find the cause.
