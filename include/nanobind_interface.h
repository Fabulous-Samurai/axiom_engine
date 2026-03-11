/**
 * @file nanobind_interface.h
 * @brief Modern Python-C++ interface using nanobind
 * 
 * Provides seamless integration between Python and C++ with
 * minimal overhead and automatic type conversions.
 */

#pragma once

#ifndef NANOBIND_INTERFACE_H
#define NANOBIND_INTERFACE_H

#ifdef ENABLE_NANOBIND

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/optional.h>
#include <nanobind/operators.h>

#ifdef ENABLE_EIGEN
#include <nanobind/eigen/dense.h>
#include "eigen_engine.h"
#endif

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

namespace nb = nanobind;
using namespace nb::literals;

namespace AXIOM {

/**
 * @brief Performance metrics for Python-C++ interop
 */
struct InteropMetrics {
    double call_overhead_us = 0.0;
    size_t data_transferred_bytes = 0;
    size_t conversions_performed = 0;
    bool zero_copy_used = false;
    std::string last_function_called;
};

/**
 * @brief Modern nanobind-based Python interface
 */
class NanobindInterface {
public:
    NanobindInterface();
    ~NanobindInterface() = default;

    // Core interface methods
    void Initialize();
    void RegisterMethods();
    void RegisterEigenMethods();
    
    // Python function calling
    nb::object CallPythonFunction(const std::string& module_name, 
                                  const std::string& function_name,
                                  const std::vector<nb::object>& args = {});
    
    // Type conversions with zero-copy when possible
    std::vector<double> ConvertFromNumPy(const nb::ndarray<nb::numpy, double>& array);
    nb::ndarray<nb::numpy, double> ConvertToNumPy(const std::vector<double>& data);
    
#ifdef ENABLE_EIGEN
    // Eigen integration with zero-copy
    AXIOM::EigenEngine::Matrix ConvertFromNumPyMatrix(const nb::ndarray<nb::numpy, double, nb::ndim<2>>& array);
    nb::ndarray<nb::numpy, double, nb::ndim<2>> ConvertToNumPyMatrix(const AXIOM::EigenEngine::Matrix& matrix);
    
    AXIOM::EigenEngine::Vector ConvertFromNumPyVector(const nb::ndarray<nb::numpy, double>& array);
    nb::ndarray<nb::numpy, double> ConvertToNumPyVector(const AXIOM::EigenEngine::Vector& vector);
#endif
    
    // Advanced Python integration
    nb::object ExecutePythonCode(const std::string& code);
    nb::object ImportModule(const std::string& module_name);
    void AddToSysPath(const std::string& path);
    
    // Error handling
    std::string GetLastPythonError() const;
    void ClearPythonError();
    
    // Performance monitoring
    InteropMetrics GetMetrics() const { return metrics_; }
    void ResetMetrics();
    std::string GetPerformanceReport() const;
    
    // Memory management
    void GarbageCollect();
    size_t GetPythonMemoryUsage() const;

private:
    mutable InteropMetrics metrics_;
    std::string last_error_;
    
    // Performance measurement helpers
    template<typename Func>
    auto MeasureCall(const std::string& function_name, Func&& func) -> decltype(func());
    
    void UpdateMetrics(const std::string& function_name, double overhead_us, 
                      size_t data_bytes, bool zero_copy);
};

/**
 * @brief RAII wrapper for Python GIL management
 */
class GILGuard {
public:
    GILGuard();
    ~GILGuard();
    
private:
    // Platform-specific GIL state
#ifdef Py_GIL_DISABLED
    // For free-threaded Python (3.13+)
    void* gil_state_;
#endif
};

/**
 * @brief High-performance Python function wrapper
 */
template<typename RetType, typename... Args>
class PythonFunction {
public:
    PythonFunction(const std::string& module_name, const std::string& function_name);
    
    RetType operator()(Args... args);
    
    // Performance options
    void EnableZeroCopy(bool enable = true) { zero_copy_enabled_ = enable; }
    void SetCaching(bool enable = true) { caching_enabled_ = enable; }
    
private:
    std::string module_name_;
    std::string function_name_;
    nb::object cached_function_;
    bool zero_copy_enabled_ = true;
    bool caching_enabled_ = true;
    bool function_loaded_ = false;
    
    void LoadFunction();
};

/**
 * @brief Zero-copy array wrapper for efficient data transfer
 */
template<typename T>
class ZeroCopyArray {
public:
    ZeroCopyArray(T* data, size_t size, bool owns_data = false);
    ~ZeroCopyArray();
    
    nb::ndarray<nb::numpy, T> ToNumPy();
    static ZeroCopyArray FromNumPy(const nb::ndarray<nb::numpy, T>& array);
    
    T* Data() const { return data_; }
    size_t Size() const { return size_; }
    bool OwnsData() const { return owns_data_; }
    
private:
    T* data_;
    size_t size_;
    bool owns_data_;
};

// Global nanobind interface instance
extern std::unique_ptr<NanobindInterface> g_nanobind_interface;

// Convenience functions for common operations
namespace Nanobind {
    
    // Quick Python execution
    nb::object Execute(const std::string& code);
    nb::object Import(const std::string& module_name);
    
    // Fast numpy conversions
    template<typename T>
    std::vector<T> FromNumPy(const nb::ndarray<nb::numpy, T>& array);
    
    template<typename T>
    nb::ndarray<nb::numpy, T> ToNumPy(const std::vector<T>& data);
    
#ifdef ENABLE_EIGEN
    // Eigen-NumPy bridge
    AXIOM::EigenEngine::Matrix MatrixFromNumPy(const nb::ndarray<nb::numpy, double, nb::ndim<2>>& array);
    nb::ndarray<nb::numpy, double, nb::ndim<2>> MatrixToNumPy(const AXIOM::EigenEngine::Matrix& matrix);
#endif
    
    // Performance helpers
    std::string GetPerformanceReport();
    void OptimizeForSpeed();
    
} // namespace Nanobind

} // namespace AXIOM

// Nanobind module definition macros
#define AXIOM_NANOBIND_MODULE(name) NB_MODULE(name, m)
#define AXIOM_NANOBIND_CLASS(cls) nb::class_<cls>(m, #cls)
#define AXIOM_NANOBIND_FUNCTION(func) m.def(#func, &func)

#endif // ENABLE_NANOBIND

#endif // NANOBIND_INTERFACE_H