#include "symbolic_parser.h"
#include "symbolic_engine.h"
#include <algorithm>
#include <cctype>
#include <vector>

namespace AXIOM {

static std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\n\r");
    size_t e = s.find_last_not_of(" \t\n\r");
    if (b == std::string::npos) return "";
    return s.substr(b, e - b + 1);
}

static std::string tolower_str(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static std::vector<std::string> split_top_level_args(const std::string& args) {
    std::vector<std::string> out;
    int depth = 0;
    size_t start = 0;
    for (size_t i = 0; i < args.size(); ++i) {
        char c = args[i];
        if (c == '(') depth++;
        if (c == ')') depth--;
        if (c == ',' && depth == 0) {
            out.push_back(trim(args.substr(start, i - start)));
            start = i + 1;
        }
    }
    if (start < args.size()) {
        out.push_back(trim(args.substr(start)));
    }
    return out;
}

static bool starts_with(const std::string& value, const char* prefix) {
    return value.rfind(prefix, 0) == 0;
}

static std::string args_in_parens(const std::string& text) {
    const size_t lp = text.find('(');
    const size_t rp = text.rfind(')');
    if (lp == std::string::npos || rp == std::string::npos || rp <= lp) {
        return std::string();
    }
    return text.substr(lp + 1, rp - lp - 1);
}

template<typename Fn>
static bool try_dispatch_unary(const std::string& lower,
                               const std::string& original,
                               const char* paren_prefix,
                               const char* spaced_prefix,
                               size_t spaced_prefix_len,
                               Fn&& fn,
                               EngineResult& out) {
    if (starts_with(lower, paren_prefix)) {
        out = fn(args_in_parens(original));
        return true;
    }
    if (starts_with(lower, spaced_prefix)) {
        out = fn(trim(original.substr(spaced_prefix_len)));
        return true;
    }
    return false;
}

static EngineResult argument_mismatch() {
    EngineResult res;
    res.error = EngineErrorResult{CalcErr::ArgumentMismatch};
    return res;
}

static bool try_parse_double(const std::string& input, double& out) {
    try {
        size_t consumed = 0;
        out = std::stod(input, &consumed);
        return consumed == input.size();
    } catch (const std::exception&) {
        return false;
    }
}

static bool try_parse_int(const std::string& input, int& out) {
    try {
        size_t consumed = 0;
        out = std::stoi(input, &consumed);
        return consumed == input.size();
    } catch (const std::exception&) {
        return false;
    }
}

EngineResult SymbolicParser::ParseAndExecute(const std::string& input) {
    std::string s = trim(input);
    std::string lower = tolower_str(s);

    EngineResult unary_result;
    if (try_dispatch_unary(lower, s, "simplify(", "simplify ", 9,
                           [this](const std::string& arg) { return engine_->Simplify(arg); }, unary_result)) {
        return unary_result;
    }
    if (try_dispatch_unary(lower, s, "expand(", "expand ", 7,
                           [this](const std::string& arg) { return engine_->Expand(arg); }, unary_result)) {
        return unary_result;
    }
    if (try_dispatch_unary(lower, s, "factor(", "factor ", 7,
                           [this](const std::string& arg) { return engine_->Factor(arg); }, unary_result)) {
        return unary_result;
    }
    if (starts_with(lower, "derive ")) {
        std::string expr = trim(s.substr(7));
        return engine_->PartialDerivative(expr, "x");
    }
    if (starts_with(lower, "differentiate ")) {
        std::string payload = trim(s.substr(14));
        auto at_pos = payload.rfind(" wrt ");
        if (at_pos == std::string::npos) {
            return engine_->PartialDerivative(payload, "x");
        }
        std::string expr = trim(payload.substr(0, at_pos));
        std::string var = trim(payload.substr(at_pos + 5));
        return engine_->PartialDerivative(expr, var.empty() ? "x" : var);
    }
    if (starts_with(lower, "diff(") || starts_with(lower, "differentiate(")) {
        std::string args = args_in_parens(s);
        auto parts = split_top_level_args(args);
        if (parts.size() != 2) {
            return argument_mismatch();
        }
        return engine_->PartialDerivative(parts[0], parts[1]);
    }
    if (starts_with(lower, "integrate(")) {
        std::string args = args_in_parens(s);
        auto parts = split_top_level_args(args);
        if (parts.size() == 2) {
            return engine_->Integrate(parts[0], parts[1]);
        }
        if (parts.size() == 4) {
            double a = 0.0;
            double b = 0.0;
            if (try_parse_double(parts[2], a) && try_parse_double(parts[3], b)) {
                return engine_->DefiniteIntegral(parts[0], parts[1], a, b);
            }
            return argument_mismatch();
        }
    }
    if (starts_with(lower, "integrate ")) {
        std::string payload = trim(s.substr(10));
        auto at_pos = payload.rfind(" wrt ");
        if (at_pos == std::string::npos) {
            return argument_mismatch();
        }
        std::string expr = trim(payload.substr(0, at_pos));
        std::string var = trim(payload.substr(at_pos + 5));
        return engine_->Integrate(expr, var);
    }
    if (starts_with(lower, "substitute(") || starts_with(lower, "subs(")) {
        std::string args = args_in_parens(s);
        auto parts = split_top_level_args(args);
        if (parts.size() == 3) {
            return engine_->Substitute(parts[0], parts[1], parts[2]);
        }
    }
    if (starts_with(lower, "solve(")) {
        std::string args = args_in_parens(s);
        auto parts = split_top_level_args(args);
        if (parts.size() == 2) {
            return engine_->SolveEquation(parts[0], parts[1]);
        }
    }
    if (starts_with(lower, "solve ")) {
        std::string payload = trim(s.substr(6));
        auto at_pos = payload.rfind(" for ");
        if (at_pos == std::string::npos) {
            return argument_mismatch();
        }
        std::string eq = trim(payload.substr(0, at_pos));
        std::string var = trim(payload.substr(at_pos + 5));
        return engine_->SolveEquation(eq, var);
    }
    if (starts_with(lower, "solve_system(")) {
        std::string args = args_in_parens(s);
        auto parts = split_top_level_args(args);
        if (parts.size() >= 2) {
            std::vector<std::string> equations;
            for (size_t i = 0; i + 1 < parts.size(); ++i) {
                equations.push_back(parts[i]);
            }
            std::vector<std::string> variables;
            variables.push_back(parts.back());
            return engine_->SolveSystem(equations, variables);
        }
    }
    if (starts_with(lower, "limit(")) {
        std::string args = args_in_parens(s);
        auto parts = split_top_level_args(args);
        if (parts.size() == 3) {
            double point = 0.0;
            if (try_parse_double(parts[2], point)) {
                return engine_->FindLimits(parts[0], parts[1], point);
            }
            return argument_mismatch();
        }
    }
    if (starts_with(lower, "roots(")) {
        std::string args = args_in_parens(s);
        auto parts = split_top_level_args(args);
        if (parts.size() == 4) {
            double min_v = 0.0;
            double max_v = 0.0;
            if (try_parse_double(parts[2], min_v) && try_parse_double(parts[3], max_v)) {
                return engine_->FindRoots(parts[0], parts[1], min_v, max_v);
            }
            return argument_mismatch();
        }
    }
    if (starts_with(lower, "taylor(")) {
        std::string args = args_in_parens(s);
        auto parts = split_top_level_args(args);
        if (parts.size() == 4) {
            double point = 0.0;
            int order = 0;
            if (try_parse_double(parts[2], point) && try_parse_int(parts[3], order)) {
                return engine_->TaylorSeries(parts[0], parts[1], point, order);
            }
            return argument_mismatch();
        }
    }

    EngineResult res;
    res.error = EngineErrorResult{CalcErr::OperationNotFound};
    return res;
}

} // namespace AXIOM
