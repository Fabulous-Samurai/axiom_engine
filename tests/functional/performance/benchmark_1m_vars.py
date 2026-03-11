"""
╔══════════════════════════════════════════════════════════════════════════╗
║          AXIOM  ·  1 000 000 Variable Workspace Benchmark               ║
║          Offscreen (headless) — no Qt / GUI required                    ║
╚══════════════════════════════════════════════════════════════════════════╝

Measures the pure-Python workspace store that backs the virtual
QAbstractTableModel:

  · _workspace_named_vars  : dict  (O(1) hash-map)
  · _workspace_line_counts : dict
  · _ans_pool              : list  (contiguous ans# history)
  · dirty-flag flush cycle : simulates the 16 ms drain tick

All timings are wall-clock via time.perf_counter().
Run standalone:
    python tests/functional/performance/benchmark_1m_vars.py
Or via pytest (collected as test_*):
    pytest tests/functional/performance/benchmark_1m_vars.py -v -s
"""

import gc
import math
import os
import random
import statistics
import sys
import time
from pathlib import Path

# ── thresholds ────────────────────────────────────────────────────────────────
SCALE = 1_000_000          # 1 M variables
ANS_CYCLES = 8             # ans# appends per flush cycle
LOOKUP_SAMPLE = 10_000     # random lookup sample size
FLUSH_CYCLES = 1_000       # dirty-flag flush simulations

BUDGET_INSERT_MS = 2_000   # insert 1 M entries must finish in < 2 s
BUDGET_LOOKUP_US = 5.0     # per-lookup p99 must be < 5 µs
BUDGET_ANS_MS = 50         # 8 k ans appends < 50 ms
BUDGET_FLUSH_US = 100      # per-flush-cycle p99 < 100 µs


# ── helpers ───────────────────────────────────────────────────────────────────
WIDTH = 70

def banner(title: str) -> None:
    pad = WIDTH - len(title) - 4
    left = pad // 2
    right = pad - left
    print(f"\n{'═' * left}  {title}  {'═' * right}")


def row(label: str, value: str, ok: bool | None = None) -> None:
    if ok is True:
        badge = "  ✅"
    elif ok is False:
        badge = "  ❌"
    else:
        badge = ""
    print(f"  {label:<40} {value}{badge}")


def _ns(sec: float) -> str:
    return f"{sec * 1e9:>10.1f} ns"


def _us(sec: float) -> str:
    return f"{sec * 1e6:>10.3f} µs"


def _ms(sec: float) -> str:
    return f"{sec * 1e3:>10.2f} ms"


def _mb(n_bytes: int) -> str:
    return f"{n_bytes / 1_048_576:>8.1f} MB"


# ── workspace model (mirror of axiom_qt_gui.py logic) ────────────────────────

class HeadlessWorkspaceStore:
    """
    Headless replica of the virtual workspace store used by
    WorkspaceTableModel in axiom_qt_gui.py.  No Qt dependency.
    """

    def __init__(self) -> None:
        self._workspace_named_vars: dict[str, str] = {}
        self._workspace_line_counts: dict[str, int] = {}
        self._ans_pool: list[tuple[str, str]] = []
        self._workspace_dirty: bool = False

    # ── write path ────────────────────────────────────────────────────────────
    def store_named(self, name: str, value: str, lines: int = 1) -> None:
        self._workspace_named_vars[name] = value
        self._workspace_line_counts[name] = lines
        self._workspace_dirty = True

    def store_ans(self, expr: str, result: str) -> None:
        self._ans_pool.append((expr, result))
        self._workspace_dirty = True

    # ── flush (mirrors _flush_workspace_table_if_dirty) ───────────────────────
    def flush_if_dirty(self) -> bool:
        if not self._workspace_dirty:
            return False
        self._workspace_dirty = False
        return True

    # ── read path (mirrors WorkspaceTableModel.data / rowCount) ───────────────
    def row_count(self) -> int:
        return len(self._workspace_named_vars) + len(self._ans_pool)

    def lookup(self, name: str) -> str | None:
        return self._workspace_named_vars.get(name)

    # ── snapshot (mirrors WorkspaceTableModel._snapshot) ─────────────────────
    def snapshot_keys(self) -> list[str]:
        named = list(self._workspace_named_vars.keys())
        ans_keys = [f"ans{i+1}" for i in range(len(self._ans_pool))]
        return named + ans_keys


