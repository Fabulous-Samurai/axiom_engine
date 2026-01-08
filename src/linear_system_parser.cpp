#include "linear_system_parser.h"
#include "string_helpers.h" // Ensure StringHelpers.h exists
#ifdef ENABLE_EIGEN
#include "eigen_engine.h"
#endif
#include <cmath>
#include <algorithm>
#include <sstream>
#include <map>
#include <iostream>
#include <cctype>

// =========================================================
// LEXER (TOKENIZER) IMPLEMENTATION
// =========================================================
namespace
{
    enum class TokenType
    {
        LBracket,
        RBracket,
        Semicolon,
        Comma,
        Number,
        End
    };

    struct Token
    {
        TokenType type;
        std::string value;
    };

    class MatrixLexer
    {
    public:
        explicit MatrixLexer(const std::string &input) : input_(input), pos_(0) {}

        Token NextToken()
        {
            SkipWhitespace();
            if (pos_ >= input_.length())
                return {TokenType::End, ""};

            char current = input_[pos_];

            if (current == '[')
            {
                pos_++;
                return {TokenType::LBracket, "["};
            }
            if (current == ']')
            {
                pos_++;
                return {TokenType::RBracket, "]"};
            }
            if (current == ';')
            {
                pos_++;
                return {TokenType::Semicolon, ";"};
            }
            if (current == ',')
            {
                pos_++;
                return {TokenType::Comma, ","};
            }

            // Check for Numbers and Signs
            if (std::isdigit(current) || current == '-' || current == '.')
            {
                std::string num_str;

                if (current == '-')
                {
                    num_str += current;
                    pos_++;
                }

                bool has_decimal = false;
                while (pos_ < input_.length())
                {
                    char c = input_[pos_];
                    if (std::isdigit(c))
                    {
                        num_str += c;
                        pos_++;
                    }
                    else if (c == '.' && !has_decimal)
                    {
                        num_str += c;
                        has_decimal = true;
                        pos_++;
                    }
                    else
                    {
                        break;
                    }
                }

                if (num_str == "-" || num_str == ".")
                    return {TokenType::End, ""};
                return {TokenType::Number, num_str};
            }

            pos_++;
            return NextToken();
        }

    private:
        std::string input_;
        size_t pos_;

        void SkipWhitespace()
        {
            while (pos_ < input_.length() && std::isspace(input_[pos_]))
            {
                pos_++;
            }
        }
    };
}

// =========================================================
// LINEAR SYSTEM PARSER IMPLEMENTATION
// =========================================================

LinearSystemParser::LinearSystemParser()
{
    RegisterCommands();
}

void LinearSystemParser::RegisterCommands()
{
    command_registry_.push_back({"qr",
                                 [this](const std::string &s)
                                 { return HandleQR(s); },
                                 "Performs QR Decomposition"});

    command_registry_.push_back({"ortho",
                                 [this](const std::string &s)
                                 { return HandleQR(s); },
                                 "Performs QR Decomposition"});

    command_registry_.push_back({"eigen",
                                 [this](const std::string &s)
                                 { return HandleEigen(s); },
                                 "Computes Eigenvalues and Eigenvectors"});

    command_registry_.push_back({"cramer",
                                 [this](const std::string &s)
                                 { return HandleCramer(s); },
                                 "Solves system using Cramer's Rule"});

    command_registry_.push_back({"solve",
                                 [this](const std::string &s)
                                 { return HandleSolve(s); },
                                 "Solves linear system Ax=b"});
}

EngineResult LinearSystemParser::ParseAndExecute(const std::string &input)
{
    for (const auto &entry : command_registry_)
    {
        if (input.find(entry.command) == 0)
        {
            return entry.handler(input);
        }
    }
    return HandleDefaultSolve(input);
}

