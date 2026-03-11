#include "symbolic_engine.h"
#include "string_helpers.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

namespace {

bool IsBlank(const std::string& value) {
    return value.find_first_not_of(" \t\n\r") == std::string::npos;
}

std::string Trim(const std::string& value) {
    return Utils::Trim(value);
}

bool IsIdentifierChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

std::string NumberToString(double value) {
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(12);
    out << value;
    std::string s = out.str();
    while (s.size() > 1 && s.back() == '0') {
        s.pop_back();
    }
    if (!s.empty() && s.back() == '.') {
        s.pop_back();
    }
    if (s.empty()) {
        return "0";
    }
    return s;
}

bool IsIntegerValue(double x) {
    return std::abs(x - std::round(x)) < 1e-9;
}

double BinomialCoeff(int n, int k) {
    if (k < 0 || k > n) {
        return 0.0;
    }
    if (k == 0 || k == n) {
        return 1.0;
    }
    k = std::min(k, n - k);
    double coeff = 1.0;
    for (int i = 1; i <= k; ++i) {
        coeff = coeff * static_cast<double>(n - (k - i)) / static_cast<double>(i);
    }
    return coeff;
}

bool ParseDoubleStrict(const std::string& token, double& out) {
    try {
        size_t parsed = 0;
        out = std::stod(token, &parsed);
        return parsed == token.size();
    } catch (const std::exception&) {
        return false;
    }
}

std::string ReplaceVariableTokens(const std::string& expr, const std::string& var, const std::string& replacement) {
    std::string out;
    out.reserve(expr.size() + 16);
    for (size_t i = 0; i < expr.size();) {
        bool starts_token = (i == 0 || !IsIdentifierChar(expr[i - 1]));
        if (starts_token && i + var.size() <= expr.size() && expr.compare(i, var.size(), var) == 0) {
            const size_t right = i + var.size();
            const bool ends_token = (right == expr.size() || !IsIdentifierChar(expr[right]));
            if (ends_token) {
                out += replacement;
                i += var.size();
                continue;
            }
        }
        out.push_back(expr[i]);
        ++i;
    }
    return out;
}

EngineResult EvalScalar(const std::string& expr, const AXIOM::StringMap<AXIOM::Number>& context = {}) {
    AXIOM::AlgebraicParser parser;
    return parser.ParseAndExecuteWithContext(expr, context);
}

bool EvalDouble(const std::string& expr, double& out, const AXIOM::StringMap<AXIOM::Number>& context = {}) {
    EngineResult res = EvalScalar(expr, context);
    auto value = res.GetDouble();
    if (!value.has_value() || !std::isfinite(*value)) {
        return false;
    }
    out = *value;
    return true;
}

bool BisectionRoot(const std::string& expr,
                  const std::string& var,
                  double left,
                  double right,
                  double& root) {
    AXIOM::StringMap<AXIOM::Number> ctx;
    auto eval = [&](double x, double& fx) -> bool {
        ctx[var] = AXIOM::Number(x);
        return EvalDouble(expr, fx, ctx);
    };

    double f_left = 0.0;
    double f_right = 0.0;
    if (!eval(left, f_left) || !eval(right, f_right)) {
        return false;
    }
    if (std::abs(f_left) < 1e-10) {
        root = left;
        return true;
    }
    if (std::abs(f_right) < 1e-10) {
        root = right;
        return true;
    }
    if (f_left * f_right > 0.0) {
        return false;
    }

    for (int i = 0; i < 80; ++i) {
        const double mid = 0.5 * (left + right);
        double f_mid = 0.0;
        if (!eval(mid, f_mid)) {
            return false;
        }

        if (std::abs(f_mid) < 1e-10 || std::abs(right - left) < 1e-10) {
            root = mid;
            return true;
        }

        if (f_left * f_mid <= 0.0) {
            right = mid;
            f_right = f_mid;
        } else {
            left = mid;
            f_left = f_mid;
        }
    }

    root = 0.5 * (left + right);
    return true;
}

} // namespace

