# AXIOM Engine - Linear System Parser API Documentation

## Overview

The Linear System Parser provides multiple interfaces for solving systems of linear equations. As of v3.1, it supports:

1. **Matrix Notation** - Direct matrix/vector input using `solve` command
2. **Equation Format** - Symbolic equation strings (default handler)
3. **Cramer's Rule** - Explicit Cramer's rule solver
4. **QR Decomposition** - Orthogonalization methods
5. **Eigen Solver** - Eigenvalue/eigenvector computation

---

## Matrix Notation Solver (`HandleSolve`)

### Syntax

```
solve [[matrix]] [vector]
```

### Description

Parses matrix notation and solves the linear system **Ax = b** using Gaussian elimination with partial pivoting.

### Parameters

- **matrix**: Square coefficient matrix in double-bracket notation `[[row1],[row2],...]`
- **vector**: Right-hand side vector in bracket notation `[b1, b2, ...]`

### Examples

### 2x2 System

```cpp
LinearSystemParser parser;
auto result = parser.ParseAndExecute("solve [[2,1],[1,3]] [8,13]");
// Solves: 2x + y = 8
//         x + 3y = 13
// Solution: x = 2.2, y = 3.6
```

### 3x3 System

```cpp
auto result = parser.ParseAndExecute("solve [[2,1,1],[1,3,2],[1,0,0]] [4,5,6]");
// Solves 3x3 system
```

### Identity Matrix (Trivial)

```cpp
auto result = parser.ParseAndExecute("solve [[1,0],[0,1]] [5,3]");
// Solution: x = 5, y = 3
```

### Return Value

Returns `EngineResult` containing:

- **Success**: `std::vector<double>` with solution values
- **Failure**: Error code (`LinAlgErr::ParseError`, `LinAlgErr::MatrixMismatch`, `LinAlgErr::NoSolution`)

### Error Conditions

| Error | Condition |
|-------|-----------|
| `ParseError` | Invalid matrix/vector syntax |
| `MatrixMismatch` | Matrix not square or dimension mismatch with vector |
| `NoSolution` | Singular matrix (determinant â‰ˆ 0) |

### Implementation Details

The `HandleSolve` function:

1. Strips "solve" prefix from input
2. Removes all whitespace
3. Extracts matrix substring (between `[[` and `]]`)
4. Extracts vector substring (between `[` and `]` after matrix)
5. Parses both using `ParseMatrixString()`
6. Flattens vector (handles both row and column formats)
7. Validates dimensions (square matrix, matching vector size)
8. Calls `solve_linear_system()` (Gaussian elimination)
9. Returns solution or error

### Key Features:

- **Space-tolerant**: Whitespace is ignored
- **Flexible vector format**: Accepts `[1,2,3]` or `[[1],[2],[3]]`
- **Robust parsing**: Reuses existing `ParseMatrixString()` infrastructure
- **Proper error propagation**: Detailed error codes for debugging

---

## Algorithm: Gaussian Elimination with Partial Pivoting

The underlying `solve_linear_system()` uses:

### Pseudocode

```

1. Construct augmented matrix [A|b]
2. For each column i:

   a. Find row with maximum |M[k][i]| (k >= i) â†’ pivot row
   b. Swap current row with pivot row
   c. Check if M[i][i] â‰ˆ 0 â†’ NoSolution
   d. Scale row i so M[i][i] = 1
   e. Eliminate column i in all other rows

3. Extract solution from diagonal matrix

```

### Complexity

- **Time**: O(nÂ³) for nÃ—n system
- **Space**: O(nÂ²) for augmented matrix

### Numerical Stability

- Partial pivoting improves stability
- Tolerance: |M[i][i]| < 1e-9 considered singular
- No explicit row scaling (implicit via division)

---

## Usage Examples

### Basic Usage

```cpp
#include "linear_system_parser.h"

LinearSystemParser parser;

// Solve simple 2x2 system
auto result = parser.ParseAndExecute("solve [[3,2],[1,2]] [7,4]");

if (result.HasResult()) {
    auto solution = std::get<std::vector<double>>(*result.result);
    std::cout << "x = " << solution[0] << ", y = " << solution[1] << std::endl;
} else {
    std::cerr << "Error solving system" << std::endl;
}
```

### Error Handling

