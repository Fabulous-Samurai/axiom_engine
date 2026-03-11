# AXIOM::Number Variant Pattern Guide

## Overview

AXIOM Engine uses a sophisticated variant system to support both real and complex numbers transparently. Understanding this pattern is crucial for correctly accessing computation results.

---

## Type Hierarchy

```
EngineResult
  â”œâ”€ std::optional<std::variant<...>>
      â”œâ”€ double                    (raw real number)
      â”œâ”€ std::complex<double>      (raw complex number)
      â”œâ”€ AXIOM::Number             (wrapped union type) â­
      â”œâ”€ Vector                    (std::vector<double>)
      â”œâ”€ Matrix                    (std::vector<std::vector<double>>)
      â””â”€ std::string               (text result)

AXIOM::Number (std::variant)
  â”œâ”€ double                        (real component)
  â””â”€ std::complex<double>          (complex component)
```

---

## The Problem: Double Wrapping

### What Happens

When you create a success result from a `double`:

```cpp
EngineResult result = EngineSuccessResult(5.0);
```

The value is **wrapped in AXIOM::Number**, not stored as a raw `double`:

```cpp
// In dynamic_calc_types.h
inline EngineResult EngineSuccessResult(double value) {
    return CreateSuccessResult(AXIOM::Number(value));  // âš ï¸ Wrapped!
}
```

### Why This Fails

Direct access assumes raw `double` in the variant:

```cpp
double val = std::get<double>(*result.result);  // âŒ WRONG!
// Throws: std::get: wrong index for variant
```

The variant actually contains `AXIOM::Number`, not `double`.

---

## The Solution: Correct Access Patterns

### Pattern 1: Using GetDouble() (Recommended)

```cpp
auto result = parser.Parse("5 + 3");

if (result.HasResult()) {
    auto val_opt = result.GetDouble();
    if (val_opt.has_value()) {
        double val = *val_opt;
        std::cout << "Result: " << val << std::endl;
    }
}
```

### Advantages:

- Handles AXIOM::Number unwrapping automatically
- Works with complex numbers (extracts real part)
- Returns `std::optional<double>` for safety
- Consistent with AXIOM design patterns

### Pattern 2: Manual Unwrapping

```cpp
auto result = stats.Mean(data);

if (result.result.has_value()) {
    // Step 1: Extract AXIOM::Number from outer variant
    AXIOM::Number num = std::get<AXIOM::Number>(*result.result);
    
    // Step 2: Unwrap double from AXIOM::Number
    double val = AXIOM::GetReal(num);
    
    std::cout << "Mean: " << val << std::endl;
}
```

### Advantages:

- Explicit control over unwrapping
- Useful when you need to inspect AXIOM::Number properties
- Required for mutable operations

### Pattern 3: One-Liner (Compact)

```cpp
double val = AXIOM::GetReal(std::get<AXIOM::Number>(*result.result));
```

### Use Cases:

- Performance-critical code
- When you're certain result exists and is real
- Internal library code

---

## Common Mistakes & Fixes

### Mistake 1: Direct std::get<double>

```cpp
// âŒ WRONG
double mean = std::get<double>(*mean_result.result);
```

### Fix:

```cpp
// âœ… CORRECT
double mean = AXIOM::GetReal(std::get<AXIOM::Number>(*mean_result.result));
```

### Mistake 2: Assuming Raw Double in Variant

```cpp
// âŒ WRONG - Assumes variant contains double
if (std::holds_alternative<double>(*result.result)) {
    double val = std::get<double>(*result.result);
}
```

### Fix:

```cpp
// âœ… CORRECT - Check for AXIOM::Number
if (std::holds_alternative<AXIOM::Number>(*result.result)) {
    double val = AXIOM::GetReal(std::get<AXIOM::Number>(*result.result));
}
```

### Mistake 3: Ignoring Complex Numbers

```cpp
// âš ï¸ INCOMPLETE - Only handles real case
double val = AXIOM::GetReal(std::get<AXIOM::Number>(*result.result));
// What if result is complex?
```

### Fix:

```cpp
// âœ… ROBUST - Handle both real and complex
AXIOM::Number num = std::get<AXIOM::Number>(*result.result);
if (AXIOM::IsReal(num)) {
    double val = AXIOM::GetReal(num);
    std::cout << "Real: " << val << std::endl;
} else {
    std::complex<double> cval = AXIOM::GetComplex(num);
    std::cout << "Complex: " << cval.real() << " + " << cval.imag() << "i" << std::endl;
}
```

---

## Helper Functions

### AXIOM::GetReal()

Extracts the real component from AXIOM::Number:

```cpp
inline double GetReal(const Number& n) {
    if (std::holds_alternative<double>(n)) {
        return std::get<double>(n);
    }
    return std::get<std::complex<double>>(n).real();
}
```

### AXIOM::GetComplex()

Converts AXIOM::Number to complex (real numbers â†’ complex with imag=0):

```cpp
inline std::complex<double> GetComplex(const Number& n) {
    if (std::holds_alternative<double>(n)) {
        return std::complex<double>(std::get<double>(n), 0.0);
    }
    return std::get<std::complex<double>>(n);
}
```

