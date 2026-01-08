#pragma once
#include "IParser.h"
#include "dynamic_calc_types.h"
#include <vector>
#include <string>
#include <functional>
#include <immintrin.h>  // SIMD support
#include <unordered_map>
#include <memory_resource>  // PMR allocators

struct LinAlgResult {
    std::optional<std::vector<double>> solution;
    LinAlgErr err;
};

class LinearSystemParser : public IParser {
public:
    LinearSystemParser(); 
    EngineResult ParseAndExecute(const std::string& input) override;

private:
    struct CommandEntry {
        std::string command;
        std::function<EngineResult(const std::string&)> handler;
        std::string description;
    };
    std::vector<CommandEntry> command_registry_;
    
    void RegisterCommands();

    EngineResult HandleQR(const std::string& input);
    EngineResult HandleEigen(const std::string& input);
    EngineResult HandleCramer(const std::string& input);
    EngineResult HandleSolve(const std::string& input);
    EngineResult HandleDefaultSolve(const std::string& input);

    LinAlgResult solve_linear_system(const std::vector<std::vector<double>>& A, const std::vector<double>& b);
    bool ParseLinearSystem(const std::string& input, std::vector<std::vector<double>>& A, std::vector<double>& b);
    Matrix ParseMatrixString(const std::string& input);

    Matrix GetMinor(const Matrix& A, int row, int col);
    double Determinant(const Matrix& A);
    std::optional<std::vector<double>> CramersRule(const Matrix& A, const std::vector<double>& b);
    Matrix Transpose(const Matrix&A);
    double DotProduct(const std::vector<double>& v1, const std::vector<double>& v2);
    double VectorNorm(const std::vector<double>& v);
    std::vector<double> VectorScale(const std::vector<double>& v, double scalar);
    std::vector<double> VectorSub(const std::vector<double>& v1, const std::vector<double>& v2);
    std::pair<Matrix,Matrix> GramSchmidt(const Matrix& A); 
    Matrix MultiplyMatrices(const Matrix &A , const Matrix& B);
    Matrix CreateIdentityMatrix(int n);
    std::vector<double> GetDiagonal(const Matrix& A);
    std::pair<std::vector<double>, Matrix> ComputeEigenvalues(const Matrix& A, int max_iterations = 100);
};

bool isCloseToZero(double value, double epsilon = 1e-9);
bool isValidNumber(const std::string& str);