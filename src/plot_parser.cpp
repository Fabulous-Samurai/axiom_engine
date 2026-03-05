#include "plot_parser.h"
#include "string_helpers.h"

#include <vector>

namespace AXIOM {

EngineResult PlotParser::ParseAndExecute(const std::string& input) {
    const std::string prefix = "plot(";
    if (input.rfind(prefix, 0) != 0 || input.back() != ')') {
        return CreateErrorResult(CalcErr::ParseError);
    }

    const std::string args_str = input.substr(prefix.size(), input.size() - prefix.size() - 1);

    std::vector<std::string> args;
    size_t start = 0;
    int paren_depth = 0;
    for (size_t i = 0; i <= args_str.size(); ++i) {
        const char c = (i < args_str.size()) ? args_str[i] : ',';
        if (c == '(') {
            ++paren_depth;
        } else if (c == ')') {
            --paren_depth;
        } else if (c == ',' && paren_depth == 0) {
            const std::string arg = Utils::Trim(args_str.substr(start, i - start));
            if (!arg.empty()) {
                args.push_back(arg);
            }
            start = i + 1;
        }
    }

    if (args.size() != 5) {
        return CreateErrorResult(CalcErr::ArgumentMismatch);
    }

    try {
        PlotConfig cfg;
        cfg.x_min = std::stod(args[1]);
        cfg.x_max = std::stod(args[2]);
        cfg.y_min = std::stod(args[3]);
        cfg.y_max = std::stod(args[4]);

        auto data = plot_engine_.ComputeFunctionData(args[0], cfg);
        return CreateSuccessResult(std::move(data));
    } catch (...) {
        return CreateErrorResult(CalcErr::ParseError);
    }
}

} // namespace AXIOM
