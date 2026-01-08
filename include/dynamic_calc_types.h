#pragma once

#include <vector>
#include <optional>
#include <variant>
#include <functional>
#include <string>
#include <cmath>
#include <limits>
#include <complex>

// AXIOM v3.1: Complex Number Support - The "Complex" Shift
namespace AXIOM {
    // Core number type that supports both real and complex arithmetic
    using Number = std::variant<double, std::complex<double>>;
    
    // Type checking utilities
    inline bool IsReal(const Number& num) {
        return std::holds_alternative<double>(num);
    }
    
    inline bool IsComplex(const Number& num) {
        return std::holds_alternative<std::complex<double>>(num);
    }
    
    // Safe conversion utilities with performance optimization
    inline double GetReal(const Number& num) {
        if (IsReal(num)) {
            return std::get<double>(num);  // Fast path for real numbers
        } else {
            return std::get<std::complex<double>>(num).real();
        }
    }
    
    inline std::complex<double> GetComplex(const Number& num) {
        if (IsReal(num)) {
            return std::complex<double>(std::get<double>(num), 0.0);
        } else {
            return std::get<std::complex<double>>(num);  // Fast path for complex
        }
    }
    
    // AXIOM v3.1: Performance-optimized complex arithmetic operations
    // Fast arithmetic that maintains Senna Speed for real numbers
    inline Number Add(const Number& a, const Number& b) {
        // Fast path: both real numbers (most common case)
        if (IsReal(a) && IsReal(b)) {
            return Number(std::get<double>(a) + std::get<double>(b));
        }
        // Complex path: convert to complex and operate
        return Number(GetComplex(a) + GetComplex(b));
    }
    
    inline Number Multiply(const Number& a, const Number& b) {
        if (IsReal(a) && IsReal(b)) {
            return Number(std::get<double>(a) * std::get<double>(b));
        }
        return Number(GetComplex(a) * GetComplex(b));
    }
    
    inline Number Divide(const Number& a, const Number& b) {
        if (IsReal(a) && IsReal(b)) {
            double divisor = std::get<double>(b);
            if (std::abs(divisor) < 1e-15) {
                // Division by zero - return complex infinity representation
                return Number(std::complex<double>(std::numeric_limits<double>::infinity(), 0.0));
            }
            return Number(std::get<double>(a) / divisor);
        }
        return Number(GetComplex(a) / GetComplex(b));
    }
    
    inline Number Subtract(const Number& a, const Number& b) {
        if (IsReal(a) && IsReal(b)) {
            return Number(std::get<double>(a) - std::get<double>(b));
        }
        return Number(GetComplex(a) - GetComplex(b));
    }
    
    // Revolutionary: sqrt(-1) = i (no longer an error!)
    inline Number Sqrt(const Number& a) {
        if (IsReal(a)) {
            double val = std::get<double>(a);
            if (val >= 0) {
                return Number(std::sqrt(val));  // Real square root
            } else {
                return Number(std::complex<double>(0.0, std::sqrt(-val)));  // Imaginary result
            }
        }
        return Number(std::sqrt(GetComplex(a)));
    }
}

enum class CalcErr
{
    None,
    DivideByZero,
    IndeterminateResult,
    OperationNotFound,
    ArgumentMismatch,
    NegativeRoot,
    DomainError,
    ParseError,
    // New stability errors
    NumericOverflow,
    StackOverflow,
    MemoryExhausted,
    InfiniteLoop
};

enum class LinAlgErr
{
    None,
    NoSolution,
    InfiniteSolutions,
    MatrixMismatch,
    ParseError
};
using Matrix = std::vector<std::vector<double>>;
using Vector = std::vector<double>;
// AXIOM v3.1: Enhanced result types with complex number support
// Type alias for result value types - legacy support maintained
// using EngineSuccessResult = std::variant<double, std::complex<double>, AXIOM::Number, Vector, Matrix, std::string>;  // REMOVED: conflicts with factory functions
using EngineErrorResult = std::variant<CalcErr, LinAlgErr>;

struct EngineResult
{
    std::optional<std::variant<double, std::complex<double>, AXIOM::Number, Vector, Matrix, std::string>> result = std::nullopt;
    std::optional<EngineErrorResult> error = std::nullopt;
    
