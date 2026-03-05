/**
 * @file dynamic_calc.cpp
 * @brief Implementation of Static Dispatch Logic
 */

#include "dynamic_calc.h"
#include "algebraic_parser.h"
#include "expression_policy.h"
#include "linear_system_parser.h"
#include "statistics_parser.h"
#include "statistics_engine.h"
#include "symbolic_parser.h"
#include "symbolic_engine.h"
#include "unit_parser.h"
#include "unit_manager.h"
#include "plot_parser.h"
#include <memory>

namespace AXIOM
{

    DynamicCalc::DynamicCalc()
    {
        // Create engines that parsers depend on
        statistics_engine_ = std::make_unique<StatisticsEngine>();
        symbolic_engine_   = std::make_unique<SymbolicEngine>();
        unit_manager_      = std::make_unique<UnitManager>();

        parsers_.emplace(CalculationMode::ALGEBRAIC,     std::make_unique<AlgebraicParser>());
        parsers_.emplace(CalculationMode::LINEAR_SYSTEM, std::make_unique<LinearSystemParser>());
        parsers_.emplace(CalculationMode::STATISTICS,    std::make_unique<StatisticsParser>(statistics_engine_.get()));
        parsers_.emplace(CalculationMode::SYMBOLIC,      std::make_unique<SymbolicParser>(symbolic_engine_.get()));
        parsers_.emplace(CalculationMode::UNITS,         std::make_unique<UnitParser>(unit_manager_.get()));
        parsers_.emplace(CalculationMode::PLOT,          std::make_unique<PlotParser>());
    }

    DynamicCalc::~DynamicCalc() = default;

    EngineResult DynamicCalc::Evaluate(const std::string &input)
    {
        const auto policy = AssessExpressionPolicy(input, current_mode_);
        if (!policy.allowed)
        {
            return CreateErrorResult(policy.error);
        }

        auto it = parsers_.find(current_mode_);
        if (it == parsers_.end())
        {
            return CreateErrorResult(CalcErr::OperationNotFound);
        }

        return it->second->ParseAndExecute(input);
    }

    bool DynamicCalc::TryEvaluateFast(double lhs, double rhs, FastArithmeticOp op, double &out) noexcept
    {
        switch (op)
        {
        case FastArithmeticOp::Add:
            out = lhs + rhs;
            return true;
        case FastArithmeticOp::Subtract:
            out = lhs - rhs;
            return true;
        case FastArithmeticOp::Multiply:
            out = lhs * rhs;
            return true;
        case FastArithmeticOp::Divide:
            if (rhs == 0.0)
            {
                return false;
            }
            out = lhs / rhs;
            return true;
        default:
            return false;
        }
    }

    EngineResult DynamicCalc::EvaluateFast(double lhs, double rhs, FastArithmeticOp op) noexcept
    {
        double out = 0.0;
        if (!TryEvaluateFast(lhs, rhs, op, out))
        {
            if (op == FastArithmeticOp::Divide && rhs == 0.0)
            {
                return CreateErrorResult(CalcErr::DivideByZero);
            }
            return CreateErrorResult(CalcErr::OperationNotFound);
        }

        return CreateSuccessResult(out);
    }

    void DynamicCalc::SetMode(CalculationMode mode)
    {
        current_mode_ = mode;
    }

} // namespace AXIOM
