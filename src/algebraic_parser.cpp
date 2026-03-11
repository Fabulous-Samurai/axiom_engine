/**
 * @file algebraic_parser.cpp
 * @brief Implementation of the Algebraic Parser using AST.
 * RESTORED: Full functionality including Evaluate, Derivative, and Simplify logic.
 */

#include "../include/algebraic_parser.h"
#include "../include/string_helpers.h"
#include <exception>
#include <iostream>
#include <cmath>
#include <numbers>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <format>
#include <set>
#include <shared_mutex>
#include <mutex>

namespace AXIOM {

// --- Constants and SafeMath (used by this translation unit) ---
namespace {
    constexpr double PI_CONST = std::numbers::pi;
    constexpr double D2R = std::numbers::pi / 180.0;
    constexpr double R2D = 180.0 / std::numbers::pi;

    struct SafeMath {
        static std::optional<double> SafeAdd(double a, double b) {
            double result = a + b;
            if (!std::isfinite(result)) return std::nullopt;
            return result;
        }
        static std::optional<double> SafePow(double base, double exp) {
            double result = std::pow(base, exp);
            if (!std::isfinite(result)) return std::nullopt;
            return result;
        }
        static bool IsFiniteAndSafe(double val) { return std::isfinite(val); }
    };
} // anonymous namespace

// --- Helpers ---

Precedence GetOpPrecedence(char op) {
    if (op == '+' || op == '-') return Precedence::AddSub;
    if (op == '*' || op == '/') return Precedence::MultiDiv;
    if (op == '^') return Precedence::Pow;
    return Precedence::None;
}


bool IsConst(const NodePtr node, double val) {
    auto res = node->Evaluate({});
    if (!res.value.has_value()) return false;
    double node_val = AXIOM::GetReal(*res.value);
    return std::abs(node_val - val) < 1e-9;
}

CalcErr NormalizeError(const EvalResult& res, CalcErr fallback = CalcErr::ArgumentMismatch) {
    return res.error == CalcErr::None ? fallback : res.error;
}

std::string FormatNumber(double val) {
    // Handle special cases
    if (std::isinf(val)) return std::signbit(val) ? "-inf" : "inf";
    if (std::isnan(val)) return "nan";
    
    // For integers or numbers that can be represented exactly
    if (val == std::floor(val) && std::abs(val) < 1e15) {
        return std::to_string(static_cast<long long>(val));
    }
    
    // For normal floating point numbers
    const double abs_val = std::abs(val);
    
    if (abs_val >= 1e6 || (abs_val > 0 && abs_val < 1e-6)) {
        return std::format("{:.6e}", val);
    }
    return std::format("{:.15g}", val);
}

// ========================================================
// AST NODE IMPLEMENTATIONS
// ========================================================

struct NumberNode : ExprNode {
    double value;
    explicit NumberNode(double v) : value(v) {}
    NodeType GetType() const override { return NodeType::Number; }
    
    
    EvalResult Evaluate(const StringUnorderedMap<AXIOM::Number>&) const override { return EvalResult::Success(value); }
    
    NodePtr Derivative(Arena& arena, std::string_view) const override { return arena.alloc<NumberNode>(0.0); }
    NodePtr Simplify(Arena& arena) const override { return arena.alloc<NumberNode>(value); }
    std::string ToString(Precedence) const override { return FormatNumber(value); }
};

struct VariableNode : ExprNode {
    std::string_view name;
    explicit VariableNode(std::string_view n) : name(n) {}
    NodeType GetType() const override { return NodeType::Variable; }
    
    EvalResult Evaluate(const StringUnorderedMap<AXIOM::Number>& vars) const override {
        std::string key(name);
        if (auto it = vars.find(key); it != vars.end()) {
            // Extract real value from AXIOM::Number
            double real_value = AXIOM::GetReal(it->second);
            return EvalResult::Success(real_value);
        }
        if (key == "Ans") return EvalResult::Success(0.0);
        
        // Mathematical constants
        if (key == "pi" || key == "PI") return EvalResult::Success(PI_CONST);
        if (key == "e" || key == "E") return EvalResult::Success(std::numbers::e);
        if (key == "phi") return EvalResult::Success(std::numbers::phi); // Golden ratio
        
        return EvalResult::Failure(CalcErr::ArgumentMismatch);
    }
    
    NodePtr Derivative(Arena& arena, std::string_view var) const override {
        if (name == var) return arena.alloc<NumberNode>(1.0);
        return arena.alloc<NumberNode>(0.0);
    }
    NodePtr Simplify(Arena& arena) const override { return arena.alloc<VariableNode>(name); }
    std::string ToString(Precedence) const override { return std::string(name); }
};

struct BinaryOpNode : ExprNode {
    char op;
    NodePtr left;
    NodePtr right;
    
    BinaryOpNode(char c, NodePtr l, NodePtr r) : op(c), left(l), right(r) {}
    NodeType GetType() const override { return NodeType::BinaryOp; }
    
    
    EvalResult Evaluate(const StringUnorderedMap<AXIOM::Number>& vars) const override {
        auto left_eval = left->Evaluate(vars);
        if (!left_eval.HasValue()) return left_eval;
        auto right_eval = right->Evaluate(vars);
        if (!right_eval.HasValue()) return right_eval;
        double l = AXIOM::GetReal(*left_eval.value);
        double r = AXIOM::GetReal(*right_eval.value);
        switch(op) {
            case '+': {
                auto safe_result = SafeMath::SafeAdd(l, r);
                return safe_result ? EvalResult::Success(*safe_result) : EvalResult::Failure(CalcErr::NumericOverflow);
            }
            case '-': {
                auto safe_result = SafeMath::SafeAdd(l, -r);
                return safe_result ? EvalResult::Success(*safe_result) : EvalResult::Failure(CalcErr::NumericOverflow);
            }
            case '*': {
                if (!SafeMath::IsFiniteAndSafe(l * r)) return EvalResult::Failure(CalcErr::NumericOverflow);
                return EvalResult::Success(l * r);
            }
            case '/': {
                if (r == 0.0) return EvalResult::Failure(CalcErr::DivideByZero);
                if (!SafeMath::IsFiniteAndSafe(l / r)) return EvalResult::Failure(CalcErr::NumericOverflow);
                return EvalResult::Success(l / r);
            }
            case '^': {
                auto safe_result = SafeMath::SafePow(l, r);
                return safe_result ? EvalResult::Success(*safe_result) : EvalResult::Failure(CalcErr::NumericOverflow);
            }
            default: return EvalResult::Success(0.0);
        }
    }
    
    NodePtr Derivative(Arena& arena, std::string_view var) const override {
        auto dl = left->Derivative(arena, var);
        auto dr = right->Derivative(arena, var);
        
        if (op == '+' || op == '-') return arena.alloc<BinaryOpNode>(op, dl, dr);
        if (op == '*') {
            auto t1 = arena.alloc<BinaryOpNode>('*', dl, right);
            auto t2 = arena.alloc<BinaryOpNode>('*', left, dr);
            return arena.alloc<BinaryOpNode>('+', t1, t2);
        }
        if (op == '/') {
            auto t1 = arena.alloc<BinaryOpNode>('*', dl, right);
            auto t2 = arena.alloc<BinaryOpNode>('*', left, dr);
            auto num = arena.alloc<BinaryOpNode>('-', t1, t2);
            auto den = arena.alloc<BinaryOpNode>('^', right, arena.alloc<NumberNode>(2.0));
            return arena.alloc<BinaryOpNode>('/', num, den);
        }
        if (op == '^') {
            auto n_minus_1 = arena.alloc<BinaryOpNode>('-', right, arena.alloc<NumberNode>(1.0));
            auto u_pow = arena.alloc<BinaryOpNode>('^', left, n_minus_1);
            auto n_times_u = arena.alloc<BinaryOpNode>('*', right, u_pow);
            return arena.alloc<BinaryOpNode>('*', n_times_u, dl);
        }
        return arena.alloc<NumberNode>(0.0); 
    }

