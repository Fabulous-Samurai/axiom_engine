#include "../include/python_parser.h"
#include <stdexcept>

namespace AXIOM {

PythonParser::PythonParser(PythonEngine* engine, PythonMode mode) : python_engine_(engine), mode_(mode) {
    if (!python_engine_) {
        throw std::runtime_error("PythonEngine cannot be null");
    }
}

EngineResult PythonParser::ParseAndExecute(const std::string& input) {
    if (!python_engine_ || !python_engine_->IsInitialized()) {
        EngineResult res;
        res.error = EngineErrorResult{CalcErr::OperationNotFound};
        return res;
    }

    std::string processed_input;

    switch (mode_) {
        case PythonMode::Interactive:
            processed_input = HandleInteractiveMode(input);
            break;
        case PythonMode::NumPy:
            processed_input = HandleNumPyMode(input);
            break;
        case PythonMode::SciPy:
            processed_input = HandleSciPyMode(input);
            break;
        case PythonMode::Matplotlib:
            processed_input = HandleMatplotlibMode(input);
            break;
        case PythonMode::Pandas:
            processed_input = HandlePandasMode(input);
            break;
        case PythonMode::SymPy:
            processed_input = HandleSymPyMode(input);
            break;
        default:
            processed_input = input;
            break;
    }

    // Execute the processed Python code
    auto result = python_engine_->EvaluatePython(processed_input);

    return result;
}

std::string PythonParser::HandleInteractiveMode(const std::string& input) {
    // Interactive mode - execute Python code as-is
    return input;
}

std::string PythonParser::HandleNumPyMode(const std::string& input) {
    // NumPy mode - automatically add np. prefix to numpy functions
    std::string processed = input;

    // Add common NumPy function prefixes
    std::vector<std::string> numpy_funcs = {
        "array", "zeros", "ones", "linspace", "arange", "reshape",
        "dot", "cross", "sum", "mean", "std", "min", "max", "sqrt",
        "exp", "log", "sin", "cos", "tan", "pi", "e"
    };

    for (const auto& func : numpy_funcs) {
        std::string pattern = func + "(";
        std::string replacement = "np." + func + "(";
        size_t pos = processed.find(pattern);
        while (pos != std::string::npos) {
            processed.replace(pos, pattern.length(), replacement);
            pos = processed.find(pattern, pos + replacement.length());
        }
    }

    return processed;
}

std::string PythonParser::HandleSciPyMode(const std::string& input) {
    // SciPy mode - add scipy prefixes
    std::string processed = HandleNumPyMode(input); // Include NumPy support

    std::vector<std::string> scipy_funcs = {
        "integrate", "optimize", "linalg", "stats", "special", "fft"
    };

    for (const auto& func : scipy_funcs) {
        std::string pattern = func + ".";
        std::string replacement = "sp." + func + ".";
        size_t pos = processed.find(pattern);
        while (pos != std::string::npos) {
            processed.replace(pos, pattern.length(), replacement);
            pos = processed.find(pattern, pos + replacement.length());
        }
    }

    return processed;
}

std::string PythonParser::HandleMatplotlibMode(const std::string& input) {
    // Matplotlib mode - add plotting shortcuts
    std::string processed = HandleNumPyMode(input); // Include NumPy support

    std::vector<std::string> plt_funcs = {
        "plot", "scatter", "bar", "hist", "show", "figure", "subplot",
        "xlabel", "ylabel", "title", "legend", "grid", "savefig"
    };

    for (const auto& func : plt_funcs) {
        std::string pattern = func + "(";
        std::string replacement = "plt." + func + "(";
        size_t pos = processed.find(pattern);
        while (pos != std::string::npos) {
            processed.replace(pos, pattern.length(), replacement);
            pos = processed.find(pattern, pos + replacement.length());
        }
    }

    return processed;
}

std::string PythonParser::HandlePandasMode(const std::string& input) {
    // Pandas mode - add pandas shortcuts
    std::string processed = HandleNumPyMode(input); // Include NumPy support

    std::vector<std::string> pd_funcs = {
        "DataFrame", "Series", "read_csv", "read_excel", "read_json"
    };

    for (const auto& func : pd_funcs) {
        std::string pattern = func + "(";
        std::string replacement = "pd." + func + "(";
        size_t pos = processed.find(pattern);
        while (pos != std::string::npos) {
            processed.replace(pos, pattern.length(), replacement);
            pos = processed.find(pattern, pos + replacement.length());
        }
    }

    return processed;
}

std::string PythonParser::HandleSymPyMode(const std::string& input) {
    // SymPy mode - add symbolic math shortcuts
    std::string processed = input;

    std::vector<std::string> sympy_funcs = {
        "Symbol", "symbols", "diff", "integrate", "solve", "expand",
        "factor", "simplify", "limit", "series"
    };

    for (const auto& func : sympy_funcs) {
        std::string pattern = func + "(";
        std::string replacement = "sp." + func + "(";
        size_t pos = processed.find(pattern);
        while (pos != std::string::npos) {
            processed.replace(pos, pattern.length(), replacement);
            pos = processed.find(pattern, pos + replacement.length());
        }
    }

    return processed;
}

} // namespace AXIOM
