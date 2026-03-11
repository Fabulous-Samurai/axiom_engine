#include "unit_parser.h"
#include "string_helpers.h"
#include <regex>
#include <sstream>

namespace AXIOM {

// Lazy evaluate regex to prevent static initialization ordering crashes
static const std::regex& GetUnitConversionPattern() {
    static const std::regex kPattern(R"((convert\s+)?([0-9]+(?:\.[0-9]+)?)\s+([a-zA-Z]+)\s+to\s+([a-zA-Z]+))");
    return kPattern;
}

EngineResult UnitParser::ParseAndExecute(const std::string& input) {
    if (IsUnitConversion(input)) {
        return ParseConversion(input);
    }
    return CreateErrorResult(CalcErr::ParseError);
}

bool UnitParser::IsUnitConversion(const std::string& input) {
    // Look for patterns like "5 meters to feet" or "convert 10 kg to lb"
    return std::regex_search(input, GetUnitConversionPattern());
}

EngineResult UnitParser::ParseConversion(const std::string& input) {
    std::smatch matches;

    if (std::regex_search(input, matches, GetUnitConversionPattern())) {
        double value = std::stod(matches[2].str());
        std::string from_unit = matches[3].str();
        std::string to_unit = matches[4].str();

        return unit_manager_->ConvertUnit(value, from_unit, to_unit);
    }

    return CreateErrorResult(CalcErr::ParseError);
}

} // namespace AXIOM
