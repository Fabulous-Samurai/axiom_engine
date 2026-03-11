// AXIOM Engine v3.0 - Edge Case & Stress Test Suite
// Tests boundary conditions, error handling, and extreme inputs

#include <iostream>
#include <vector>
#include <limits>
#include <cmath>

#include "../include/algebraic_parser.h"
#include "../include/statistics_engine.h"
#include "../include/linear_system_parser.h"
#include "../include/symbolic_engine.h"
#include "../include/unit_manager.h"

class EdgeCaseRunner {
public:
    EdgeCaseRunner() {}

    template<typename Func>
    void RunTest(const std::string& name, Func test_func) {
        total_tests_++;
        std::cout << "  [TEST] " << name << " ... ";
        try {
            bool result = test_func();
            if (result) {
                std::cout << "\x1b[32mPASS\x1b[0m" << std::endl;
                passed_tests_++;
            } else {
                std::cout << "\x1b[31mFAIL\x1b[0m (assertion failed)" << std::endl;
            }
        } catch (const std::runtime_error& e) {
            std::cout << "\x1b[31mFAIL\x1b[0m (runtime error: " << e.what() << ")" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "\x1b[31mFAIL\x1b[0m (exception: " << e.what() << ")" << std::endl;
        }
    }

    void StartSection(const std::string& name) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  " << name << std::endl;
        std::cout << "========================================" << std::endl;
    }

    void EndSection() {}

    void PrintSummary() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "           TEST SUMMARY" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Total:   " << total_tests_ << std::endl;
        std::cout << "Passed:  " << passed_tests_ << std::endl;
        std::cout << "Failed:  " << (total_tests_ - passed_tests_) << std::endl;

        if (passed_tests_ == total_tests_) {
            std::cout << "\n\x1b[32m✓ ALL EDGE CASE TESTS PASSED!\x1b[0m\n" << std::endl;
        } else {
            std::cout << "\n\x1b[31m✗ SOME EDGE CASE TESTS FAILED!\x1b[0m\n" << std::endl;
        }
    }

    int GetFailedCount() const { return total_tests_ - passed_tests_; }

private:
    int total_tests_{0};
    int passed_tests_{0};
};

void TestAlgebraicParserEdgeCases(EdgeCaseRunner& runner) {
    runner.StartSection("ALGEBRAIC PARSER EDGE CASES");
    
    AlgebraicParser parser;

    // Empty input
    runner.RunTest("Empty string input", [&parser]() {
        auto result = parser.ParseAndExecute("");
        return !result.HasResult(); // Should fail gracefully
    });

    // Very long expression
    runner.RunTest("Very long expression chain", [&parser]() {
        std::string expr = "1";
        for (int i = 0; i < 100; ++i) {
            expr += "+1";
        }
        auto result = parser.ParseAndExecute(expr);
        return result.HasResult() && std::abs(*result.GetDouble() - 101.0) < 0.01;
    });

    // Division by zero
    runner.RunTest("Division by zero", [&parser]() {
        auto result = parser.ParseAndExecute("10/0");
        // Should return infinity or error
        return !result.HasResult() || std::isinf(*result.GetDouble());
    });

    // Very small numbers
    runner.RunTest("Very small number (1e-100)", [&parser]() {
        auto result = parser.ParseAndExecute("1e-100");
        return result.HasResult();
    });

    // Very large numbers
    runner.RunTest("Very large number (1e100)", [&parser]() {
        auto result = parser.ParseAndExecute("1e100");
        return result.HasResult();
    });

    // Negative zero
    runner.RunTest("Negative zero arithmetic", [&parser]() {
        auto result = parser.ParseAndExecute("-0 + 0");
        return result.HasResult() && std::abs(*result.GetDouble()) < 1e-10;
    });

    // Nested parentheses (deep)
    runner.RunTest("Deeply nested parentheses (10 levels)", [&parser]() {
        std::string expr = "((((((((((1+1))))))))))";
        auto result = parser.ParseAndExecute(expr);
        return result.HasResult() && std::abs(*result.GetDouble() - 2.0) < 0.01;
    });

    // Enormous complex expression (Sandbox check)
    runner.RunTest("Enormous complex expression (Stress Test)", [&parser]() {
        std::string expr = "sin(cos(tan(log(sqrt(144)))))";
        for (int i = 0; i < 50; ++i) {
            expr += " + " + std::to_string(i) + " * sin(" + std::to_string(i) + ")";
        }
        auto result = parser.ParseAndExecute(expr);
        return result.HasResult();
    });

    // Missing operand
    runner.RunTest("Missing operand (5+)", [&parser]() {
        auto result = parser.ParseAndExecute("5+");
        return !result.HasResult(); // Should fail
    });

    // Unknown function
    runner.RunTest("Unknown function (foo(5))", [&parser]() {
        auto result = parser.ParseAndExecute("foo(5)");
        return !result.HasResult(); // Should fail
    });

    // Undefined variable
    runner.RunTest("Undefined variable (x+5)", [&parser]() {
        auto result = parser.ParseAndExecute("x+5");
        return !result.HasResult(); // Should fail without context
    });

    // Special characters
    runner.RunTest("Special characters (!@#$%)", [&parser]() {
        auto result = parser.ParseAndExecute("5!@#$%3");
        return !result.HasResult(); // Should fail
    });

    // NaN operations
    runner.RunTest("sqrt of negative number", [&parser]() {
        auto result = parser.ParseAndExecute("sqrt(-1)");
        // Should handle complex numbers or return NaN
        return result.HasResult();
    });

    runner.EndSection();
}

