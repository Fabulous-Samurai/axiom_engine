/**
 * @file nanobind_interface.cpp
 * @brief Implementation of modern Python-C++ interface using nanobind
 */

#ifdef ENABLE_NANOBIND

#include "nanobind_interface.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace AXIOM {

std::unique_ptr<NanobindInterface> g_nanobind_interface;

NanobindInterface::NanobindInterface() {
    ResetMetrics();
    try {
        Initialize();
    } catch (const std::exception& e) {
        last_error_ = e.what();
    }
}

void NanobindInterface::Initialize() {
    if (!Py_IsInitialized()) {
        Py_Initialize();
    }

    RegisterMethods();
#ifdef ENABLE_EIGEN
    RegisterEigenMethods();
#endif
}

void NanobindInterface::RegisterMethods() {
    AddToSysPath(".");
}

void NanobindInterface::RegisterEigenMethods() {
#ifdef ENABLE_EIGEN
#endif
}

nb::object NanobindInterface::CallPythonFunction(
    const std::string& module_name,
    const std::string& function_name,
    const std::vector<nb::object>& args) {

    return MeasureCall(module_name + "." + function_name, [&]() -> nb::object {
        nb::gil_scoped_acquire gil;
        auto module = ImportModule(module_name);
        auto function = module.attr(function_name.c_str());

        if (args.empty()) {
            return function();
        }

        PyObject* py_args = PyTuple_New(static_cast<Py_ssize_t>(args.size()));
        if (!py_args) {
            throw std::runtime_error("Failed to allocate Python args tuple");
        }

        for (size_t i = 0; i < args.size(); ++i) {
            PyObject* item = args[i].ptr();
            Py_INCREF(item);
            PyTuple_SET_ITEM(py_args, static_cast<Py_ssize_t>(i), item);
        }

        PyObject* py_result = PyObject_CallObject(function.ptr(), py_args);
        Py_DECREF(py_args);

        if (!py_result) {
            throw nb::python_error();
        }

        return nb::steal<nb::object>(py_result);
    });
}

std::vector<double> NanobindInterface::ConvertFromNumPy(const nb::ndarray<nb::numpy, double>& array) {
    return MeasureCall("ConvertFromNumPy", [&]() -> std::vector<double> {
        std::vector<double> out(array.size());

        const auto* base = reinterpret_cast<const char*>(array.data());
        const ssize_t stride = array.stride(0);

        for (size_t i = 0; i < array.size(); ++i) {
            const auto* elem = reinterpret_cast<const double*>(base + static_cast<ssize_t>(i) * stride);
            out[i] = *elem;
        }

        UpdateMetrics("ConvertFromNumPy", 1.0, out.size() * sizeof(double), false);
        return out;
    });
}

nb::ndarray<nb::numpy, double> NanobindInterface::ConvertToNumPy(const std::vector<double>& data) {
    return MeasureCall("ConvertToNumPy", [&]() -> nb::ndarray<nb::numpy, double> {
        nb::gil_scoped_acquire gil;
        auto np = nb::module_::import_("numpy");
        auto arr = np.attr("array")(data, "dtype"_a = "float64");
        UpdateMetrics("ConvertToNumPy", 1.0, data.size() * sizeof(double), false);
        return nb::cast<nb::ndarray<nb::numpy, double>>(arr);
    });
}

#ifdef ENABLE_EIGEN
AXIOM::EigenEngine::Matrix NanobindInterface::ConvertFromNumPyMatrix(
    const nb::ndarray<nb::numpy, double, nb::ndim<2>>& array) {

    return MeasureCall("ConvertFromNumPyMatrix", [&]() -> AXIOM::EigenEngine::Matrix {
        const size_t rows = array.shape(0);
        const size_t cols = array.shape(1);

        AXIOM::EigenEngine::Matrix matrix(static_cast<Eigen::Index>(rows), static_cast<Eigen::Index>(cols));

        const auto* base = reinterpret_cast<const char*>(array.data());
        const ssize_t s0 = array.stride(0);
        const ssize_t s1 = array.stride(1);

        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                const auto* elem = reinterpret_cast<const double*>(
                    base + static_cast<ssize_t>(i) * s0 + static_cast<ssize_t>(j) * s1);
                matrix(static_cast<Eigen::Index>(i), static_cast<Eigen::Index>(j)) = *elem;
            }
        }

        UpdateMetrics("ConvertFromNumPyMatrix", 2.0, rows * cols * sizeof(double), false);
        return matrix;
    });
}