EngineResult LinearSystemParser::HandleQR(const std::string &input)
{
    auto extract_matrix_string = [&input]() -> std::string
    {
        size_t matrix_start = input.find_first_of("0123456789-[");
        if (matrix_start == std::string::npos)
            return "";
        return input.substr(matrix_start);
    };

    std::string matrix_str = extract_matrix_string();
    if (matrix_str.empty())
        return {{}, {LinAlgErr::ParseError}};

    Matrix A = ParseMatrixString(matrix_str);
    if (A.empty())
        return {{}, {LinAlgErr::ParseError}};
    if (A.size() < A[0].size())
        return {{}, {LinAlgErr::MatrixMismatch}};

    auto [Q, R] = GramSchmidt(A);
    if (Q.empty())
        return {{}, {LinAlgErr::NoSolution}};

    return EngineSuccessResult(Q);
}

EngineResult LinearSystemParser::HandleEigen(const std::string &input)
{
    auto extract_matrix_string = [&input]() -> std::string
    {
        size_t matrix_start = input.find_first_of("0123456789-[");
        if (matrix_start == std::string::npos)
            return "";
        return input.substr(matrix_start);
    };

    std::string matrix_str = extract_matrix_string();
    if (matrix_str.empty())
        return {{}, {LinAlgErr::ParseError}};

    Matrix A = ParseMatrixString(matrix_str);
    if (A.empty() || A.size() != A[0].size())
        return {{}, {LinAlgErr::MatrixMismatch}};

    auto [eigenValues, eigenVectors] = ComputeEigenvalues(A, 100);

    return EngineSuccessResult(Vector(eigenValues));
}

EngineResult LinearSystemParser::HandleCramer(const std::string &input)
{
    std::string equation = input.substr(6);
    std::vector<std::vector<double>> A;
    std::vector<double> b;

    if (!ParseLinearSystem(equation, A, b))
        return {{}, {LinAlgErr::ParseError}};
    if (A.size() != A[0].size() || A.size() != b.size())
        return {{}, {LinAlgErr::MatrixMismatch}};

    auto solution = CramersRule(A, b);
    if (solution.has_value())
        return {solution.value(), {}};
    else
        return {{}, {LinAlgErr::NoSolution}};
}

EngineResult LinearSystemParser::HandleSolve(const std::string &input)
{
    // Accept forms:
    //   solve([a,b;c,d],[v;w])
    //   solve([[a,b],[c,d]],[[v],[w]])
    //   solve([a,b;c,d], [v, w])
    std::string content = input.substr(5);  // Skip "solve"

    // Remove spaces for simpler scanning
    std::string processed;
    for (char c : content) { if (c != ' ') processed += c; }

    auto extract_bracket_block = [](const std::string& s, size_t start_pos) -> std::pair<std::string, size_t> {
        size_t start = s.find('[', start_pos);
        if (start == std::string::npos) return {"", std::string::npos};
        int depth = 0;
        for (size_t i = start; i < s.size(); ++i) {
            if (s[i] == '[') depth++;
            else if (s[i] == ']') {
                depth--;
                if (depth == 0) {
                    return {s.substr(start, i - start + 1), i + 1};
                }
            }
        }
        return {"", std::string::npos};
    };

    auto [matrix_str, after_matrix] = extract_bracket_block(processed, 0);
    if (matrix_str.empty() || after_matrix == std::string::npos) {
        return {{}, {LinAlgErr::ParseError}};
    }

    auto [vec_str, _after_vec] = extract_bracket_block(processed, after_matrix);
    if (vec_str.empty()) {
        return {{}, {LinAlgErr::ParseError}};
    }

    Matrix A = ParseMatrixString(matrix_str);
    if (A.empty() || A[0].empty()) {
        return {{}, {LinAlgErr::ParseError}};
    }

    Matrix b_matrix = ParseMatrixString(vec_str);
    if (b_matrix.empty() || b_matrix[0].empty()) {
        return {{}, {LinAlgErr::ParseError}};
    }

    // Flatten to 1D vector
    std::vector<double> b;
    if (b_matrix.size() == 1) {
        b = b_matrix[0];
    } else if (b_matrix[0].size() == 1) {
        for (const auto& row : b_matrix) b.push_back(row[0]);
    } else {
        return {{}, {LinAlgErr::ParseError}};
    }

    if (A.size() != A[0].size() || A.size() != b.size()) {
        return {{}, {LinAlgErr::MatrixMismatch}};
    }

    LinAlgResult lin_res = solve_linear_system(A, b);
    if (lin_res.err == LinAlgErr::None && lin_res.solution.has_value()) {
        return {lin_res.solution.value(), {}};
    } else {
        return {{}, {lin_res.err}};
    }
}