EngineResult SymbolicEngine::Expand(const std::string& expression) {
    if (IsBlank(expression)) {
        return CreateErrorResult(CalcErr::ArgumentMismatch);
    }

    const std::string expr = Trim(expression);
    if (expr.size() >= 6 && expr.front() == '(' && expr.find(')') != std::string::npos && expr.back() >= '0' && expr.back() <= '9') {
        const size_t close = expr.find(')');
        if (close + 2 < expr.size() && expr[close + 1] == '^') {
            const std::string inside = expr.substr(1, close - 1);
            const std::string power_s = expr.substr(close + 2);
            double power_d = 0.0;
            if (ParseDoubleStrict(power_s, power_d) && IsIntegerValue(power_d)) {
                const int n = static_cast<int>(std::round(power_d));
                if (n >= 0 && n <= 16) {
                    size_t split = inside.find('+');
                    char op = '+';
                    if (split == std::string::npos) {
                        split = inside.find('-', 1);
                        op = '-';
                    }
                    if (split != std::string::npos) {
                        const std::string a = Trim(inside.substr(0, split));
                        const std::string b_raw = Trim(inside.substr(split + 1));
                        const std::string b = (op == '-') ? ("-" + b_raw) : b_raw;

                        bool a_is_var = !a.empty() && std::isalpha(static_cast<unsigned char>(a[0]));
                        double b_num = 0.0;
                        if (a_is_var && ParseDoubleStrict(b, b_num)) {
                            std::ostringstream out;
                            bool first = true;
                            for (int k = 0; k <= n; ++k) {
                                const double coeff = BinomialCoeff(n, k) * std::pow(b_num, k);
                                const int var_pow = n - k;
                                if (std::abs(coeff) < 1e-12) {
                                    continue;
                                }

                                if (!first) {
                                    out << (coeff >= 0.0 ? " + " : " - ");
                                } else if (coeff < 0.0) {
                                    out << "-";
                                }

                                const double abs_coeff = std::abs(coeff);
                                const bool emit_coeff = (var_pow == 0) || std::abs(abs_coeff - 1.0) > 1e-12;
                                if (emit_coeff) {
                                    out << NumberToString(abs_coeff);
                                    if (var_pow > 0) {
                                        out << "*";
                                    }
                                }
                                if (var_pow > 0) {
                                    out << a;
                                    if (var_pow > 1) {
                                        out << "^" << var_pow;
                                    }
                                }

                                first = false;
                            }
                            return CreateSuccessResult(out.str());
                        }
                    }
                }
            }
        }
    }

    return CreateSuccessResult(expr);
}

EngineResult SymbolicEngine::Factor(const std::string& expression) {
    if (IsBlank(expression)) {
        return CreateErrorResult(CalcErr::ArgumentMismatch);
    }

    std::string candidate = Trim(expression);
    std::erase_if(candidate, [](unsigned char ch) { return std::isspace(ch) != 0; });

    // Minimal integer-root factorization for x^2+bx+c.
    size_t x2 = candidate.find("x^2");
    if (x2 != std::string::npos) {
        std::string tail = candidate.substr(x2 + 3);
        // Expected format: [+|-]bx[+|-]c
        size_t x_pos = tail.find('x');
        if (x_pos != std::string::npos) {
            std::string b_str = tail.substr(0, x_pos);
            std::string c_str = tail.substr(x_pos + 1);
            if (b_str.empty() || b_str == "+") {
                b_str = "1";
            } else if (b_str == "-") {
                b_str = "-1";
            }

            double b = 0.0;
            double c = 0.0;
            if (ParseDoubleStrict(b_str, b) && ParseDoubleStrict(c_str, c) && IsIntegerValue(b) && IsIntegerValue(c)) {
                const int bi = static_cast<int>(std::round(b));
                const int ci = static_cast<int>(std::round(c));
                for (int p = -64; p <= 64; ++p) {
                    if (p == 0 || ci % p != 0) {
                        continue;
                    }
                    const int q = ci / p;
                    if (p + q == bi) {
                        const std::string p_s = (p >= 0) ? (" + " + std::to_string(p)) : (" - " + std::to_string(-p));
                        const std::string q_s = (q >= 0) ? (" + " + std::to_string(q)) : (" - " + std::to_string(-q));
                        return CreateSuccessResult("(x" + p_s + ")*(x" + q_s + ")");
                    }
                }
            }
        }
    }

    return CreateSuccessResult(Trim(expression));
}