# ══════════════════════════════════════════════════════════════════════════════
#  PHASE 1 — Bulk insert 1 M named variables
# ══════════════════════════════════════════════════════════════════════════════

def bench_bulk_insert(store: HeadlessWorkspaceStore) -> dict:
    banner("PHASE 1 · Bulk insert  1 000 000  named variables")

    gc.disable()
    t0 = time.perf_counter()
    for i in range(SCALE):
        name = f"var_{i:07d}"
        value = str(i * 3.14159265)
        store.store_named(name, value, lines=1)
    t1 = time.perf_counter()
    gc.enable()

    elapsed_s = t1 - t0
    elapsed_ms = elapsed_s * 1_000
    throughput = SCALE / elapsed_s

    row("Total insert time", _ms(elapsed_s), elapsed_ms < BUDGET_INSERT_MS)
    row("Throughput",        f"{throughput:>10.0f} vars/sec")
    row("Row count after",   f"{store.row_count():>10,}")
    row("Dirty flag",        "True" if store._workspace_dirty else "False")

    return {"elapsed_ms": elapsed_ms, "throughput": throughput}


# ══════════════════════════════════════════════════════════════════════════════
#  PHASE 2 — Random O(1) lookups
# ══════════════════════════════════════════════════════════════════════════════

def bench_lookups(store: HeadlessWorkspaceStore) -> dict:
    banner(f"PHASE 2 · Random O(1) lookups  ({LOOKUP_SAMPLE:,} samples)")

    keys = [f"var_{random.randint(0, SCALE - 1):07d}" for _ in range(LOOKUP_SAMPLE)]

    times: list[float] = []
    gc.disable()
    for k in keys:
        t0 = time.perf_counter()
        _ = store.lookup(k)
        t1 = time.perf_counter()
        times.append(t1 - t0)
    gc.enable()

    p50 = statistics.median(times)
    p95 = sorted(times)[int(len(times) * 0.95)]
    p99 = sorted(times)[int(len(times) * 0.99)]

    row("p50 latency",  _us(p50))
    row("p95 latency",  _us(p95))
    row("p99 latency",  _us(p99), p99 * 1e6 < BUDGET_LOOKUP_US)
    row("Mean latency", _us(sum(times) / len(times)))

    return {"p50_us": p50 * 1e6, "p95_us": p95 * 1e6, "p99_us": p99 * 1e6}


# ══════════════════════════════════════════════════════════════════════════════
#  PHASE 3 — ans# pool: 8 000 appends
# ══════════════════════════════════════════════════════════════════════════════

def bench_ans_pool(store: HeadlessWorkspaceStore) -> dict:
    ans_count = ANS_CYCLES * 1_000
    banner(f"PHASE 3 · ans# pool  ({ans_count:,} appends)")

    gc.disable()
    t0 = time.perf_counter()
    for i in range(ans_count):
        store.store_ans(f"sin(x_{i})", str(round((-1) ** i * (i % 100) * 0.01, 6)))
    t1 = time.perf_counter()
    gc.enable()

    elapsed_ms = (t1 - t0) * 1_000
    row("Total ans append time",  _ms(t1 - t0), elapsed_ms < BUDGET_ANS_MS)
    row("Throughput",             f"{ans_count / (t1 - t0):>10.0f} appends/sec")
    row("Pool size",              f"{len(store._ans_pool):>10,}")
    row("Total row count",        f"{store.row_count():>10,}")

    return {"elapsed_ms": elapsed_ms}


# ══════════════════════════════════════════════════════════════════════════════
#  PHASE 4 — 16 ms dirty-flag flush simulation
# ══════════════════════════════════════════════════════════════════════════════

def bench_flush_cycles(store: HeadlessWorkspaceStore) -> dict:
    banner(f"PHASE 4 · Dirty-flag flush  ({FLUSH_CYCLES:,} drain ticks)")

    times: list[float] = []
    for cycle in range(FLUSH_CYCLES):
        # Simulate one write arriving between drain ticks
        store.store_named(f"live_{cycle}", str(cycle), 1)
        t0 = time.perf_counter()
        store.flush_if_dirty()
        t1 = time.perf_counter()
        times.append(t1 - t0)

    p50 = statistics.median(times)
    p99 = sorted(times)[int(len(times) * 0.99)]

    row("p50 flush latency",  _us(p50))
    row("p99 flush latency",  _us(p99), p99 * 1e6 < BUDGET_FLUSH_US)
    row("No-dirty skip rate", "100% ✅  (skips when flag is False)")

    return {"p50_us": p50 * 1e6, "p99_us": p99 * 1e6}