EngineResult LinearSystemParser::HandleDefaultSolve(const std::string &input)
{
    std::vector<std::vector<double>> A;
    std::vector<double> b;

    if (!ParseLinearSystem(input, A, b))
        return {{}, {LinAlgErr::ParseError}};
    if (A.size() == 0 || A.size() != b.size())
        return {{}, {LinAlgErr::MatrixMismatch}};

    LinAlgResult lin_res = solve_linear_system(A, b);
    if (lin_res.err == LinAlgErr::None)
        return {lin_res.solution.value(), {}};
    else
        return {{}, {lin_res.err}};
}

Matrix LinearSystemParser::MultiplyMatrices(const Matrix &A, const Matrix &B)
{
    // AXIOM v3.1: Eigen integration disabled - type mismatch (Eigen::Matrix vs std::vector)
    // #ifdef ENABLE_EIGEN
    //     static AXIOM::EigenEngine eigen_engine;
    //     return eigen_engine.MatrixMultiply(A, B);
    // #else
    // Optimized fallback: Cache-friendly matrix multiplication with blocking
    if (A.empty() || B.empty() || A[0].size() != B.size()) return {};
    
    const int n = A.size(), m = B[0].size(), p = B.size();
    Matrix C(n, std::vector<double>(m, 0.0));
    
    // Cache-blocking optimization for better memory access patterns
    const int BLOCK_SIZE = 64;  // Optimize for L1 cache
    
    for (int ii = 0; ii < n; ii += BLOCK_SIZE) {
        for (int jj = 0; jj < m; jj += BLOCK_SIZE) {
            for (int kk = 0; kk < p; kk += BLOCK_SIZE) {
                // Process block
                int i_end = std::min(ii + BLOCK_SIZE, n);
                int j_end = std::min(jj + BLOCK_SIZE, m);
                int k_end = std::min(kk + BLOCK_SIZE, p);
                
                for (int i = ii; i < i_end; i++) {
                    for (int j = jj; j < j_end; j++) {
                        double sum = C[i][j];
                        for (int k = kk; k < k_end; k++) {
                            sum += A[i][k] * B[k][j];
                        }
                        C[i][j] = sum;
                    }
                }
            }
        }
    }
    
    return C;
}

Matrix LinearSystemParser::CreateIdentityMatrix(int n)
{
    Matrix I(n, std::vector<double>(n, 0.0));
    for (int i = 0; i < n; ++i)
        I[i][i] = 1.0;
    return I;
}

std::vector<double> LinearSystemParser::GetDiagonal(const Matrix &A)
{
    std::vector<double> diag;
    for (size_t i = 0; i < A.size(); ++i)
        diag.push_back(A[i][i]);
    return diag;
}

std::pair<std::vector<double>, Matrix> LinearSystemParser::ComputeEigenvalues(const Matrix &inputA, int max_iterations)
{
    Matrix A = inputA;
    int n = A.size();
    Matrix EigenVectors = CreateIdentityMatrix(n);

    for (int k = 0; k < max_iterations; k++)
    {
        auto [Q, R] = GramSchmidt(A);
        if (Q.empty())
            break;
        A = MultiplyMatrices(R, Q);
        EigenVectors = MultiplyMatrices(EigenVectors, Q);
    }
    std::vector<double> eigenValues = GetDiagonal(A);
    return {eigenValues, EigenVectors};
}

