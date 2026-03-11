# GUI Test Commands

Test these commands in the AXIOM GUI to verify all fixes:

## 1. Algebraic Mode (should already work)

```
2 + 3 * 4
sqrt(16)
sin(pi/2)
```

## 2. Linear System Mode

```
solve([2,3;1,4],[5;6])
solve([[2,3],[1,4]],[[5],[6]])
solve([1,2,3;4,5,6;7,8,10],[1;2;3])
```

## 3. Statistics Mode

```
mean[1,2,3,4,5]
variance[1,2,3,4,5]
std[1,2,3,4,5]ian[1,2,3,4,5]
correlation([1,2,3],[2,4,6])
```

## 4. Symbolic Mode

```
simplify((x^2-1)/(x-1))
expand((x+1)^2)
factor(x^2+2*x+1)
diff(x^2+3*x, x)
```

## 5. Plotting Mode (Python matplotlib rendering)

```
plot(sin(x), -3.14, 3.14, -1.5, 1.5)
plot(x^2, -5, 5, 0, 25)
plot(cos(x)*exp(-x/5), 0, 10, -1, 1)
```

## 6. Units Mode

```
5 m to ft
100 km/h to m/s
1 year to seconds
```

## Expected Results:

- **Algebraic**: Returns numeric results
- **Linear**: Returns solution vectors (not "Parse error")
- **Statistics**: Returns computed statistics (not "Operation not found")
- **Symbolic**: Returns simplified expressions (not "Operation not found")
- **Plotting**: Opens matplotlib window with plot (not ASCII art)
- **Units**: Returns converted values

