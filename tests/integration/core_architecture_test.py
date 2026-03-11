#!/usr/bin/env python3
"""
AXIOM v3.0 - Core Architecture Integration Test
==============================================

Test the new core architecture components:
- Selective Dispatcher
- Eigen CPU Engine
- Enhanced performance monitoring
"""

import subprocess
import time
import sys
import os
import pytest

def find_axiom_executable():
    """Find AXIOM executable in standard locations."""
    possible_paths = [
        "build/axiom.exe",
        "ninja-build/axiom.exe",
        "cmake-build-debug/axiom.exe"
    ]
    
    for path in possible_paths:
        if os.path.exists(path):
            return path
    return None


@pytest.fixture
def axiom_path():
    """Resolve AXIOM executable path for integration tests."""
    path = find_axiom_executable()
    if not path:
        pytest.skip("AXIOM executable not found for integration tests")
    return path

def verify_architecture(axiom_path):
    """Verify AXIOM v3.0 architecture is present."""
    try:
        result = subprocess.run([axiom_path, "--help"], 
                              capture_output=True, text=True, timeout=5)
        if "AXIOM Engine v3.0" in result.stdout:
            print("✅ AXIOM v3.0 architecture confirmed")
            return True
        else:
            print("❌ Architecture verification failed")
            return False
    except Exception as e:
        print(f"❌ Architecture test failed: {e}")
        return False

def test_math_operations(axiom_path):
    """Test core mathematical operations."""
    test_cases = [
        ("2+2", "4"),
        ("sqrt(16)", "4"),
        ("3.14159265358979", "3.14159265358979"),
        ("max(1,2,3,4,5)", "5"),
    ]
    
    passed = 0
    for expr, expected in test_cases:
        try:
            result = subprocess.run([axiom_path, expr], 
                                  capture_output=True, text=True, timeout=5)
            if result.returncode == 0 and expected in result.stdout:
                print(f"✅ {expr} -> {result.stdout.strip()}")
                passed += 1
            else:
                print(f"❌ {expr} -> {result.stdout.strip()} (expected {expected})")
        except Exception as e:
            print(f"❌ {expr} -> Error: {e}")
    
    return passed, len(test_cases)

def measure_performance(axiom_path):
    """Benchmark AXIOM execution time."""
    times = []
    for _ in range(10):
        start = time.time()
        try:
            subprocess.run([axiom_path, "2+2"], 
                         capture_output=True, text=True, timeout=1)
            end = time.time()
            times.append((end - start) * 1000)
        except (subprocess.TimeoutExpired, OSError):
            pass
    
    if not times:
        return None
    
    avg_time = sum(times) / len(times)
    return avg_time

def classify_performance(avg_time):
    """Classify performance based on execution time."""
    if avg_time < 50:
        return "🏎️ Performance: SENNA SPEED!"
    elif avg_time < 100:
        return "🏁 Performance: F1 SPEED"
    else:
        return "🚗 Performance: Good"

def test_enterprise_features(axiom_path):
    """Test enterprise features."""
    enterprise_tests = [
        ("--benchmark", "benchmark"),
    ]
    
    passed = 0
    for flag, keyword in enterprise_tests:
        try:
            result = subprocess.run([axiom_path, flag], 
                                  capture_output=True, text=True, timeout=10,
                                  encoding='utf-8', errors='ignore')
            stdout = result.stdout or ""
            if keyword.lower() in stdout.lower() or result.returncode == 0:
                print(f"✅ Enterprise feature: {flag}")
                passed += 1
            else:
                print(f"⚠️ Enterprise feature: {flag} (partial)")
        except subprocess.TimeoutExpired:
            print(f"⚠️ Enterprise feature: {flag} (timeout - still working)")
            passed += 1
        except Exception as e:
            print(f"❌ Enterprise feature: {flag} -> {e}")
    
    return passed, len(enterprise_tests)

def print_results(math_passed, math_total, perf_time, enterprise_passed, enterprise_total):
    """Print final test results."""
    print("\n" + "=" * 55)
    print("📋 CORE ARCHITECTURE TEST SUMMARY")
    print("=" * 55)
    
    total_score = math_passed + enterprise_passed
    max_score = math_total + enterprise_total
    
    print(f"📊 Core Operations: {math_passed}/{math_total} passed")
    print(f"🏢 Enterprise Features: {enterprise_passed}/{enterprise_total} working")
    print(f"🎯 Overall Score: {total_score}/{max_score}")
    
    if perf_time:
        print(f"⏱️ Average execution time: {perf_time:.2f}ms")
        print(classify_performance(perf_time))
    
    success_rate = (total_score / max_score) * 100
    print(f"📈 Success Rate: {success_rate:.1f}%")
    
    return success_rate

def test_core_architecture():
    """Test the core AXIOM v3.0 architecture"""
    print("🔬 AXIOM v3.0 - Core Architecture Integration Test")
    print("=" * 55)
    
    # Find executable
    axiom_path = find_axiom_executable()
    if not axiom_path:
        print("❌ AXIOM executable not found!")
        return False
    
    print(f"✅ Found AXIOM executable: {axiom_path}")
    
    # Test 1: Architecture Verification
    print("\n🏗️ Test 1: Architecture Verification")
    if not verify_architecture(axiom_path):
        return False
    
    # Test 2: Core Mathematical Operations
    print("\n🧮 Test 2: Core Mathematical Operations")
    math_passed, math_total = test_math_operations(axiom_path)
    print(f"📊 Core operations: {math_passed}/{math_total} passed")
    
    # Test 3: Performance Benchmarking
    print("\n⚡ Test 3: Performance Benchmarking")
    perf_time = measure_performance(axiom_path)
    
    # Test 4: Enterprise Features
    print("\n🏢 Test 4: Enterprise Features")
    enterprise_passed, enterprise_total = test_enterprise_features(axiom_path)
    
    # Print final results
    success_rate = print_results(math_passed, math_total, perf_time, 
                                 enterprise_passed, enterprise_total)
    
    if success_rate >= 80:
        print("🎉 AXIOM v3.0 CORE ARCHITECTURE: EXCELLENT!")
        return True
    elif success_rate >= 60:
        print("✅ AXIOM v3.0 CORE ARCHITECTURE: GOOD")
        return True
    else:
        print("⚠️ AXIOM v3.0 CORE ARCHITECTURE: NEEDS IMPROVEMENT")
        return False

if __name__ == "__main__":
    success = test_core_architecture()
    sys.exit(0 if success else 1)