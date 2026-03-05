"""
🧪 Quick Performance Test - Verify Persistent Subprocess & Optimizations
"""
import sys
import time
from pathlib import Path

repo_root = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(repo_root / "gui" / "python"))
from gui_helpers import CppEngineInterface


def resolve_axiom_exe() -> str:
    candidates = [
        repo_root / "build" / "axiom.exe",
        repo_root / "ninja-build" / "axiom.exe",
    ]
    for candidate in candidates:
        if candidate.exists():
            return str(candidate)
    raise FileNotFoundError("axiom.exe not found in build/ or ninja-build/")

print("=" * 70)
print("🚀 AXIOM Persistent Subprocess Performance Test")
print("=" * 70)

# Initialize engine
engine = CppEngineInterface(resolve_axiom_exe())
print(f"\n✅ Engine initialized: {'Persistent mode' if engine.process else 'Fallback mode'}\n")

# Test cases - progressively more complex
tests = [
    ("Basic", "2+2", None),
    ("Arithmetic", "5*7-3", None),
    ("Power", "2^10", None),
    ("Trig", "sin(90)", None),
    ("Function", "sqrt(144)", None),
    ("Modulo (function)", "mod(17, 5)", None),
    ("Composition", "sin(cos(0))", None),
    ("Moderate", "x^2 + 2*x + 1", {"x": 5}),
    ("Exponential", "exp(1)", None),
    ("Logarithm", "ln(10)", None),
]

total_time = 0
results = []

print("Test Results:")
print("-" * 70)
print(f"{'#':<3} {'Test Name':<15} {'Expression':<25} {'Time (ms)':<12} {'Speed':<10}")
print("-" * 70)

for i, (name, expr, context) in enumerate(tests, 1):
    # Replace x if needed
    if context:
        # For now just test as-is, engine will use default x
        expr_to_run = expr.replace("x", "5")
    else:
        expr_to_run = expr
    
    start = time.time()
    result = engine.execute_command(expr_to_run)
    elapsed_ms = (time.time() - start) * 1000
    total_time += elapsed_ms
    
    if result["success"]:
        if elapsed_ms < 5:
            speed = "⚡ SENNA"
        elif elapsed_ms < 20:
            speed = "🏎️  F1"
        else:
            speed = "🚗 NORMAL"
        status = "✅"
    else:
        speed = "❌ ERROR"
        status = "⚠️ "
    
    print(f"{i:<3} {name:<15} {expr_to_run:<25} {elapsed_ms:>10.2f}ms  {speed:<10}")
    results.append((name, expr_to_run, elapsed_ms, result["success"]))

print("-" * 70)
avg_time = total_time / len(tests)
print("\n📊 Statistics:")
print(f"   Total commands: {len(tests)}")
print(f"   Total time: {total_time:.1f}ms")
print(f"   Average: {avg_time:.2f}ms per command")
print(f"   Persistent mode: {engine.process is not None and engine.process.poll() is None}")

# Performance rating
if avg_time < 5:
    rating = "🏆 EXCELLENT - Hyper-optimized!"
elif avg_time < 10:
    rating = "🥇 VERY GOOD - Blazing fast!"
elif avg_time < 20:
    rating = "🥈 GOOD - Fast enough"
elif avg_time < 50:
    rating = "🥉 ACCEPTABLE - Could be faster"
else:
    rating = "⚠️ SLOW - Optimization needed"

print(f"\n{rating}\n")

# Cleanup
engine.close()
print("✅ Test complete!")
print("=" * 70)
