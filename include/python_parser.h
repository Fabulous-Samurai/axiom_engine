#pragma once

#include "IParser.h"
#include "python_engine.h"

namespace AXIOM {

class PythonParser : public IParser {
public:
    explicit PythonParser(PythonEngine* engine, PythonMode mode = PythonMode::Interactive);
    ~PythonParser() = default;

    EngineResult ParseAndExecute(const std::string& input) override;

private:
    PythonEngine* python_engine_;
    PythonMode mode_;

    // Mode-specific handlers
    std::string HandleInteractiveMode(const std::string& input);
    std::string HandleNumPyMode(const std::string& input);
    std::string HandleSciPyMode(const std::string& input);
    std::string HandleMatplotlibMode(const std::string& input);
    std::string HandlePandasMode(const std::string& input);
    std::string HandleSymPyMode(const std::string& input);
};

} // namespace AXIOM