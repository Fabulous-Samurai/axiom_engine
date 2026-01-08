/**
 * @file selective_dispatcher.h
 * @brief Intelligent operation dispatcher for optimal performance
 * 
 * Routes mathematical operations to the best available engine
 * (C++ native, Eigen CPU, or Python) based on operation type,
 * data size, and performance characteristics.
 */

#ifndef SELECTIVE_DISPATCHER_H
#define SELECTIVE_DISPATCHER_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <chrono>
#include <variant>

#ifdef ENABLE_EIGEN
#include "eigen_engine.h"
#endif

#ifdef ENABLE_NANOBIND
#include "nanobind_interface.h"
#endif

#include "dynamic_calc_types.h"

namespace AXIOM {

/**
 * @brief Available computation engines
 */
enum class ComputeEngine {
    Native,     // Pure C++ implementation
    Eigen,      // Eigen-based CPU engine
    Python,     // Python fallback
    Auto        // Automatic selection
};

/**
 * @brief Operation complexity classification
 */
enum class OperationComplexity {
    Simple,     // Basic arithmetic: +, -, *, /
    Medium,     // Functions: sin, cos, exp, log
    Complex,    // Advanced: matrices, linear algebra
    VeryComplex // Symbolic, optimization, advanced calculus
};

/**
 * @brief Performance characteristics for different engines
 */
struct EnginePerformance {
    double avg_execution_time_ms = 0.0;
    double memory_overhead_mb = 0.0;
    double accuracy_score = 1.0;  // 0.0 to 1.0
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

/**
 * @brief Intelligent selective dispatcher for mathematical operations
 */
class SelectiveDispatcher {
public:
    SelectiveDispatcher();
    ~SelectiveDispatcher() = default;

    // Core dispatch interface
    EngineResult DispatchOperation(const std::string& operation, 
                                 const std::vector<std::string>& args = {},
                                 ComputeEngine preferred_engine = ComputeEngine::Auto);
    
    // Specialized dispatchers
    EngineResult DispatchArithmetic(const std::string& expression);
    EngineResult DispatchMatrixOperation(const std::string& operation, 
                                       const std::vector<Matrix>& matrices);
    EngineResult DispatchFunction(const std::string& function_name,
                                const std::vector<double>& args);
    EngineResult DispatchSymbolic(const std::string& expression);
    
    // Configuration
    void SetPreferredEngine(ComputeEngine engine) { preferred_engine_ = engine; }
    void EnableFallback(bool enable = true) { fallback_enabled_ = enable; }
    void SetPerformanceThreshold(double threshold_ms) { performance_threshold_ms_ = threshold_ms; }
    void EnableLearning(bool enable = true) { learning_enabled_ = enable; }
    
    // Engine management
    void RegisterEngine(ComputeEngine engine, bool available);
    bool IsEngineAvailable(ComputeEngine engine) const;
    std::vector<ComputeEngine> GetAvailableEngines() const;
    
    // Performance monitoring
    DispatchMetrics GetLastDispatchMetrics() const { return last_metrics_; }
    std::map<ComputeEngine, EnginePerformance> GetEnginePerformance() const;
    std::string GetPerformanceReport() const;
    
    // Learning and optimization
    void UpdateEnginePerformance(ComputeEngine engine, 
                                const std::string& operation,
                                double execution_time_ms,
                                bool success);
    void OptimizeForWorkload(const std::string& workload_profile);
    
    // Benchmarking
    void RunBenchmarks();
    void BenchmarkOperation(const std::string& operation, size_t iterations = 1000);
    
    // Diagnostics
    std::string GetEngineStatus() const;
    void PrintDispatchDecision(const DispatchMetrics& metrics) const;

private:
    ComputeEngine preferred_engine_;
    bool fallback_enabled_;
    double performance_threshold_ms_;
    bool learning_enabled_;
    
    mutable DispatchMetrics last_metrics_;
    std::map<ComputeEngine, bool> engine_availability_;
    std::map<ComputeEngine, std::map<std::string, EnginePerformance>> engine_performance_;
    
#ifdef ENABLE_EIGEN
    std::unique_ptr<EigenEngine> eigen_engine_;
#endif

#ifdef ENABLE_NANOBIND
    std::unique_ptr<NanobindInterface> nanobind_interface_;
#endif

    // Decision engine
    ComputeEngine SelectBestEngine(const std::string& operation, 
                                  OperationComplexity complexity,
                                  size_t data_size) const;
    
    OperationComplexity ClassifyOperation(const std::string& operation) const;
    size_t EstimateDataSize(const std::vector<std::string>& args) const;
    
    // Engine-specific dispatchers
    EngineResult DispatchToNative(const std::string& operation, 
                                const std::vector<std::string>& args);
    
#ifdef ENABLE_EIGEN
    EngineResult DispatchToEigen(const std::string& operation, 
                               const std::vector<std::string>& args);
#endif

#ifdef ENABLE_NANOBIND
    EngineResult DispatchToPython(const std::string& operation, 
                                const std::vector<std::string>& args);
#endif

    // Performance tracking
    void RecordDispatchMetrics(ComputeEngine engine, 
                              const std::string& operation,
                              OperationComplexity complexity,
                              double execution_time_ms,
                              size_t data_size,
                              bool success,
                              const std::string& reason);
    
    // Learning algorithms
    void UpdateLearningModel(const DispatchMetrics& metrics);
    double PredictExecutionTime(ComputeEngine engine, 
                               const std::string& operation,
                               size_t data_size) const;
    
    // Utilities
    std::string EngineToString(ComputeEngine engine) const;
    std::string ComplexityToString(OperationComplexity complexity) const;
};

/**
 * @brief RAII performance timer for dispatch operations
 */
class DispatchTimer {
public:
    DispatchTimer(const std::string& operation_name);
    ~DispatchTimer();
    
    double GetElapsedMs() const;
    void Stop();
    
private:
    std::string operation_name_;
    std::chrono::high_resolution_clock::time_point start_time_;
    bool stopped_;
};

/**
 * @brief Global dispatcher instance
 */
extern std::unique_ptr<SelectiveDispatcher> g_dispatcher;

/**
 * @brief Convenience functions for common dispatch operations
 */
namespace Dispatch {
    
    // Quick dispatch functions
    EngineResult Calculate(const std::string& expression);
    EngineResult Matrix(const std::string& operation, const std::vector<Matrix>& matrices);
    EngineResult Function(const std::string& function_name, const std::vector<double>& args);
    
    // Engine preference
    void PreferEngine(ComputeEngine engine);
    void EnableFallback(bool enable = true);
    
    // Performance
    std::string GetReport();
    void Benchmark();
    void OptimizeForSpeed();
    
    // Initialize dispatcher
    void Initialize();
    void Shutdown();
    
} // namespace Dispatch

// Macros for performance measurement
#define DISPATCH_TIMER(op) DispatchTimer timer(op)
#define SENNA_DISPATCH(op) DispatchTimer timer(std::string(op))

} // namespace AXIOM

#endif // SELECTIVE_DISPATCHER_H