EngineResult SymbolicEngine::Simplify(const std::string& expression) {
    if (IsBlank(expression)) {
        return CreateErrorResult(CalcErr::ArgumentMismatch);
    }

    double value = 0.0;
    if (EvalDouble(expression, value)) {
        return CreateSuccessResult(NumberToString(value));
    }
    return CreateSuccessResult(Trim(expression));
}

EngineResult SymbolicEngine::Substitute(const std::string& expr, const std::string& var, const std::string& value) {
    if (IsBlank(expr) || IsBlank(var) || IsBlank(value)) {
        return CreateErrorResult(CalcErr::ArgumentMismatch);
    }

    const std::string replaced = ReplaceVariableTokens(expr, Trim(var), "(" + Trim(value) + ")");
    return CreateSuccessResult(replaced);
}

EngineResult SymbolicEngine::Integrate(const std::string& expression, const std::string& variable) {
    if (IsBlank(expression) || IsBlank(variable)) {
        return CreateErrorResult(CalcErr::ArgumentMismatch);
    }

    const std::string expr = Trim(expression);
    const std::string var = Trim(variable);

    double c = 0.0;
    if (ParseDoubleStrict(expr, c)) {
        return CreateSuccessResult(NumberToString(c) + "*" + var);
    }

    if (expr == var) {
        return CreateSuccessResult("0.5*" + var + "^2");
    }

    if (expr == "sin(" + var + ")") {
        return CreateSuccessResult("-cos(" + var + ")");
    }
    if (expr == "cos(" + var + ")") {
        return CreateSuccessResult("sin(" + var + ")");
    }

    if (expr.rfind(var + "^", 0) == 0) {
        double n = 0.0;
        if (ParseDoubleStrict(expr.substr(var.size() + 1), n) && std::abs(n + 1.0) > 1e-12) {
            const double p = n + 1.0;
            return CreateSuccessResult("(" + NumberToString(1.0 / p) + ")*" + var + "^" + NumberToString(p));
        }
    }

    return CreateErrorResult(CalcErr::OperationNotFound);
}

EngineResult SymbolicEngine::DefiniteIntegral(const std::string& expr, const std::string& var, double a, double b) {
    if (IsBlank(expr) || IsBlank(var)) {
        return CreateErrorResult(CalcErr::ArgumentMismatch);
    }

    const std::string command = "integrate(" + Trim(expr) + ", " + Trim(var) + ", " + NumberToString(a) + ", " + NumberToString(b) + ")";
    EngineResult res = EvalScalar(command);
    if (res.HasResult()) {
        return res;
    }
    return CreateErrorResult(CalcErr::DomainError);
}

EngineResult SymbolicEngine::PartialDerivative(const std::string& expr, const std::string& var) {
    if (IsBlank(expr) || IsBlank(var)) {
        return CreateErrorResult(CalcErr::ArgumentMismatch);
    }

    const std::string variable = Trim(var);
    const std::string source = Trim(expr);
    std::string derivative_target = source;

    if (variable != "x") {
        derivative_target = ReplaceVariableTokens(source, variable, "x");
    }

    AXIOM::AlgebraicParser parser;
    EngineResult res = parser.ParseAndExecute("derive " + derivative_target);
    if (!res.result.has_value()) {
        return CreateErrorResult(CalcErr::OperationNotFound);
    }

    const std::string* deriv_str = std::get_if<std::string>(&*res.result);
    if (deriv_str == nullptr) {
        return CreateErrorResult(CalcErr::OperationNotFound);
    }

    std::string mapped_back = *deriv_str;
    if (variable != "x") {
        mapped_back = ReplaceVariableTokens(mapped_back, "x", variable);
    }
    return CreateSuccessResult(mapped_back);
}