### AXIOM::IsReal()

Checks if AXIOM::Number contains a pure real value:

```cpp
inline bool IsReal(const Number& n) {
    return std::holds_alternative<double>(n);
}
```

---

## Real-World Examples

### Example 1: Statistics Engine (Fixed)

### Before (Broken):

```cpp
EngineResult StatisticsEngine::Variance(const Vector& data) {
    auto mean_result = Mean(data);
    double mean_val = std::get<double>(*mean_result.result);  // âŒ CRASH
    // ...
}
```

### After (Working):

```cpp
EngineResult StatisticsEngine::Variance(const Vector& data) {
    auto mean_result = Mean(data);
    double mean_val = AXIOM::GetReal(std::get<AXIOM::Number>(*mean_result.result));  // âœ…
    // ...
}
```

### Example 2: Test Validation

### Before (Brittle):

```cpp
runner.RunTest("Mean test", [&]() {
    auto result = stats.Mean(data);
    return std::get<double>(*result.result) == 3.0;  // âŒ Type error
});
```

### After (Robust):

```cpp
runner.RunTest("Mean test", [&]() {
    auto result = stats.Mean(data);
    return result.HasResult() && 
           std::abs(*result.GetDouble() - 3.0) < 0.01;  // âœ…
});
```

### Example 3: Engine Integration

```cpp
class MyCustomEngine {
    EngineResult ProcessNumber(double input) {
        // Process input...
        double result = input * 2;
        
        // Return wrapped in AXIOM::Number
        return EngineSuccessResult(result);  // Automatically wraps
    }
    
    void UseResult(EngineResult& res) {
        if (res.HasResult()) {
            // Correct unwrapping
            double val = AXIOM::GetReal(std::get<AXIOM::Number>(*res.result));
            std::cout << "Processed: " << val << std::endl;
        }
    }
};
```

---

## Design Rationale

### Why AXIOM::Number?

1. **Unified Type System**
   - Single type handles both real and complex numbers
   - Seamless promotion (real â†’ complex) when needed
   - sqrt(-1) returns complex without errors

2. **Type Safety**
   - Compile-time type checking
   - Explicit unwrapping prevents silent bugs
   - std::variant ensures correctness

3. **Performance**
   - Fast path for real-only operations
   - No virtual dispatch overhead
   - Inline-friendly helper functions

4. **API Consistency**
   - All engines return EngineResult
   - Uniform access patterns
   - Predictable error handling

---

## Best Practices

### âœ… DO

1. **Use GetDouble() for simple cases**

   ```cpp
   auto val = result.GetDouble();
   ```

2. **Check HasResult() before access**

   ```cpp
   if (result.HasResult()) {
       double val = *result.GetDouble();
   }
   ```

3. **Use AXIOM helpers for unwrapping**

   ```cpp
   AXIOM::GetReal(std::get<AXIOM::Number>(...))
   ```

4. **Handle complex numbers when possible**

   ```cpp
   if (AXIOM::IsReal(num)) { /* ... */ }
   else { /* complex case */ }
   ```

### âŒ DON'T

1. **Don't use std::get<double> directly**

   ```cpp
   std::get<double>(*result.result)  // âŒ
   ```

2. **Don't assume variant contains raw double**

   ```cpp
   if (std::holds_alternative<double>(...))  // âŒ
   ```

3. **Don't ignore complex number possibility**

   ```cpp
   // Only handling real case is incomplete
   ```

4. **Don't access result without checking**

   ```cpp
   double val = *result.result;  // âŒ No safety check
   ```

---

## Migration Guide

If you have existing code using the old pattern:

### Step 1: Find All Direct Access

```bash
grep -r "std::get<double>.*result\.result" src/
```

### Step 2: Replace with Correct Pattern

```cpp

# Before

double val = std::get<double>(*result.result);

# After

double val = AXIOM::GetReal(std::get<AXIOM::Number>(*result.result));
```

### Step 3: Add Safety Checks

```cpp

# Add before accessing

if (!result.HasResult()) {
    return error_state;
}
```

### Step 4: Test Thoroughly

Run the full test suite to verify changes:
```bash
./giga_test_suite
```

---

## Troubleshooting

### Error: "std::get: wrong index for variant"

**Cause:** Trying to access raw `double` when variant contains `AXIOM::Number`

**Solution:** Use `AXIOM::GetReal(std::get<AXIOM::Number>(...))` instead

### Error: "std::bad_optional_access"

**Cause:** Accessing result without checking `HasResult()`

**Solution:** Add `if (result.HasResult())` check

### Error: Complex number when expecting real

**Cause:** Operation produced complex result (e.g., sqrt(-1))

**Solution:** Check `AXIOM::IsReal()` or use `GetComplex()` instead

---

## See Also

- [EngineResult Documentation](dynamic_calc_types.h)
- [AXIOM::Number Type Definition](dynamic_calc_types.h)
- [Fix Report - Variant Access Bugs](../FIX_REPORT_100_PERCENT.md)
- [Test Examples](../../tests/giga_test_suite.cpp)

