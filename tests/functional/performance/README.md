# Performance Tests

High-performance benchmarking and validation tests for AXIOM engine.

## Test Files

### comprehensive_test_suite.py

Complete test coverage across all mathematical operations.

### Categories (41 tests total):

1. Basic Arithmetic (8 tests)
2. Mathematical Functions (10 tests)
3. Function Compositions (5 tests)
4. Special Operations (6 tests)
5. Advanced Functions (6 tests)
6. Edge Cases (6 tests)

### Performance Metrics:

- Per-test timing
- Category averages
- Overall throughput
- Speed ratings

### Usage:

```bash
python comprehensive_test_suite.py
```

### Expected Output:

-  ~41/41 tests passing
-  <1ms average response time
-  EXCELLENT rating

### quick_perf_test.py

Fast performance check with 10 critical operations.

### Test Coverage:

- Basic: `2+2`, `5*7-3`
- Power: `2^10`
- Trigonometry: `sin(90)`
- Functions: `sqrt(144)`, `exp(1)`, `ln(10)`
- Modulo: `mod(17, 5)`
- Composition: `sin(cos(0))`

### Usage:

```bash
python quick_perf_test.py
```

### Expected Output:

-  10/10 tests passing
- 0.00ms average
-  SENNA speed

## Performance Baselines

| Category | Target | Excellent | Acceptable | Slow |
|----------|--------|-----------|------------|------|
| Single Op | <1ms | <5ms | <20ms | >50ms |
| Complex | <5ms | <20ms | <50ms | >100ms |
| Throughput | 1000/s | 200/s | 50/s | <50/s |

## Running Benchmarks

### Full Benchmark Suite

```bash

# Run both tests and compare

python comprehensive_test_suite.py
python quick_perf_test.py
```

### With Profiling

```bash

# Add timing details

python -m cProfile -o profile.stats comprehensive_test_suite.py
```

### Continuous Monitoring

```bash

# Run every 5 minutes

while true; do python quick_perf_test.py; sleep 300; done
```

## Interpreting Results

### Speed Ratings

-  **SENNA**: <5ms - Hyper-optimized, sub-millisecond execution
-  **F1**: 5-20ms - Very fast, optimized paths
-  **NORMAL**: 20-50ms - Acceptable performance
-  **SLOW**: >50ms - Needs optimization

### Status Indicators

-  **EXCELLENT**: <5ms average, 100% pass rate
-  **VERY GOOD**: <10ms average, >95% pass rate
-  **GOOD**: <20ms average, >90% pass rate
-  **ACCEPTABLE**: <50ms average, >80% pass rate
-  **SLOW**: >50ms average or <80% pass rate

## Performance Optimization Tips

1. **Use Persistent Subprocess Mode**
   - Eliminates 10-15ms process startup overhead
   - Achieves sub-millisecond execution

2. **Enable Result Caching**
   - 100-item LRU cache
   - Instant results for repeated expressions

3. **Batch Operations**
   - Group multiple calculations
   - Reduce IPC overhead

4. **Simplify Expressions**
   - Avoid deeply nested functions
   - Break complex expressions into parts

## Troubleshooting Slow Tests

### Common Causes

### 1. Cold Start Penalty

- First run after build is slower
- Subsequent runs use cached data
- **Solution**: Run tests twice, measure second run

### 2. Complex Expressions

- Recursive parser has exponential complexity
- Multiple function compositions slow down
- **Solution**: See [PERFORMANCE_SLOWDOWN_ANALYSIS.md](../../docs/reports/PERFORMANCE_SLOWDOWN_ANALYSIS.md)

### 3. Subprocess Fallback

- Persistent mode failed, using single-shot
- Each command spawns new process (+10-15ms)
- **Solution**: Check `engine.process` is alive

### 4. Debug Build

- Debug symbols slow execution
- **Solution**: Use Release build (`-DCMAKE_BUILD_TYPE=Release`)

## Regression Testing

Compare results over time:

```bash

# Baseline

python comprehensive_test_suite.py > baseline.txt

# After changes

python comprehensive_test_suite.py > current.txt

# Compare

diff baseline.txt current.txt
```

## Performance Reports

Detailed reports available at:

- [OPTIMIZATION_REPORT_DEC25.md](../../docs/reports/OPTIMIZATION_REPORT_DEC25.md)
- [PERFORMANCE_SLOWDOWN_ANALYSIS.md](../../docs/reports/PERFORMANCE_SLOWDOWN_ANALYSIS.md)