EngineResult SymbolicEngine::TaylorSeries(const std::string& expr, const std::string& var, double point, int order) {
    if (IsBlank(expr) || IsBlank(var) || order < 0) {
        return CreateErrorResult(CalcErr::ArgumentMismatch);
    }

    const std::string variable = Trim(var);
    std::string current = Trim(expr);
    AXIOM::AlgebraicParser parser;

    double fact = 1.0;
    std::ostringstream series;
    bool any_term = false;

    for (int k = 0; k <= order; ++k) {
        AXIOM::StringMap<AXIOM::Number> context;
        context[variable] = AXIOM::Number(point);
        double coeff_val = 0.0;
        if (EvalDouble(current, coeff_val, context)) {
            const double coeff = coeff_val / fact;
            if (std::abs(coeff) > 1e-12) {
                if (any_term) {
                    series << (coeff >= 0.0 ? " + " : " - ");
                } else if (coeff < 0.0) {
                    series << "-";
                }
                const double abs_coeff = std::abs(coeff);
                if (k == 0) {
                    series << NumberToString(abs_coeff);
                } else {
                    if (std::abs(abs_coeff - 1.0) > 1e-12) {
                        series << NumberToString(abs_coeff) << "*";
                    }
                    series << "(" << variable << " - " << NumberToString(point) << ")";
                    if (k > 1) {
                        series << "^" << k;
                    }
                }
                any_term = true;
            }
        }

        if (k < order) {
            EngineResult d = parser.ParseAndExecute("derive " + current);
            const std::string* d_str = (d.result.has_value()) ? std::get_if<std::string>(&*d.result) : nullptr;
            if (d_str == nullptr) {
                break;
            }
            current = *d_str;
            fact *= static_cast<double>(k + 1);
        }
    }

    if (!any_term) {
        return CreateErrorResult(CalcErr::OperationNotFound);
    }
    return CreateSuccessResult(series.str());
}

EngineResult SymbolicEngine::SolveEquation(const std::string& equation, const std::string& variable) {
    if (IsBlank(equation) || IsBlank(variable)) {
        return CreateErrorResult(CalcErr::ArgumentMismatch);
    }

    const std::string var = Trim(variable);
    const std::string eq = Trim(equation);
    const size_t equal_pos = eq.find('=');
    std::string expr = eq;
    if (equal_pos != std::string::npos) {
        expr = "(" + Trim(eq.substr(0, equal_pos)) + ")-(" + Trim(eq.substr(equal_pos + 1)) + ")";
    }

    std::vector<double> roots;
    const double left = -1000.0;
    const double right = 1000.0;
    const int steps = 400;
    const double step = (right - left) / static_cast<double>(steps);

    AXIOM::StringMap<AXIOM::Number> ctx;
    auto eval = [&](double x, double& fx) -> bool {
        ctx[var] = AXIOM::Number(x);
        return EvalDouble(expr, fx, ctx);
    };

    double prev_x = left;
    double prev_f = 0.0;
    bool prev_ok = eval(prev_x, prev_f);
    for (int i = 1; i <= steps; ++i) {
        const double x = left + i * step;
        double fx = 0.0;
        bool ok = eval(x, fx);
        if (ok && prev_ok) {
            if (std::abs(fx) < 1e-8) {
                roots.push_back(x);
            } else if (prev_f * fx < 0.0) {
                double root = 0.0;
                if (BisectionRoot(expr, var, prev_x, x, root)) {
                    roots.push_back(root);
                }
            }
        }
        prev_x = x;
        prev_f = fx;
        prev_ok = ok;
    }

    if (roots.empty()) {
        return CreateErrorResult(CalcErr::OperationNotFound);
    }

    std::ranges::sort(roots);
    std::vector<double> unique_roots;
    for (double r : roots) {
        if (unique_roots.empty() || std::abs(unique_roots.back() - r) > 1e-5) {
            unique_roots.push_back(r);
        }
    }
    return CreateSuccessResult(unique_roots);
}

