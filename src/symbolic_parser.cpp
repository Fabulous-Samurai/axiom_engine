#include "symbolic_parser.h"
#include <algorithm>
#include <cctype>

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

EngineResult SymbolicParser::ParseAndExecute(const std::string& input) {
    std::string s = input;
    std::string lower = tolower_str(s);

    auto args_in_parens = [](const std::string& t) -> std::string {
        size_t lp = t.find('(');
        size_t rp = t.rfind(')');
        if (lp == std::string::npos || rp == std::string::npos || rp <= lp) return std::string();
        return t.substr(lp + 1, rp - lp - 1);
    };

    if (lower.find("simplify(") == 0) {
        return engine_->Simplify(args_in_parens(s));
    }
    if (lower.find("expand(") == 0) {
        return engine_->Expand(args_in_parens(s));
    }
    if (lower.find("factor(") == 0) {
        return engine_->Factor(args_in_parens(s));
    }
    if (lower.find("diff(") == 0 || lower.find("differentiate(") == 0) {
        std::string args = args_in_parens(s);
        size_t comma = args.find(',');
        if (comma == std::string::npos) return EngineResult{{},{CalcErr::ArgumentMismatch}};
        std::string expr = trim(args.substr(0, comma));
        std::string var = trim(args.substr(comma + 1));
        return engine_->PartialDerivative(expr, var);
    }

    return {{},{CalcErr::OperationNotFound}};
}