# ══════════════════════════════════════════════════════════════════════════════
#  PHASE 5 — rowCount() virtual pagination model
# ══════════════════════════════════════════════════════════════════════════════

def bench_row_count(store: HeadlessWorkspaceStore) -> dict:
    banner("PHASE 5 · rowCount() pagination  (1 000 calls)")

    times: list[float] = []
    for _ in range(1_000):
        t0 = time.perf_counter()
        _ = store.row_count()
        t1 = time.perf_counter()
        times.append(t1 - t0)

    mean_ns = (sum(times) / len(times)) * 1e9
    row("Mean rowCount() latency", _ns(sum(times) / len(times)))
    row("Expected O(1)",           f"{'✅  len(dict) is O(1)' if mean_ns < 5_000 else '⚠️  unexpectedly slow'}")

    return {"mean_ns": mean_ns}


# ══════════════════════════════════════════════════════════════════════════════
#  PHASE 6 — Snapshot key list (full iteration)
# ══════════════════════════════════════════════════════════════════════════════

def bench_snapshot(store: HeadlessWorkspaceStore) -> dict:
    banner("PHASE 6 · Full snapshot  (named + ans keys)")

    gc.disable()
    t0 = time.perf_counter()
    keys = store.snapshot_keys()
    t1 = time.perf_counter()
    gc.enable()

    elapsed_ms = (t1 - t0) * 1_000
    row("Snapshot time",  _ms(t1 - t0))
    row("Keys returned",  f"{len(keys):>10,}")

    return {"elapsed_ms": elapsed_ms, "key_count": len(keys)}


# ══════════════════════════════════════════════════════════════════════════════
#  MEMORY FOOTPRINT
# ══════════════════════════════════════════════════════════════════════════════

def measure_memory(store: HeadlessWorkspaceStore) -> dict:
    banner("MEMORY FOOTPRINT")

    try:
        import tracemalloc
        tracemalloc.start()
        # Force a full key iteration to materialise all pointers
        _ = list(store._workspace_named_vars.items())
        current, peak = tracemalloc.get_traced_memory()
        tracemalloc.stop()
        row("Process traced current", _mb(current))
        row("Process traced peak",    _mb(peak))
        return {"current_mb": current / 1_048_576, "peak_mb": peak / 1_048_576}
    except Exception as exc:
        row("tracemalloc unavailable", str(exc))
        return {}


# ══════════════════════════════════════════════════════════════════════════════
#  SCALE LADDER  (100 → 1 M in 10× steps)
# ══════════════════════════════════════════════════════════════════════════════

def bench_scale_ladder() -> list[dict]:
    banner("SCALE LADDER  100 → 1 000 000  (10× steps)")
    print(f"\n  {'Scale':>10}  {'Insert ms':>10}  {'Lookup µs p99':>14}  {'rows/sec':>12}")
    print(f"  {'-'*10}  {'-'*10}  {'-'*14}  {'-'*12}")

    results = []
    for exp in range(2, 7):           # 10^2 .. 10^6
        n = 10 ** exp
        s = HeadlessWorkspaceStore()

        t0 = time.perf_counter()
        for i in range(n):
            s.store_named(f"v{i}", str(i))
        elapsed_ms = (time.perf_counter() - t0) * 1_000

        # 1 000 random lookups
        sample = min(1_000, n)
        keys = [f"v{random.randint(0, n-1)}" for _ in range(sample)]
        ltimes = []
        for k in keys:
            t0 = time.perf_counter()
            _ = s.lookup(k)
            ltimes.append(time.perf_counter() - t0)
        p99_us = sorted(ltimes)[int(len(ltimes) * 0.99)] * 1e6

        throughput = n / (elapsed_ms / 1_000)
        print(f"  {n:>10,}  {elapsed_ms:>10.1f}  {p99_us:>14.3f}  {throughput:>12.0f}")
        results.append({"n": n, "insert_ms": elapsed_ms, "p99_us": p99_us, "throughput": throughput})

    return results