EngineResult SymbolicEngine::SolveSystem(const std::vector<std::string>& equations, const std::vector<std::string>& variables) {
    if (equations.empty() || variables.empty()) {
        return CreateErrorResult(CalcErr::ArgumentMismatch);
    }
    for (const auto& eq : equations) {
        if (IsBlank(eq)) {
            return CreateErrorResult(CalcErr::ArgumentMismatch);
        }
    }
    for (const auto& variable : variables) {
        if (IsBlank(variable)) {
            return CreateErrorResult(CalcErr::ArgumentMismatch);
        }
    }

    std::ostringstream command;
    command << "solve_nl {";
    for (size_t i = 0; i < equations.size(); ++i) {
        if (i > 0) {
            command << ";";
        }
        command << equations[i];
    }
    command << "} [";
    for (size_t i = 0; i < variables.size(); ++i) {
        if (i > 0) {
            command << ",";
        }
        command << "1";
    }
    command << "]";

    AXIOM::AlgebraicParser parser;
    EngineResult res = parser.ParseAndExecute(command.str());
    if (res.HasResult()) {
        return res;
    }
    return CreateErrorResult(CalcErr::OperationNotFound);
}

EngineResult SymbolicEngine::FindLimits(const std::string& expr, const std::string& var, double approach_point) {
    if (IsBlank(expr) || IsBlank(var)) {
        return CreateErrorResult(CalcErr::ArgumentMismatch);
    }

    const std::string cmd = "limit(" + Trim(expr) + ", " + Trim(var) + ", " + NumberToString(approach_point) + ")";
    EngineResult res = EvalScalar(cmd);
    if (res.HasResult()) {
        return res;
    }
    return CreateErrorResult(CalcErr::DomainError);
}

EngineResult SymbolicEngine::FindRoots(const std::string& expr, const std::string& var, double range_min, double range_max) {
    if (IsBlank(expr) || IsBlank(var) || range_min > range_max) {
        return CreateErrorResult(CalcErr::ArgumentMismatch);
    }

    std::vector<double> roots;
    const int samples = 256;
    const double step = (range_max - range_min) / static_cast<double>(samples);
    const std::string variable = Trim(var);

    AXIOM::StringMap<AXIOM::Number> ctx;
    auto eval = [&](double x, double& fx) -> bool {
        ctx[variable] = AXIOM::Number(x);
        return EvalDouble(expr, fx, ctx);
    };

    double prev_x = range_min;
    double prev_f = 0.0;
    bool prev_ok = eval(prev_x, prev_f);
    for (int i = 1; i <= samples; ++i) {
        const double x = range_min + step * static_cast<double>(i);
        double fx = 0.0;
        const bool ok = eval(x, fx);
        if (ok && prev_ok) {
            if (std::abs(fx) < 1e-8) {
                roots.push_back(x);
            } else if (prev_f * fx < 0.0) {
                double root = 0.0;
                if (BisectionRoot(expr, variable, prev_x, x, root)) {
                    roots.push_back(root);
                }
            }
        }
        prev_x = x;
        prev_f = fx;
        prev_ok = ok;
    }

    if (roots.empty()) {
        return CreateErrorResult(CalcErr::OperationNotFound);
    }
    std::ranges::sort(roots);
    std::vector<double> unique_roots;
    for (double r : roots) {
        if (unique_roots.empty() || std::abs(unique_roots.back() - r) > 1e-5) {
            unique_roots.push_back(r);
        }
    }
    return CreateSuccessResult(unique_roots);
}