    NodePtr Simplify(Arena& arena) const override {
        auto simple_left = left->Simplify(arena);
        auto simple_right = right->Simplify(arena);

       
        bool l_const = false, r_const = false;
        double l_val = 0, r_val = 0;
        auto l_eval = simple_left->Evaluate({});
        if (l_eval.value.has_value()) { l_const = true; l_val = AXIOM::GetReal(*l_eval.value); }
        auto r_eval = simple_right->Evaluate({});
        if (r_eval.value.has_value()) { r_const = true; r_val = AXIOM::GetReal(*r_eval.value); }

        if (l_const && r_const) {
            if (op == '+') return arena.alloc<NumberNode>(l_val + r_val);
            if (op == '-') return arena.alloc<NumberNode>(l_val - r_val);
            if (op == '*') return arena.alloc<NumberNode>(l_val * r_val);
            if (op == '/' && r_val != 0) return arena.alloc<NumberNode>(l_val / r_val);
            if (op == '^') return arena.alloc<NumberNode>(std::pow(l_val, r_val));
        }

        if (op == '+') {
            if (IsConst(simple_right, 0.0)) return simple_left;
            if (IsConst(simple_left, 0.0)) return simple_right;
            if (simple_left->ToString(Precedence::None) == simple_right->ToString(Precedence::None)) 
                return arena.alloc<BinaryOpNode>('*', arena.alloc<NumberNode>(2.0), simple_left);
        }
        else if (op == '*') {
            if (IsConst(simple_right, 0.0) || IsConst(simple_left, 0.0)) return arena.alloc<NumberNode>(0.0);
            if (IsConst(simple_right, 1.0)) return simple_left;
            if (IsConst(simple_left, 1.0)) return simple_right;
        }
        else if (op == '^') {
            if (IsConst(simple_right, 1.0)) return simple_left;
            if (IsConst(simple_right, 0.0)) return arena.alloc<NumberNode>(1.0);
        }
        else if (op == '/') {
            if (IsConst(simple_left, 0.0)) return arena.alloc<NumberNode>(0.0);
            if (IsConst(simple_right, 1.0)) return simple_left;
            if (simple_left->ToString(Precedence::None) == simple_right->ToString(Precedence::None)) 
                return arena.alloc<NumberNode>(1.0);
        }

        return arena.alloc<BinaryOpNode>(op, simple_left, simple_right);
    }

    std::string ToString(Precedence parent_prec) const override {
        Precedence my_prec = GetOpPrecedence(op);
        std::string result = left->ToString(my_prec) + " " + op + " " + right->ToString(my_prec); 
        if (static_cast<int>(my_prec) < static_cast<int>(parent_prec)) return "(" + result + ")";
        return result;
    }
};

struct UnaryOpNode : ExprNode {
    std::string_view func; NodePtr operand;
    UnaryOpNode(std::string_view f, NodePtr op) : func(f), operand(op) {}
    NodeType GetType() const override { return NodeType::UnaryOp; }
    
    /**
     * @brief O(1) function lookup table for mathematical operations
     * COGNITIVE COMPLEXITY FIX: Replaced massive if-else chain with hash map lookup
     * FIX: Changed to std::string keys to avoid string_view comparison issues with arena-allocated strings
     */
    static const std::unordered_map<std::string, std::function<double(double)>>& GetFunctionMap() {
        static const std::unordered_map<std::string, std::function<double(double)>> func_map = {
            // Trigonometric functions (degrees)
            {"sin", [](double x) { return std::sin(x * D2R); }},
            {"cos", [](double x) { return std::cos(x * D2R); }},
            {"tan", [](double x) { return std::tan(x * D2R); }},
            {"cot", [](double x) { return 1.0 / std::tan(x * D2R); }},
            {"sec", [](double x) { return 1.0 / std::cos(x * D2R); }},
            {"csc", [](double x) { return 1.0 / std::sin(x * D2R); }},
            
            // Inverse trigonometric functions (return degrees)
            {"asin", [](double x) { return std::asin(x) * R2D; }},
            {"acos", [](double x) { return std::acos(x) * R2D; }},
            {"atan", [](double x) { return std::atan(x) * R2D; }},
            {"acot", [](double x) { return std::atan(1.0 / x) * R2D; }},
            {"asec", [](double x) { return std::acos(1.0 / x) * R2D; }},
            {"acsc", [](double x) { return std::asin(1.0 / x) * R2D; }},
            
            // Hyperbolic functions
            {"sinh", [](double x) { return std::sinh(x); }},
            {"cosh", [](double x) { return std::cosh(x); }},
            {"tanh", [](double x) { return std::tanh(x); }},
            {"coth", [](double x) { return 1.0 / std::tanh(x); }},
            {"sech", [](double x) { return 1.0 / std::cosh(x); }},
            {"csch", [](double x) { return 1.0 / std::sinh(x); }},
            
            // Inverse hyperbolic functions
            {"asinh", [](double x) { return std::asinh(x); }},
            {"acosh", [](double x) { return std::acosh(x); }},
            {"atanh", [](double x) { return std::atanh(x); }},
            {"acoth", [](double x) { return std::atanh(1.0 / x); }},
            {"asech", [](double x) { return std::acosh(1.0 / x); }},
            {"acsch", [](double x) { return std::asinh(1.0 / x); }},
            
            // Root and power functions
            {"sqrt", [](double x) { return std::sqrt(x); }},
            {"cbrt", [](double x) { return std::cbrt(x); }},
            
            // Logarithmic and exponential
            {"ln", [](double x) { return std::log(x); }},
            {"log", [](double x) { return std::log10(x); }},
            {"log2", [](double x) { return std::log2(x); }},
            {"lg", [](double x) { return std::log2(x); }},
            {"exp", [](double x) { return std::exp(x); }},
            
            // Other functions
            {"abs", [](double x) { return std::abs(x); }},
            {"u-", [](double x) { return -x; }},
        };
        return func_map;
    }
    
    EvalResult Evaluate(const StringUnorderedMap<AXIOM::Number>& vars) const override {
        auto inner = operand->Evaluate(vars);
        if (!inner.HasValue()) return inner;
        
        double val = AXIOM::GetReal(*inner.value);
        
        // Special cases that need validation before execution
        if (func == "sqrt" && val < 0) {
            return EvalResult::Failure(CalcErr::NegativeRoot);
        }
        if ((func == "ln" || func == "log" || func == "log2" || func == "lg") && val <= 0) {
            return EvalResult::Failure(CalcErr::DomainError);
        }
        
        // Factorial requires special handling (not in lookup table)
        if (func == "factorial") {
            if (val < 0 || val != std::floor(val) || val > 170) {
                return EvalResult::Failure(CalcErr::DomainError);
            }
            double result = 1.0;
            for (int i = 2; i <= static_cast<int>(val); ++i) {
                result *= i;
            }
            return EvalResult::Success(result);
        }
        
        const auto& func_map = GetFunctionMap();
        if (auto it = func_map.find(std::string(func)); it != func_map.end()) {
            try {
                double result = it->second(val);
                if (!std::isfinite(result)) {
                    return EvalResult::Failure(CalcErr::DomainError);
                }
                return EvalResult::Success(result);
            } catch (const std::domain_error&) {
                return EvalResult::Failure(CalcErr::DomainError);
            } catch (const std::invalid_argument&) {
                return EvalResult::Failure(CalcErr::DomainError);
            }
        }
        
        // Unknown function
        return EvalResult::Success(0.0);
    }
    
    using DerivFunc = std::function<NodePtr(Arena&, NodePtr, NodePtr)>;
    
