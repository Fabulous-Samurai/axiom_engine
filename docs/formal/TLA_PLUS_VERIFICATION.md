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

## Extension Backlog

Planned follow-up models:

- daemon pipe request queue fairness
- timeout and restart semantics for persistent engine
- mode-specific parser dispatch assumptions
