# Manual Plotting Test Cases for C++ Dynamic Calculator

## How to Test Plotting Functions

### Basic Plotting Mode

```bash

# Switch to plotting mode first

mode plot

# Or use plotting commands directly in algebraic mode

```

##  **Basic Function Plotting Tests**

### 1. Linear Functions

```bash

# Test 1: Simple linear function

plot(x, -5, 5, -5, 5)

# Expected: Diagonal line from bottom-left to top-right

# Test 2: Steep linear function  

plot(2*x, -5, 5, -10, 10)

# Expected: Steeper diagonal line

# Test 3: Linear with offset

plot(x + 2, -5, 5, -3, 7)

# Expected: Diagonal line shifted up by 2 units

# Test 4: Negative slope

plot(-x + 3, -5, 5, -2, 8)

# Expected: Line sloping down from left to right

```

### 2. Quadratic Functions

```bash

# Test 5: Basic parabola

plot(x^2, -3, 3, 0, 9)

# Expected: U-shaped curve (parabola opening upward)

# Test 6: Inverted parabola

plot(-x^2 + 4, -3, 3, -5, 4)

# Expected: Inverted U-shape (parabola opening downward)

# Test 7: Shifted parabola

plot((x-1)^2 + 2, -2, 4, 0, 10)

# Expected: Parabola with vertex at (1, 2)

# Test 8: Wide parabola

plot(x^2/4, -4, 4, 0, 4)

# Expected: Wider U-shape than basic parabola

```

### 3. Cubic and Higher-Order Functions

```bash

# Test 9: Simple cubic

plot(x^3, -2, 2, -8, 8)

# Expected: S-shaped curve passing through origin

# Test 10: Cubic with coefficient

plot(x^3 - 3*x, -3, 3, -6, 6)

# Expected: S-curve with local max and min

# Test 11: Quartic function

plot(x^4 - 4*x^2, -3, 3, -4, 10)

# Expected: W-shaped curve with two valleys

```

##  **Trigonometric Function Tests**

### 4. Sine and Cosine Functions

```bash

# Test 12: Sine function (degrees)

plot(sin(x), 0, 360, -1.5, 1.5)

# Expected: Smooth wave starting at 0, peak at 90, back to 0 at 180

# Test 13: Cosine function (degrees)

plot(cos(x), 0, 360, -1.5, 1.5)

# Expected: Wave starting at 1, down to 0 at 90, -1 at 180

# Test 14: Scaled sine

plot(2*sin(x), 0, 360, -2.5, 2.5)

# Expected: Sine wave with amplitude 2 (taller peaks/valleys)

# Test 15: Frequency modulation

plot(sin(2*x), 0, 360, -1.5, 1.5)

# Expected: Sine wave with double frequency (2 complete cycles)

# Test 16: Phase shift

plot(sin(x + 45), 0, 360, -1.5, 1.5)

# Expected: Sine wave shifted left by 45 degrees

```

### 5. Tangent Functions

```bash

# Test 17: Tangent function (careful with domain!)

plot(tan(x), 0, 180, -5, 5)

# Expected: Vertical asymptotes, should show discontinuities

# Test 18: Scaled tangent

plot(tan(x)/2, 0, 180, -3, 3)

# Expected: Compressed tangent function

```

##  **Advanced Mathematical Functions**

### 6. Exponential and Logarithmic Functions

```bash

# Test 19: Exponential growth

plot(exp(x), -2, 2, 0, 8)

# Expected: Exponential curve, steep rise on right side

# Test 20: Exponential decay

plot(exp(-x), -2, 2, 0, 8)

# Expected: Exponential curve, steep drop on right side

# Test 21: Natural logarithm

plot(ln(x), 0.1, 5, -2, 2)

# Expected: Logarithmic curve, steep rise near x=0, gentle rise for large x

# Test 22: Base-10 logarithm

plot(log(x), 1, 100, -2, 2)

# Expected: Similar to ln but different scale

```

### 7. Absolute Value and Piecewise Functions

