#pragma once

#ifndef AXIOM_SIGNAL_EXEC_TRAITS_H
#define AXIOM_SIGNAL_EXEC_TRAITS_H

#include <array>
#include <cstdint>
#include <string_view>

#include "dynamic_calc_types.h"

namespace AXIOM::SignalExec {

enum class DispatchTier : std::uint8_t {
    Scalar = 0,
    Vectorized = 1,
    Matrix = 2,
};

// Three-tap low-pass coefficients used by lightweight executive smoothing paths.
constinit inline std::array<double, 3> kSignalFilter3Tap{{0.25, 0.50, 0.25}};

consteval bool IsSquareOnlyOperationCt(std::string_view operation) {
    return operation == "determinant" || operation == "det" ||
           operation == "inverse" || operation == "inv" ||
           operation == "eigenvalues" || operation == "eigvals" ||
           operation == "trace";
}

constexpr bool IsSquareOnlyOperation(std::string_view operation) {
    return operation == "determinant" || operation == "det" ||
           operation == "inverse" || operation == "inv" ||
           operation == "eigenvalues" || operation == "eigvals" ||
           operation == "trace";
}

template <typename T>
constexpr T BlendAtCompileOrRuntime(T lhs, T rhs) {
    if consteval {
        return (lhs + rhs) / static_cast<T>(2);
    }
    return (lhs + rhs) / static_cast<T>(2);
}

consteval CalcErr FastMathErrorAtCompileTime(bool divide_by_zero) {
    return divide_by_zero ? CalcErr::DivideByZero : CalcErr::None;
}

} // namespace AXIOM::SignalExec

#endif // AXIOM_SIGNAL_EXEC_TRAITS_H
