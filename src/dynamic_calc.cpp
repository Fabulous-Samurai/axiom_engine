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

namespace {

constexpr std::size_t kInvalidModeIndex = static_cast<std::size_t>(-1);

} // namespace

namespace AXIOM
{

    std::size_t DynamicCalc::ModeToIndex(CalculationMode mode) noexcept
    {
        const auto raw = static_cast<std::size_t>(mode);
        return raw < kModeSlots ? raw : kInvalidModeIndex;
    }

    IParser* DynamicCalc::ParserForMode(CalculationMode mode) const noexcept
    {
        const std::size_t idx = ModeToIndex(mode);
        if (idx == kInvalidModeIndex) [[unlikely]]
        {
            return nullptr;
        }

        const auto& slot = parsers_[idx];
        return slot ? slot.get() : nullptr;
    }

    DynamicCalc::DynamicCalc()
    {
        // Create engines that parsers depend on
        statistics_engine_ = std::make_unique<StatisticsEngine>();
        symbolic_engine_   = std::make_unique<SymbolicEngine>();
        unit_manager_      = std::make_unique<UnitManager>();

        parsers_[ModeToIndex(CalculationMode::ALGEBRAIC)] = std::make_unique<AlgebraicParser>();
        parsers_[ModeToIndex(CalculationMode::LINEAR_SYSTEM)] = std::make_unique<LinearSystemParser>();
        parsers_[ModeToIndex(CalculationMode::STATISTICS)] = std::make_unique<StatisticsParser>(statistics_engine_.get());
        parsers_[ModeToIndex(CalculationMode::SYMBOLIC)] = std::make_unique<SymbolicParser>(symbolic_engine_.get());
        parsers_[ModeToIndex(CalculationMode::UNITS)] = std::make_unique<UnitParser>(unit_manager_.get());
        parsers_[ModeToIndex(CalculationMode::PLOT)] = std::make_unique<PlotParser>();
    }

    DynamicCalc::~DynamicCalc() = default;

    EngineResult DynamicCalc::Evaluate(const std::string &input)
    {
        const auto policy = AssessExpressionPolicy(input, current_mode_);
        if (!policy.allowed) [[unlikely]]
        {
            return CreateErrorResult(policy.error);
        }

        IParser* parser = ParserForMode(current_mode_);
        if (parser == nullptr) [[unlikely]]
        {
            return CreateErrorResult(CalcErr::OperationNotFound);
        }

        return parser->ParseAndExecute(input);
    }

    // TryEvaluateFast is now inlined in dynamic_calc.h for zero-overhead dispatch.

    EngineResult DynamicCalc::EvaluateFast(double lhs, double rhs, FastArithmeticOp op) noexcept
    {
        using enum FastArithmeticOp;
        double out = 0.0;
        if (!TryEvaluateFast(lhs, rhs, op, out))
        {
            if (op == Divide && rhs == 0.0)
            {
                return CreateErrorResult(CalcErr::DivideByZero);
            }
            return CreateErrorResult(CalcErr::OperationNotFound);
        }

        return CreateSuccessResult(out);
    }

    void DynamicCalc::SetMode(CalculationMode mode) noexcept
    {
        current_mode_ = mode;
    }

} // namespace AXIOM
