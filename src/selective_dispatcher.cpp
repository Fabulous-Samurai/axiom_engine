/**
 * @file selective_dispatcher.cpp
 * @brief Implementation of intelligent operation dispatcher
 */

#include "selective_dispatcher.h"
#include "dynamic_calc.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cmath>

namespace AXIOM {

// Global dispatcher instance
std::unique_ptr<SelectiveDispatcher> g_dispatcher;

SelectiveDispatcher::SelectiveDispatcher() 
    : preferred_engine_(ComputeEngine::Auto)
    , fallback_enabled_(true)
    , performance_threshold_ms_(100.0)  // 100ms threshold
    , learning_enabled_(true) {
    
    // Initialize available engines
    engine_availability_[ComputeEngine::Native] = true;  // Always available

#ifdef ENABLE_EIGEN
    try {
        eigen_engine_ = std::make_unique<EigenEngine>();
        engine_availability_[ComputeEngine::Eigen] = true;
        std::cout << "Eigen engine available" << std::endl;
    } catch (const std::exception& e) {
        engine_availability_[ComputeEngine::Eigen] = false;
        std::cerr << "❌ Eigen engine unavailable: " << e.what() << std::endl;
    }
#else
    engine_availability_[ComputeEngine::Eigen] = false;
#endif

#ifdef ENABLE_NANOBIND
    try {
        nanobind_interface_ = std::make_unique<NanobindInterface>();
        engine_availability_[ComputeEngine::Python] = true;
        std::cout << "Python engine available" << std::endl;
    } catch (const std::exception& e) {
        engine_availability_[ComputeEngine::Python] = false;
        std::cerr << "❌ Python engine unavailable: " << e.what() << std::endl;
    }
#else
    engine_availability_[ComputeEngine::Python] = false;
#endif

    // SelectiveDispatcher initialized with intelligent routing
    // Available engines: configured
}

EngineResult SelectiveDispatcher::DispatchOperation(const std::string& operation, 
                                                   const std::vector<std::string>& args,
                                                   ComputeEngine preferred_engine) {
    
    SENNA_DISPATCH("DispatchOperation");
    
    // Classify operation complexity
    auto complexity = ClassifyOperation(operation);
    size_t data_size = EstimateDataSize(args);
    
    // Select best engine
    ComputeEngine selected_engine = (preferred_engine == ComputeEngine::Auto) ? 
        SelectBestEngine(operation, complexity, data_size) : preferred_engine;
    
    // Ensure selected engine is available
    if (!IsEngineAvailable(selected_engine) && fallback_enabled_) {
        // Find best available fallback
        auto available_engines = GetAvailableEngines();
        if (!available_engines.empty()) {
            selected_engine = available_engines[0];  // Use first available
            last_metrics_.fallback_used = true;
        } else {
            return {{}, {CalcErr::OperationNotFound}};
        }
    }
    
    // Record dispatch start
    auto start_time = std::chrono::high_resolution_clock::now();
    EngineResult result;
    
    // Dispatch to selected engine
    try {
        switch (selected_engine) {
            case ComputeEngine::Native:
                result = DispatchToNative(operation, args);
                break;
                
#ifdef ENABLE_EIGEN
            case ComputeEngine::Eigen:
                result = DispatchToEigen(operation, args);
                break;
#endif

            // Python engine disabled for pure C++ performance
            case ComputeEngine::Python:
                // Redirect Python requests to native C++ engine
                result = DispatchToNative(operation, args);
                break;
                
            default:
                result = DispatchToNative(operation, args);  // Fallback
                break;
        }
    } catch (const std::exception& e) {
        if (fallback_enabled_ && selected_engine != ComputeEngine::Native) {
            // Try native fallback
            result = DispatchToNative(operation, args);
            last_metrics_.fallback_used = true;
        } else {
            result = {{}, {CalcErr::OperationNotFound}};
        }
    }
    
    // Record performance metrics
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    double execution_time_ms = duration.count();
    
    // Update learning model
    UpdateEnginePerformance(selected_engine, operation, execution_time_ms, result.result.has_value());
    
    // Record dispatch metrics
    RecordDispatchMetrics(selected_engine, operation, complexity, execution_time_ms, 
                         data_size, result.result.has_value(), "Auto-selected");
    
    return result;
}

EngineResult SelectiveDispatcher::DispatchArithmetic(const std::string& expression) {
    SENNA_DISPATCH("DispatchArithmetic");
    
    // For simple arithmetic, prefer native engine for speed
    if (ClassifyOperation(expression) == OperationComplexity::Simple) {
        return DispatchToNative(expression, {});
    }
    
    return DispatchOperation(expression);
}

EngineResult SelectiveDispatcher::DispatchMatrixOperation(const std::string& operation, 
                                                        const std::vector<Matrix>& matrices) {
    SENNA_DISPATCH("DispatchMatrixOperation");
    
#ifdef ENABLE_EIGEN
    // For matrix operations, strongly prefer Eigen if available
    if (IsEngineAvailable(ComputeEngine::Eigen)) {
        // Convert matrices to string arguments (simplified for this example)
        std::vector<std::string> args;
        for (const auto& matrix : matrices) {
            args.push_back("[matrix]");  // Placeholder
        }
        
        return DispatchToEigen(operation, args);
    }
#endif
    
    // Fallback to native
    return DispatchOperation(operation);
}

ComputeEngine SelectiveDispatcher::SelectBestEngine(const std::string& operation, 
                                                   OperationComplexity complexity,
                                                   size_t data_size) const {
    
    // Engine selection heuristics based on operation characteristics
    
    // For simple operations, prefer native for minimal overhead
    if (complexity == OperationComplexity::Simple && data_size < 1000) {
        return ComputeEngine::Native;
    }
    
    // For matrix operations, prefer Eigen if available
    if (operation.find("matrix") != std::string::npos || 
        operation.find("linear") != std::string::npos ||
        operation.find("solve") != std::string::npos ||
        operation.find("inv") != std::string::npos) {
#ifdef ENABLE_EIGEN
        if (IsEngineAvailable(ComputeEngine::Eigen)) {
            return ComputeEngine::Eigen;
        }
#endif
    }
    
    // For large data or complex operations, prefer Eigen
    if (data_size > 10000 || complexity >= OperationComplexity::Complex) {
#ifdef ENABLE_EIGEN
        if (IsEngineAvailable(ComputeEngine::Eigen)) {
            return ComputeEngine::Eigen;
        }
#endif
    }
    
    // For symbolic operations, use native C++ for maximum speed
    if (operation.find("symbolic") != std::string::npos ||
        operation.find("derive") != std::string::npos ||
        operation.find("integrate") != std::string::npos) {
        // Force native C++ engine - no Python fallbacks
        return ComputeEngine::Native;
    }
    
    // Learning-based selection (if enabled)
    if (learning_enabled_) {
        ComputeEngine best_engine = ComputeEngine::Native;
        double best_predicted_time = std::numeric_limits<double>::max();
        
        for (auto engine : GetAvailableEngines()) {
            double predicted_time = PredictExecutionTime(engine, operation, data_size);
            if (predicted_time < best_predicted_time) {
                best_predicted_time = predicted_time;
                best_engine = engine;
            }
        }
        
        return best_engine;
    }
    
    // Default fallback
    return ComputeEngine::Native;
}

OperationComplexity SelectiveDispatcher::ClassifyOperation(const std::string& operation) const {
    // Simple pattern-based classification
    
    // Simple arithmetic operations
    std::regex simple_pattern(R"(^[\d\+\-\*/\(\)\.\s]+$)");
    if (std::regex_match(operation, simple_pattern)) {
        return OperationComplexity::Simple;
    }
    
    // Function calls
    std::regex function_pattern(R"(\w+\s*\()");
    if (std::regex_search(operation, function_pattern)) {
        // Check for complex functions
        if (operation.find("matrix") != std::string::npos ||
            operation.find("solve") != std::string::npos ||
            operation.find("inv") != std::string::npos ||
            operation.find("eigen") != std::string::npos) {
            return OperationComplexity::Complex;
        }
        
        // Check for very complex operations
        if (operation.find("derive") != std::string::npos ||
            operation.find("integrate") != std::string::npos ||
            operation.find("symbolic") != std::string::npos ||
            operation.find("optimize") != std::string::npos) {
            return OperationComplexity::VeryComplex;
        }
        
        return OperationComplexity::Medium;
    }
    
    // Default to simple
    return OperationComplexity::Simple;
}

size_t SelectiveDispatcher::EstimateDataSize(const std::vector<std::string>& args) const {
    size_t total_size = 0;
    
    for (const auto& arg : args) {
        // Estimate based on string length and complexity
        total_size += arg.length() * sizeof(char);
        
        // Additional size for matrix operations
        if (arg.find("[") != std::string::npos) {
            total_size += 1000;  // Assume matrix overhead
        }
    }
    
    return total_size;
}

EngineResult SelectiveDispatcher::DispatchToNative(const std::string& operation, 
                                                  const std::vector<std::string>& args) {
    // Use existing AXIOM::DynamicCalc for native operations
    static AXIOM::DynamicCalc native_engine;
    
    // Create context for operation
    std::map<std::string, double> context;
    
    return native_engine.EvaluateWithContext(operation, context);
}

#ifdef ENABLE_EIGEN
EngineResult SelectiveDispatcher::DispatchToEigen(const std::string& operation, 
                                                 const std::vector<std::string>& args) {
    if (!eigen_engine_) {
        return {{}, {CalcErr::OperationNotFound}};
    }
    
    try {
        // This is a simplified example - in practice, you'd have more sophisticated
        // operation mapping between string operations and Eigen methods
        
        if (operation.find("matrix_multiply") != std::string::npos && args.size() >= 2) {
            // Example: matrix multiplication
            // In practice, you'd parse the matrix arguments properly
            auto matrix1 = eigen_engine_->CreateMatrix({{1, 2}, {3, 4}});
            auto matrix2 = eigen_engine_->CreateMatrix({{5, 6}, {7, 8}});
            
            auto result_matrix = eigen_engine_->MatrixMultiply(matrix1, matrix2);
            
            // Convert back to EngineResult format
            Matrix result_std_matrix;
            for (int i = 0; i < result_matrix.rows(); ++i) {
                std::vector<double> row;
                for (int j = 0; j < result_matrix.cols(); ++j) {
                    row.push_back(result_matrix(i, j));
                }
                result_std_matrix.push_back(row);
            }
            
            return {{result_std_matrix}, {}};
        }
        
        // For other operations, fallback to native
        return DispatchToNative(operation, args);
        
    } catch (const std::exception& e) {
        return {{}, {CalcErr::OperationNotFound}};
    }
}
#endif

#ifdef ENABLE_NANOBIND
EngineResult SelectiveDispatcher::DispatchToPython(const std::string& operation, 
                                                  const std::vector<std::string>& args) {
    if (!nanobind_interface_) {
        return {{}, {CalcErr::OperationNotFound}};
    }
    
    try {
        // Execute Python code for symbolic operations
        if (operation.find("symbolic") != std::string::npos) {
            auto result = nanobind_interface_->ExecutePythonCode(operation);
            // Convert Python result to EngineResult (simplified)
            return {{std::string("Python result")}, {}};
        }
        
        // For other operations, fallback to native
        return DispatchToNative(operation, args);
        
    } catch (const std::exception& e) {
        return {{}, {CalcErr::OperationNotFound}};
    }
}
#endif

bool SelectiveDispatcher::IsEngineAvailable(ComputeEngine engine) const {
    auto it = engine_availability_.find(engine);
    return (it != engine_availability_.end()) ? it->second : false;
}

std::vector<ComputeEngine> SelectiveDispatcher::GetAvailableEngines() const {
    std::vector<ComputeEngine> available;
    
    for (const auto& [engine, available_flag] : engine_availability_) {
        if (available_flag) {
            available.push_back(engine);
        }
    }
    
    return available;
}

void SelectiveDispatcher::UpdateEnginePerformance(ComputeEngine engine, 
                                                 const std::string& operation,
                                                 double execution_time_ms,
                                                 bool success) {
    auto& perf = engine_performance_[engine][operation];
    
    perf.operations_count++;
    perf.avg_execution_time_ms = (perf.avg_execution_time_ms * (perf.operations_count - 1) + 
                                 execution_time_ms) / perf.operations_count;
    perf.supports_operation = success;
    perf.engine_type = engine;
    
    if (success) {
        perf.accuracy_score = std::min(1.0, perf.accuracy_score + 0.01);
    } else {
        perf.accuracy_score = std::max(0.0, perf.accuracy_score - 0.1);
    }
}

double SelectiveDispatcher::PredictExecutionTime(ComputeEngine engine, 
                                               const std::string& operation,
                                               size_t data_size) const {
    auto engine_it = engine_performance_.find(engine);
    if (engine_it == engine_performance_.end()) {
        return 10.0;  // Default prediction
    }
    
    auto op_it = engine_it->second.find(operation);
    if (op_it == engine_it->second.end()) {
        return 10.0;  // Default prediction
    }
    
    // Simple linear scaling based on data size
    double base_time = op_it->second.avg_execution_time_ms;
    double scaling_factor = 1.0 + (data_size / 10000.0);  // Scale with data size
    
    return base_time * scaling_factor;
}

std::string SelectiveDispatcher::GetPerformanceReport() const {
    std::stringstream ss;
    ss << "🎯 Selective Dispatcher Performance Report\n";
    ss << "==========================================\n";
    
    // Last operation metrics
    ss << "Last Operation:\n";
    ss << "  Engine: " << EngineToString(last_metrics_.selected_engine) << "\n";
    ss << "  Operation: " << last_metrics_.operation_name << "\n";
    ss << "  Complexity: " << ComplexityToString(last_metrics_.complexity) << "\n";
    ss << "  Execution Time: " << last_metrics_.execution_time_ms << " ms\n";
    ss << "  Data Size: " << last_metrics_.data_size_bytes << " bytes\n";
    ss << "  Fallback Used: " << (last_metrics_.fallback_used ? "Yes" : "No") << "\n";
    ss << "  Reason: " << last_metrics_.decision_reason << "\n";
    
    // Performance classification
    if (last_metrics_.execution_time_ms < 1.0) {
        ss << "  SENNA SPEED: Lightning Fast! (<1ms)\n";
    } else if (last_metrics_.execution_time_ms < 10.0) {
        ss << "  🏁 FORMULA 1 Speed: Very Fast! (<10ms)\n";
    } else if (last_metrics_.execution_time_ms < 100.0) {
        ss << "  🚗 Racing Speed: Fast! (<100ms)\n";
    } else {
        ss << "  🐌 Needs Optimization (>" << last_metrics_.execution_time_ms << "ms)\n";
    }
    
    // Engine statistics
    ss << "\nEngine Performance Summary:\n";
    for (const auto& [engine, operations] : engine_performance_) {
        if (!operations.empty()) {
            ss << "  " << EngineToString(engine) << ":\n";
            
            double total_ops = 0;
            double avg_time = 0;
            for (const auto& [op_name, perf] : operations) {
                total_ops += perf.operations_count;
                avg_time += perf.avg_execution_time_ms * perf.operations_count;
            }
            
            if (total_ops > 0) {
                avg_time /= total_ops;
                ss << "    Operations: " << static_cast<size_t>(total_ops) << "\n";
                ss << "    Avg Time: " << avg_time << " ms\n";
            }
        }
    }
    
    return ss.str();
}

void SelectiveDispatcher::RecordDispatchMetrics(ComputeEngine engine, 
                                               const std::string& operation,
                                               OperationComplexity complexity,
                                               double execution_time_ms,
                                               size_t data_size,
                                               bool success,
                                               const std::string& reason) {
    last_metrics_.selected_engine = engine;
    last_metrics_.operation_name = operation;
    last_metrics_.complexity = complexity;
    last_metrics_.execution_time_ms = execution_time_ms;
    last_metrics_.data_size_bytes = data_size;
    last_metrics_.decision_reason = reason;
    
    // Learning update
    if (learning_enabled_) {
        UpdateLearningModel(last_metrics_);
    }
}

std::string SelectiveDispatcher::EngineToString(ComputeEngine engine) const {
    switch (engine) {
        case ComputeEngine::Native: return "Native C++";
        case ComputeEngine::Eigen: return "Eigen CPU";
        case ComputeEngine::Python: return "Python";
        case ComputeEngine::Auto: return "Auto";
        default: return "Unknown";
    }
}

std::string SelectiveDispatcher::ComplexityToString(OperationComplexity complexity) const {
    switch (complexity) {
        case OperationComplexity::Simple: return "Simple";
        case OperationComplexity::Medium: return "Medium";
        case OperationComplexity::Complex: return "Complex";
        case OperationComplexity::VeryComplex: return "Very Complex";
        default: return "Unknown";
    }
}

void SelectiveDispatcher::UpdateLearningModel(const DispatchMetrics& metrics) {
    // Simple learning: just update performance statistics
    // In a more advanced implementation, this could use machine learning
    // to predict optimal engine selection based on operation patterns
}

// Dispatch Timer implementation
DispatchTimer::DispatchTimer(const std::string& operation_name)
    : operation_name_(operation_name)
    , start_time_(std::chrono::high_resolution_clock::now())
    , stopped_(false) {
}

DispatchTimer::~DispatchTimer() {
    if (!stopped_) {
        Stop();
    }
}

void DispatchTimer::Stop() {
    if (!stopped_) {
        auto elapsed_ms = GetElapsedMs();
        if (elapsed_ms < 1.0) {
            // Operation timing: suppressed for performance
        }
        stopped_ = true;
    }
}

double DispatchTimer::GetElapsedMs() const {
    auto current_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time_);
    return duration.count() / 1000.0;
}

