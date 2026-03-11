---
description: How to run TLA+ formal verification for the Mantis A* engine
---

# Run TLA+ Formal Verification

// turbo-all

## Overview

Three TLA+ models live in `formal/tla/`. Each has a `.tla` spec and a `.cfg` config.
They are auto-run in CI (`formal-verification` job) when files under `formal/tla/` change.

| Model | What it proves |
|---|---|
| `MantisAStarCorrectness` | A* closed-set monotonicity, `f=g+h` type safety, eventual termination |
| `MantisHeuristicDispatch` | FMA3/scalar produce identical dot products + same node ordering |
| `MantisDogThreshold` | Normalization branch fires iff `score > threshold`; score formula contract |
| `MantisFixedMinHeap` | Zero-allocation array-based heap maintains size invariants and `f_cost` parent <= children structural constraints. |

## Step 1 — Run Python structural tests (no TLC required)

```bash
python -m pytest tests/unit/test_tla_specs.py -v
```

Expected: all `TestTlaSpecsStructural` tests pass; `TestTlcModelCheck` tests are skipped if Java is absent.

## Step 2 — Run full TLC model checking (requires Java + tla2tools.jar)

Place `tla2tools.jar` in the repo root (download from https://github.com/tlaplus/tlaplus/releases),
or set `TLC_JAR=/path/to/tla2tools.jar` in your environment.

```bash
java -jar tla2tools.jar -config formal/tla/MantisAStarCorrectness.cfg formal/tla/MantisAStarCorrectness.tla
java -jar tla2tools.jar -config formal/tla/MantisHeuristicDispatch.cfg formal/tla/MantisHeuristicDispatch.tla
java -jar tla2tools.jar -config formal/tla/MantisDogThreshold.cfg formal/tla/MantisDogThreshold.tla
java -jar tla2tools.jar -config formal/tla/MantisFixedMinHeap.cfg formal/tla/MantisFixedMinHeap.tla
```

Expected output for each: `No error has been found.`

## Step 3 — Verify all invariant names match

Each `.cfg` INVARIANTS block must match the invariant names declared in the corresponding `.tla`.
The Python test harness (`tests/unit/test_tla_specs.py`) checks this automatically.

## Troubleshooting

- **`ASSUME violated`** — a constant in `.cfg` violates the `ASSUME` guard in the spec (e.g., N < 2).
- **`Deadlock reached`** — only meaningful if the terminal state is not intended. `MantisAStarCorrectness` uses a `Done` stutter step to avoid false deadlock alarms.
- **`TLC` not found** — ensure `tla2tools.jar` is at repo root or `TLC_JAR` is set.

## Adding a new Mantis TLA+ spec

1. Create `formal/tla/MyNewSpec.tla` and `formal/tla/MyNewSpec.cfg`
2. Add an entry to the `SPECS` list in `tests/unit/test_tla_specs.py`
3. Document in `docs/formal/TLA_PLUS_VERIFICATION.md`
4. CI picks it up automatically on push

## Protocol conformance

This workflow adheres to `.agents/global_agent_process_protocol.md` requirements.
- `Inputs`: `formal/tla/*.tla`, `formal/tla/*.cfg`
- `Outputs`: TLC logs in `output/formal/` and `output/tlc_reports/`
- Validation: `tests/unit/test_tla_specs.py` must pass; TLC must report `No error has been found.` for checked models.

## Purpose

Run and validate formal verification for Mantis/TLA+ models.

## Preconditions

- Java and `tla2tools.jar` available if TLC model checking is to be run
- Python test harness present

## Inputs

- `formal/tla/*.tla` and `formal/tla/*.cfg`

## Outputs

- `output/formal/tlc-<model>-<timestamp>.log`
- `output/formal/summary.txt`

## Steps

1. Run structural tests: `python -m pytest tests/unit/test_tla_specs.py -q`
2. If TLC is available: `java -jar tla2tools.jar -config <cfg> <tla>` per model, direct logs to `output/formal/`
3. Collect run results into `output/formal/summary.txt`

## Validation

- `tests/unit/test_tla_specs.py` passes
- TLC reports `No error has been found.` for each fully model-checked spec

## Rollback

- If a new spec causes a regression in model checking, revert the spec changes and open an investigation PR.
