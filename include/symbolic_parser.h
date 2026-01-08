#pragma once
#include "IParser.h"
#include "symbolic_engine.h"
#include "dynamic_calc_types.h"
#include <string>

class SymbolicParser : public IParser {
public:
    explicit SymbolicParser(SymbolicEngine* engine) : engine_(engine) {}
    EngineResult ParseAndExecute(const std::string& input) override;
private:
    SymbolicEngine* engine_;
};
