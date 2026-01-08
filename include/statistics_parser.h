#pragma once
#include "IParser.h"
#include "statistics_engine.h"
#include "dynamic_calc_types.h"
#include <string>
#include <vector>

class StatisticsParser : public IParser {
public:
    explicit StatisticsParser(StatisticsEngine* engine) : engine_(engine) {}
    EngineResult ParseAndExecute(const std::string& input) override;
private:
    StatisticsEngine* engine_;
    static std::vector<double> ParseVector(const std::string& s);
};