    static const std::unordered_map<std::string, DerivFunc>& GetDerivativeMap() {
        static const std::unordered_map<std::string, DerivFunc> deriv_map = {
            {"u-", [](Arena& a, NodePtr op, NodePtr d_in) { return a.alloc<UnaryOpNode>("u-", d_in); }},
            {"sin", [](Arena& a, NodePtr op, NodePtr d_in) { 
                return a.alloc<BinaryOpNode>('*', a.alloc<UnaryOpNode>("cos", op), d_in); 
            }},
            {"cos", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto sin_u = a.alloc<UnaryOpNode>("sin", op);
                auto neg_sin = a.alloc<UnaryOpNode>("u-", sin_u);
                return a.alloc<BinaryOpNode>('*', neg_sin, d_in);
            }},
            {"tan", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto sec_u = a.alloc<UnaryOpNode>("sec", op);
                auto sec_sq = a.alloc<BinaryOpNode>('^', sec_u, a.alloc<NumberNode>(2.0));
                return a.alloc<BinaryOpNode>('*', sec_sq, d_in);
            }},
            {"cot", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto csc_u = a.alloc<UnaryOpNode>("csc", op);
                auto csc_sq = a.alloc<BinaryOpNode>('^', csc_u, a.alloc<NumberNode>(2.0));
                auto neg = a.alloc<UnaryOpNode>("u-", csc_sq);
                return a.alloc<BinaryOpNode>('*', neg, d_in);
            }},
            {"sec", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto sec_u = a.alloc<UnaryOpNode>("sec", op);
                auto tan_u = a.alloc<UnaryOpNode>("tan", op);
                auto prod = a.alloc<BinaryOpNode>('*', sec_u, tan_u);
                return a.alloc<BinaryOpNode>('*', prod, d_in);
            }},
            {"csc", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto csc_u = a.alloc<UnaryOpNode>("csc", op);
                auto cot_u = a.alloc<UnaryOpNode>("cot", op);
                auto prod = a.alloc<BinaryOpNode>('*', csc_u, cot_u);
                auto neg = a.alloc<UnaryOpNode>("u-", prod);
                return a.alloc<BinaryOpNode>('*', neg, d_in);
            }},
            {"ln", [](Arena& a, NodePtr op, NodePtr d_in) { 
                return a.alloc<BinaryOpNode>('/', d_in, op); 
            }},
            {"log2", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto ln2 = a.alloc<NumberNode>(std::log(2.0));
                auto denom = a.alloc<BinaryOpNode>('*', op, ln2);
                return a.alloc<BinaryOpNode>('/', d_in, denom);
            }},
            {"lg", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto ln2 = a.alloc<NumberNode>(std::log(2.0));
                auto denom = a.alloc<BinaryOpNode>('*', op, ln2);
                return a.alloc<BinaryOpNode>('/', d_in, denom);
            }},
            {"sqrt", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto two = a.alloc<NumberNode>(2.0);
                auto sqrt_u = a.alloc<UnaryOpNode>("sqrt", op);
                auto denom = a.alloc<BinaryOpNode>('*', two, sqrt_u);
                return a.alloc<BinaryOpNode>('/', d_in, denom);
            }},
            {"exp", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto exp_u = a.alloc<UnaryOpNode>("exp", op);
                return a.alloc<BinaryOpNode>('*', exp_u, d_in);
            }},
            {"asin", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto one = a.alloc<NumberNode>(1.0);
                auto inner_sq = a.alloc<BinaryOpNode>('^', op, a.alloc<NumberNode>(2.0));
                auto radicand = a.alloc<BinaryOpNode>('-', one, inner_sq);
                auto denom = a.alloc<UnaryOpNode>("sqrt", radicand);
                return a.alloc<BinaryOpNode>('/', d_in, denom);
            }},
            {"acos", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto one = a.alloc<NumberNode>(1.0);
                auto inner_sq = a.alloc<BinaryOpNode>('^', op, a.alloc<NumberNode>(2.0));
                auto radicand = a.alloc<BinaryOpNode>('-', one, inner_sq);
                auto denom = a.alloc<UnaryOpNode>("sqrt", radicand);
                auto neg = a.alloc<UnaryOpNode>("u-", a.alloc<BinaryOpNode>('/', d_in, denom));
                return neg;
            }},
            {"atan", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto one = a.alloc<NumberNode>(1.0);
                auto inner_sq = a.alloc<BinaryOpNode>('^', op, a.alloc<NumberNode>(2.0));
                auto denom = a.alloc<BinaryOpNode>('+', one, inner_sq);
                return a.alloc<BinaryOpNode>('/', d_in, denom);
            }},
            {"acot", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto one = a.alloc<NumberNode>(1.0);
                auto inner_sq = a.alloc<BinaryOpNode>('^', op, a.alloc<NumberNode>(2.0));
                auto denom = a.alloc<BinaryOpNode>('-', inner_sq, one);
                auto neg = a.alloc<UnaryOpNode>("u-", a.alloc<BinaryOpNode>('/', d_in, denom));
                return neg;
            }},
            {"asec", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto one = a.alloc<NumberNode>(1.0);
                auto inner_sq = a.alloc<BinaryOpNode>('^', op, a.alloc<NumberNode>(2.0));
                auto radicand = a.alloc<BinaryOpNode>('-', inner_sq, one);
                auto sqrt = a.alloc<UnaryOpNode>("sqrt", radicand);
                auto denom = a.alloc<BinaryOpNode>('*', op, sqrt);
                return a.alloc<BinaryOpNode>('/', d_in, denom);
            }},
            {"acsc", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto one = a.alloc<NumberNode>(1.0);
                auto inner_sq = a.alloc<BinaryOpNode>('^', op, a.alloc<NumberNode>(2.0));
                auto radicand = a.alloc<BinaryOpNode>('-', inner_sq, one);
                auto sqrt = a.alloc<UnaryOpNode>("sqrt", radicand);
                auto denom = a.alloc<BinaryOpNode>('*', op, sqrt);
                auto neg = a.alloc<UnaryOpNode>("u-", a.alloc<BinaryOpNode>('/', d_in, denom));
                return neg;
            }},
            {"sinh", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto cosh_u = a.alloc<UnaryOpNode>("cosh", op);
                return a.alloc<BinaryOpNode>('*', cosh_u, d_in);
            }},
            {"cosh", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto sinh_u = a.alloc<UnaryOpNode>("sinh", op);
                return a.alloc<BinaryOpNode>('*', sinh_u, d_in);
            }},
            {"tanh", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto sech_u = a.alloc<UnaryOpNode>("sech", op);
                auto sech_sq = a.alloc<BinaryOpNode>('^', sech_u, a.alloc<NumberNode>(2.0));
                return a.alloc<BinaryOpNode>('*', sech_sq, d_in);
            }},
            {"coth", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto csch_u = a.alloc<UnaryOpNode>("csch", op);
                auto csch_sq = a.alloc<BinaryOpNode>('^', csch_u, a.alloc<NumberNode>(2.0));
                auto neg = a.alloc<UnaryOpNode>("u-", csch_sq);
                return a.alloc<BinaryOpNode>('*', neg, d_in);
            }},
            {"sech", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto sech_u = a.alloc<UnaryOpNode>("sech", op);
                auto tanh_u = a.alloc<UnaryOpNode>("tanh", op);
                auto prod = a.alloc<BinaryOpNode>('*', sech_u, tanh_u);
                auto neg = a.alloc<UnaryOpNode>("u-", prod);
                return a.alloc<BinaryOpNode>('*', neg, d_in);
            }},
            {"csch", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto csch_u = a.alloc<UnaryOpNode>("csch", op);
                auto coth_u = a.alloc<UnaryOpNode>("coth", op);
                auto prod = a.alloc<BinaryOpNode>('*', csch_u, coth_u);
                auto neg = a.alloc<UnaryOpNode>("u-", prod);
                return a.alloc<BinaryOpNode>('*', neg, d_in);
            }},
            {"asinh", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto inner_sq = a.alloc<BinaryOpNode>('^', op, a.alloc<NumberNode>(2.0));
                auto radicand = a.alloc<BinaryOpNode>('+', inner_sq, a.alloc<NumberNode>(1.0));
                auto sqrt = a.alloc<UnaryOpNode>("sqrt", radicand);
                return a.alloc<BinaryOpNode>('/', d_in, sqrt);
            }},
            {"acosh", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto one = a.alloc<NumberNode>(1.0);
                auto minus = a.alloc<BinaryOpNode>('-', op, one);
                auto plus = a.alloc<BinaryOpNode>('+', op, one);
                auto sqrt1 = a.alloc<UnaryOpNode>("sqrt", minus);
                auto sqrt2 = a.alloc<UnaryOpNode>("sqrt", plus);
                auto denom = a.alloc<BinaryOpNode>('*', sqrt1, sqrt2);
                return a.alloc<BinaryOpNode>('/', d_in, denom);
            }},
            {"atanh", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto one = a.alloc<NumberNode>(1.0);
                auto inner_sq = a.alloc<BinaryOpNode>('^', op, a.alloc<NumberNode>(2.0));
                auto denom = a.alloc<BinaryOpNode>('-', one, inner_sq);
                return a.alloc<BinaryOpNode>('/', d_in, denom);
            }},
            {"acoth", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto one = a.alloc<NumberNode>(1.0);
                auto inner_sq = a.alloc<BinaryOpNode>('^', op, a.alloc<NumberNode>(2.0));
                auto denom = a.alloc<BinaryOpNode>('-', one, inner_sq);
                return a.alloc<BinaryOpNode>('/', d_in, denom);
            }},
            {"asech", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto one = a.alloc<NumberNode>(1.0);
                auto inner_sq = a.alloc<BinaryOpNode>('^', op, a.alloc<NumberNode>(2.0));
                auto radicand = a.alloc<BinaryOpNode>('-', one, inner_sq);
                auto sqrt = a.alloc<UnaryOpNode>("sqrt", radicand);
                auto denom = a.alloc<BinaryOpNode>('*', op, sqrt);
                auto neg = a.alloc<UnaryOpNode>("u-", a.alloc<BinaryOpNode>('/', d_in, denom));
                return neg;
            }},
            {"acsch", [](Arena& a, NodePtr op, NodePtr d_in) { 
                auto one = a.alloc<NumberNode>(1.0);
                auto inner_sq = a.alloc<BinaryOpNode>('^', op, a.alloc<NumberNode>(2.0));
                auto radicand = a.alloc<BinaryOpNode>('+', inner_sq, one);
                auto sqrt = a.alloc<UnaryOpNode>("sqrt", radicand);
                auto denom = a.alloc<BinaryOpNode>('*', op, sqrt);
                auto neg = a.alloc<UnaryOpNode>("u-", a.alloc<BinaryOpNode>('/', d_in, denom));
                return neg;
            }}
        };
        return deriv_map;
    }

    NodePtr Derivative(Arena& arena, std::string_view var) const override {
        auto d_inner = operand->Derivative(arena, var);
        const auto& deriv_map = GetDerivativeMap();
        if (auto it = deriv_map.find(std::string(func)); it != deriv_map.end()) {
            return it->second(arena, operand, d_inner);
        }
        return arena.alloc<NumberNode>(0.0);
    }
    
    NodePtr Simplify(Arena& arena) const override {
        auto simple_inner = operand->Simplify(arena);
        return arena.alloc<UnaryOpNode>(func, simple_inner);
    }

    std::string ToString(Precedence) const override {
        if (func == "u-") return "-" + operand->ToString(Precedence::Unary);
        return std::string(func) + "(" + operand->ToString(Precedence::None) + ")";
    }
};