void TestStatisticsEngineEdgeCases(EdgeCaseRunner& runner) {
    runner.StartSection("STATISTICS ENGINE EDGE CASES");
    
    StatisticsEngine stats;

    // Empty vector
    runner.RunTest("Mean of empty vector", [&stats]() {
        std::vector<double> empty;
        auto result = stats.Mean(empty);
        return !result.HasResult(); // Should fail
    });

    // Single element
    runner.RunTest("Mean of single element", [&stats]() {
        std::vector<double> single = {5.0};
        auto result = stats.Mean(single);
        return result.HasResult() && std::abs(*result.GetDouble() - 5.0) < 0.01;
    });

    // All same values
    runner.RunTest("Variance of constant values", [&stats]() {
        std::vector<double> constant(100, 5.0);
        auto result = stats.Variance(constant);
        return result.HasResult() && std::abs(*result.GetDouble()) < 0.01; // Should be 0
    });

    // Extreme values
    runner.RunTest("Mean with very large values", [&stats]() {
        std::vector<double> large = {1e100, 1e100, 1e100};
        auto result = stats.Mean(large);
        return result.HasResult() && *result.GetDouble() > 1e99;
    });

    // Negative values
    runner.RunTest("Mean of all negative values", [&stats]() {
        std::vector<double> negative = {-1.0, -2.0, -3.0, -4.0, -5.0};
        auto result = stats.Mean(negative);
        return result.HasResult() && std::abs(*result.GetDouble() + 3.0) < 0.01;
    });

    // Mixed positive/negative
    runner.RunTest("Variance with mixed signs", [&stats]() {
        std::vector<double> mixed = {-10.0, -5.0, 0.0, 5.0, 10.0};
        auto result = stats.Variance(mixed);
        return result.HasResult() && *result.GetDouble() > 0;
    });

    // Correlation with mismatched sizes
    runner.RunTest("Correlation with different vector sizes", [&stats]() {
        std::vector<double> x = {1.0, 2.0, 3.0};
        std::vector<double> y = {1.0, 2.0};
        auto result = stats.Correlation(x, y);
        return !result.HasResult(); // Should fail
    });

    // Perfect correlation
    runner.RunTest("Perfect positive correlation", [&stats]() {
        std::vector<double> x = {1.0, 2.0, 3.0, 4.0, 5.0};
        std::vector<double> y = {2.0, 4.0, 6.0, 8.0, 10.0}; // y = 2x
        auto result = stats.Correlation(x, y);
        return result.HasResult() && std::abs(*result.GetDouble() - 1.0) < 0.01;
    });

    // No correlation
    runner.RunTest("Zero correlation (uncorrelated)", [&stats]() {
        std::vector<double> x = {1.0, 2.0, 3.0, 4.0, 5.0};
        std::vector<double> y = {5.0, 3.0, 5.0, 3.0, 5.0}; // constant alternating
        auto result = stats.Correlation(x, y);
        return result.HasResult(); // Should succeed with near-zero correlation
    });

    // Percentile edge cases
    runner.RunTest("0th percentile", [&stats]() {
        std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0};
        auto result = stats.Percentile(data, 0.0);
        return result.HasResult() && std::abs(*result.GetDouble() - 1.0) < 0.01;
    });

    runner.RunTest("100th percentile", [&stats]() {
        std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0};
        auto result = stats.Percentile(data, 100.0);
        return result.HasResult() && std::abs(*result.GetDouble() - 5.0) < 0.01;
    });

    runner.RunTest("Invalid percentile (150)", [&stats]() {
        std::vector<double> data = {1.0, 2.0, 3.0};
        auto result = stats.Percentile(data, 150.0);
        return !result.HasResult(); // Should fail
    });

    // Linear regression with insufficient data
    runner.RunTest("Linear regression with 1 point", [&stats]() {
        std::vector<double> x = {1.0};
        std::vector<double> y = {2.0};
        auto result = stats.LinearRegression(x, y);
        return !result.HasResult(); // Should fail (need at least 2 points)
    });

    runner.EndSection();
}

