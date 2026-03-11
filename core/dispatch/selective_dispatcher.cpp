/**
 * @file selective_dispatcher.cpp
 * @brief Implementation of the SelectiveDispatcher using Pimpl architecture.
 */

#include "selective_dispatcher.h"
#include "signal_exec_traits.h"
#include "dynamic_calc.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <chrono>
#include <cctype>
#include <initializer_list>
#include <string_view>

#ifdef ENABLE_EIGEN
#include "eigen_engine.h"
#endif

#ifdef ENABLE_NANOBIND
#include "nanobind_interface.h"
#endif

namespace AXIOM {

namespace {

std::string ToLower(std::string text) {
    std::ranges::transform(text, text.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

bool MatchesOperation(std::string_view operation,
                      std::initializer_list<const char*> aliases) {
    for (const char* alias : aliases) {
        if (operation == alias) {
            return true;
        }
    }
    return false;
}

std::string BuildOperationInput(std::string_view operation,
                                const std::vector<std::string>& args) {
    std::string full_op{operation};
    size_t capacity = operation.size();
    for (const auto& arg : args) {
        capacity += 1 + arg.size();
    }
    full_op.reserve(capacity);

    for (const auto& arg : args) {
        full_op += ' ';
        full_op += arg;
    }
    return full_op;
}

ComputeEngine ResolveEngine(const ComputeEngine preferred_default,
                            const std::map<ComputeEngine, bool>& availability,
                            const ComputeEngine preferred_engine) {
    ComputeEngine engine = (preferred_engine == ComputeEngine::Auto)
        ? preferred_default : preferred_engine;

    const bool engine_ok = availability.contains(engine) &&
                           availability.at(engine);
    if (!engine_ok || engine == ComputeEngine::Auto) {
        return ComputeEngine::Native;
    }
    return engine;
}

bool RequiresSquareMatrix(std::string_view operation)
{
    return SignalExec::IsSquareOnlyOperation(operation);
}

CalcErr MapDispatchException(const std::exception& exception) {
    using enum CalcErr;
    const std::string message = ToLower(exception.what());

    if (message.contains("divide") && message.contains("zero")) {
        return DivideByZero;
    }
    if (message.contains("parse") ||
        message.contains("syntax") ||
        message.contains("token")) {
        return ParseError;
    }
    if (message.contains("negative") && message.contains("root")) {
        return NegativeRoot;
    }
    if (message.contains("domain")) {
        return DomainError;
    }
    if (message.contains("argument") ||
        message.contains("mismatch") ||
        message.contains("invalid")) {
        return ArgumentMismatch;
    }
    if (message.contains("overflow")) {
        return NumericOverflow;
    }
    if (message.contains("memory") ||
        message.contains("alloc") ||
        message.contains("bad_alloc")) {
        return MemoryExhausted;
    }

    return OperationNotFound;
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
    const auto start = std::chrono::high_resolution_clock::now();
    const ComputeEngine engine = ResolveEngine(pimpl_->preferred_engine_,
                                               pimpl_->engine_availability_,
                                               preferred_engine);

    EngineResult result;
    try {
        thread_local DynamicCalc native;
        const std::string full_op = BuildOperationInput(operation, args);
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
    if (!(pimpl_->engine_availability_[ComputeEngine::Eigen] && pimpl_->eigen_engine_)) [[unlikely]] {
        return CreateErrorResult(CalcErr::OperationNotFound);
    }

    const int rows = static_cast<int>(matrix_data.rows());
    const int cols = static_cast<int>(matrix_data.cols());
    if (RequiresSquareMatrix(operation) && rows != cols) [[unlikely]] {
        return CreateErrorResult(CalcErr::ArgumentMismatch);
    }

    AXIOM::Matrix mat(static_cast<size_t>(rows), std::vector<double>(static_cast<size_t>(cols)));
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            mat[static_cast<size_t>(r)][static_cast<size_t>(c)] = matrix_data(r, c);
        }
    }

    const auto to_axiom_matrix = [](const auto& eigen_matrix) {
        AXIOM::Matrix out(static_cast<size_t>(eigen_matrix.rows()),
                          std::vector<double>(static_cast<size_t>(eigen_matrix.cols()), 0.0));
        for (int r = 0; r < eigen_matrix.rows(); ++r) {
            for (int c = 0; c < eigen_matrix.cols(); ++c) {
                out[static_cast<size_t>(r)][static_cast<size_t>(c)] = eigen_matrix(r, c);
            }
        }
        return out;
    };

    if (MatchesOperation(operation, {"determinant", "det"})) {
        const auto eigen_mat = pimpl_->eigen_engine_->CreateMatrix(mat);
        return CreateSuccessResult(pimpl_->eigen_engine_->Determinant(eigen_mat));
    }

    if (MatchesOperation(operation, {"transpose", "trans"})) {
        const auto eigen_mat = pimpl_->eigen_engine_->CreateMatrix(mat);
        auto t = pimpl_->eigen_engine_->Transpose(eigen_mat);
        return CreateSuccessResult(to_axiom_matrix(t));
    }

    if (MatchesOperation(operation, {"inverse", "inv"})) {
        const auto eigen_mat = pimpl_->eigen_engine_->CreateMatrix(mat);
        auto inv = pimpl_->eigen_engine_->Inverse(eigen_mat);
        return CreateSuccessResult(to_axiom_matrix(inv));
    }

    if (MatchesOperation(operation, {"eigenvalues", "eigvals"})) {
        const auto eigen_mat = pimpl_->eigen_engine_->CreateMatrix(mat);
        const auto decomposition = pimpl_->eigen_engine_->EigenDecomposition(eigen_mat);
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