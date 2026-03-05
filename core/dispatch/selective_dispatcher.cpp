/**
 * @file selective_dispatcher.cpp
 * @brief Implementation of the SelectiveDispatcher using Pimpl architecture.
 */

#include "selective_dispatcher.h"
#include "dynamic_calc.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <chrono>
#include <cctype>

#ifdef ENABLE_EIGEN
#include "eigen_engine.h"
#endif

#ifdef ENABLE_NANOBIND
#include "nanobind_interface.h"
#endif

namespace AXIOM {

namespace {

std::string ToLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

bool RequiresSquareMatrix(const std::string& operation)
{
    return operation == "determinant" || operation == "det" ||
           operation == "inverse" || operation == "inv" ||
           operation == "eigenvalues" || operation == "eigvals" ||
           operation == "trace";
}

CalcErr MapDispatchException(const std::exception& exception) {
    const std::string message = ToLower(exception.what());

    if (message.find("divide") != std::string::npos && message.find("zero") != std::string::npos) {
        return CalcErr::DivideByZero;
    }
    if (message.find("parse") != std::string::npos ||
        message.find("syntax") != std::string::npos ||
        message.find("token") != std::string::npos) {
        return CalcErr::ParseError;
    }
    if (message.find("negative") != std::string::npos && message.find("root") != std::string::npos) {
        return CalcErr::NegativeRoot;
    }
    if (message.find("domain") != std::string::npos) {
        return CalcErr::DomainError;
    }
    if (message.find("argument") != std::string::npos ||
        message.find("mismatch") != std::string::npos ||
        message.find("invalid") != std::string::npos) {
        return CalcErr::ArgumentMismatch;
    }
    if (message.find("overflow") != std::string::npos) {
        return CalcErr::NumericOverflow;
    }
    if (message.find("memory") != std::string::npos ||
        message.find("alloc") != std::string::npos ||
        message.find("bad_alloc") != std::string::npos) {
        return CalcErr::MemoryExhausted;
    }

    return CalcErr::OperationNotFound;
}

} // namespace

struct SelectiveDispatcher::Impl {
    ComputeEngine preferred_engine_{ComputeEngine::Auto};
    bool fallback_enabled_{true};
    double performance_threshold_ms_{100.0};
    bool learning_enabled_{true};
    
