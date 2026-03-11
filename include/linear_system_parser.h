#pragma once
#include "IParser.h"
#include "dynamic_calc_types.h"
#include <vector>
#include <string>
#include <functional>
#include <immintrin.h> // SIMD support
#include <unordered_map>
#include <memory_resource> // PMR allocators

namespace AXIOM
{

    // LinAlgResult struct'ı BURADAN SİLİNDİ çünkü dynamic_calc_types.h içinde zaten var!

    class LinearSystemParser : public IParser
    {
    public:
        LinearSystemParser();
        EngineResult ParseAndExecute(const std::string &input) override;

    private:
        struct CommandEntry
        {
            std::string command;
            std::function<EngineResult(const std::string&)> handler;
            std::string description;
        };
        std::vector<CommandEntry> command_registry_;

        void RegisterCommands();

        EngineResult HandleQR(const std::string &input) const;
        EngineResult HandleEigen(const std::string &input) const;
        EngineResult HandleCramer(const std::string &input) const;
        EngineResult HandleSolve(const std::string &input) const;
        EngineResult HandleDefaultSolve(const std::string &input) const;

        LinAlgResult solve_linear_system(const std::vector<std::vector<double>> &A, const std::vector<double> &b) const;
        bool ParseLinearSystem(const std::string &input, std::vector<std::vector<double>> &A, std::vector<double> &b) const;
        Matrix ParseMatrixString(const std::string &input) const;

        Matrix GetMinor(const Matrix &A, int row, int col) const;
        double Determinant(const Matrix &A) const;
        std::optional<std::vector<double>> CramersRule(const Matrix &A, const std::vector<double> &b) const;
        Matrix Transpose(const Matrix &A) const;
        double DotProduct(const std::vector<double> &v1, const std::vector<double> &v2) const;
        double VectorNorm(const std::vector<double> &v) const;
        std::vector<double> VectorScale(const std::vector<double> &v, double scalar) const;
        std::vector<double> VectorSub(const std::vector<double> &v1, const std::vector<double> &v2) const;
        std::pair<Matrix, Matrix> GramSchmidt(const Matrix &A) const;
        Matrix MultiplyMatrices(const Matrix &A, const Matrix &B) const;
        Matrix CreateIdentityMatrix(int n) const;
        std::vector<double> GetDiagonal(const Matrix &A) const;
        std::pair<std::vector<double>, Matrix> ComputeEigenvalues(const Matrix &A, int max_iterations = 100) const;
    };

    bool isCloseToZero(double value, double epsilon = 1e-9);
    bool isValidNumber(const std::string &str);

} // namespace AXIOM