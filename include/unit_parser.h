#pragma once
#include "IParser.h"
#include "unit_manager.h"
#include <string>
#include <regex>

namespace AXIOM {

class UnitParser : public IParser {
private:
    UnitManager* unit_manager_;

public:
    explicit UnitParser(UnitManager* manager) noexcept : unit_manager_(manager) {}

    EngineResult ParseAndExecute(const std::string& input) override;

private:
    bool IsUnitConversion(const std::string& input);
    EngineResult ParseConversion(const std::string& input);
};

} // namespace AXIOM