nb::ndarray<nb::numpy, double, nb::ndim<2>> NanobindInterface::ConvertToNumPyMatrix(
    const AXIOM::EigenEngine::Matrix& matrix) {

    return MeasureCall("ConvertToNumPyMatrix", [&]() -> nb::ndarray<nb::numpy, double, nb::ndim<2>> {
        nb::gil_scoped_acquire gil;
        auto np = nb::module_::import_("numpy");

        const size_t rows = static_cast<size_t>(matrix.rows());
        const size_t cols = static_cast<size_t>(matrix.cols());

        std::vector<std::vector<double>> nested(rows, std::vector<double>(cols));
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                nested[i][j] = matrix(static_cast<Eigen::Index>(i), static_cast<Eigen::Index>(j));
            }
        }

        auto arr = np.attr("array")(nested, "dtype"_a = "float64");
        UpdateMetrics("ConvertToNumPyMatrix", 1.5, rows * cols * sizeof(double), false);
        return nb::cast<nb::ndarray<nb::numpy, double, nb::ndim<2>>>(arr);
    });
}

AXIOM::EigenEngine::Vector NanobindInterface::ConvertFromNumPyVector(const nb::ndarray<nb::numpy, double>& array) {
    return MeasureCall("ConvertFromNumPyVector", [&]() -> AXIOM::EigenEngine::Vector {
        AXIOM::EigenEngine::Vector v(static_cast<Eigen::Index>(array.size()));

        const auto* base = reinterpret_cast<const char*>(array.data());
        const ssize_t s0 = array.stride(0);

        for (size_t i = 0; i < array.size(); ++i) {
            const auto* elem = reinterpret_cast<const double*>(base + static_cast<ssize_t>(i) * s0);
            v(static_cast<Eigen::Index>(i)) = *elem;
        }

        UpdateMetrics("ConvertFromNumPyVector", 1.2, array.size() * sizeof(double), false);
        return v;
    });
}

nb::ndarray<nb::numpy, double> NanobindInterface::ConvertToNumPyVector(const AXIOM::EigenEngine::Vector& vector) {
    return MeasureCall("ConvertToNumPyVector", [&]() -> nb::ndarray<nb::numpy, double> {
        std::vector<double> tmp(static_cast<size_t>(vector.size()));
        for (Eigen::Index i = 0; i < vector.size(); ++i) {
            tmp[static_cast<size_t>(i)] = vector(i);
        }
        return ConvertToNumPy(tmp);
    });
}
#endif

nb::object NanobindInterface::ExecutePythonCode(const std::string& code) {
    return MeasureCall("ExecutePythonCode", [&]() -> nb::object {
        nb::gil_scoped_acquire gil;

        nb::object globals = nb::steal<nb::object>(PyDict_New());
        nb::object locals = nb::steal<nb::object>(PyDict_New());
        PyDict_SetItemString(globals.ptr(), "__builtins__", PyEval_GetBuiltins());

        PyObject* eval_result = PyRun_StringFlags(
            code.c_str(),
            Py_eval_input,
            reinterpret_cast<PyObject*>(globals.ptr()),
            reinterpret_cast<PyObject*>(locals.ptr()),
            nullptr);

        if (eval_result != nullptr) {
            return nb::steal<nb::object>(eval_result);
        }

        PyErr_Clear();

        PyObject* exec_result = PyRun_StringFlags(
            code.c_str(),
            Py_file_input,
            reinterpret_cast<PyObject*>(globals.ptr()),
            reinterpret_cast<PyObject*>(locals.ptr()),
            nullptr);

        if (exec_result != nullptr) {
            Py_DECREF(exec_result);
            return nb::none();
        }

        throw nb::python_error();
    });
}

nb::object NanobindInterface::ImportModule(const std::string& module_name) {
    nb::gil_scoped_acquire gil;
    return nb::module_::import_(module_name.c_str());
}

void NanobindInterface::AddToSysPath(const std::string& path) {
    try {
        nb::gil_scoped_acquire gil;
        auto sys = nb::module_::import_("sys");
        sys.attr("path").attr("insert")(0, path);
    } catch (const std::exception& e) {
        last_error_ = e.what();
    }
}