void TestLinearSystemParserEdgeCases(EdgeCaseRunner& runner) {
    runner.StartSection("LINEAR SYSTEM PARSER EDGE CASES");
    
    LinearSystemParser parser;

    // Singular matrix (no unique solution)
    runner.RunTest("Singular matrix (determinant = 0)", [&parser]() {
        auto result = parser.ParseAndExecute("solve [[1,2],[2,4]] [3,6]");
        // Should fail or return infinite solutions
        return !result.HasResult();
    });

    // 1x1 system
    runner.RunTest("1x1 system (trivial)", [&parser]() {
        auto result = parser.ParseAndExecute("solve [[5]] [10]");
        return result.HasResult(); // Solution: x = 2
    });

    // Overdetermined system
    runner.RunTest("Empty matrix", [&parser]() {
        auto result = parser.ParseAndExecute("solve [[]] []");
        return !result.HasResult(); // Should fail
    });

    // Mismatched dimensions
    runner.RunTest("Matrix-vector dimension mismatch", [&parser]() {
        auto result = parser.ParseAndExecute("solve [[1,2],[3,4]] [5]");
        return !result.HasResult(); // Should fail
    });

    // Non-square matrix
    runner.RunTest("Non-square matrix (3x2)", [&parser]() {
        auto result = parser.ParseAndExecute("solve [[1,2],[3,4],[5,6]] [7,8,9]");
        return !result.HasResult(); // Should fail for square system solver
    });

    // Very large coefficients
    runner.RunTest("Large coefficients", [&parser]() {
        auto result = parser.ParseAndExecute("solve [[1000000,2000000],[3000000,4000000]] [5000000,11000000]");
        return result.HasResult(); // Should handle scaling
    });

    // Very small coefficients
    runner.RunTest("Small coefficients (near zero)", [&parser]() {
        auto result = parser.ParseAndExecute("solve [[0.0001,0.0002],[0.0003,0.0004]] [0.0005,0.0011]");
        return result.HasResult(); // Numerical stability test
    });

    // Malformed input
    runner.RunTest("Malformed matrix notation", [&parser]() {
        auto result = parser.ParseAndExecute("solve [1,2],[3,4 [5,6]");
        return !result.HasResult(); // Should fail parsing
    });

    runner.EndSection();
}

