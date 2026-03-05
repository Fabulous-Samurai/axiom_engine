/**
 * @file expression_policy.h
 * @brief Lightweight expression risk checks for stability guardrails.
 */

#pragma once

#include "dynamic_calc_types.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <string>

namespace AXIOM
{

    inline std::size_t ReadPolicySizeEnv(const char *name, std::size_t fallback)
    {
        const char *raw = std::getenv(name);
        if (raw == nullptr || *raw == '\0')
        {
            return fallback;
        }

        char *end = nullptr;
        const unsigned long long parsed = std::strtoull(raw, &end, 10);
        if (end == raw || *end != '\0' || parsed == 0ULL)
        {
            return fallback;
        }

        return static_cast<std::size_t>(parsed);
    }

    struct ExpressionPolicyDecision
    {
        bool allowed = true;
        CalcErr error = CalcErr::None;
        std::string reason;
    };

    inline ExpressionPolicyDecision AssessExpressionPolicy(
        const std::string &input,
        CalculationMode mode)
    {
        const std::size_t kMaxCharsDefault =
            ReadPolicySizeEnv("AXIOM_POLICY_MAX_CHARS_DEFAULT", 8192);
        const std::size_t kMaxCharsSymbolic =
            ReadPolicySizeEnv("AXIOM_POLICY_MAX_CHARS_SYMBOLIC", 16384);
        const std::size_t kMaxTokens =
            ReadPolicySizeEnv("AXIOM_POLICY_MAX_TOKENS", 2048);
        const std::size_t kMaxDepthDefault =
            ReadPolicySizeEnv("AXIOM_POLICY_MAX_DEPTH_DEFAULT", 128);
        const std::size_t kMaxDepthSymbolic =
            ReadPolicySizeEnv("AXIOM_POLICY_MAX_DEPTH_SYMBOLIC", 256);
        const std::size_t kMaxCaretOps =
            ReadPolicySizeEnv("AXIOM_POLICY_MAX_CARET_OPS", 64);
        const std::size_t kMaxMatrixElements =
            ReadPolicySizeEnv("AXIOM_POLICY_MAX_MATRIX_ELEMENTS", 40000);

        const std::size_t max_chars =
            (mode == CalculationMode::SYMBOLIC) ? kMaxCharsSymbolic : kMaxCharsDefault;
        const std::size_t max_depth =
            (mode == CalculationMode::SYMBOLIC) ? kMaxDepthSymbolic : kMaxDepthDefault;

        if (input.empty())
        {
            return {false, CalcErr::ArgumentMismatch, "empty_input"};
        }

        if (input.size() > max_chars)
        {
            return {false, CalcErr::MemoryExhausted, "input_too_long"};
        }

        std::size_t token_count = 0;
        std::size_t max_observed_depth = 0;
        std::size_t current_depth = 0;
        std::size_t caret_ops = 0;
        std::size_t approx_matrix_elements = 0;
        bool in_token = false;

        for (char ch : input)
        {
            const unsigned char uch = static_cast<unsigned char>(ch);
            const bool token_char = std::isalnum(uch) || ch == '_' || ch == '.';
            if (token_char)
            {
                if (!in_token)
                {
                    in_token = true;
                    ++token_count;
                }
            }
            else
            {
                in_token = false;
            }

            if (ch == '(' || ch == '[' || ch == '{')
            {
                ++current_depth;
                max_observed_depth = std::max(max_observed_depth, current_depth);
            }
            else if (ch == ')' || ch == ']' || ch == '}')
            {
                if (current_depth == 0)
                {
                    return {false, CalcErr::ParseError, "unbalanced_brackets"};
                }
                --current_depth;
            }

            if (ch == '^')
            {
                ++caret_ops;
            }

            if (ch == ',' || ch == ';')
            {
                ++approx_matrix_elements;
            }
        }

        if (current_depth != 0)
        {
            return {false, CalcErr::ParseError, "unbalanced_brackets"};
        }

        if (token_count > kMaxTokens)
        {
            return {false, CalcErr::StackOverflow, "token_budget_exceeded"};
        }

        if (max_observed_depth > max_depth)
        {
            return {false, CalcErr::StackOverflow, "nesting_too_deep"};
        }

        if (caret_ops > kMaxCaretOps)
        {
            return {false, CalcErr::InfiniteLoop, "exponentiation_budget_exceeded"};
        }

        // Very rough matrix workload approximation: separators + one seed element.
        if (approx_matrix_elements > 0 && (approx_matrix_elements + 1) > kMaxMatrixElements)
        {
            return {false, CalcErr::MemoryExhausted, "matrix_budget_exceeded"};
        }

        return {true, CalcErr::None, "ok"};
    }

} // namespace AXIOM
