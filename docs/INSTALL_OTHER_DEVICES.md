# Install AXIOM on Other Devices

This guide provides reproducible setup for new machines.

## Quick Health Baseline

Current project validation status:

- CMake build: PASS
- `run_tests`: PASS
- `giga_test_suite`: PASS
- TLA+ models (`AxiomIpcProtocol`, `AxiomDaemonQueueFairness`): PASS

## Recommended Paths

## 1. Windows (standard CPython)

Run from repository root in PowerShell:

```powershell
./scripts/setup_other_device_windows.ps1 -WithGui
```

What it does:

- creates `.venv`
- installs Python optional dependencies
- installs `PySide6` (when `-WithGui` is passed)
- configures and builds with CMake preset
- runs `build/run_tests.exe`

## 2. Windows (MSYS2 UCRT)

Use this if your toolchain is MSYS2/UCRT:

```powershell
./scripts/setup_other_device_msys2_ucrt.ps1
```

What it does:

- installs UCRT packages via pacman, including `mingw-w64-ucrt-x86_64-pyside6`
- creates `.venv` with UCRT python
- installs Python optional dependencies
- configures/builds and runs baseline tests

## 3. Linux/macOS

```bash
chmod +x scripts/setup_other_device_unix.sh
./scripts/setup_other_device_unix.sh
```

## Post-Install Checks

## Optional Harmonic Arena backend

Enable at configure time when you want lock-free arena fast-path in PoolManager:

```bash
cmake -B build -G Ninja -DAXIOM_ENABLE_HARMONIC_ARENA=ON
cmake --build build --config Release
```

## Native tests

```bash

# Windows

build\run_tests.exe
build\giga_test_suite.exe

# Unix

./build/run_tests
./build/giga_test_suite
```

## Formal checks (TLA+)

```bash
java -jar tools/tla/tla2tools.jar -cleanup -config formal/tla/AxiomIpcProtocol.cfg formal/tla/AxiomIpcProtocol.tla
java -jar tools/tla/tla2tools.jar -cleanup -config formal/tla/AxiomDaemonQueueFairness.cfg formal/tla/AxiomDaemonQueueFairness.tla
```

## Notes

- If VS Code reports `PySide6` unresolved import, ensure the workspace interpreter is `${workspaceFolder}/.venv/bin/python.exe`.
- On MinGW/UCRT python, `pip install PySide6` may fail; use the MSYS2 package route in script 2.

