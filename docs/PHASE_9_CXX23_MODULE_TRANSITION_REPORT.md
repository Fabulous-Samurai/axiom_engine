# Phase 9: Axiom Harmonic C++23 Modernization and Module Transition

## 1. Analysis and Readiness

### Compiler Audit

- Toolchain detected: `g++ 15.2.0` (MSYS2 UCRT64).
- Project standard upgraded to `CMAKE_CXX_STANDARD 23`.
- `if consteval` is now valid in project code without extension warnings.

### Dependency Scan (include pressure)

Top repeated include directives across `include/`, `src/`, `core/`:

- `#include <string>`: 20
- `#include <vector>`: 20
- `#include <algorithm>`: 15
- `#include <sstream>`: 13
- `#include <memory>`: 12
- `#include <iostream>`: 12
- `#include "dynamic_calc_types.h"`: 11

Priority transition candidate chosen:

- `core/dispatch/selective_dispatcher.cpp`
- `core/dispatch/selective_dispatcher.h`

Reason:

- Executive orchestration layer for operation routing.
- Couples DynamicCalc, Eigen, and optional nanobind boundaries.
- High leverage for reducing preprocessor and include fanout.

### Size Baseline

- Pre-change binary (observed): `build/axiom.exe = 1,206,299 bytes`.

## 2. Implementation Strategy and Execution

### Module Transition (executive layer)

Implemented module pilot unit:

- `modules/axiom_signal_exec_module.cpp`

Added compile-time executive traits header used by dispatcher:

- `core/dispatch/signal_exec_traits.h`

Dispatcher integration:

- `core/dispatch/selective_dispatcher.cpp`
  - `RequiresSquareMatrix(...)` now uses `SignalExec::IsSquareOnlyOperation(...)`.

CMake integration:

- `modules/axiom_signal_exec_module.cpp` added to `axiom_modules_pilot`.
- GNU module build is guarded off by default path due current GCC modules-ts pipeline failure on this environment.

### Meta-programming Optimization

Added compile-time primitives in `core/dispatch/signal_exec_traits.h`:

- `constinit`:
  - `kSignalFilter3Tap`
- `consteval`:
  - `IsSquareOnlyOperationCt(...)`
  - `FastMathErrorAtCompileTime(...)`
- `if consteval`:
  - `BlendAtCompileOrRuntime(...)`

### Linker and Size Controls

CMake updates in `CMakeLists.txt`:

- Added options:
  - `AXIOM_ENABLE_MOLD_LINKER` (default OFF)
  - `AXIOM_ENABLE_SIZE_GUARDS` (default ON)
- Size controls enabled for Release:
  - `-ffunction-sections -fdata-sections`
  - `-Wl,--gc-sections -s`
- Existing LTO path preserved.

Mold status:

- Configured as optional; auto-used only if requested and found in PATH.

## 3. Verification and Guardrails

### System Integrity

- `run_tests.exe`: `86 passed, 0 failed`.
- `giga_test_suite.exe`: `56 passed, 0 failed`.

Total validated tests in this run set: `142/142`.

### Performance Benchmark

From `benchmark_results.json`:

- `typed_fast_path`: `3.47222e+08 ops/sec` (~347M ops/sec)
- `scalar_throughput`: `77631.6 ops/sec`

Guardrail outcome:

- Throughput target (`>= 200M ops/sec`) preserved and exceeded.

Resonance metric note:

- Explicit `5.0039 ns resonance` metric is not currently emitted by in-repo benchmark binaries.
- Closest available hot-path proxy is `typed_fast_path` latency: `0.00288 us` (2.88 ns).

### Binary Size Audit

- Final binary: `build/axiom.exe = 448,512 bytes`.
- Target (`<= 1.2 MB`) satisfied with wide margin.

## 4. Risk Notes

- GCC modules-ts path was attempted and failed for pilot target with:

  - `inputs may not also have inputs`

- Mitigation applied:

  - Keep module units in-repo for migration continuity.
  - Keep GNU module compilation disabled in this profile until stable compiler/CMake pipeline is available.
