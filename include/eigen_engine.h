/**
 * @file eigen_engine.h
 * @brief High-performance CPU computation engine using Eigen library
 * 
 * Provides advanced matrix operations, linear algebra, and mathematical
 * computations with SIMD optimizations and memory efficiency.
 */

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
    Matrix CreateMatrix(const std::vector<std::vector<double>>& data);
    Vector CreateVector(const std::vector<double>& data);
    
    // High-performance linear algebra
    Matrix MatrixMultiply(const Matrix& A, const Matrix& B);
    Vector MatrixVectorMultiply(const Matrix& A, const Vector& x);
    Matrix MatrixAdd(const Matrix& A, const Matrix& B);
    Matrix MatrixSubtract(const Matrix& A, const Matrix& B);
    
    // Advanced operations
    double Determinant(const Matrix& A);
    Matrix Inverse(const Matrix& A);
    Matrix Transpose(const Matrix& A);
    Matrix PseudoInverse(const Matrix& A);
    
    // Eigenvalue/Eigenvector computations
    std::pair<Vector, Matrix> EigenDecomposition(const Matrix& A);
    std::tuple<Matrix, Vector, Matrix> SVD(const Matrix& A);
    
    // System solving (ultra-fast)
    Vector SolveLinearSystem(const Matrix& A, const Vector& b);
    Matrix SolveMultipleRHS(const Matrix& A, const Matrix& B);
    
    // Specialized mathematical functions
    Matrix MatrixExponential(const Matrix& A);
    Matrix MatrixLogarithm(const Matrix& A);
    Matrix MatrixPower(const Matrix& A, double power);
    
    // Signal processing operations
    Vector FFT(const Vector& signal);
    Vector IFFT(const Vector& spectrum);
    Vector Convolution(const Vector& signal1, const Vector& signal2);
    Vector CrossCorrelation(const Vector& signal1, const Vector& signal2);
    
    // Statistical operations (vectorized)
    double Mean(const Vector& data);
    double StandardDeviation(const Vector& data);
    double Variance(const Vector& data);
    Vector Normalize(const Vector& data);
    Matrix Covariance(const Matrix& data);
    Vector PolynomialFit(const Vector& x, const Vector& y, int degree);
    
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
    CPUOptimizationLevel optimization_level_;
    bool simd_enabled_;
    int num_threads_;
    // THREAD-SAFETY FIX: mutable allows modification in const methods without const_cast
    mutable CPUPerformanceMetrics last_metrics_;
    
    // Internal cache for performance
    mutable std::unordered_map<std::string, Matrix> matrix_cache_;
    mutable std::unordered_map<std::string, Vector> vector_cache_;
    
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
    PerformanceTimer(const std::string& operation_name);
    ~PerformanceTimer();
    
    double GetElapsedMs() const;
    
private:
    std::string operation_name_;
    std::chrono::high_resolution_clock::time_point start_time_;
};

// Convenience macros for performance measurement (disabled for maximum speed)
#define MEASURE_PERFORMANCE(op_name) do {} while(0)
#define SENNA_SPEED_EIGEN(op_name) do {} while(0)

} // namespace AXIOM

#endif // ENABLE_EIGEN

#endif // EIGEN_ENGINE_H