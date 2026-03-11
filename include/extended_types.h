/**
 * @file extended_types.h
 * @brief Extended types for AXIOM Engine v3.0 enterprise features
 */
#pragma once

#include "dynamic_calc_types.h"
#include <string>

namespace AXIOM {

inline std::string mode_to_string(CalculationMode mode) {
    switch (mode) {
        case CalculationMode::ALGEBRAIC:     return "algebraic";
        case CalculationMode::LINEAR_SYSTEM: return "linear";
        case CalculationMode::STATISTICS:    return "statistics";
        case CalculationMode::SYMBOLIC:      return "symbolic";
        case CalculationMode::UNITS:         return "units";
        case CalculationMode::PLOT:          return "plot";
        case CalculationMode::PYTHON:        return "python";
        default:                             return "unknown";
    }
}

/**
 * @brief Extended result structure for enterprise features
 */
struct ExtendedEngineResult {
    bool success = false;
    double value = 0.0;
    std::string error_message = "";
    
    // Extended result types
    bool has_linear_result = false;
    bool has_stats_result = false;
    bool has_symbolic_result = false;
    bool has_unit_result = false;
    bool has_plot_result = false;
    
    // Linear system result
    struct {
        std::vector<double> solution;
    } linear_result;
    
    // Statistics result
    struct {
        double mean = 0.0;
        double std_dev = 0.0;
        double min = 0.0;
        double max = 0.0;
        size_t count = 0;
    } stats_result;
    
    // Symbolic result
    struct {
        std::string result;
        std::string simplified;
    } symbolic_result;
    
    // Unit conversion result
    struct {
        double value = 0.0;
        std::string target_unit;
    } unit_result;
    
    // Plot result
    struct {
        std::string filename;
        double x_min = 0.0;
        double x_max = 0.0;
    } plot_result;
    
    // Convert from basic EngineResult
    static ExtendedEngineResult from_engine_result(const EngineResult& basic_result) {
        ExtendedEngineResult extended;
        
        if (basic_result.result.has_value()) {
            extended.success = true;
            
            const auto& result_value = basic_result.result.value();
            
            if (std::holds_alternative<double>(result_value)) {
                extended.value = std::get<double>(result_value);
            }
            else if (std::holds_alternative<AXIOM::Number>(result_value)) {
                // Handle AXIOM::Number type (complex numbers, reals, etc.)
                extended.value = AXIOM::GetReal(std::get<AXIOM::Number>(result_value));
            }
            else if (std::holds_alternative<std::complex<double>>(result_value)) {
                // Handle std::complex<double>
                extended.value = std::get<std::complex<double>>(result_value).real();
            }
            else if (std::holds_alternative<Vector>(result_value)) {
                const auto& vec = std::get<Vector>(result_value);
                if (!vec.empty()) {
                    extended.value = vec[0]; // Primary value
                    extended.has_linear_result = true;
                    extended.linear_result.solution = vec;
                }
            }
            else if (std::holds_alternative<std::string>(result_value)) {
                extended.error_message = std::get<std::string>(result_value);
                extended.has_symbolic_result = true;
                extended.symbolic_result.result = std::get<std::string>(result_value);
            }
        } else if (basic_result.error.has_value()) {
            extended.success = false;
            extended.error_message = "Calculation error occurred";
        }
        
        return extended;
    }
};

} // namespace AXIOM