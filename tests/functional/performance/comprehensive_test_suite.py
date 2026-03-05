"""
🧪 COMPREHENSIVE AXIOM TEST SUITE
All-in-one testing for basic ops, math functions, plotting, and edge cases
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

print("\n" + "="*80)
print("🚀 COMPREHENSIVE AXIOM ENGINE TEST SUITE v3.0")
print("="*80)

# Initialize engine once
engine = CppEngineInterface(resolve_axiom_exe())
persistent_mode = engine.process is not None and engine.process.poll() is None
print(f"\n✅ Engine Mode: {'🟢 PERSISTENT' if persistent_mode else '🔵 FALLBACK'}")
print(f"{'='*80}\n")

# Test categories
all_results = []
category_times = {}

def run_test(category, name, expression, expected_contains=None):
    """Run a single test and record results"""
    start = time.time()
    result = engine.execute_command(expression)
    elapsed_ms = (time.time() - start) * 1000
    
    # Track category times
    if category not in category_times:
        category_times[category] = []
    category_times[category].append(elapsed_ms)
    
    success = result["success"]
    if expected_contains and success:
        success = expected_contains in str(result.get("result", ""))
    
    if elapsed_ms < 5:
        speed = "⚡ SENNA"
    elif elapsed_ms < 20:
        speed = "🏎️  F1"
    elif elapsed_ms < 50:
        speed = "🚗 NORMAL"
    else:
        speed = "🐌 SLOW"
    status = "✅" if success else "❌"
    
    all_results.append({
        "category": category,
        "name": name,
        "expression": expression,
        "time_ms": elapsed_ms,
        "success": success,
        "speed": speed,
        "status": status
    })
    
    return elapsed_ms, success

# ============================================================================
# 1. BASIC ARITHMETIC
# ============================================================================
print("1️⃣  BASIC ARITHMETIC TESTS")
print("-" * 80)
print(f"{'Test':<25} {'Expression':<30} {'Time':<12} {'Status'}")
print("-" * 80)

tests_basic = [
    ("Addition", "2+2"),
    ("Subtraction", "10-3"),
    ("Multiplication", "5*7"),
    ("Division", "20/4"),
    ("Power", "2^10"),
    ("Complex expr", "3+4*5-2"),
    ("Nested power", "2^3^2"),
    ("Decimal", "3.14 * 2"),
]

for name, expr in tests_basic:
    t, ok = run_test("Basic", name, expr)
    result_str = "✅" if ok else "❌"
    print(f"{name:<25} {expr:<30} {t:>10.2f}ms  {result_str}")

# ============================================================================
# 2. MATHEMATICAL FUNCTIONS
# ============================================================================
print("\n2️⃣  MATHEMATICAL FUNCTIONS")
print("-" * 80)
print(f"{'Test':<25} {'Expression':<30} {'Time':<12} {'Status'}")
print("-" * 80)

tests_math = [
    ("Sine", "sin(90)"),
    ("Cosine", "cos(0)"),
    ("Tangent", "tan(45)"),
    ("Square root", "sqrt(144)"),
    ("Cube root", "cbrt(8)"),
    ("Absolute value", "abs(-5)"),
    ("Natural log", "ln(10)"),
    ("Exponential", "exp(1)"),
    ("Log base 2", "log2(8)"),
    ("Factorial", "factorial(5)"),
]

for name, expr in tests_math:
    t, ok = run_test("Math", name, expr)
    result_str = "✅" if ok else "❌"
    print(f"{name:<25} {expr:<30} {t:>10.2f}ms  {result_str}")

# ============================================================================
# 3. FUNCTION COMPOSITIONS
# ============================================================================
print("\n3️⃣  FUNCTION COMPOSITIONS")
print("-" * 80)
print(f"{'Test':<25} {'Expression':<30} {'Time':<12} {'Status'}")
print("-" * 80)

tests_composition = [
    ("Sin of cos", "sin(cos(0))"),
    ("Log of exp", "ln(exp(2))"),
    ("Sqrt of power", "sqrt(9*4)"),
    ("Nested trig", "sin(cos(tan(0)))"),
    ("Double exp", "exp(exp(1))"),
]

for name, expr in tests_composition:
    t, ok = run_test("Composition", name, expr)
    result_str = "✅" if ok else "❌"
    print(f"{name:<25} {expr:<30} {t:>10.2f}ms  {result_str}")

# ============================================================================
# 4. MODULO & SPECIAL OPERATIONS
# ============================================================================
print("\n4️⃣  MODULO & SPECIAL OPERATIONS")
print("-" * 80)
print(f"{'Test':<25} {'Expression':<30} {'Time':<12} {'Status'}")
print("-" * 80)

tests_special = [
    ("Modulo function", "mod(17, 5)"),
    ("Modulo large", "mod(938247290474946, 342423423)"),
    ("GCD", "gcd(48, 18)"),
    ("LCM", "lcm(12, 18)"),
    ("Max function", "max(3, 7, 2)"),
    ("Min function", "min(3, 7, 2)"),
]

for name, expr in tests_special:
    t, ok = run_test("Special", name, expr)
    result_str = "✅" if ok else "❌"
    print(f"{name:<25} {expr:<30} {t:>10.2f}ms  {result_str}")

# ============================================================================
# 5. HYPERBOLIC & INVERSE FUNCTIONS
# ============================================================================
print("\n5️⃣  HYPERBOLIC & INVERSE FUNCTIONS")
print("-" * 80)
print(f"{'Test':<25} {'Expression':<30} {'Time':<12} {'Status'}")
print("-" * 80)

tests_advanced = [
    ("Sinh", "sinh(1)"),
    ("Cosh", "cosh(0)"),
    ("Tanh", "tanh(1)"),
    ("ArcSin", "asin(0.5)"),
    ("ArcCos", "acos(0.5)"),
    ("ArcTan", "atan(1)"),
]

for name, expr in tests_advanced:
    t, ok = run_test("Advanced", name, expr)
    result_str = "✅" if ok else "❌"
    print(f"{name:<25} {expr:<30} {t:>10.2f}ms  {result_str}")

# ============================================================================
# 6. SIMPLE PLOTTING EXPRESSIONS (SKIP - requires variable definitions)
# ============================================================================
print("\n6️⃣  SIMPLE PLOTTING EXPRESSIONS (SKIPPED - requires :plot command)")
print("-" * 80)
print("Note: Plot expressions with variables like 'sin(x)' require the :plot command")
print("      and are not valid as standalone algebraic expressions.")
print("-" * 80)

# ============================================================================
# 7. EDGE CASES & STRESS TESTS
# ============================================================================
print("\n7️⃣  EDGE CASES & STRESS TESTS")
print("-" * 80)
print(f"{'Test':<25} {'Expression':<30} {'Time':<12} {'Status'}")
print("-" * 80)

tests_edge = [
    ("Zero", "0"),
    ("Large number", "1000000000"),
    ("Small decimal", "0.0001"),
    ("Very small exp", "exp(-10)"),
    ("Pi approximation", "3.14159"),
    ("E approximation", "2.71828"),
]

for name, expr in tests_edge:
    t, ok = run_test("Edge Cases", name, expr)
    result_str = "✅" if ok else "❌"
    print(f"{name:<25} {expr:<30} {t:>10.2f}ms  {result_str}")

# ============================================================================
# SUMMARY & STATISTICS
# ============================================================================
print("\n" + "="*80)
print("📊 TEST SUMMARY & PERFORMANCE ANALYSIS")
print("="*80)

total_tests = len(all_results)
passed_tests = sum(1 for r in all_results if r["success"])
failed_tests = total_tests - passed_tests

print(f"\n✅ Tests Passed: {passed_tests}/{total_tests}")
print(f"❌ Tests Failed: {failed_tests}/{total_tests}")
print(f"📈 Pass Rate: {(passed_tests/total_tests)*100:.1f}%")

print("\n⏱️  PERFORMANCE BY CATEGORY:")
print("-" * 80)
print(f"{'Category':<20} {'Count':<8} {'Total (ms)':<12} {'Avg (ms)':<12} {'Speed'}")
print("-" * 80)

for category in sorted(category_times.keys()):
    times = category_times[category]
    count = len(times)
    total = sum(times)
    avg = total / count if count > 0 else 0
    if avg < 1:
        speed = "⚡ SENNA"
    elif avg < 5:
        speed = "🏎️  F1"
    elif avg < 20:
        speed = "🚗 NORMAL"
    else:
        speed = "🐌 SLOW"
    print(f"{category:<20} {count:<8} {total:>10.2f}ms  {avg:>10.2f}ms  {speed}")

all_times = [r["time_ms"] for r in all_results]
overall_total = sum(all_times)
overall_avg = overall_total / len(all_times)

print("-" * 80)
print(f"{'OVERALL':<20} {len(all_times):<8} {overall_total:>10.2f}ms  {overall_avg:>10.2f}ms", end="")

if overall_avg < 1:
    print("  🏆 EXCELLENT")
elif overall_avg < 5:
    print("  🥇 VERY GOOD")
elif overall_avg < 10:
    print("  🥈 GOOD")
elif overall_avg < 20:
    print("  🥉 ACCEPTABLE")
else:
    print("  ⚠️ SLOW")

# ============================================================================
# DETAILED RESULTS TABLE
# ============================================================================
print("\n" + "="*80)
print("📋 DETAILED RESULTS")
print("="*80)
print(f"{'Category':<15} {'Test':<20} {'Time (ms)':<12} {'Result'}")
print("-" * 80)

for result in all_results:
    status_icon = "✅" if result["success"] else "❌"
    print(f"{result['category']:<15} {result['name']:<20} {result['time_ms']:>10.2f}ms  {status_icon}")

# ============================================================================
# FINAL METRICS
# ============================================================================
print("\n" + "="*80)
print("🎯 FINAL METRICS")
print("="*80)
print(f"\n📱 Persistent Mode: {'🟢 ACTIVE' if persistent_mode else '🔴 INACTIVE'}")
print(f"⚡ Average Response Time: {overall_avg:.2f}ms")
print(f"🚀 Throughput: {1000/overall_avg:.1f} commands/second")
print(f"✅ Reliability: {(passed_tests/total_tests)*100:.1f}%")

if overall_avg < 5 and persistent_mode and passed_tests == total_tests:
    print("\n🏆 STATUS: ✨ HYPER SENNA SPEED - PERFECT OPTIMIZATION! ✨")
elif overall_avg < 10 and persistent_mode and passed_tests > (total_tests * 0.95):
    print("\n🥇 STATUS: Excellent performance with persistent subprocess!")
elif overall_avg < 20 and persistent_mode:
    print("\n🥈 STATUS: Good performance, room for improvement")
else:
    print("\n⚠️ STATUS: Performance below expectations")

# Cleanup
engine.close()
print("\n✅ All tests complete!")
print("="*80 + "\n")