std::string NanobindInterface::GetLastPythonError() const {
    return last_error_;
}

void NanobindInterface::ClearPythonError() {
    last_error_.clear();
}

void NanobindInterface::ResetMetrics() {
    metrics_ = InteropMetrics{};
}

std::string NanobindInterface::GetPerformanceReport() const {
    std::stringstream ss;
    ss << "Nanobind Interface Performance Report\n";
    ss << "Last Function: " << metrics_.last_function_called << "\n";
    ss << "Call Overhead (us): " << metrics_.call_overhead_us << "\n";
    ss << "Data Transferred (bytes): " << metrics_.data_transferred_bytes << "\n";
    ss << "Conversions: " << metrics_.conversions_performed << "\n";
    ss << "Zero-Copy Used: " << (metrics_.zero_copy_used ? "Yes" : "No") << "\n";
    return ss.str();
}

void NanobindInterface::GarbageCollect() {
    try {
        nb::gil_scoped_acquire gil;
        auto gc = nb::module_::import_("gc");
        gc.attr("collect")();
    } catch (const std::exception& e) {
        last_error_ = e.what();
    }
}

size_t NanobindInterface::GetPythonMemoryUsage() const {
    return 0;
}

template<typename Func>
auto NanobindInterface::MeasureCall(const std::string& function_name, Func&& func) -> decltype(func()) {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = func();
    auto end = std::chrono::high_resolution_clock::now();

    metrics_.last_function_called = function_name;
    metrics_.call_overhead_us = std::chrono::duration<double, std::micro>(end - start).count();
    metrics_.conversions_performed++;
    return result;
}

void NanobindInterface::UpdateMetrics(const std::string& function_name,
                                      double overhead_us,
                                      size_t data_bytes,
                                      bool zero_copy) {
    metrics_.last_function_called = function_name;
    metrics_.call_overhead_us = overhead_us;
    metrics_.data_transferred_bytes = data_bytes;
    metrics_.zero_copy_used = zero_copy;
}

GILGuard::GILGuard() {}
GILGuard::~GILGuard() {}

namespace Nanobind {

nb::object Execute(const std::string& code) {
    if (!g_nanobind_interface) {
        g_nanobind_interface = std::make_unique<NanobindInterface>();
    }
    return g_nanobind_interface->ExecutePythonCode(code);
}

nb::object Import(const std::string& module_name) {
    if (!g_nanobind_interface) {
        g_nanobind_interface = std::make_unique<NanobindInterface>();
    }
    return g_nanobind_interface->ImportModule(module_name);
}

template<typename T>
std::vector<T> FromNumPy(const nb::ndarray<nb::numpy, T>& array) {
    std::vector<T> out(array.size());
    const auto* base = reinterpret_cast<const char*>(array.data());
    const ssize_t stride = array.stride(0);
    for (size_t i = 0; i < array.size(); ++i) {
        const auto* elem = reinterpret_cast<const T*>(base + static_cast<ssize_t>(i) * stride);
        out[i] = *elem;
    }
    return out;
}

template<typename T>
nb::ndarray<nb::numpy, T> ToNumPy(const std::vector<T>& data) {
    nb::gil_scoped_acquire gil;
    auto np = nb::module_::import_("numpy");
    auto arr = np.attr("array")(data);
    return nb::cast<nb::ndarray<nb::numpy, T>>(arr);
}

std::string GetPerformanceReport() {
    if (!g_nanobind_interface) {
        return "Nanobind interface not initialized";
    }
    return g_nanobind_interface->GetPerformanceReport();
}

void OptimizeForSpeed() {
    if (!g_nanobind_interface) {
        g_nanobind_interface = std::make_unique<NanobindInterface>();
    }
}

template std::vector<double> FromNumPy(const nb::ndarray<nb::numpy, double>& array);
template std::vector<float> FromNumPy(const nb::ndarray<nb::numpy, float>& array);
template std::vector<int> FromNumPy(const nb::ndarray<nb::numpy, int>& array);

template nb::ndarray<nb::numpy, double> ToNumPy(const std::vector<double>& data);
template nb::ndarray<nb::numpy, float> ToNumPy(const std::vector<float>& data);
template nb::ndarray<nb::numpy, int> ToNumPy(const std::vector<int>& data);

} // namespace Nanobind

} // namespace AXIOM

#endif // ENABLE_NANOBIND
