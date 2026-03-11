#include "statistics_parser.h"
#include <algorithm>
#include <cctype>

namespace AXIOM {

static std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\n\r");
    size_t e = s.find_last_not_of(" \t\n\r");
    if (b == std::string::npos) return "";
    return s.substr(b, e - b + 1);
}

std::vector<double> StatisticsParser::ParseVector(const std::string& s) {
    std::vector<double> out;
    // Find outer brackets
    size_t lb = s.find('[');
    size_t rb = s.rfind(']');
    if (lb == std::string::npos || rb == std::string::npos || rb <= lb) return out;
    std::string body = s.substr(lb + 1, rb - lb - 1);
    // Split by comma or semicolon
    std::string token;
    for (char c : body) {
        if (c == ',' || c == ';') {
            if (!token.empty()) {
                out.push_back(std::stod(trim(token)));
                token.clear();
            }
        } else {
            token.push_back(c);
        }
    }
    if (!token.empty()) out.push_back(std::stod(trim(token)));
    return out;
}

EngineResult StatisticsParser::ParseAndExecute(const std::string& input) {
    std::string s = input;
    // Lowercase for command detection
    std::string lower;
    lower.reserve(s.size());
    for (char c : s) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

    if (lower.find("mean") == 0 || lower.find("statsmean") == 0 || lower.find("stats mean") == 0) {
        return engine_->Mean(ParseVector(s));
    }
    if (lower.find("variance") == 0 || lower.find("statsvariance") == 0 || lower.find("stats variance") == 0) {
        return engine_->Variance(ParseVector(s));
    }
    if (lower.find("std") == 0 || lower.find("standarddeviation") == 0 || lower.find("statsstd") == 0) {
        return engine_->StandardDeviation(ParseVector(s));
    }
    if (lower.find("median") == 0 || lower.find("statsmedian") == 0) {
        auto vec = ParseVector(s);
        return engine_->Median(vec);
    }
    if (lower.find("mode") == 0 || lower.find("statsmode") == 0) {
        return engine_->Mode(ParseVector(s));
    }

    // correlation([x],[y])
    if (lower.find("correlation(") == 0 || lower.find("statscorrelation(") == 0) {
        size_t lp = s.find('(');
        size_t rp = s.rfind(')');
        if (lp != std::string::npos && rp != std::string::npos && rp > lp) {
            std::string args = s.substr(lp + 1, rp - lp - 1);
            // Split on '],' boundary
            size_t mid = args.find("],[");
            if (mid != std::string::npos) {
                std::string xpart = args.substr(0, mid + 1);
                std::string ypart = args.substr(mid + 1);
                return engine_->Correlation(ParseVector(xpart), ParseVector(ypart));
            }
        }
        EngineResult res;
        res.error = EngineErrorResult{CalcErr::ArgumentMismatch};
        return res;
    }

    EngineResult res;
    res.error = EngineErrorResult{CalcErr::OperationNotFound};
    return res;
}

} // namespace AXIOM
