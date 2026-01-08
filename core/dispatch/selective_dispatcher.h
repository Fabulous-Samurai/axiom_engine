/**
 * @file selective_dispatcher.h
 * @brief AXIOM v3.0 - Intelligent Selective Dispatcher Header
 * 
 * Advanced operation routing system that intelligently selects optimal 
 * computational engines based on expression analysis and performance metrics.
 */

#pragma once

#include "../../include/dynamic_calc_types.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <chrono>

// Forward declare global PythonEngine to allow unique_ptr without pulling
// Python headers into the AXIOM namespace (to avoid symbol pollution).
#ifdef ENABLE_PYTHON_FFI
class PythonEngine;
#endif

namespace AXIOM {

/**
 * @brief Available computational engines
 */
enum class ComputeEngine {
    Auto,      ///< Automatic engine selection
    Native,    ///< Native C++ AXIOM engine
    Eigen,     ///< Eigen-optimized CPU engine
    Python     ///< Python/nanobind engine for symbolic math
};

/**
 * @brief Operation complexity classification
 */
enum class OperationComplexity {
    Simple,    ///< Basic arithmetic operations
    Medium,    ///< Function calls, small matrices
    Complex,   ///< Large matrices, advanced functions
    Extreme    ///< Symbolic operations, huge datasets
};

/**
 * @brief Performance metrics for engine operations
 */
struct EnginePerformance {
    double avg_execution_time_ms = 0.0;
    double memory_overhead_mb = 0.0;
    double accuracy_score = 1.0;  ///< 0.0 to 1.0 accuracy rating
    size_t operations_count = 0;
    bool supports_operation = false;
    ComputeEngine engine_type;
};

/**
 * @brief Dispatch decision metrics
 */
struct DispatchMetrics {
    ComputeEngine selected_engine;
    OperationComplexity complexity;
    double decision_time_us = 0.0;
    double execution_time_ms = 0.0;
    size_t data_size_bytes = 0;
    std::string operation_name;
    std::string decision_reason;
    bool fallback_used = false;
};

// Forward declarations for optional engine types
#ifdef ENABLE_EIGEN
class EigenEngine;
#endif

/**
 * @brief Intelligent Selective Dispatcher
 * 
 * Routes mathematical operations to optimal computational engines based on:
 * - Expression complexity analysis
 * - Engine availability and performance
 * - Historical performance data
 * - Fallback mechanisms for reliability
 */
class SelectiveDispatcher {
public:
    SelectiveDispatcher();
    ~SelectiveDispatcher();

    // Core dispatch operations
    EngineResult DispatchOperation(const std::string& expression, 
                                 OperationComplexity complexity = OperationComplexity::Simple);

    // Configuration
    void SetPreferredEngine(ComputeEngine engine);
    void EnableLearning(bool enable);
    void SetPerformanceThreshold(double threshold_ms);

    // Monitoring and diagnostics
    DispatchMetrics GetLastMetrics() const;
    std::string GetPerformanceReport() const;
    bool IsEngineAvailable(ComputeEngine engine) const;

private:
    // Engine selection logic
    ComputeEngine SelectOptimalEngine(const std::string& expression, 
                                     OperationComplexity complexity);
    EngineResult ExecuteWithFallback(const std::string& expression, 
                                    ComputeEngine engine);
    
    // Native execution
    EngineResult ExecuteNative(const std::string& expression);
    
    // Expression analysis
    size_t EstimateDataSize(const std::string& expression) const;
    bool HasMatrixOperations(const std::string& expression) const;
    bool HasSymbolicOperations(const std::string& expression) const;
    
    // Performance tracking
    void RecordMetrics(ComputeEngine engine, 
                      const std::string& expression,
                      OperationComplexity complexity,
                      double execution_time_ms);
    
    // Utilities
    std::string EngineToString(ComputeEngine engine) const;

    // Configuration
    ComputeEngine preferred_engine_;
    bool fallback_enabled_;
    double performance_threshold_ms_;
    bool learning_enabled_;
    
    // Performance tracking
    DispatchMetrics last_metrics_;
    std::unordered_map<ComputeEngine, bool> engine_availability_;
    std::unordered_map<ComputeEngine, 
                      std::unordered_map<std::string, EnginePerformance>> engine_performance_;
    
    // Engine instances (conditionally compiled)
    #ifdef ENABLE_EIGEN
    std::unique_ptr<EigenEngine> eigen_engine_;
    #endif

    // Python engine (enabled when ENABLE_PYTHON_FFI)
    #ifdef ENABLE_PYTHON_FFI
    std::unique_ptr<::PythonEngine> python_engine_;
    #endif
};

// Convenience namespace for global dispatcher access
namespace Dispatch {
    EngineResult Calculate(const std::string& expression);
    void PreferEngine(ComputeEngine engine);
    std::string GetReport();
    void OptimizeForSpeed();
    void Initialize();
    void Shutdown();
}

} // namespace AXIOM