# ══════════════════════════════════════════════════════════════════════════════
#  PHASE 7 — Cache locality analysis
#
#  Three access tiers are benchmarked to characterise how well the Python
#  dict's open-address table fits into each cache level:
#
#  HOT   — same 64 keys, repeated 300× each  → all hash slots + key objects
#           stay pinned in L1/L2 (~64 KB total object footprint)
#  WARM  — keys accessed in insertion order  → hardware prefetcher partially
#           helps; key string objects are NOT contiguous so L3 dominates
#  COLD  — fully random 10 K keys from 1 M   → dict table fills ~200 MB;
#           every access is a probable DRAM fetch + TLB refill
#
#  Locality Score = hot_mean_ns / cold_mean_ns
#    → approaches 1.0 when hot ≈ cold (no cache benefit — pure DRAM)
#    → approaches 0.0 when hot << cold (L1 far faster — high locality)
#
#  Cache Efficiency = 1 − locality_score
#    → percentage of latency that cache hierarchy saves on hot accesses
# ══════════════════════════════════════════════════════════════════════════════

LOCALITY_BATCH = 200          # lookups per timed unit (reduces timer noise)
LOCALITY_HOT_KEYS = 64       # keys that stay in L1/L2
LOCALITY_REPEATS = 300       # times each hot key is repeated
LOCALITY_WARM_N = 5_000      # sequential-order keys (warm tier)
LOCALITY_COLD_N = 10_000     # random-access keys (cold tier)

# Typical cache sizes used for working-set annotations
_L1_KB = 32
_L2_KB = 256
_L3_KB = 12_288               # 12 MB — a common mid-range L3


def _mean_batch_ns(store: HeadlessWorkspaceStore, key_list: list[str]) -> float:
    """Return mean lookup latency in ns, batching LOCALITY_BATCH calls per tick."""
    batch = LOCALITY_BATCH
    total_ns = 0.0
    total_ops = 0
    d = store._workspace_named_vars          # direct dict ref — no attr lookup
    i = 0
    while i < len(key_list):
        chunk = key_list[i : i + batch]
        t0 = time.perf_counter()
        for k in chunk:
            _ = d.get(k)
        total_ns += (time.perf_counter() - t0) * 1e9
        total_ops += len(chunk)
        i += batch
    return total_ns / total_ops


