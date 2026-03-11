/**
 * @file eigen_engine.h
 * @brief High-performance CPU computation engine using Eigen library
 * 
 * Provides advanced matrix operations, linear algebra, and mathematical
 * computations with SIMD optimizations and memory efficiency.
 */

#pragma once

#ifndef EIGEN_ENGINE_H
#define EIGEN_ENGINE_H

#ifdef ENABLE_EIGEN

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <Eigen/Eigenvalues>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>

namespace AXIOM {

/**
 * @brief Performance metrics for CPU operations
 */
struct CPUPerformanceMetrics {
    double execution_time_ms = 0.0;
    size_t memory_used_bytes = 0;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    bool simd_used = false;
    std::string operation_type;
};

/**
 * @brief CPU optimization levels
 */
enum class CPUOptimizationLevel {
    Basic,      // No special optimizations
    SIMD,       // Use SIMD instructions
    Parallel,   // Multi-threaded operations
    Vectorized, // Full vectorization
    Extreme     // All optimizations + custom kernels
};

/**
 * @brief Eigen-based high-performance CPU engine
 */
class EigenEngine {
public:
    using Matrix = Eigen::MatrixXd;
    using Vector = Eigen::VectorXd;
    using SparseMatrix = Eigen::SparseMatrix<double>;
    using Complex = std::complex<double>;
    using ComplexMatrix = Eigen::MatrixXcd;

    EigenEngine();
    ~EigenEngine() = default;

    // Configuration
    void SetOptimizationLevel(CPUOptimizationLevel level);
    void EnableSIMD(bool enable = true);
    void SetNumThreads(int num_threads = -1); // -1 for auto

    // Matrix Operations (🏎️ Senna Speed with Eigen!)
    Matrix CreateMatrix(const std::vector<std::vector<double>>& data) const;
    Vector CreateVector(const std::vector<double>& data) const;
    
    // High-performance linear algebra
    Matrix MatrixMultiply(const Matrix& A, const Matrix& B) const;
    Vector MatrixVectorMultiply(const Matrix& A, const Vector& x) const;
    Matrix MatrixAdd(const Matrix& A, const Matrix& B) const;
    Matrix MatrixSubtract(const Matrix& A, const Matrix& B) const;
    
    // Advanced operations
    double Determinant(const Matrix& A) const;
    Matrix Inverse(const Matrix& A) const;
    Matrix Transpose(const Matrix& A) const;
    Matrix PseudoInverse(const Matrix& A) const;
    
    // Eigenvalue/Eigenvector computations
    std::pair<Vector, Matrix> EigenDecomposition(const Matrix& A) const;
    std::tuple<Matrix, Vector, Matrix> SVD(const Matrix& A) const;
    
    // System solving (ultra-fast)
    Vector SolveLinearSystem(const Matrix& A, const Vector& b) const;
    Matrix SolveMultipleRHS(const Matrix& A, const Matrix& B) const;
    
    // Specialized mathematical functions
    Matrix MatrixExponential(const Matrix& A) const;
    Matrix MatrixLogarithm(const Matrix& A) const;
    Matrix MatrixPower(const Matrix& A, double power) const;
    
    // Signal processing operations
    Vector FFT(const Vector& signal) const;
    Vector IFFT(const Vector& spectrum) const;
    Vector Convolution(const Vector& signal1, const Vector& signal2) const;
    Vector CrossCorrelation(const Vector& signal1, const Vector& signal2) const;
    
    // Statistical operations (vectorized)
    double Mean(const Vector& data) const;
    double StandardDeviation(const Vector& data) const;
    double Variance(const Vector& data) const;
    Vector Normalize(const Vector& data) const;
    Matrix Covariance(const Matrix& data) const;
    Vector PolynomialFit(const Vector& x, const Vector& y, int degree) const;
    
    // Advanced calculus (numerical)
    Vector Gradient(const std::function<double(const Vector&)>& func, const Vector& x);
    Matrix Hessian(const std::function<double(const Vector&)>& func, const Vector& x);
    double Integrate1D(const std::function<double(double)>& func, double a, double b, int method = 0);
    double FindRoot(const std::function<double(double)>& func, double initial_guess);
    
    // Optimization
    Vector OptimizeFunction(const std::function<double(const Vector&)>& func, 
                           const Vector& initial_guess,
                           const std::string& method = "BFGS");
    
    // Performance monitoring
    CPUPerformanceMetrics GetLastMetrics() const { return last_metrics_; }
    void ResetMetrics();
    std::string GetPerformanceReport() const;
    
    // Memory management
    void ClearCache();
    size_t GetMemoryUsage() const;
    
    // Utility functions
    std::string MatrixToString(const Matrix& mat, int precision = 6) const;
    std::string VectorToString(const Vector& vec, int precision = 6) const;
    Matrix LoadMatrix(const std::string& filename);
    void SaveMatrix(const Matrix& mat, const std::string& filename);

private:
    CPUOptimizationLevel optimization_level_{CPUOptimizationLevel::SIMD};
    bool simd_enabled_{true};
    int num_threads_{static_cast<int>(std::thread::hardware_concurrency())};
    // THREAD-SAFETY FIX: mutable allows modification in const methods without const_cast
    mutable CPUPerformanceMetrics last_metrics_;
    
    // Internal cache for performance
    struct StringHash {
        using is_transparent = void;
        size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
    };
    mutable std::unordered_map<std::string, Matrix, StringHash, std::equal_to<>> matrix_cache_;
    mutable std::unordered_map<std::string, Vector, StringHash, std::equal_to<>> vector_cache_;
    
    // Performance helpers
    template<typename Func>
    auto MeasurePerformance(Func&& func, const std::string& operation) const -> decltype(func());
    
    void UpdateMetrics(const std::string& operation, double time_ms, size_t memory_bytes) const;
    std::string GenerateCacheKey(const std::string& operation, const void* data, size_t size) const;
    
    // Optimization implementations
    Matrix OptimizedMatMul(const Matrix& A, const Matrix& B) const;
    Vector OptimizedMatVecMul(const Matrix& A, const Vector& x) const;
    
    // SIMD helpers (when available)
#ifdef __AVX2__
    void EnableAVX2();
#endif
#ifdef __SSE4_1__
    void EnableSSE41();
#endif
};

/**
 * @brief RAII Performance Timer for automatic measurement
 */
class PerformanceTimer {
public:
    explicit PerformanceTimer(const std::string& operation_name);
    ~PerformanceTimer();
    
    double GetElapsedMs() const;
    
private:
    std::string operation_name_;
    std::chrono::high_resolution_clock::time_point start_time_{std::chrono::high_resolution_clock::now()};
};

// Convenience macros for performance measurement (disabled for maximum speed)
#define MEASURE_PERFORMANCE(op_name) do {} while(0)
#define SENNA_SPEED_EIGEN(op_name) do {} while(0)

} // namespace AXIOM

#endif // ENABLE_EIGEN

#endif // EIGEN_ENGINE_H