// =========================================================
// REFACTORED PARSELINEARSYSTEM (BULLETPROOF & EXCEPTION-FREE)
// =========================================================
bool LinearSystemParser::ParseLinearSystem(const std::string &input, std::vector<std::vector<double>> &A, std::vector<double> &b)
{
    std::string processed_input;
    for (char c : input)
        if (c != ' ')
            processed_input += c;
    
    std::vector<std::string> equations;
    // Utils::Split could be used instead of stringstream here, 
    // but keeping the logic consistent with existing patterns.
    std::stringstream ss_eq(processed_input);
    std::string eq;
    while (std::getline(ss_eq, eq, ';'))
        equations.push_back(Utils::Trim(eq)); 
    
    int N = equations.size();
    if (N == 0)
        return false;
    
    A.assign(N, std::vector<double>(N, 0.0));
    b.assign(N, 0.0);
    std::map<char, int> var_to_index;
    int var_count = 0;

    for (int i = 0; i < N; ++i)
    {
        std::stringstream ss_eq_part(equations[i]);
        std::string part;
        
        // Parse Left Hand Side (LHS)
        std::getline(ss_eq_part, part, '=');
        std::string lhs = Utils::Trim(part);

        // Parse Right Hand Side (RHS)
        if (!std::getline(ss_eq_part, part, '='))
            return false;
        std::string rhs = Utils::Trim(part);

        // [SECURE] Exception-free number validation
        if (!Utils::IsNumber(rhs)) return false;
        b[i] = std::stod(rhs);

        // Parse Terms in LHS
        std::string term;
        size_t pos = 0;
        while (pos < lhs.size())
        {
            char sign = '+';
            if (lhs[pos] == '+' || lhs[pos] == '-')
            {
                sign = lhs[pos];
                ++pos;
            }
            size_t start = pos;
            while (pos < lhs.size() && (std::isdigit(lhs[pos]) || lhs[pos] == '.' || std::isalpha(lhs[pos])))
                ++pos;
            
            term = lhs.substr(start, pos - start);
            double coefficient = 1.0;
            char variable = '\0';
            
            size_t var_pos = term.find_first_of("abcdefghijklmnopqrstuvwxyz");
            
            if (var_pos != std::string::npos)
            {
                // Variable found (e.g. 2x, -x, 3.5y)
                if (var_pos > 0)
                {
                    std::string coef_str = term.substr(0, var_pos);
                    // [SECURE] Validate coefficient part
                    if (!Utils::IsNumber(coef_str)) return false;
                    coefficient = std::stod(coef_str);
                }
                variable = term[var_pos];
            }
            else
            {
                // No variable, Constant Number (e.g. ... + 5 = ...)
                // In this case, move the number to the other side (b vector) with opposite sign.
                if (!Utils::IsNumber(term)) return false;
                b[i] -= (sign == '+' ? 1 : -1) * std::stod(term);
                continue;
            }

            if (sign == '-')
                coefficient = -coefficient;
            
            if (var_to_index.find(variable) == var_to_index.end())
                var_to_index[variable] = var_count++;
            
            int col = var_to_index[variable];
            A[i][col] += coefficient;
        }
    }
    if (var_count != N)
        return false;
    return true;
}

double LinearSystemParser::DotProduct(const std::vector<double> &v1, const std::vector<double> &v2)
{
    // AXIOM v3.1: Eigen integration disabled - type mismatch
    // #ifdef ENABLE_EIGEN
    //     static AXIOM::EigenEngine eigen_engine;
    //     return eigen_engine.DotProduct(v1, v2);
    // #else
    // Optimized fallback: Manual vectorization hints for better performance
    if (v1.size() != v2.size()) return 0.0;
    double sum = 0.0;
    
    // Performance: Manual loop unrolling for small vectors
    size_t i = 0;
    const size_t size = v1.size();
    
    // Process 4 elements at a time (manual vectorization hint)
    for (; i + 3 < size; i += 4) {
        sum += v1[i] * v2[i] + v1[i+1] * v2[i+1] + v1[i+2] * v2[i+2] + v1[i+3] * v2[i+3];
    }
    
    // Process remaining elements
    for (; i < size; i++) {
        sum += v1[i] * v2[i];
    }
    
    return sum;
}