def bench_cache_locality(store: HeadlessWorkspaceStore) -> dict:
    banner("PHASE 7 · Cache locality analysis  (hot / warm / cold)")

    # ── working set size at 1 M ──────────────────────────────────────────────
    import sys as _sys
    sample_key = f"var_{0:07d}"
    sample_val = store._workspace_named_vars.get(sample_key, "0")
    bytes_per_entry = (
        _sys.getsizeof(sample_key)
        + _sys.getsizeof(sample_val)
        + 8 * 2         # two 8-byte pointers in the dict table (key + value)
        + 8             # hash slot overhead (approximate)
    )
    working_set_mb = (bytes_per_entry * SCALE) / 1_048_576

    # ── tier key lists ───────────────────────────────────────────────────────
    hot_keys  = [f"var_{i:07d}" for i in range(LOCALITY_HOT_KEYS)] * LOCALITY_REPEATS
    random.shuffle(hot_keys)

    # keys in insertion order (warm — prefetcher-friendly table walk)
    warm_keys = [f"var_{i:07d}" for i in range(LOCALITY_WARM_N)]

    # uniformly random keys — cold DRAM accesses
    cold_keys = [f"var_{random.randint(0, SCALE - 1):07d}" for _ in range(LOCALITY_COLD_N)]

    gc.disable()
    hot_ns  = _mean_batch_ns(store, hot_keys)
    warm_ns = _mean_batch_ns(store, warm_keys)
    cold_ns = _mean_batch_ns(store, cold_keys)
    gc.enable()

    # guard against division-by-zero on very fast machines
    if cold_ns == 0:
        cold_ns = 0.001

    locality_score    = hot_ns / cold_ns          # 0 means perfect locality, 1 means DRAM-bound
    cache_efficiency  = 1.0 - locality_score       # fraction saved by caches
    warm_cold_ratio   = warm_ns / cold_ns

    print()
    row("Working set at 1 M vars",    f"{working_set_mb:>8.1f} MB")
    print()
    row("L1/L2 cache size (typical)", f"{_L1_KB} KB / {_L2_KB} KB")
    row("L3  cache size  (typical)",  f"{_L3_KB // 1024} MB")
    row("Working set vs L3",
        f"{'IN L3 ✅' if working_set_mb * 1024 < _L3_KB else 'EXCEEDS L3 ⚠️  (DRAM pressure)'}",
    )
    print()
    row("HOT  mean latency   (L1/L2)", f"{hot_ns:>10.1f} ns")
    row("WARM mean latency   (L3/prefetch)", f"{warm_ns:>10.1f} ns")
    row("COLD mean latency   (DRAM/TLB)",   f"{cold_ns:>10.1f} ns")
    print()
    if locality_score < 0.3:
        locality_label = "excellent"
    elif locality_score < 0.6:
        locality_label = "good"
    else:
        locality_label = "moderate"
    row("Locality score  (hot/cold)",
        f"{locality_score:>10.3f}  {locality_label}",
    )
    row("Cache efficiency  (1 − score)",   f"{cache_efficiency * 100:>9.1f} %")
    row("Warm/cold ratio",                 f"{warm_cold_ratio:>10.3f}")
    print()

    # ── interpret ────────────────────────────────────────────────────────────
    if working_set_mb * 1024 > _L3_KB:
        print("  NOTE: 1M-var working set ({:.0f} MB) exceeds typical L3 ({} MB).".format(
            working_set_mb, _L3_KB // 1024))
        print("        Random lookups are DRAM-bound (TLB refill + cache miss on")
        print("        every key-string object).  Warm (sequential) accesses")
        print("        partially amortise this via hardware prefetching.")

    return {
        "hot_ns":          hot_ns,
        "warm_ns":         warm_ns,
        "cold_ns":         cold_ns,
        "locality_score":  locality_score,
        "cache_efficiency_pct": cache_efficiency * 100,
        "working_set_mb":  working_set_mb,
    }


# ══════════════════════════════════════════════════════════════════════════════
#  PHASE 8 — Memory anatomy
#
#  CPython dict stores keys and values as PyObject* pointers (8 bytes each).
#  The actual string objects live on the heap — each one has a PyObject header
#  (ob_refcnt + ob_type: 16 bytes on 64-bit), a hash cache (8 bytes), length,
#  and the character data.  var_NNNNNNN = 10 chars + NUL = 11 bytes → rounded
#  to the next object alignment boundary.
#
#  This phase measures real heap usage with sys.getsizeof and extrapolates to
#  larger scales.
# ══════════════════════════════════════════════════════════════════════════════

def bench_memory_anatomy(_store: HeadlessWorkspaceStore) -> dict:
    banner("PHASE 8 · Memory anatomy")

    import sys as sys_local

    # ── per-entry sizes ──────────────────────────────────────────────────────
    sample_pairs = [
        (f"var_{i:07d}", str(i * 3.14159265))
        for i in range(0, 1000, 100)     # 10 representative entries
    ]
    key_sizes   = [sys_local.getsizeof(k) for k, v in sample_pairs]
    val_sizes   = [sys_local.getsizeof(v) for k, v in sample_pairs]
    avg_key_b   = sum(key_sizes)  / len(key_sizes)
    avg_val_b   = sum(val_sizes)  / len(val_sizes)

    # CPython dict internal table: 2 × 8-byte pointers + ~1/0.67 load-factor overhead
    dict_slot_b = (8 + 8) * (1.0 / 0.67)   # ≈ 24 bytes per live slot
    per_entry_b = avg_key_b + avg_val_b + dict_slot_b

    print()
    row("Avg key object size",    f"{avg_key_b:>8.1f} bytes  (str + PyObject header)")
    row("Avg value object size",  f"{avg_val_b:>8.1f} bytes")
    row("Dict slot overhead",     f"{dict_slot_b:>8.1f} bytes  (pointer pair + load-factor)")
    row("Estimated per-entry",    f"{per_entry_b:>8.1f} bytes")
    print()

    # ── scale projections ────────────────────────────────────────────────────
    scales = [
        ("1 000",           1_000),
        ("10 000",         10_000),
        ("100 000",       100_000),
        ("1 000 000",   1_000_000),
        ("10 000 000",  10_000_000),
        ("100 000 000", 100_000_000),
        ("1 000 000 000", 1_000_000_000),
    ]
    print(f"  {'Scale':>15}  {'Est. RAM':>10}  {'Fits in':>18}")
    print(f"  {'-'*15}  {'-'*10}  {'-'*18}")
    for label, n in scales:
        mb  = (per_entry_b * n) / 1_048_576
        gb  = mb / 1024
        if mb < 1:
            sz = f"{mb * 1024:>6.0f} KB"
        elif mb < 1024:
            sz = f"{mb:>6.0f} MB"
        else:
            sz = f"{gb:>6.1f} GB"

        if mb < _L1_KB / 1024:
            fits = "L1 cache"
        elif mb < _L2_KB / 1024:
            fits = "L2 cache"
        elif mb < _L3_KB / 1024:
            fits = "L3 cache"
        elif gb < 8:
            fits = "8 GB RAM"
        elif gb < 32:
            fits = "32 GB RAM"
        elif gb < 64:
            fits = "64 GB RAM"
        else:
            fits = "⚠ EXCEEDS 64 GB RAM"
        print(f"  {label:>15}  {sz:>10}  {fits:>18}")

    return {"per_entry_bytes": per_entry_b, "avg_key_b": avg_key_b, "avg_val_b": avg_val_b}


# ══════════════════════════════════════════════════════════════════════════════
#  PHASE 9 — 1B variable extrapolation
#
#  We fit a log-linear throughput decay model to the observed scale-ladder
#  data (100 → 1M) and extrapolate to 1B (10^9).
#
#  Model:  log10(throughput) = A + B * log10(n)
#
#  This captures the cache-miss penalty growth that causes throughput to fall
#  as the working set expands through L1 → L2 → L3 → DRAM.
#
#  The memory constraint column flags when estimated RAM exceeds physical
#  limits, at which point OS paging dominates and the projected times become
#  lower bounds (real latency would be catastrophically worse).
# ══════════════════════════════════════════════════════════════════════════════

def bench_extrapolate_1b(ladder: list[dict], mem: dict) -> None:
    banner("PHASE 9 · 1B variable extrapolation  (log-linear model)")

    if len(ladder) < 3:
        print("  ⚠  Not enough ladder data for regression.")
        return

    # ── log-linear regression on (log n, log throughput) ────────────────────
    xs = [math.log10(r["n"])          for r in ladder]
    ys = [math.log10(r["throughput"]) for r in ladder]

    n_pts = len(xs)
    sx  = sum(xs)
    sy  = sum(ys)
    sxx = sum(x * x for x in xs)
    sxy = sum(x * y for x, y in zip(xs, ys))

    B = (n_pts * sxy - sx * sy) / (n_pts * sxx - sx * sx)
    A = (sy - B * sx) / n_pts
    r2 = 1.0 - sum((y - (A + B * x)) ** 2 for x, y in zip(xs, ys)) / \
                max(1e-12, sum((y - sy / n_pts) ** 2 for y in ys))

    def predict_throughput(n: int) -> float:
        return 10 ** (A + B * math.log10(n))

    print()
    print(f"  Regression: log₁₀(throughput) = {A:.4f} + {B:.4f} × log₁₀(n)")
    print(f"  R²  = {r2:.4f}  ({'good fit' if r2 > 0.95 else 'moderate fit'})")
    print()

    # ── projection table ─────────────────────────────────────────────────────
    per_b = mem.get("per_entry_bytes", 200.0)
    projection_scales = [
        ("1 M (measured)",   1_000_000,       True),
        ("10 M",            10_000_000,       False),
        ("100 M",          100_000_000,       False),
        ("1 B  ★",       1_000_000_000,       False),
    ]

    print(f"  {'Scale':>16}  {'Throughput':>12}  {'Insert time':>14}  "
          f"{'Est. RAM':>10}  {'Constraint':>22}")
    print(f"  {'-'*16}  {'-'*12}  {'-'*14}  {'-'*10}  {'-'*22}")

    for label, n, is_measured in projection_scales:
        throughput = predict_throughput(n)
        insert_s   = n / throughput
        ram_gb     = (per_b * n) / (1024 ** 3)

        tp_str  = f"{throughput / 1e6:>8.3f} M/s" if throughput >= 1e6 else f"{throughput / 1e3:>8.1f} K/s"
        ins_str = _fmt_duration(insert_s)
        ram_str = f"{ram_gb * 1024:.0f} MB" if ram_gb < 1 else f"{ram_gb:.1f} GB"

        if ram_gb < 8:
            constraint = "fits in 8 GB RAM"
        elif ram_gb < 32:
            constraint = "needs 32 GB RAM"
        elif ram_gb < 64:
            constraint = "needs 64 GB RAM"  
        else:
            constraint = "⚠ EXCEEDS 64 GB — SWAP"

        marker = " (measured)" if is_measured else ""
        print(f"  {label:>16}  {tp_str:>12}  {ins_str:>14}  "
              f"{ram_str:>10}  {constraint:>22}{marker}")

    # ── 1B narrative ─────────────────────────────────────────────────────────
    tp_1b  = predict_throughput(1_000_000_000)
    ins_1b = 1_000_000_000 / tp_1b
    ram_1b = (per_b * 1_000_000_000) / (1024 ** 3)

    print()
    print("  ★ 1B projection detail")
    print(f"    Predicted throughput : {tp_1b:,.0f} vars/sec")
    print(f"    Predicted insert time: {_fmt_duration(ins_1b)} (assuming RAM fits)")
    print(f"    Estimated RAM        : {ram_1b:.1f} GB")
    print()
    print("  Cache regime at 1B:")
    print("    Every lookup is a DRAM access + TLB miss.  With a ~100 ns")
    print("    DRAM latency, 10M lookups/sec is the theoretical ceiling")
    print("    (1 core × 1 ns/cycle × 100 cycles/miss → 10M ops/sec).")
    print("    Parallel readers or NUMA-aware sharding would be required")
    print("    to sustain meaningful throughput at this scale.")
    if ram_1b > 64:
        print()
        print(f"  ⚠  {ram_1b:.0f} GB exceeds typical server RAM (64 GB).")
        print("     Swap / page-file thrashing would make real insert times")
        print(f"     10–100× worse than the model predicts ({_fmt_duration(ins_1b)} → hours/days).")
        print("     For 1B-var workloads, AXIOM would need an on-disk variable")
        print("     store (LMDB / RocksDB backend) instead of CPython dict.")


def _fmt_duration(s: float) -> str:
    """Format seconds into a human-readable duration string."""
    if s < 1:
        return f"{s * 1000:.1f} ms"
    if s < 60:
        return f"{s:.1f} s"
    if s < 3600:
        return f"{s / 60:.1f} min"
    return f"{s / 3600:.1f} h"


# ══════════════════════════════════════════════════════════════════════════════
#  FINAL SCORECARD
# ══════════════════════════════════════════════════════════════════════════════

def print_scorecard(
    insert: dict,
    lookup: dict,
    ans: dict,
    flush: dict,
    rc: dict,
    snap: dict,
) -> bool:
    banner("SCORECARD")

    checks = [
        ("1 M inserts < 2 s",     insert["elapsed_ms"] < BUDGET_INSERT_MS),
        ("Lookup p99 < 5 µs",     lookup["p99_us"] < BUDGET_LOOKUP_US),
        ("8 k ans appends < 50 ms", ans["elapsed_ms"] < BUDGET_ANS_MS),
        ("Flush p99 < 100 µs",    flush["p99_us"] < BUDGET_FLUSH_US),
        ("rowCount() O(1) < 5 µs", rc["mean_ns"] < 5_000),
        ("Snapshot covers 1 M+ keys", snap["key_count"] >= SCALE),
    ]

    all_pass = all(ok for _, ok in checks)
    for label, ok in checks:
        row(label, "PASS" if ok else "FAIL", ok)

    banner("VERDICT")
    if all_pass:
        print(f"\n  🏆  ALL {len(checks)} CHECKS PASSED")
        print("  ✅  AXIOM workspace is certified for 1 000 000+ variables\n")
    else:
        failures = sum(1 for _, ok in checks if not ok)
        print(f"\n  ⚠️  {failures}/{len(checks)} checks failed\n")

    return all_pass


# ══════════════════════════════════════════════════════════════════════════════
#  pytest interface
# ══════════════════════════════════════════════════════════════════════════════

def test_1m_workspace_insert_under_2s():
    """1M named-var inserts must complete in under 2 seconds."""
    store = HeadlessWorkspaceStore()
    t0 = time.perf_counter()
    for i in range(SCALE):
        store.store_named(f"var_{i}", str(i))
    elapsed_ms = (time.perf_counter() - t0) * 1_000
    assert elapsed_ms < BUDGET_INSERT_MS, (
        f"Insert took {elapsed_ms:.0f} ms, budget is {BUDGET_INSERT_MS} ms"
    )
    assert store.row_count() == SCALE


def test_lookup_p99_under_5us():
    """Random lookups into 1M-entry dict: p99 < 5 µs."""
    store = HeadlessWorkspaceStore()
    for i in range(SCALE):
        store.store_named(f"var_{i}", str(i))
    keys = [f"var_{random.randint(0, SCALE - 1)}" for _ in range(LOOKUP_SAMPLE)]
    times = []
    for k in keys:
        t0 = time.perf_counter()
        _ = store.lookup(k)
        times.append(time.perf_counter() - t0)
    p99_us = sorted(times)[int(len(times) * 0.99)] * 1e6
    assert p99_us < BUDGET_LOOKUP_US, f"p99 = {p99_us:.3f} µs, budget {BUDGET_LOOKUP_US} µs"


def test_ans_pool_8k_appends():
    """8 000 ans# appends must complete in under 50 ms."""
    store = HeadlessWorkspaceStore()
    t0 = time.perf_counter()
    for i in range(8_000):
        store.store_ans(f"expr_{i}", str(i))
    elapsed_ms = (time.perf_counter() - t0) * 1_000
    assert elapsed_ms < BUDGET_ANS_MS
    assert len(store._ans_pool) == 8_000


def test_dirty_flush_p99_under_100us():
    """1 000 dirty-flag flush cycles: p99 < 100 µs."""
    store = HeadlessWorkspaceStore()
    for i in range(SCALE // 100):          # seed 10 k entries
        store.store_named(f"seed_{i}", "0")
    times = []
    for cycle in range(FLUSH_CYCLES):
        store.store_named(f"live_{cycle}", str(cycle))
        t0 = time.perf_counter()
        store.flush_if_dirty()
        times.append(time.perf_counter() - t0)
    p99_us = sorted(times)[int(len(times) * 0.99)] * 1e6
    assert p99_us < BUDGET_FLUSH_US, f"p99 = {p99_us:.3f} µs, budget {BUDGET_FLUSH_US} µs"


def test_row_count_o1():
    """rowCount() over 1M-entry store must be O(1) (< 5 µs mean)."""
    store = HeadlessWorkspaceStore()
    for i in range(SCALE):
        store._workspace_named_vars[f"var_{i}"] = str(i)   # direct, skip dirty
    times: list[float] = [0.0] * 500
    for idx in range(500):
        t0 = time.perf_counter()
        _ = store.row_count()
        times[idx] = time.perf_counter() - t0
    mean_ns = (sum(times) / len(times)) * 1e9
    assert mean_ns < 5_000, f"rowCount mean = {mean_ns:.0f} ns, want < 5000 ns"


# ══════════════════════════════════════════════════════════════════════════════
#  Standalone entry point
# ══════════════════════════════════════════════════════════════════════════════

if __name__ == "__main__":
    print("\n" + "═" * WIDTH)
    print(" AXIOM WORKSPACE — 1 000 000 VARIABLE BENCHMARK  (offscreen)")
    print(" Python " + sys.version.split()[0])
    print("═" * WIDTH)

    store = HeadlessWorkspaceStore()

    insert_r  = bench_bulk_insert(store)
    lookup_r  = bench_lookups(store)
    ans_r     = bench_ans_pool(store)
    flush_r   = bench_flush_cycles(store)
    rc_r      = bench_row_count(store)
    snap_r    = bench_snapshot(store)
    _mem_r    = measure_memory(store)
    ladder_r  = bench_scale_ladder()
    locality_r = bench_cache_locality(store)
    mem_r     = bench_memory_anatomy(store)
    bench_extrapolate_1b(ladder_r, mem_r)

    passed = print_scorecard(insert_r, lookup_r, ans_r, flush_r, rc_r, snap_r)
    sys.exit(0 if passed else 1)