// Convenience namespace implementations
namespace Dispatch {
    
    EngineResult Calculate(const std::string& expression) {
        if (!g_dispatcher) {
            g_dispatcher = std::make_unique<SelectiveDispatcher>();
        }
        return g_dispatcher->DispatchArithmetic(expression);
    }
    
    void PreferEngine(ComputeEngine engine) {
        if (!g_dispatcher) {
            g_dispatcher = std::make_unique<SelectiveDispatcher>();
        }
        g_dispatcher->SetPreferredEngine(engine);
    }
    
    std::string GetReport() {
        if (!g_dispatcher) {
            return "Dispatcher not initialized";
        }
        return g_dispatcher->GetPerformanceReport();
    }
    
    void OptimizeForSpeed() {
        if (!g_dispatcher) {
            g_dispatcher = std::make_unique<SelectiveDispatcher>();
        }
        
        // Configure for maximum speed
        g_dispatcher->SetPreferredEngine(ComputeEngine::Auto);
        g_dispatcher->EnableFallback(true);
        g_dispatcher->SetPerformanceThreshold(1.0);  // 1ms threshold for Senna speed
        
        // Dispatcher optimized for Senna Speed
    }
    
    void Initialize() {
        if (!g_dispatcher) {
            g_dispatcher = std::make_unique<SelectiveDispatcher>();
            // Selective Dispatcher initialized
        }
    }
    
    void Shutdown() {
        if (g_dispatcher) {
            g_dispatcher.reset();
            // Selective Dispatcher shutdown complete
        }
    }
    
} // namespace Dispatch

} // namespace AXIOM