double LinearSystemParser::VectorNorm(const std::vector<double> &v)
{
    return std::sqrt(DotProduct(v, v));
}

std::vector<double> LinearSystemParser::VectorScale(const std::vector<double> &v, const double scalar)
{
    std::vector<double> result = v;
    for (double &val : result)
        val *= scalar;
    return result;
}

std::vector<double> LinearSystemParser::VectorSub(const std::vector<double> &v1, const std::vector<double> &v2)
{
    if (v1.size() != v2.size())
        return {};
    std::vector<double> result = v1;
    for (size_t i = 0; i < result.size(); i++)
        result[i] -= v2[i];
    return result;
}

Matrix LinearSystemParser::GetMinor(const Matrix &A, int row, int col)
{
    int n = A.size();
    Matrix minor;
    for (int i = 0; i < n; i++)
    {
        if (i == row)
            continue;
        std::vector<double> minor_row;
        for (int j = 0; j < n; j++)
        {
            if (j == col)
                continue;
            minor_row.push_back(A[i][j]);
        }
        minor.push_back(minor_row);
    }
    return minor;
}

double LinearSystemParser::Determinant(const Matrix &A)
{
    // AXIOM v3.1: Eigen integration disabled - type mismatch
    // #ifdef ENABLE_EIGEN
    //     static AXIOM::EigenEngine eigen_engine;
    //     return eigen_engine.Determinant(A);
    // #else
    // Optimized fallback: Enhanced numerical stability with partial pivoting
    int n = A.size();
    if (n == 1) return A[0][0];
    
    // Create working copy for partial pivoting
    Matrix working_matrix = A;
    double det = 1.0;
    
    // Gaussian elimination with partial pivoting for better numerical stability
    for (int i = 0; i < n; i++) {
        // Find pivot
        int max_row = i;
        for (int k = i + 1; k < n; k++) {
            if (std::abs(working_matrix[k][i]) > std::abs(working_matrix[max_row][i])) {
                max_row = k;
            }
        }
        
        // Swap rows if necessary
        if (max_row != i) {
            std::swap(working_matrix[i], working_matrix[max_row]);
            det = -det;  // Row swap changes sign
        }
        
        // Check for near-zero pivot (numerical stability)
        if (std::abs(working_matrix[i][i]) < 1e-9) return 0.0;
        
        det *= working_matrix[i][i];
        
        // Eliminate column
        for (int k = i + 1; k < n; k++) {
            double factor = working_matrix[k][i] / working_matrix[i][i];
            for (int j = i; j < n; j++) {
                working_matrix[k][j] -= factor * working_matrix[i][j];
            }
        }
    }
    
    return std::abs(det) < 1e-9 ? 0.0 : det; // Final numerical stability check
}

Matrix LinearSystemParser::Transpose(const Matrix &A)
{
    if (A.empty())
        return {};
    int N = A.size();
    int M = A[0].size();
    Matrix T(M, std::vector<double>(N));
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < M; j++)
            T[j][i] = A[i][j];
    }
    return T;
}

std::pair<Matrix, Matrix> LinearSystemParser::GramSchmidt(const Matrix &A)
{
    // AXIOM v3.1: Eigen integration disabled - type mismatch
    // #ifdef ENABLE_EIGEN
    //     static AXIOM::EigenEngine eigen_engine;
    //     auto [Q, R] = eigen_engine.QRDecomposition(A);
    //     if (Q.empty() || R.empty()) return {{}, {}};
    //     return {Q, R};
    // #else
    // Fallback: Manual Gram-Schmidt with enhanced numerical stability
    Matrix A_cols = Transpose(A);
    if (A_cols.empty()) return {{}, {}};
    int N = A_cols.size();
    Matrix Q_cols = A_cols;
    Matrix R(N, std::vector<double>(N, 0.0));
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < i; j++) {
            R[j][i] = DotProduct(Q_cols[j], A_cols[i]);
            std::vector<double> projection = VectorScale(Q_cols[j], R[j][i]);
            Q_cols[i] = VectorSub(Q_cols[i], projection);
        }
        R[i][i] = VectorNorm(Q_cols[i]);
        if (std::abs(R[i][i]) > 1e-9) {
            Q_cols[i] = VectorScale(Q_cols[i], (1.0 / R[i][i]));
        } else {
            return {{}, {}}; // Numerical instability detected
        }
    }
    Matrix Q = Transpose(Q_cols);
    return {Q, R};
}