// ========================================================
// MULTI-ARGUMENT FUNCTION NODE (FOR CALCULUS OPERATIONS)
// ========================================================
struct MultiArgFunctionNode : ExprNode {
    std::string_view func;
    std::vector<NodePtr> args;
    
    MultiArgFunctionNode(std::string_view f, std::vector<NodePtr> arguments) 
        : func(f), args(std::move(arguments)) {}
        
    NodeType GetType() const override { return NodeType::MultiArgFunction; }
    
    EvalResult Evaluate(const StringUnorderedMap<AXIOM::Number>& vars) const override {
        if (func == "limit") {
            if (args.size() != 3) return EvalResult::Failure(CalcErr::ArgumentMismatch);
            
            // limit(expression, variable, point)
            // Use epsilon-delta numerical limit calculation
            if (args[1]->GetType() != NodeType::Variable) return EvalResult::Failure(CalcErr::ArgumentMismatch);
            auto var_node = static_cast<VariableNode*>(args[1]);
            
            std::string var_name = std::string(var_node->name);
            auto point_result = args[2]->Evaluate(vars);
            if (!point_result.HasValue()) return point_result;
            double approach_point = AXIOM::GetReal(*point_result.value);
            
            // Check for infinite limit
            if (std::isinf(approach_point)) {
                return EvaluateLimitAtInfinity(vars, var_name, approach_point > 0);
            }
            
            return EvaluateNumericalLimit(vars, var_name, approach_point);
        }
        
        if (func == "integrate") {
            if (args.size() != 4) return EvalResult::Failure(CalcErr::ArgumentMismatch);
            
            // integrate(expression, variable, lower_bound, upper_bound)
            if (args[1]->GetType() != NodeType::Variable) return EvalResult::Failure(CalcErr::ArgumentMismatch);
            auto var_node = static_cast<VariableNode*>(args[1]);
            
            std::string var_name = std::string(var_node->name);
            auto lower_result = args[2]->Evaluate(vars);
            auto upper_result = args[3]->Evaluate(vars);
            
            if (!lower_result.HasValue() || !upper_result.HasValue()) {
                return EvalResult::Failure(CalcErr::DomainError);
            }
            
            double a = AXIOM::GetReal(*lower_result.value);
            double b = AXIOM::GetReal(*upper_result.value);
            
            // Check for improper integrals
            if (std::isinf(a) || std::isinf(b)) {
                return EvaluateImproperIntegral(vars, var_name, a, b);
            }
            
            return EvaluateNumericalIntegral(vars, var_name, a, b);
        }
        
        // Basic multi-argument functions
        if (func == "max") {
            if (args.empty()) return EvalResult::Failure(CalcErr::ArgumentMismatch);
            double max_val = -std::numeric_limits<double>::infinity();
            for (const auto& arg : args) {
                auto result = arg->Evaluate(vars);
                if (!result.HasValue()) return result;
                max_val = std::max(max_val, AXIOM::GetReal(*result.value));
            }
            return EvalResult::Success(max_val);
        }
        
        if (func == "min") {
            if (args.empty()) return EvalResult::Failure(CalcErr::ArgumentMismatch);
            double min_val = std::numeric_limits<double>::infinity();
            for (const auto& arg : args) {
                auto result = arg->Evaluate(vars);
                if (!result.HasValue()) return result;
                min_val = std::min(min_val, AXIOM::GetReal(*result.value));
            }
            return EvalResult::Success(min_val);
        }
        
        if (func == "gcd") {
            if (args.size() != 2) return EvalResult::Failure(CalcErr::ArgumentMismatch);
            auto a_result = args[0]->Evaluate(vars);
            auto b_result = args[1]->Evaluate(vars);
            if (!a_result.HasValue() || !b_result.HasValue()) return EvalResult::Failure(CalcErr::ArgumentMismatch);
            
            long long a = static_cast<long long>(AXIOM::GetReal(*a_result.value));
            long long b = static_cast<long long>(AXIOM::GetReal(*b_result.value));
            a = std::abs(a); b = std::abs(b);
            
            while (b != 0) {
                long long temp = b;
                b = a % b;
                a = temp;
            }
            return EvalResult::Success(static_cast<double>(a));
        }
        
        if (func == "lcm") {
            if (args.size() != 2) return EvalResult::Failure(CalcErr::ArgumentMismatch);
            auto a_result = args[0]->Evaluate(vars);
            auto b_result = args[1]->Evaluate(vars);
            if (!a_result.HasValue() || !b_result.HasValue()) return EvalResult::Failure(CalcErr::ArgumentMismatch);
            
            long long a = static_cast<long long>(AXIOM::GetReal(*a_result.value));
            long long b = static_cast<long long>(AXIOM::GetReal(*b_result.value));
            a = std::abs(a); b = std::abs(b);
            
            if (a == 0 || b == 0) return EvalResult::Success(0.0);
            
            // Calculate GCD first
            long long gcd_val = a;
            long long temp_b = b;
            while (temp_b != 0) {
                long long temp = temp_b;
                temp_b = gcd_val % temp_b;
                gcd_val = temp;
            }
            
            long long lcm_val = (a / gcd_val) * b;
            return EvalResult::Success(static_cast<double>(lcm_val));
        }
        
        if (func == "mod" || func == "modulo") {
            if (args.size() != 2) return EvalResult::Failure(CalcErr::ArgumentMismatch);
            auto a_result = args[0]->Evaluate(vars);
            auto b_result = args[1]->Evaluate(vars);
            if (!a_result.HasValue() || !b_result.HasValue()) return EvalResult::Failure(CalcErr::ArgumentMismatch);
            
            double a = AXIOM::GetReal(*a_result.value);
            double b = AXIOM::GetReal(*b_result.value);
            if (b == 0) return EvalResult::Failure(CalcErr::DivideByZero);
            
            return EvalResult::Success(std::fmod(a, b));
        }
        
        return EvalResult::Failure(CalcErr::OperationNotFound);
    }
    