    DispatchMetrics last_metrics_{};
    std::map<ComputeEngine, bool> engine_availability_{};
    std::map<ComputeEngine, std::map<std::string, EnginePerformance>> engine_performance_{};
    
#ifdef ENABLE_EIGEN
    std::unique_ptr<EigenEngine> eigen_engine_;
#endif

#ifdef ENABLE_NANOBIND
    std::unique_ptr<NanobindInterface> nanobind_interface_;
#endif
};

std::unique_ptr<SelectiveDispatcher> g_dispatcher;

SelectiveDispatcher::SelectiveDispatcher() : pimpl_(std::make_unique<Impl>()) {
    pimpl_->engine_availability_[ComputeEngine::Native] = true;

#ifdef ENABLE_EIGEN
    try {
        pimpl_->eigen_engine_ = std::make_unique<EigenEngine>();
        pimpl_->engine_availability_[ComputeEngine::Eigen] = true;
    } catch (const std::exception& e) {
        pimpl_->engine_availability_[ComputeEngine::Eigen] = false;
        std::cerr << "[AXIOM] EigenEngine initialization failed: " << e.what() << '\n';
    }
#endif

#ifdef ENABLE_NANOBIND
    try {
        pimpl_->nanobind_interface_ = std::make_unique<NanobindInterface>();
        pimpl_->engine_availability_[ComputeEngine::Python] = true;
    } catch (const std::exception& e) {
        pimpl_->engine_availability_[ComputeEngine::Python] = false;
        std::cerr << "[AXIOM] NanobindInterface initialization failed: " << e.what() << '\n';
    }
#endif
}

SelectiveDispatcher::~SelectiveDispatcher() = default;

void SelectiveDispatcher::SetPreferredEngine(ComputeEngine engine) {
    pimpl_->preferred_engine_ = engine;
}

void SelectiveDispatcher::EnableFallback(bool enable) {
    pimpl_->fallback_enabled_ = enable;
}

EngineResult SelectiveDispatcher::DispatchOperation(const std::string& operation,
                                                   const std::vector<std::string>& args,
                                                   ComputeEngine preferred_engine) {
    auto start = std::chrono::high_resolution_clock::now();

    ComputeEngine engine = (preferred_engine == ComputeEngine::Auto)
        ? pimpl_->preferred_engine_ : preferred_engine;

    // Always ensure we have a working engine — fall back to Native if requested one unavailable
    bool engine_ok = pimpl_->engine_availability_.count(engine) &&
                     pimpl_->engine_availability_.at(engine);
    if (!engine_ok || engine == ComputeEngine::Auto) {
        engine = ComputeEngine::Native;
    }

    EngineResult result;
    try {
        thread_local DynamicCalc native;
        std::string full_op = operation;
        for (const auto& arg : args) {
            full_op += ' ';
            full_op += arg;
        }
        result = native.Evaluate(full_op);
    } catch (const std::exception& e) {
        std::cerr << "[AXIOM Dispatch] Evaluation exception: " << e.what() << '\n';
        result = CreateErrorResult(MapDispatchException(e));
    }

    auto end = std::chrono::high_resolution_clock::now();
    pimpl_->last_metrics_.execution_time_ms =
        std::chrono::duration<double, std::milli>(end - start).count();
    pimpl_->last_metrics_.selected_engine  = engine;
    pimpl_->last_metrics_.operation_name   = operation;
    pimpl_->last_metrics_.fallback_used    = (engine != preferred_engine &&
                                              preferred_engine != ComputeEngine::Auto);
    return result;
}

EngineResult SelectiveDispatcher::DispatchMatrixOperation(const std::string& operation,
                                                         Eigen::Ref<const Eigen::MatrixXd> matrix_data) {
#ifdef ENABLE_EIGEN
    if (pimpl_->engine_availability_[ComputeEngine::Eigen] && pimpl_->eigen_engine_) {
        // Convert Eigen::Ref to AXIOM::Matrix (std::vector<std::vector<double>>)
        const int rows = static_cast<int>(matrix_data.rows());
        const int cols = static_cast<int>(matrix_data.cols());

        if (RequiresSquareMatrix(operation) && rows != cols) {
            return CreateErrorResult(CalcErr::ArgumentMismatch);
        }

        AXIOM::Matrix mat(rows, std::vector<double>(cols));
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                mat[r][c] = matrix_data(r, c);

        if (operation == "determinant" || operation == "det") {
            auto eigen_mat = pimpl_->eigen_engine_->CreateMatrix(mat);
            double det = pimpl_->eigen_engine_->Determinant(eigen_mat);
            return CreateSuccessResult(det);
        }

        if (operation == "transpose" || operation == "trans") {
            auto eigen_mat = pimpl_->eigen_engine_->CreateMatrix(mat);
            auto t = pimpl_->eigen_engine_->Transpose(eigen_mat);
            AXIOM::Matrix out(static_cast<size_t>(t.rows()), std::vector<double>(static_cast<size_t>(t.cols()), 0.0));
            for (int r = 0; r < t.rows(); ++r) {
                for (int c = 0; c < t.cols(); ++c) {
                    out[static_cast<size_t>(r)][static_cast<size_t>(c)] = t(r, c);
                }
            }
            return CreateSuccessResult(std::move(out));
        }

        if (operation == "inverse" || operation == "inv") {
            auto eigen_mat = pimpl_->eigen_engine_->CreateMatrix(mat);
            auto inv = pimpl_->eigen_engine_->Inverse(eigen_mat);
            AXIOM::Matrix out(static_cast<size_t>(inv.rows()), std::vector<double>(static_cast<size_t>(inv.cols()), 0.0));
            for (int r = 0; r < inv.rows(); ++r) {
                for (int c = 0; c < inv.cols(); ++c) {
                    out[static_cast<size_t>(r)][static_cast<size_t>(c)] = inv(r, c);
                }
            }
            return CreateSuccessResult(std::move(out));
        }

        if (operation == "eigenvalues" || operation == "eigvals") {
            auto eigen_mat = pimpl_->eigen_engine_->CreateMatrix(mat);
            auto decomposition = pimpl_->eigen_engine_->EigenDecomposition(eigen_mat);
            const auto& eigenvalues = decomposition.first;
            AXIOM::Vector out;
            out.reserve(static_cast<size_t>(eigenvalues.size()));
            for (int i = 0; i < eigenvalues.size(); ++i) {
                out.push_back(eigenvalues(i));
            }
            return CreateSuccessResult(std::move(out));
        }

        if (operation == "trace") {
            double trace = 0.0;
            for (int i = 0; i < rows; ++i) {
                trace += mat[static_cast<size_t>(i)][static_cast<size_t>(i)];
            }
            return CreateSuccessResult(trace);
        }

        return CreateErrorResult(CalcErr::OperationNotFound);
    }
#endif
    return CreateErrorResult(CalcErr::OperationNotFound);
}

std::string SelectiveDispatcher::GetPerformanceReport() const {
    std::ostringstream oss;
    oss << "[AXIOM Dispatch Report]\n";
    oss << "  Last operation : " << pimpl_->last_metrics_.operation_name << '\n';
    oss << "  Engine used    : " << static_cast<int>(pimpl_->last_metrics_.selected_engine) << '\n';
    oss << "  Exec time (ms) : " << pimpl_->last_metrics_.execution_time_ms << '\n';
    oss << "  Fallback used  : " << (pimpl_->last_metrics_.fallback_used ? "yes" : "no") << '\n';
    return oss.str();
}

} // namespace AXIOM