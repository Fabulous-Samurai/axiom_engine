/**
 * @file selective_dispatcher.h
 * @brief High-performance, zero-copy operation dispatcher for AXIOM Engine.
 *
 * Utilizes the Pimpl idiom to provide a stable ABI, isolate heavy template 
 * instantiations (e.g., Eigen), and enforce a compilation firewall.
 */

#pragma once

#ifndef SELECTIVE_DISPATCHER_H
#define SELECTIVE_DISPATCHER_H

#include <string>
#include <vector>
#include <memory>

#include <Eigen/Core> 
#include "dynamic_calc_types.h"

namespace AXIOM {

enum class ComputeEngine { Native, Eigen, Python, Auto };
enum class OperationComplexity { Simple, Medium, Complex, VeryComplex };

struct EnginePerformance {
    double avg_execution_time_ms{0.0};
    double memory_overhead_mb{0.0};
    double accuracy_score{1.0};
    size_t operations_count{0};
    bool supports_operation{false};
    ComputeEngine engine_type;
};

struct DispatchMetrics {
    ComputeEngine selected_engine;
    OperationComplexity complexity;
    double decision_time_us{0.0};
    double execution_time_ms{0.0};
    size_t data_size_bytes{0};
    std::string operation_name;
    std::string decision_reason;
    bool fallback_used{false};
};

class SelectiveDispatcher {
public:
    SelectiveDispatcher();
    ~SelectiveDispatcher();

    // Prevent copying to maintain unique ownership of Pimpl
    SelectiveDispatcher(const SelectiveDispatcher&) = delete;
    SelectiveDispatcher& operator=(const SelectiveDispatcher&) = delete;

    EngineResult DispatchOperation(const std::string& operation, 
                                 const std::vector<std::string>& args = {},
                                 ComputeEngine preferred_engine = ComputeEngine::Auto);
    
    EngineResult DispatchMatrixOperation(const std::string& operation, 
                                         Eigen::Ref<const Eigen::MatrixXd> matrix_data);
    
    void SetPreferredEngine(ComputeEngine engine);
    void EnableFallback(bool enable = true);
    std::string GetPerformanceReport() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_; 
};

} // namespace AXIOM

#endif // SELECTIVE_DISPATCHER_H