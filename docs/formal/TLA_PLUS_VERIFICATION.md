# TLA+ Verification Guide

This guide defines the initial formal verification flow for AXIOM using TLA+.

## Scope

Current TLA+ model targets interactive IPC behavior between GUI and engine:

- mode switch sequencing (`:mode <name>`)
- command progression
- response frame termination with `__END__`

Model files:

- `formal/tla/AxiomIpcProtocol.tla`
- `formal/tla/AxiomIpcProtocol.cfg`
- `formal/tla/AxiomDaemonQueueFairness.tla`
- `formal/tla/AxiomDaemonQueueFairness.cfg`

## What Is Verified

The model checks these properties:

- state machine type safety (`TypeInv`)
- no command is queued while mode ack is pending (`NoCommandWithoutModeAck`)
- frame termination only after payload phase (`EndOnlyAfterResult`)
- eventual termination of response frame (`EventuallyTerminates`)

Daemon fairness model checks:

- request state typing (`TypeInv`)
- eventually start after enqueue (`EventuallyStart`)
- eventually complete after processing (`EventuallyComplete`)
- eventual global completion (`EventuallyAllDone`)

Fairness assumption used by daemon model:

- weak fairness on progress actions (`Enqueue`, `Start`, `Complete`) to prevent unrealistic infinite stutter while work is still enabled

Report interpretation notes:

- `No error has been found`: all configured invariants/properties passed for explored state space.
- `Deadlock reached`: model has a state with no enabled transition. This is only meaningful if that state is not intended as terminal.
- In daemon fairness model, terminal `Done` state is intentionally represented with stutter transition to avoid false deadlock alarms.

## Run With TLC

Prerequisites:

- Java runtime
- TLA+ Toolbox or `tla2tools.jar`

### Option A: TLA+ Toolbox

1. Open Toolbox.
2. Import spec from `formal/tla/AxiomIpcProtocol.tla`.
3. Create model using config file `formal/tla/AxiomIpcProtocol.cfg`.
4. Run model check.

### Option B: CLI

```bash
java -jar tla2tools.jar -config formal/tla/AxiomIpcProtocol.cfg formal/tla/AxiomIpcProtocol.tla
```

## CI Integration Plan

The repository CI includes a stage named `formal-verification` that:

1. runs TLC for each `formal/tla/*.cfg` model
2. fails build on invariant/property violation
3. stores TLC output as artifact (`tlc-output`)

Optimization behavior:

- formal job runs TLC only when formal-related files change (`formal/tla/**`, `docs/formal/**`, `.github/workflows/ci.yml`)

Workflow file:

- `.github/workflows/ci.yml` (job: `formal-verification`)

## Mantis A* Heuristic Models

Added in the Mantis vectorized heuristic work. Model files:

- `formal/tla/MantisAStarCorrectness.tla`
- `formal/tla/MantisAStarCorrectness.cfg`
- `formal/tla/MantisHeuristicDispatch.tla`
- `formal/tla/MantisHeuristicDispatch.cfg`
- `formal/tla/MantisDogThreshold.tla`
- `formal/tla/MantisDogThreshold.cfg`

### MantisAStarCorrectness

Models A* search over a fixed-capacity node set (4-node linear graph). Verifies:

- `TypeInv` — every node holds a valid cost triple: `f = g + h`
- `MonotoneExploration` — once a node is closed, its `g_cost` is frozen; no SIMD reordering can re-settle it with a higher cost (state-skip prevention)
- `OpenSetValidity` — only nodes with finite `g` exist in the open set
- `ClosedImmutable` — closed nodes are never re-added to the open set
- `EventuallyTerminates` *(liveness)* — search always reaches the goal or exhausts the open set under weak fairness

### MantisHeuristicDispatch

Models the compile-time SIMD dispatch decision (`FMA3` vs `Scalar`). Verifies:

- `TypeInv` — dispatch path is always in `{PATH_FMA3, PATH_SCALAR}`
- `DeterministicOutput` — for any input, both paths compute the same dot-product value
- `OrderingEquivalence` — the ordering of two nodes' heuristic scores is identical across paths; SIMD cannot swap the A* expansion order
- `NoSilentDrop` — every call to evaluate always produces a result

### MantisDogThreshold

Models the conditional Dog-threshold normalization branch. Verifies:

- `TypeInv` — raw and normalised scores are bounded integers
- `DogBranchConsistency` — normalization fires **iff** `score > threshold`; never skipped when triggered, never spurious
- `NormSafety` — normalised score equals the reference formula `score * scale + bias`
- `IdentityWhenBelowThreshold` — when the branch is not taken, the score is unchanged

### Run With TLC

```bash
# Individual spec
java -jar tla2tools.jar -config formal/tla/MantisAStarCorrectness.cfg \
                         formal/tla/MantisAStarCorrectness.tla

java -jar tla2tools.jar -config formal/tla/MantisHeuristicDispatch.cfg \
                         formal/tla/MantisHeuristicDispatch.tla

java -jar tla2tools.jar -config formal/tla/MantisDogThreshold.cfg \
                         formal/tla/MantisDogThreshold.tla
```

### Python Test Harness

Structural tests (no TLC required) and TLC integration tests:

```bash
python -m pytest tests/unit/test_tla_specs.py -v
```

TLC tests are automatically skipped when Java is not on PATH.
Place `tla2tools.jar` in the repo root or set the `TLC_JAR` environment
variable to enable full model-checking locally.

## Extension Backlog

Planned follow-up models:

- daemon pipe request queue fairness
- timeout and restart semantics for persistent engine
- mode-specific parser dispatch assumptions