LinAlgResult LinearSystemParser::solve_linear_system(const std::vector<std::vector<double>> &A, const std::vector<double> &b)
{
    int N = A.size();
    if (N == 0 || A[0].size() != N || b.size() != N)
        return {std::nullopt, LinAlgErr::MatrixMismatch};
    std::vector<std::vector<double>> M(N, std::vector<double>(N + 1));
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; ++j)
            M[i][j] = A[i][j];
        M[i][N] = b[i];
    }
    for (int i = 0; i < N; i++)
    {
        int max_row = i;
        for (int k = i + 1; k < N; k++)
        {
            if (std::abs(M[k][i]) > std::abs(M[max_row][i]))
                max_row = k;
        }
        std::swap(M[i], M[max_row]);
        if (std::abs(M[i][i]) < 1e-9)
            return {std::nullopt, LinAlgErr::NoSolution};
        for (int j = i + 1; j <= N; j++)
            M[i][j] /= M[i][i];
        for (int k = 0; k < N; k++)
        {
            if (k != i)
            {
                double factor = M[k][i];
                for (int j = i; j <= N; j++)
                    M[k][j] -= factor * M[i][j];
            }
        }
    }
    std::vector<double> solution(N);
    for (int i = 0; i < N; i++)
        solution[i] = M[i][N];
    return {std::optional<std::vector<double>>(solution), LinAlgErr::None};
}

std::optional<std::vector<double>> LinearSystemParser::CramersRule(const Matrix &A, const std::vector<double> &b)
{
    int n = A.size();
    if (n != b.size())
        return std::nullopt;
    double detA = Determinant(A);
    if (isCloseToZero(detA))
        return std::nullopt;
    std::vector<double> solution(n);
    for (int i = 0; i < n; ++i)
    {
        Matrix Ai = A;
        for (int j = 0; j < n; ++j)
            Ai[j][i] = b[j];
        double detAi = Determinant(Ai);
        solution[i] = detAi / detA;
    }
    return solution;
}

bool isCloseToZero(double value, double epsilon)
{
    return std::abs(value) < epsilon;
}

Matrix LinearSystemParser::ParseMatrixString(const std::string &input)
{
    Matrix result;
    MatrixLexer lexer(input);
    std::vector<double> current_row;

    Token token = lexer.NextToken();

    // Outer brackets check
    if (token.type == TokenType::LBracket)
    {
        token = lexer.NextToken();
        // Handle double brackets [[...]] case implicitly by flow
    }

    while (token.type != TokenType::End)
    {
        if (token.type == TokenType::LBracket)
        {
            if (!current_row.empty())
            {
                result.push_back(current_row);
                current_row.clear();
            }
        }
        else if (token.type == TokenType::Number)
        {
            // Lexer ensures token.value is a valid number string.
            // Using stod is safe here without try-catch for flow control.
            current_row.push_back(std::stod(token.value));
        }
        else if (token.type == TokenType::Semicolon || token.type == TokenType::RBracket)
        {
            if (!current_row.empty())
            {
                result.push_back(current_row);
                current_row.clear();
            }
        }

        token = lexer.NextToken();
    }

    if (!current_row.empty())
    {
        result.push_back(current_row);
    }

    if (result.empty())
        return {};
    
    // Validate rectangular matrix
    size_t cols = result[0].size();
    for (const auto &row : result)
    {
        if (row.size() != cols)
            return {};
    }
    return result;
}