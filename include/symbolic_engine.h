/**
 * @file symbolic_engine.h  
 * @brief Advanced symbolic computation capabilities
 * Adds algebraic manipulation, equation solving, and symbolic integration
 */
#pragma once

#include "algebraic_parser.h"
#include <unordered_set>

class SymbolicEngine {
private:
    Arena symbolic_arena_;
    std::unordered_map<std::string, AXIOM::NodePtr> symbol_table_;
    
public:
    // Advanced algebraic operations
    EngineResult Expand(const std::string& expression);         // (x+1)^2 -> x^2 + 2x + 1
    EngineResult Factor(const std::string& expression);         // x^2 - 1 -> (x-1)(x+1)  
    EngineResult Simplify(const std::string& expression);       // Complex simplification
    EngineResult Substitute(const std::string& expr, const std::string& var, const std::string& value);
    
    // Calculus operations  
    EngineResult Integrate(const std::string& expression, const std::string& variable);
    EngineResult DefiniteIntegral(const std::string& expr, const std::string& var, double a, double b);
    EngineResult PartialDerivative(const std::string& expr, const std::string& var);
    EngineResult Differentiate(const std::string& expr, const std::string& var) { return PartialDerivative(expr, var); }
    EngineResult TaylorSeries(const std::string& expr, const std::string& var, double point, int order);
    
    // Equation solving
    EngineResult SolveEquation(const std::string& equation, const std::string& variable);
    EngineResult SolveSystem(const std::vector<std::string>& equations, const std::vector<std::string>& variables);
    
    // Advanced features
    EngineResult FindLimits(const std::string& expr, const std::string& var, double approach_point);
    EngineResult FindRoots(const std::string& expr, const std::string& var, double range_min, double range_max);
};