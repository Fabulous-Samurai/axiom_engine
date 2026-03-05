/**
 * @file dynamic_calc.h
 * @brief Optimized DynamicCalc using runtime polymorphism via IParser interface.
 */

#pragma once

#include "dynamic_calc_types.h"
#include "IParser.h"
#include <map>
#include <memory>

// Forward declarations for engine members (complete type only needed in .cpp)
class SymbolicEngine;
class StatisticsEngine;
class UnitManager;

namespace AXIOM
{

    enum class FastArithmeticOp
    {
        Add,
        Subtract,
        Multiply,
        Divide
    };

    class DynamicCalc
    {
    private:
        std::map<CalculationMode, std::unique_ptr<IParser>> parsers_;
        CalculationMode current_mode_ = CalculationMode::ALGEBRAIC;

        std::unique_ptr<SymbolicEngine> symbolic_engine_;
        std::unique_ptr<StatisticsEngine> statistics_engine_;
        std::unique_ptr<UnitManager> unit_manager_;

    public:
        DynamicCalc();
        ~DynamicCalc();

        EngineResult Evaluate(const std::string &input);
        EngineResult EvaluateFast(double lhs, double rhs, FastArithmeticOp op) noexcept;

        static bool TryEvaluateFast(double lhs, double rhs, FastArithmeticOp op, double &out) noexcept;
        void SetMode(CalculationMode mode);
    };

} // namespace AXIOM