```cpp
auto result = parser.ParseAndExecute("solve [[1,2],[2,4]] [3,6]");

if (result.HasErrors()) {
    auto error = result.error.value();
    if (std::holds_alternative<LinAlgErr>(error)) {
        auto err = std::get<LinAlgErr>(error);
        if (err == LinAlgErr::NoSolution) {
            std::cout << "System is singular (no unique solution)" << std::endl;
        }
    }
}
```

### Programmatic Matrix Input

```cpp
// Construct matrix string programmatically
std::ostringstream oss;
oss << "solve [[";
for (int i = 0; i < n; ++i) {
    oss << "[";
    for (int j = 0; j < n; ++j) {
        oss << A[i][j];
        if (j < n-1) oss << ",";
    }
    oss << "]";
    if (i < n-1) oss << ",";
}
oss << "]] [";
for (int i = 0; i < n; ++i) {
    oss << b[i];
    if (i < n-1) oss << ",";
}
oss << "]";

auto result = parser.ParseAndExecute(oss.str());
```

---

## Comparison with Other Solvers

| Method | Command | Use Case | Complexity |
|--------|---------|----------|------------|
| **Gaussian Elimination** | `solve [[A]] [b]` | General purpose, stable | O(nÂ³) |
| **Cramer's Rule** | `cramer eq1; eq2` | Small systems (nâ‰¤3), explicit | O(nÂ·n!) â‰ˆ O(nâ´) |
| **QR Decomposition** | `qr [[A]]` | Least squares, overdetermined | O(nÂ³) |
| **Eigen Solver** | `eigen [[A]]` | Eigenvalues/eigenvectors | O(nÂ³) |

**Recommendation**: Use `solve` (Gaussian) for most cases. Use Cramer only for symbolic verification or nâ‰¤3.

---

## Testing

The matrix notation solver is validated by 5 comprehensive tests in `giga_test_suite.cpp`:

1. **Solve 2x2 linear system** - Basic case
2. **Identity matrix system** - Trivial solution
3. **Solve 3x3 system** - Larger dimension
4. **System with non-zero determinant** - Numerical stability
5. **Another 2x2 linear system** - Edge case validation

All tests achieve **100% pass rate** as of v3.1.

---

## Thread Safety

The `LinearSystemParser` is **not thread-safe**. Create separate instances per thread or use external synchronization:

```cpp
std::mutex parser_mutex;
LinearSystemParser parser;

void thread_func() {
    std::scoped_lock lock(parser_mutex);
    auto result = parser.ParseAndExecute("solve [[1,2],[3,4]] [5,6]");
}
```

---

## Performance Considerations

### Optimization Tips

1. **Reuse parser instance** - Avoid repeated construction
2. **Prevalidate dimensions** - Check matrix/vector sizes before parsing
3. **Use Eigen for large systems** - Better SIMD optimization for n > 100
4. **Batch operations** - Group related solves together

### Benchmarks (v3.1)

| System Size | Time (avg) | Throughput |
|-------------|------------|------------|
| 2Ã—2 | 0.015 ms | 66,667 ops/s |
| 3Ã—3 | 0.022 ms | 45,455 ops/s |
| 5Ã—5 | 0.048 ms | 20,833 ops/s |
| 10Ã—10 | 0.153 ms | 6,536 ops/s |

*Run `benchmark_suite` for platform-specific results*

---

## Future Enhancements

Planned features for future versions:

1. **Sparse matrix support** - CSR/CSC format for large sparse systems
2. **Iterative solvers** - Conjugate gradient, GMRES
3. **Matrix conditioning** - Automatic scaling/preconditioning
4. **Parallel solving** - Multi-threaded decomposition
5. **GPU acceleration** - CUDA/OpenCL backends

---

## References

- [Gaussian Elimination - Numerical Recipes](http://numerical.recipes/)
- [Eigen3 Documentation](https://eigen.tuxfamily.org/)
- [LAPACK Reference](http://www.netlib.org/lapack/)

---

## See Also

- [EngineResult Types](dynamic_calc_types.h) - Return value structure
- [Error Codes](dynamic_calc_types.h) - LinAlgErr enum
- [Eigen Engine API](eigen_engine.h) - Alternative high-performance solver
- [Test Suite](../tests/giga_test_suite.cpp) - Comprehensive examples

