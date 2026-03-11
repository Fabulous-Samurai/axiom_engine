# Giga Test Suite

## Overview

The **Giga Test Suite** is a monolithic, production-grade test suite for AXIOM Engine v3.0 that validates all core components with comprehensive coverage.

## Features

###  Resilient Test Runner

- **Continues on failure**: If a test fails or throws an exception, execution continues to the next test
- **Try-catch wrapping**: Every test is wrapped in exception handling
- **No abort**: Process never terminates early - all tests always run

###  Visual Reporting

- **ANSI color codes**: 
  -  Green = PASS
  -  Red = FAIL  
  -  Cyan = Section headers and INFO
- **Progress headers**: Clear section separation (e.g., "=== TESTING SYMBOLIC ENGINE ===")
- **Detailed failure messages**: Shows exception messages and failed test names

###  Performance Metrics

- **Timing**: Each test section is timed with millisecond precision
- **Summary table**: Final report shows:

  ```
  [ SUMMARY ] Total: 50 | Passed: 48 | Failed: 2 | Time: 15ms
  ```

###  Comprehensive Coverage

#### 1. Algebraic Parser (10 tests)

- Basic arithmetic operations
- Trigonometric functions (sin, cos, tan)
- Logarithms (log, ln)
- Complex expressions combining multiple operations
- Context variables (setting x=10, etc.)
- AXIOM::Number compatibility
- Power operations
- Square root calculations
- Division operations
- Nested operations with parentheses

#### 2. Linear System Parser (5 tests)

- ParseMatrixString for 2x2 and 3x3 matrices
- Solve 2x2 linear systems
- Identity matrix systems
- 3x3 system solving
- Determinant calculations

#### 3. Statistics Engine (8 tests)

- Mean calculation
- Standard deviation
- Median
- Variance
- Linear regression
- Correlation coefficient
- Mode
- Percentile calculations

#### 4. Symbolic Engine (6 tests)

- Expand expressions: (x+1)^2  x^2 + 2x + 1
- Simplify expressions
- Symbolic integration
- Symbolic differentiation
- Factorization
- Variable substitution

#### 5. Unit Manager (7 tests)

- Length conversions (km to m, m to cm)
- Mass conversions (kg to g)
- Unit compatibility checks
- Incompatibility detection
- Temperature conversions (C to F, F to C)

#### 6. Plot Engine (5 tests)

- Plot sin(x)
- Plot linear functions
- Plot quadratic functions
- Plot data points
- Generate histograms

#### 7. Eigen Engine (7 tests) - *Conditional*

Only runs if ENABLE_EIGEN is defined:

- Matrix creation
- Matrix multiplication
- Matrix inverse
- Matrix transpose
- Determinant calculation
- Matrix addition
- Solve linear systems (Ax=b)

#### 8. Dynamic Calc Integration (3 tests)

- Algebraic mode evaluation
- Mode switching
- Complex expressions

## Building & Running

### Build with CMake

```bash
cd axiom_engine
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja giga_test_suite
```

### Run Tests

```bash

# From build directory

./giga_test_suite

# Or from scripts directory

cd ../scripts
./ninja_build.bat  # Windows
./ninja_build.sh   # Unix/Linux
```

### Expected Output

```

                                                               
           AXIOM ENGINE v3.0 - GIGA TEST SUITE                 
                                                               
        Production-Grade Comprehensive Validation              
                                                               


========================================
  ALGEBRAIC PARSER TESTS
========================================
  [TEST] Basic addition ... PASS
  [TEST] sin(30) calculation ... PASS
  [TEST] log(100) calculation ... PASS
  ...
Section completed in 12ms

========================================
           TEST SUMMARY
========================================
Total:   50
Passed:  50
Failed:  0
Time:    125ms

 ALL TESTS PASSED!
```

## Exit Codes

- **0**: All tests passed (CI/CD success)
- **1**: One or more tests failed (CI/CD failure)

## CI/CD Integration

The test suite is designed for continuous integration:

```yaml

# .github/workflows/test.yml

- name: Run Giga Test Suite

  run: |
    cd build
    ./giga_test_suite
```

The exit code will cause CI to fail if any test fails.

## Configuration Options

### CMake Options

```cmake

# Disable giga test suite build

cmake -DBUILD_GIGA_TESTS=OFF ..

# Enable Eigen tests

cmake -DENABLE_EIGEN=ON ..
```

### Test Tuning

Edit `tests/giga_test_suite.cpp` to adjust:

- Epsilon values for floating-point comparisons: `approx_equal(a, b, 1e-6)`
- Test timeouts
- Additional test cases

## Architecture

### TestRunner Class

```cpp
class TestRunner {
    // Tracks: total_tests, passed_tests, failed_tests
    // Methods:
    // - StartSection(name): Begin a test section
    // - RunTest(name, func): Execute a test with exception handling
    // - EndSection(): Print section timing
    // - PrintSummary(): Final report
    // - GetExitCode(): Returns 0 or 1
};
```

### Test Function Pattern

```cpp
runner.RunTest("Test Name", [&]() {
    // Test implementation
    auto result = engine.DoSomething();
    return result.HasResult() && approx_equal(*result.GetDouble(), expected);
});
```

## Troubleshooting

### Eigen Tests Not Running

If you see:
```
[INFO] Eigen tests skipped (ENABLE_EIGEN not defined)
```

Solution:
```bash
cmake -DENABLE_EIGEN=ON ..
```

### Link Errors

Ensure all required sources are linked in CMakeLists.txt:

- algebraic_parser.cpp
- linear_system_parser.cpp
- statistics_engine.cpp
- symbolic_engine.cpp
- unit_manager.cpp
- plot_engine.cpp
- dynamic_calc.cpp
- string_helpers.cpp

### Compilation Errors

Check C++20 compatibility:
```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

## Extending the Test Suite

### Adding a New Test

```cpp
runner.RunTest("My New Test", [&]() {
    // Your test logic
    auto result = some_engine.Method();
    return result.HasResult() && /* validation */;
});
```

### Adding a New Section

```cpp
void TestMyNewEngine(TestRunner& runner) {
    runner.StartSection("MY NEW ENGINE TESTS");
    
    MyEngine engine;
    
    runner.RunTest("Test 1", [&]() { /* ... */ });
    runner.RunTest("Test 2", [&]() { /* ... */ });
    
    runner.EndSection();
}

// In main():
TestMyNewEngine(runner);
```

## Performance Benchmarks

Typical execution times (Release build):

- **Algebraic Parser**: 10-15ms
- **Linear System**: 5-8ms
- **Statistics Engine**: 3-5ms
- **Symbolic Engine**: 8-12ms
- **Unit Manager**: 2-3ms
- **Plot Engine**: 15-20ms
- **Eigen Engine**: 10-15ms
- **Total**: 50-100ms

## License

Same as AXIOM Engine - see LICENSE file.