    NodePtr Derivative(Arena& arena, std::string_view var) const override {
        // Fundamental Theorem of Calculus for integrals
        if (func == "integrate" && args.size() == 4) {
            if (args[1]->GetType() == NodeType::Variable) {
                auto var_node = static_cast<VariableNode*>(args[1]);
                if (var_node->name == var) {
                    // d/dx âˆ«[a(x)]^[b(x)] f(t) dt = f(b(x))Â·b'(x) - f(a(x))Â·a'(x)
                    auto f_at_b = args[0]; // Need to substitute var with upper bound
                    auto f_at_a = args[0]; // Need to substitute var with lower bound
                    auto b_prime = args[3]->Derivative(arena, var);
                    auto a_prime = args[2]->Derivative(arena, var);
                    
                    // This is a simplified implementation
                    // In practice, we'd need proper substitution
                    return arena.alloc<NumberNode>(0.0); // Placeholder
                }
            }
        }
        
        // For limits, derivative is complex and context-dependent
        return arena.alloc<NumberNode>(0.0); // Placeholder
    }
    
    NodePtr Simplify(Arena& arena) const override {
        std::vector<NodePtr> simplified_args;
        simplified_args.reserve(args.size());
        for (const auto& arg : args) {
            simplified_args.emplace_back(arg->Simplify(arena));
        }
        return arena.alloc<MultiArgFunctionNode>(func, std::move(simplified_args));
    }
    
    std::string ToString(Precedence) const override {
        std::string result = std::string(func) + "(";
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) result += ", ";
            result += args[i]->ToString(Precedence::None);
        }
        result += ")";
        return result;
    }

private:
    EvalResult EvaluateNumericalLimit(const StringUnorderedMap<AXIOM::Number>& vars, 
                                    const std::string& var_name, double approach_point) const {
        constexpr double epsilon = 1e-6;  // Relaxed tolerance
        constexpr int max_iterations = 20; // Reduced iterations for faster convergence
        
        // Cache the variable map to prevent continuous hashing inside the iteration loop
        StringUnorderedMap<AXIOM::Number> local_vars = vars;
        
        // Insert a dummy value initially to ensure the hash bucket is allocated before looping
        local_vars[var_name] = AXIOM::Number(approach_point);
        // Take a direct reference to the bucket's value payload to bypass string lookups
        AXIOM::Number& cached_var = local_vars[var_name];
        
        auto evaluate_at = [&](double x) -> std::optional<double> {
            cached_var = AXIOM::Number(x); // O(1) in-place memory mutation (No hashing or strings)
            auto result = args[0]->Evaluate(local_vars);
            return result.HasValue() ? std::optional<double>(AXIOM::GetReal(*result.value)) : std::nullopt;
        };
        
        // Try direct evaluation first (for continuous functions)
        auto direct_eval = evaluate_at(approach_point);
        if (direct_eval.has_value() && std::isfinite(*direct_eval)) {
            return EvalResult::Success(*direct_eval);
        }
        
        // Approach from both sides with progressively smaller steps
        std::optional<double> left_limit, right_limit;
        
        for (int i = 1; i <= max_iterations; ++i) {
            double h = std::pow(0.1, i);  // More aggressive step reduction
            
            // Left approach
            auto left_val = evaluate_at(approach_point - h);
            if (left_val.has_value() && std::isfinite(*left_val)) {
                left_limit = *left_val;
            }
            
            // Right approach  
            auto right_val = evaluate_at(approach_point + h);
            if (right_val.has_value() && std::isfinite(*right_val)) {
                right_limit = *right_val;
            }
            
            // Check convergence
            if (left_limit.has_value() && right_limit.has_value()) {
                if (std::abs(*left_limit - *right_limit) < epsilon) {
                    return EvalResult::Success((*left_limit + *right_limit) / 2.0);
                }
            }
        }
        
        
        if (left_limit.has_value()) return EvalResult::Success(*left_limit);
        if (right_limit.has_value()) return EvalResult::Success(*right_limit);
        
        return EvalResult::Failure(CalcErr::IndeterminateResult);
    }
    
    EvalResult EvaluateLimitAtInfinity(const StringUnorderedMap<AXIOM::Number>& vars,
                                     const std::string& var_name, bool positive_infinity) const {
        constexpr int max_iterations = 20;
        
        // Cache map buckets to skip string hashes
        StringUnorderedMap<AXIOM::Number> local_vars = vars;
        local_vars[var_name] = AXIOM::Number(0.0);
        AXIOM::Number& cached_var = local_vars[var_name];
        
        auto evaluate_at = [&](double x) -> std::optional<double> {
            cached_var = AXIOM::Number(x);
            auto result = args[0]->Evaluate(local_vars);
            return result.HasValue() ? std::optional<double>(AXIOM::GetReal(*result.value)) : std::nullopt;
        };
        
        std::optional<double> prev_val;
        
        for (int i = 1; i <= max_iterations; ++i) {
            double x = positive_infinity ? std::pow(10.0, i) : -std::pow(10.0, i);
            auto current_val = evaluate_at(x);
            
            if (!current_val.has_value() || !std::isfinite(*current_val)) {
                continue;
            }
            
            if (prev_val.has_value()) {
                double diff = std::abs(*current_val - *prev_val);
                if (diff < 1e-10) {
                    return EvalResult::Success(*current_val);
                }
            }
            prev_val = *current_val;
        }
        
        // Check for infinite limits
        if (prev_val.has_value()) {
            if (std::abs(*prev_val) > 1e10) {
                return EvalResult::Success(positive_infinity ? 
                    std::numeric_limits<double>::infinity() : 
                    -std::numeric_limits<double>::infinity());
            }
            return EvalResult::Success(*prev_val);
        }
        
        return EvalResult::Failure(CalcErr::IndeterminateResult);
    }
    
    EvalResult EvaluateNumericalIntegral(const StringUnorderedMap<AXIOM::Number>& vars,
                                       const std::string& var_name, double a, double b) const {
        // Adaptive Simpson's Rule with error control
        constexpr double tolerance = 1e-12;
        constexpr int max_recursion = 15;
        
        // Optimization: O(1) bucket cache bypassing string hashing in innermost loop (thousands of hits per integral)
        StringUnorderedMap<AXIOM::Number> local_vars = vars;
        local_vars[var_name] = AXIOM::Number(a); 
        AXIOM::Number& cached_var = local_vars[var_name];
        
        auto f = [&](double x) -> double {
            cached_var = AXIOM::Number(x); // Direct reference write
            auto result = args[0]->Evaluate(local_vars);
            return result.HasValue() ? AXIOM::GetReal(*result.value) : 0.0;
        };
        
        std::function<double(double, double, double, double, double, int)> simpson_adaptive = 
            [&](double a, double b, double fa, double fb, double fc, int depth) -> double {
                
            double h = (b - a) / 2.0;
            double c = a + h;
            double fd = f(a + h/2.0);
            double fe = f(c + h/2.0);
            
            double S1 = h/3.0 * (fa + 4*fc + fb);  // Original estimate
            double S2 = h/6.0 * (fa + 4*fd + 2*fc + 4*fe + fb);  // Refined estimate
            
            if (depth >= max_recursion || std::abs(S2 - S1) < 15*tolerance) {
                return S2 + (S2 - S1)/15.0;  // Richardson extrapolation
            }
            
            return simpson_adaptive(a, c, fa, fc, fd, depth+1) + 
                   simpson_adaptive(c, b, fc, fb, fe, depth+1);
        };
        
        try {
            double fa = f(a);
            double fb = f(b);
            double fc = f((a + b) / 2.0);
            
            // Check for discontinuities or infinite values
            if (!std::isfinite(fa) || !std::isfinite(fb) || !std::isfinite(fc)) {
                return EvalResult::Failure(CalcErr::DomainError);
            }
            
            double result = simpson_adaptive(a, b, fa, fb, fc, 0);
            return EvalResult::Success(result);
            
        } catch (const std::domain_error&) {
            return EvalResult::Failure(CalcErr::DomainError);
        }
    }
    
    EvalResult EvaluateImproperIntegral(const StringUnorderedMap<AXIOM::Number>& vars,
                                      const std::string& var_name, double a, double b) const {
        // Handle improper integrals by taking limits
        constexpr double large_val = 1e6;
        
        double effective_a = std::isinf(a) ? (a > 0 ? large_val : -large_val) : a;
        double effective_b = std::isinf(b) ? (b > 0 ? large_val : -large_val) : b;
        
        return EvaluateNumericalIntegral(vars, var_name, effective_a, effective_b);
    }
};