    // AXIOM v3.1: Enhanced accessors for complex number support
    std::optional<double> GetDouble() const {
        if (!result.has_value()) return std::nullopt;
        
        return std::visit([](const auto& val) -> std::optional<double> {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, double>) {
                return val;
            } else if constexpr (std::is_same_v<T, std::complex<double>>) {
                return val.real();  // Return real part for complex numbers
            } else if constexpr (std::is_same_v<T, AXIOM::Number>) {
                return AXIOM::GetReal(val);
            } else {
                return std::nullopt;  // Vector, Matrix, string
            }
        }, result.value());
    }
    
    std::optional<std::complex<double>> GetComplex() const {
        if (!result.has_value()) return std::nullopt;
        
        return std::visit([](const auto& val) -> std::optional<std::complex<double>> {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, double>) {
                return std::complex<double>(val, 0.0);
            } else if constexpr (std::is_same_v<T, std::complex<double>>) {
                return val;
            } else if constexpr (std::is_same_v<T, AXIOM::Number>) {
                return AXIOM::GetComplex(val);
            } else {
                return std::nullopt;  // Vector, Matrix, string
            }
        }, result.value());
    }
    
    bool HasResult() const { return result.has_value() && !error.has_value(); }
    bool HasErrors() const { return error.has_value(); }
};

// Enhanced factory functions for AXIOM::Number support
inline EngineResult CreateSuccessResult(double value) {
    EngineResult res;
    res.result = value;  // Store plain double for legacy expectations
    return res;
}

inline EngineResult CreateSuccessResult(const std::complex<double>& value) {
    EngineResult res;
    res.result = AXIOM::Number(value);
    return res;
}

inline EngineResult CreateSuccessResult(const AXIOM::Number& value) {
    EngineResult res;
    // Preserve complex values, but normalize real Numbers to plain double for callers
    if (AXIOM::IsComplex(value)) {
        res.result = AXIOM::GetComplex(value);
    } else {
        res.result = AXIOM::GetReal(value);
    }
    return res;
}



// Legacy compatibility aliases - these maintain backward compatibility
using EngineSuccessResult_Legacy = EngineResult;
inline EngineResult EngineSuccessResult(double value) { return CreateSuccessResult(value); }
inline EngineResult EngineSuccessResult(const Vector& value) { EngineResult res; res.result = value; return res; }
inline EngineResult EngineSuccessResult(const Matrix& value) { EngineResult res; res.result = value; return res; }
inline EngineResult EngineSuccessResult(const std::string& value) { EngineResult res; res.result = value; return res; }

const double PI_CONST = 3.14159265358979323846;
const double D2R = PI_CONST / 180.0;
const double R2D = 180.0 / PI_CONST;

#include <cmath>

// Safe arithmetic operations
namespace SafeMath {
    constexpr double MAX_SAFE_DOUBLE = 1e100;
    constexpr double MIN_SAFE_DOUBLE = -1e100;
    
    inline bool IsFiniteAndSafe(double val) {
        return std::isfinite(val) && val <= MAX_SAFE_DOUBLE && val >= MIN_SAFE_DOUBLE;
    }
    
    inline std::optional<double> SafeAdd(double a, double b) {
        if (!IsFiniteAndSafe(a) || !IsFiniteAndSafe(b)) return std::nullopt;
        double result = a + b;
        return IsFiniteAndSafe(result) ? std::optional<double>(result) : std::nullopt;
    }
    
    inline std::optional<double> SafePow(double base, double exp) {
        if (!IsFiniteAndSafe(base) || !IsFiniteAndSafe(exp)) return std::nullopt;
        if (std::abs(exp) > 100 || std::abs(base) > 1e10) return std::nullopt; // Prevent huge exponents
        double result = std::pow(base, exp);
        return IsFiniteAndSafe(result) ? std::optional<double>(result) : std::nullopt;
    }
}

enum class Precedence : int
{
    None = 0,
    AddSub = 1,
    MultiDiv = 2,
    Pow = 3,
    Mod = 2,
    Unary = 4
};

using Operation = std::function<EngineResult(const std::vector<double> &args)>;
using UnaryOperation = std::function<EngineResult(const std::vector<double> &args)>;

struct OperatorDetails
{
    Operation operation;
    Precedence precedence;
};

struct UnaryOperatorDetails
{
    UnaryOperation operation;
    Precedence precedence = Precedence::Unary;
};