void TestSymbolicEngineEdgeCases(EdgeCaseRunner& runner) {
    runner.StartSection("SYMBOLIC ENGINE EDGE CASES");
    
    SymbolicEngine symbolic;

    // Empty expression
    runner.RunTest("Empty expression", [&symbolic]() {
        auto result = symbolic.Expand("");
        return !result.HasResult(); // Should fail
    });

    // Constant expression
    runner.RunTest("Expand constant (5)", [&symbolic]() {
        auto result = symbolic.Expand("5");
        return result.HasResult();
    });

    // Very complex polynomial
    runner.RunTest("Expand (x+1)^10", [&symbolic]() {
        auto result = symbolic.Expand("(x+1)^10");
        return result.HasResult(); // Should produce 11-term polynomial
    });

    // Undefined variable in substitution
    runner.RunTest("Substitute undefined variable", [&symbolic]() {
        auto result = symbolic.Substitute("x+y", "z", "5");
        return result.HasResult(); // Should leave x+y unchanged
    });

    // Integration of constant
    runner.RunTest("Integrate constant", [&symbolic]() {
        auto result = symbolic.Integrate("5", "x");
        return result.HasResult(); // Should be 5*x
    });

    // Differentiation of constant
    runner.RunTest("Differentiate constant", [&symbolic]() {
        auto result = symbolic.Differentiate("5", "x");
        return result.HasResult(); // Should be 0
    });

    // Factor prime number
    runner.RunTest("Factor prime expression", [&symbolic]() {
        auto result = symbolic.Factor("x^2 + x + 1");
        return result.HasResult(); // May not factor over reals
    });

    // Nested functions
    runner.RunTest("Differentiate sin(cos(x))", [&symbolic]() {
        auto result = symbolic.Differentiate("sin(cos(x))", "x");
        return result.HasResult(); // Chain rule
    });

    runner.EndSection();
}

void TestUnitManagerEdgeCases(EdgeCaseRunner& runner) {
    runner.StartSection("UNIT MANAGER EDGE CASES");
    
    UnitManager units;

    // Zero conversion
    runner.RunTest("Convert 0 km to m", [&units]() {
        auto result = units.ConvertUnit(0.0, "km", "m");
        return result.HasResult() && std::abs(*result.GetDouble()) < 0.01;
    });

    // Negative values
    runner.RunTest("Convert -5 m to cm", [&units]() {
        auto result = units.ConvertUnit(-5.0, "m", "cm");
        return result.HasResult() && std::abs(*result.GetDouble() + 500.0) < 0.1;
    });

    // Very large values
    runner.RunTest("Convert 1e10 m to km", [&units]() {
        auto result = units.ConvertUnit(1e10, "m", "km");
        return result.HasResult();
    });

    // Same unit conversion
    runner.RunTest("Convert m to m (identity)", [&units]() {
        auto result = units.ConvertUnit(5.0, "m", "m");
        return result.HasResult() && std::abs(*result.GetDouble() - 5.0) < 0.01;
    });

    // Unknown source unit
    runner.RunTest("Convert from unknown unit", [&units]() {
        auto result = units.ConvertUnit(5.0, "foobar", "m");
        return !result.HasResult(); // Should fail
    });

    // Unknown target unit
    runner.RunTest("Convert to unknown unit", [&units]() {
        auto result = units.ConvertUnit(5.0, "m", "foobar");
        return !result.HasResult(); // Should fail
    });

    // Incompatible units
    runner.RunTest("Incompatible units (kg to m)", [&units]() {
        auto compatible = units.AreCompatible("kg", "m");
        return !compatible; // Should be false
    });

    runner.EndSection();
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                                               ║" << std::endl;
    std::cout << "║        AXIOM ENGINE v3.0 - EDGE CASE TEST SUITE               ║" << std::endl;
    std::cout << "║                                                               ║" << std::endl;
    std::cout << "║         Boundary Conditions & Error Handling                  ║" << std::endl;
    std::cout << "║                                                               ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════╝" << std::endl;

    EdgeCaseRunner runner;

    TestAlgebraicParserEdgeCases(runner);
    TestStatisticsEngineEdgeCases(runner);
    TestLinearSystemParserEdgeCases(runner);
    TestSymbolicEngineEdgeCases(runner);
    TestUnitManagerEdgeCases(runner);

    runner.PrintSummary();

    return runner.GetFailedCount();
}