// ========================================================
// ALGEBRAIC PARSER IMPLEMENTATION
// ========================================================

AlgebraicParser::AlgebraicParser() { RegisterSpecialCommands(); }

void AlgebraicParser::RegisterSpecialCommands() {
    special_commands_.emplace_back("quadratic", [this](const std::string& s){ return HandleQuadratic(s); });
    special_commands_.emplace_back("solve_nl",  [this](const std::string& s){ return HandleNonLinearSolve(s); });
    special_commands_.emplace_back("derive",    [this](const std::string& s){ return HandleDerivative(s); });
}

// helper used by multiple parsing locations to split comma-separated arguments
// accepts a parsing function so it can recursively call into the current parser instance
static std::vector<NodePtr> SplitArgsWithParen(std::string_view args_str,
                                               const std::function<NodePtr(std::string_view)>& parse) {
    std::vector<NodePtr> args;
    size_t start = 0;
    int paren_depth = 0;
    for (size_t i = 0; i <= args_str.size(); ++i) {
        char c = (i < args_str.size()) ? args_str[i] : ','; // end treat as comma
        if (c == '(') paren_depth++;
        else if (c == ')') paren_depth--;
        else if (c == ',' && paren_depth == 0) {
            auto arg_str = std::string_view(args_str).substr(start, i - start);
            while (!arg_str.empty() && std::isspace(static_cast<unsigned char>(arg_str.front()))) arg_str.remove_prefix(1);
            while (!arg_str.empty() && std::isspace(static_cast<unsigned char>(arg_str.back()))) arg_str.remove_suffix(1);
            if (!arg_str.empty()) {
                args.push_back(parse(arg_str));
            }
            start = i + 1;
        }
    }
    return args;
}


NodePtr AlgebraicParser::ParseExpression(std::string_view input) {
    std::string trimmed = Utils::Trim(input);
    input = trimmed; // work on trimmed copy as string_view

    auto parse_binary = [&](std::string_view operators, bool right_to_left) -> NodePtr {
        int bracket_depth = 0;
        int start = right_to_left ? input.size() - 1 : 0;
        int end = right_to_left ? -1 : input.size();
        int step = right_to_left ? -1 : 1;

        for (int i = start; i != end; i += step) {
            char c = input[i];
            if (c == ')') {
                bracket_depth++;
                continue;
            }
            if (c == '(') {
                bracket_depth--;
                continue;
            }
            if (bracket_depth != 0) continue;

            if (operators.find(c) == std::string_view::npos) continue;

            // Check if this is actually a unary operator (specifically for +/-)
            if ((c == '-' || c == '+') && i == 0) continue; // Skip unary at start

            if ((c == '-' || c == '+') && i > 0) {
                char prev = input[i-1];
                if (prev == '(' || prev == '+' || prev == '-' || prev == '*' || prev == '/' || prev == '^') {
                    continue; // Skip unary after operator
                }
            }
            
            // This is a binary operator
            return arena_.alloc<BinaryOpNode>(c, 
                ParseExpression(input.substr(0, i)), 
                ParseExpression(input.substr(i + 1)));
        }
        return nullptr;
    };

    if (auto node = parse_binary("+-", true)) return node;
    if (auto node = parse_binary("*/", true)) return node;

    // Handle unary operators (after binary parsing fails)
    if (!input.empty() && input.front() == '-') {
        // This is a unary minus
        auto operand = ParseExpression(input.substr(1));
        return arena_.alloc<UnaryOpNode>("u-", operand);
    }
    
    if (!input.empty() && input.front() == '+') {
        // Unary plus (identity operator) - just skip it
        return ParseExpression(input.substr(1));
    }

    // Implicit Mult
    if (input.size() > 1) { 
        int bracket_depth = 0;
        for (size_t i = 0; i < input.size() - 1; ++i) {
            char curr = input[i];
            char next = input[i+1];
            if (curr == '(') bracket_depth++;
            else if (curr == ')') bracket_depth--;
            
            if (bracket_depth != 0) continue;

            bool digit_alpha = std::isdigit(static_cast<unsigned char>(curr)) && std::isalpha(static_cast<unsigned char>(next));
            bool prev_is_ident = false;
            if (i > 0) {
                unsigned char prev = static_cast<unsigned char>(input[i - 1]);
                prev_is_ident = std::isalnum(prev) || prev == '_';
            }
            // Treat N( ... ) as implicit multiplication, but avoid splitting function names like log2(...).
            bool digit_paren = std::isdigit(static_cast<unsigned char>(curr)) && next == '(' && !prev_is_ident;
            bool paren_alpha = (curr == ')') && std::isalpha(static_cast<unsigned char>(next));
            bool paren_paren = (curr == ')') && next == '(';
            
            if (digit_alpha || digit_paren || paren_alpha || paren_paren) {
                return arena_.alloc<BinaryOpNode>('*', 
                        ParseExpression(input.substr(0, i + 1)), 
                        ParseExpression(input.substr(i + 1)));
            }
        }
    }

    if (auto node = parse_binary("^", false)) return node;

    if (input.size() >= 2 && input.front() == '(' && input.back() == ')') {
        return ParseExpression(input.substr(1, input.size() - 2));
    }

    size_t paren_start = input.find('(');
    if (paren_start != std::string_view::npos && input.back() == ')') {
        auto func_name = input.substr(0, paren_start);
        while(!func_name.empty() && std::isspace(static_cast<unsigned char>(func_name.back()))) func_name.remove_suffix(1);
        auto args_str = input.substr(paren_start + 1, input.size() - paren_start - 2);
        
        // Check if this is a multi-argument function
        if (func_name == "limit" || func_name == "integrate" || func_name == "plot" || 
            func_name == "max" || func_name == "min" || func_name == "gcd" || 
            func_name == "lcm" || func_name == "mod" || func_name == "modulo") {
            auto args = SplitArgsWithParen(args_str, [this](std::string_view v){ return ParseExpression(v); });
            return arena_.alloc<MultiArgFunctionNode>(arena_.allocString(func_name), std::move(args));
        } else {
            // Single-argument function (existing behavior)
            return arena_.alloc<UnaryOpNode>(arena_.allocString(func_name), ParseExpression(args_str));
        }
    }
    
    size_t space_pos = input.find(' ');
    if (space_pos != std::string_view::npos) {
        auto func_name = input.substr(0, space_pos);
        auto arg = input.substr(space_pos + 1);
        bool is_func = true;
        for(char c : func_name) if(!std::isalpha(c)) is_func = false;
        if (is_func && !func_name.empty()) {
             return arena_.alloc<UnaryOpNode>(arena_.allocString(func_name), ParseExpression(arg));
        }
    }

    if (Utils::IsNumber(input)) {
        return arena_.alloc<NumberNode>(std::stod(std::string(input)));
    } else {
        if (input.empty()) return arena_.alloc<NumberNode>(0.0);
        return arena_.alloc<VariableNode>(arena_.allocString(input));
    }
}