```bash

# Test 23: Absolute value

plot(abs(x), -5, 5, 0, 5)

# Expected: V-shaped curve with vertex at origin

# Test 24: Absolute value shifted

plot(abs(x - 2), -2, 6, 0, 4)

# Expected: V-shaped curve with vertex at x=2

# Test 25: Complex absolute expression

plot(abs(x^2 - 4), -3, 3, 0, 6)

# Expected: W-shaped curve with valleys at x=-2 and x=2

```

##  **Calculus Integration Tests**

### 8. Testing Our New Calculus Functions in Plots

```bash

# Test 26: Plot a function and its limit

# First plot: f(x) = (x^2 - 1)/(x - 1) near x = 1

plot(x + 1, 0, 2, 1, 3)

# Expected: Linear function y = x + 1

# Note: This is the limit of (x^2-1)/(x-1) as x approaches 1

# Test 27: Derivative visualization

# Plot f(x) = x^2 and its derivative 2x

plot(x^2, -2, 2, -1, 4)

# Then plot: plot(2*x, -2, 2, -4, 4)

# Expected: Compare parabola with its linear derivative

# Test 28: Integration area visualization

plot(x, 0, 3, 0, 3)

# Expected: Linear function - visualize area under curve from 0 to 2

# This should help verify integrate(x, x, 0, 2) = 2

```

##  **Edge Case and Error Testing**

### 9. Boundary and Error Conditions

```bash

# Test 29: Function with discontinuity

plot(1/x, -5, 5, -10, 10)

# Expected: Hyperbola with vertical asymptote at x=0

# Test 30: Function with undefined regions

plot(sqrt(x), -2, 4, -1, 2)

# Expected: Square root curve, undefined for x < 0

# Test 31: Very steep function

plot(x^10, -1.5, 1.5, -1, 1)

# Expected: Very flat near x=0, very steep at edges

# Test 32: Oscillating function

plot(sin(10*x), 0, 360, -1.5, 1.5)

# Expected: Rapidly oscillating sine wave

```

##  **Configuration Testing**

### 10. Different Plot Settings

```bash

# Test 33: Wide plot range

plot(x^2, -10, 10, 0, 100)

# Expected: Very wide parabola view

# Test 34: Narrow plot range

plot(sin(x), 0, 90, -1.1, 1.1)

# Expected: Quarter of sine wave, high detail

# Test 35: Asymmetric ranges

plot(exp(x), -1, 3, 0, 20)

# Expected: Exponential curve with good detail on growth region

```

##  **Visual Verification Guidelines**

### What to Look For:

1. **Continuity**: Smooth curves should appear continuous (no gaps)
2. **Symmetry**: Functions like x^2, cos(x) should show expected symmetry
3. **Intercepts**: Check if curves cross axes at expected points
4. **Asymptotes**: Functions like 1/x should show vertical/horizontal asymptotes
5. **Periodicity**: Trigonometric functions should show repeating patterns
6. **Growth Rates**: Exponential functions should show characteristic growth

### Common Issues to Report:

-  Missing points in continuous functions
-  Incorrect axis crossing points
-  Wrong curve shapes (parabolas not U-shaped, etc.)
-  Scaling issues (functions appearing too compressed/stretched)
-  Asymptote handling problems

##  **Advanced Testing**

### 11. Combined Function Tests

```bash

# Test 36: Sum of functions

plot(sin(x) + cos(x), 0, 360, -2, 2)

# Expected: Wave with amplitude 2

# Test 37: Product of functions

plot(x * sin(x), 0, 360, -360, 360)

# Expected: Sine wave with increasing amplitude

# Test 38: Composite functions

plot(sin(x^2), -5, 5, -1.5, 1.5)

# Expected: Increasingly rapid oscillations away from origin

# Test 39: Function with our calculus operations

plot(x^2, 0, 3, 0, 9)

# Verify this matches integrate(2*x, x, 0, t) for various t values

```

---

##  **Quick Test Commands**

Copy-paste these for rapid testing:

```bash
mode plot
plot(x^2, -3, 3, 0, 9)
plot(sin(x), 0, 360, -1.5, 1.5)
plot(abs(x), -5, 5, 0, 5)
plot(exp(x), -2, 2, 0, 8)
plot(1/x, -5, 5, -10, 10)
```

These tests will help verify that the plotting engine correctly renders mathematical functions and integrates well with your new calculus capabilities! 