EngineResult AlgebraicParser::ParseAndExecute(const std::string& input) {
    StringUnorderedMap<AXIOM::Number> empty_context;
    return ParseAndExecuteWithContext(input, empty_context); 
}

/**
 * @brief Thread-safe expression parsing and execution with context
 * THREAD SAFETY: Uses shared_lock for cache reads, unique_lock for cache writes
 */
EngineResult AlgebraicParser::ParseAndExecuteWithContext(const std::string& input, const StringUnorderedMap<AXIOM::Number>& context) {
    // Basic syntax validation
    std::string trimmed = Utils::Trim(input);
    if (trimmed.empty()) {
        return CreateErrorResult(CalcErr::ParseError);
    }
    
    // THREAD SAFETY: Check cache with shared lock (concurrent reads allowed)
    {
        std::shared_lock<std::shared_mutex> read_lock(mutex_s);
        if (auto cache_it = eval_cache_.find(trimmed); cache_it != eval_cache_.end() && context.empty()) {
            const auto& cached = cache_it->second;
            if (cached.HasValue()) {
                double val = AXIOM::GetReal(*cached.value);
                return CreateSuccessResult(val);
            }
            EngineResult err_result;
            err_result.error = EngineErrorResult(cached.error);
            return err_result;
        }
    }
    
    // Check for invalid consecutive operators
    for (size_t i = 0; i < trimmed.size() - 1; ++i) {
        char c1 = trimmed[i];
        char c2 = trimmed[i + 1];
        if ((c1 == '+' || c1 == '-' || c1 == '*' || c1 == '/') && 
            (c2 == '+' || c2 == '*' || c2 == '/')) {
            return CreateErrorResult(CalcErr::ParseError);
        }
    }
    
    // Check for balanced parentheses
    int paren_count = 0;
    for (char c : trimmed) {
        if (c == '(') paren_count++;
        else if (c == ')') paren_count--;
        if (paren_count < 0) {
            return CreateErrorResult(CalcErr::ParseError);
        }
    }
    if (paren_count != 0) {
        return CreateErrorResult(CalcErr::ParseError);
    }
    
    // Check for unknown functions (basic validation) - OPTIMIZED to avoid slowdown with many functions
    static const std::set<std::string, std::less<>> known_functions = {
        "sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh",
        "asinh", "acosh", "atanh", "log", "ln", "log2", "exp", "sqrt", "cbrt",
        "abs", "factorial", "limit", "integrate", "max", "min", "gcd", "lcm",
        "mod", "modulo", "sec", "csc", "cot", "asec", "acsc", "acot",
        "sech", "csch", "coth", "asech", "acsch", "acoth"
    };
    
    // Optimized function validation - limit iterations for very long strings
    size_t pos = 0;
    int func_check_count = 0;
    const int max_func_checks = 1000;  // Safety limit to prevent O(nÂ²) behavior
    
    while ((pos = trimmed.find('(', pos)) != std::string::npos && func_check_count < max_func_checks) {
        func_check_count++;
        // Find the start of the potential function name
        size_t func_start = pos;
        while (func_start > 0 && (std::isalnum(static_cast<unsigned char>(trimmed[func_start - 1])) || trimmed[func_start - 1] == '_')) {
            func_start--;
        }
        
        if (func_start >= pos) {
            pos++;
            continue;
        }

        std::string func_name = trimmed.substr(func_start, pos - func_start);
        // Only validate function-like identifiers here. Numeric tokens such as
        // "2(...)" are handled later as implicit multiplication.
        unsigned char first_char = static_cast<unsigned char>(func_name.front());
        bool starts_like_identifier = std::isalpha(first_char) || first_char == '_';
        if (starts_like_identifier && !known_functions.contains(func_name)) {
            return CreateErrorResult(CalcErr::ParseError);
        }
        pos++;
    }
    
    // Arena reset and expression parsing
    arena_.reset();
    std::string processed_input = input; 
    std::stringstream ss(processed_input);
    std::string first_token;
    ss >> first_token;

    for (const auto& entry : special_commands_) {
        if (first_token == entry.command) {
            return entry.handler(processed_input);
        }
    }

    try {
        NodePtr root = ParseExpression(processed_input);
        auto evaluation = root->Evaluate(context);
        
        // THREAD SAFETY: Exclusive lock for writing to cache
        if (context.empty() && evaluation.HasValue()) {
            std::scoped_lock write_lock(mutex_s);
            
            // Limit cache size to prevent memory growth
            if (eval_cache_.size() >= MAX_CACHE_SIZE) {
                eval_cache_.clear();
            }
            eval_cache_[trimmed] = evaluation;
        }
        
        if (evaluation.value.has_value()) {
            double result_val = AXIOM::GetReal(*evaluation.value);
            return CreateSuccessResult(result_val);
        }
        CalcErr err = evaluation.error == CalcErr::None ? CalcErr::ArgumentMismatch : evaluation.error;
        EngineResult err_result;
        err_result.error = EngineErrorResult(err);
        return err_result;
    }
    catch (const std::invalid_argument&) {
        EngineResult err_result;
        err_result.error = EngineErrorResult(CalcErr::ParseError);
        return err_result;
    }
    catch (const std::domain_error&) {
        EngineResult err_result;
        err_result.error = EngineErrorResult(CalcErr::ArgumentMismatch);
        return err_result;
    }
}

EngineResult AlgebraicParser::HandleQuadratic(const std::string& input) {
    std::stringstream ss(input);
    std::string cmd;
    double a, b, c;
    ss >> cmd;
    if (!(ss >> a >> b >> c)) return CreateErrorResult(CalcErr::ArgumentMismatch);
    return SolveQuadratic(a, b, c);
}

EngineResult AlgebraicParser::HandleNonLinearSolve(const std::string& input) {
    auto open_brace = input.find('{');
    auto close_brace = input.find('}');
    if (open_brace == std::string::npos || close_brace == std::string::npos) return CreateErrorResult(CalcErr::ArgumentMismatch);
    std::string eq_content = input.substr(open_brace + 1, close_brace - open_brace - 1);
    
    auto open_bracket = input.find('[', close_brace);
    auto close_bracket = input.find(']', open_bracket);
    if (open_bracket == std::string::npos || close_bracket == std::string::npos) return CreateErrorResult(CalcErr::ArgumentMismatch);
    std::string guess_content = input.substr(open_bracket + 1, close_bracket - open_bracket - 1);

    auto raw_eqs = Utils::Split(eq_content, ';'); 
    std::vector<std::string> final_equations;
    for (const auto& raw_eq : raw_eqs) {
        std::string eq = Utils::Trim(raw_eq);
        if (eq.empty()) continue;
        size_t eq_sign_pos = eq.find('=');
        if (eq_sign_pos != std::string::npos) {
            std::string lhs = eq.substr(0, eq_sign_pos);
            std::string rhs = eq.substr(eq_sign_pos + 1);
            eq = "(" + lhs + ") - (" + rhs + ")";
        }
        final_equations.emplace_back(eq);
    }

    auto raw_guesses = Utils::Split(guess_content, ',');
    std::vector<double> guess_values;
    for (const auto& val_str : raw_guesses) {
        std::string trimmed = Utils::Trim(val_str);
        if(Utils::IsNumber(trimmed)) guess_values.emplace_back(std::stod(trimmed));
    }

    StringUnorderedMap<double> guess_map;
    std::vector<std::string> var_names = {"x", "y", "z", "a", "b", "c"};
    for(size_t i=0; i<guess_values.size(); ++i) {
        if(i < var_names.size()) guess_map[var_names[i]] = guess_values[i];
    }

    return SolveNonLinearSystem(final_equations, guess_map);
}

EngineResult AlgebraicParser::HandleDerivative(const std::string& input) {
    std::stringstream ss(input);
    std::string cmd, expression, var;
    ss >> cmd;
    std::getline(ss, expression);
    expression = Utils::Trim(expression);
    if (!expression.empty() && expression.back() == ';') expression.pop_back();
    var = "x"; 
    try {
        NodePtr root = ParseExpression(expression);
        NodePtr derivative = root->Derivative(arena_, var);
        NodePtr simplified = derivative->Simplify(arena_)->Simplify(arena_);
        return CreateSuccessResult(simplified->ToString(Precedence::None)); 
    } catch (const std::invalid_argument&) {
        EngineResult err_result;
        err_result.error = EngineErrorResult(CalcErr::ParseError);
        return err_result;
    }
}

EngineResult AlgebraicParser::SolveQuadratic(double a, double b, double c) {
    if (a == 0.0) {
        EngineResult err_result;
        err_result.error = EngineErrorResult(CalcErr::IndeterminateResult);
        return err_result;
    }
    double d = b * b - 4 * a * c;
    if (d < 0) {
        EngineResult err_result;
        err_result.error = EngineErrorResult(CalcErr::NegativeRoot);
        return err_result;
    }
    double s = std::sqrt(d);
    return CreateSuccessResult(Vector({(-b + s) / (2 * a), (-b - s) / (2 * a)}));
}

EngineResult AlgebraicParser::SolveNonLinearSystem(const std::vector<std::string>& equation_strs, StringUnorderedMap<double>& guess) {
    const int max_iter = 50;
    const double epsilon = 1e-5;
    std::vector<NodePtr> roots;
    for(const auto& eq : equation_strs) roots.emplace_back(ParseExpression(eq));
    std::vector<std::string> var_names;
    for(auto const& [key, val] : guess) var_names.emplace_back(key);
    int n = var_names.size();
    
    // Convert guess to AXIOM::Number context
    StringUnorderedMap<AXIOM::Number> num_guess;
    for(const auto& [key, val] : guess) {
        num_guess[key] = AXIOM::Number(val);
    }
    
    for (int iter = 0; iter < max_iter; ++iter) {
        std::vector<double> F(n);
        for(int i=0; i<n; ++i) {
            auto eval = roots[i]->Evaluate(num_guess);
            if (!eval.value.has_value()) {
                return CreateErrorResult(NormalizeError(eval, CalcErr::DomainError));
            }
            F[i] = AXIOM::GetReal(*eval.value);
        }
        double err = 0; for(double v:F) err+=v*v;
        if(std::sqrt(err) < 1e-6) break;
        std::vector<std::vector<double>> J(n, std::vector<double>(n));
        for (int j = 0; j < n; ++j) {
            std::string v = var_names[j];
            double old = AXIOM::GetReal(num_guess[v]);
            num_guess[v] = AXIOM::Number(old + epsilon);
            for (int i = 0; i < n; ++i) {
                auto eval = roots[i]->Evaluate(num_guess);
                if (!eval.value.has_value()) {
                    return CreateErrorResult(NormalizeError(eval, CalcErr::DomainError));
                }
                J[i][j] = (AXIOM::GetReal(*eval.value) - F[i]) / epsilon;
            }
            num_guess[v] = AXIOM::Number(old);
        }
        std::vector<double> neg_F = F;
        for(double& val : neg_F) val = -val;
        auto SolveLinearSystemSmall = [](std::vector<std::vector<double>> A, std::vector<double> b) {
            int n = A.size();
            for (int i=0; i<n; ++i) {
                int p=i; for(int k=i+1; k<n; ++k) if(std::abs(A[k][i]) > std::abs(A[p][i])) p=k;
                std::swap(A[i], A[p]); std::swap(b[i], b[p]);
                double div = A[i][i];
                for (int j=i; j<n; ++j) A[i][j] /= div;
                b[i] /= div;
                for (int k=0; k<n; ++k) {
                    if (k == i) continue;
                    double factor = A[k][i];
                    for (int j=i; j<n; ++j) A[k][j] -= factor * A[i][j];
                    b[k] -= factor * b[i];
                }
            }
            return b;
        };
        std::vector<double> d = SolveLinearSystemSmall(J, neg_F);
        for(int i=0; i<n; ++i) {
            double old_val = AXIOM::GetReal(num_guess[var_names[i]]);
            num_guess[var_names[i]] = AXIOM::Number(old_val + d[i]);
        }
    }
    std::vector<double> res;
    for(auto& name : var_names) res.emplace_back(AXIOM::GetReal(num_guess[name]));
    return CreateSuccessResult(res);
}

EngineResult AlgebraicParser::HandlePlotFunction(const std::string& input) {
    // Parse plot(expression, x_min, x_max, y_min, y_max)
    size_t paren_start = input.find('(');
    size_t paren_end = input.rfind(')');
    
    if (paren_start == std::string::npos || paren_end == std::string::npos) {
        return CreateErrorResult(CalcErr::ArgumentMismatch);
    }
    
    std::string args_str = input.substr(paren_start + 1, paren_end - paren_start - 1);
    
    // Split arguments by comma (handling nested parentheses)
    std::vector<std::string> args;
    size_t start = 0;
    int paren_depth = 0;
    
    for (size_t i = 0; i <= args_str.size(); ++i) {
        char c = (i < args_str.size()) ? args_str[i] : ',';
        
        if (c == '(') paren_depth++;
        else if (c == ')') paren_depth--;
        else if (c == ',' && paren_depth == 0) {
            std::string arg = Utils::Trim(std::string_view(args_str).substr(start, i - start));
            if (!arg.empty()) {
                args.push_back(arg);
            }
            start = i + 1;
        }
    }
    
    if (args.size() != 5) {
        EngineResult err_result;
        err_result.error = EngineErrorResult(CalcErr::ArgumentMismatch);
        return err_result;
    }
    
    // For now, return a special string result to indicate this is a plot command
    // The actual plotting will be handled by the CalcEngine
    std::string plot_command = "PLOT_FUNCTION:" + args[0] + "," + args[1] + "," + args[2] + "," + args[3] + "," + args[4];
    return CreateSuccessResult(std::move(plot_command));
}

} // namespace AXIOM