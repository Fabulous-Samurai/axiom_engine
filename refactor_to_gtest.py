import re

# 1. Convert tests.cpp to GTest
tests_path = "tests/tests.cpp"
with open(tests_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Remove the old micro framework
content = re.sub(r'int g_tests_passed = 0;\nint g_tests_failed = 0;\n+', '', content)
content = re.sub(r'#define ASSERT_EQ[^\n]+\n(?:[^\n]+\n)*?(?=\n\n|\nvoid)', '', content, flags=re.MULTILINE)
content = re.sub(r'#define ASSERT_NEAR[^\n]+\n(?:[^\n]+\n)*?(?=\n\n|\nvoid)', '', content, flags=re.MULTILINE)
content = re.sub(r'#define RUN_TEST[^\n]+\n(?:[^\n]+\n)*?(?=\n\n|\n//)', '', content, flags=re.MULTILINE)

# Replace void Test_XXX() with TEST(AxiomEngine, Test_XXX)
content = re.sub(r'void (Test_\w+)\(\) \{', r'TEST(AxiomEngine, \1) {', content)

# Remove main() function totally
content = re.sub(r'int main\(\) \{.*\}', '', content, flags=re.DOTALL)

# Change ASSERT_EQ to EXPECT_EQ and ASSERT_NEAR to EXPECT_NEAR
# Note: GoogleTest's EXPECT_NEAR takes (val1, val2, tol) like we did
content = content.replace('ASSERT_EQ(', 'EXPECT_EQ(')
content = content.replace('ASSERT_NEAR(', 'EXPECT_NEAR(')

# Inject gtest header
content = "#include <gtest/gtest.h>\n" + content

with open(tests_path, 'w', encoding='utf-8') as f:
    f.write(content)

# 2. Convert benchmark_suite.cpp to GBenchmark
# Actually, the quickest is just to rewrite the core parts we want.
bench_code = """
#include "../include/dynamic_calc.h"
#include "../include/daemon_engine.h"
#include "../include/mantis_heuristic.h"
#include "../include/mantis_solver.h"
#include <benchmark/benchmark.h>
#include <vector>
#include <thread>

using namespace AXIOM;

static void BM_ScalarThroughput(benchmark::State& state) {
    DynamicCalc engine;
    engine.SetMode(CalculationMode::ALGEBRAIC);
    for (auto _ : state) {
        auto res = engine.Evaluate("2+2");
        benchmark::DoNotOptimize(res);
    }
}
BENCHMARK(BM_ScalarThroughput);

static void BM_TypedFastPath(benchmark::State& state) {
    DynamicCalc engine;
    double sink = 0.0;
    int i = 0;
    for (auto _ : state) {
        const double lhs = static_cast<double>(i);
        const double rhs = static_cast<double>((i % 97) + 1);
        const auto result = engine.EvaluateFast(lhs, rhs, FastArithmeticOp::Multiply);
        if (result.HasResult()) {
            if (auto val = result.GetDouble()) {
                sink += *val;
            }
        }
        i++;
    }
    benchmark::DoNotOptimize(sink);
}
BENCHMARK(BM_TypedFastPath);

static void BM_ZeroCopyVector(benchmark::State& state) {
    size_t vector_size = state.range(0);
    for (auto _ : state) {
        std::vector<double> large_vec(vector_size, 1.0);
        auto res = CreateSuccessResult(std::move(large_vec));
        benchmark::DoNotOptimize(res);
    }
}
BENCHMARK(BM_ZeroCopyVector)->RangeMultiplier(2)->Range(1024, 8192);

static void BM_LockFreeQueueIPC(benchmark::State& state) {
    LockFreeRingBuffer<DaemonEngine::Request, 1024> queue;
    int i = 0;
    for (auto _ : state) {
        DaemonEngine::Request req;
        req.request_id = i++;
        req.command = "benchmark_cmd";
        while (!queue.push(req)) std::this_thread::yield();

        DaemonEngine::Request popped_req;
        while (!queue.pop(popped_req)) std::this_thread::yield();
    }
}
BENCHMARK(BM_LockFreeQueueIPC);

static AXIOM::Mantis::NodeFeatureVecF32 make_test_node_f32() {
    AXIOM::Mantis::NodeFeatureVecF32 node;
    for (size_t i = 0; i < AXIOM::Mantis::kFeatureDimFP32; ++i) {
        node.data[i] = static_cast<float>(i + 1) * 0.1f;
    }
    return node;
}

static AXIOM::Mantis::TargetProfileF32 make_test_profile_f32() {
    AXIOM::Mantis::TargetProfileF32 profile;
    for (size_t i = 0; i < AXIOM::Mantis::kFeatureDimFP32; ++i) {
        profile.weights[i] = static_cast<float>(8 - i) * 0.15f;
    }
    return profile;
}

static void BM_MantisScalarF32(benchmark::State& state) {
    const auto node = make_test_node_f32();
    const auto profile = make_test_profile_f32();
    float sink = 0.0f;
    for (auto _ : state) {
        sink = sink + AXIOM::Mantis::dot_scalar_f32(node, profile);
    }
    benchmark::DoNotOptimize(sink);
}
BENCHMARK(BM_MantisScalarF32);

static void BM_MantisSIMDF32(benchmark::State& state) {
    const auto node = make_test_node_f32();
    const auto profile = make_test_profile_f32();
    float sink = 0.0f;
    for (auto _ : state) {
        sink = sink + AXIOM::Mantis::MantisHeuristic::evaluate_f32(node, profile);
    }
    benchmark::DoNotOptimize(sink);
}
BENCHMARK(BM_MantisSIMDF32);
"""

with open("tests/benchmark_suite.cpp", 'w', encoding='utf-8') as f:
    f.write(bench_code)
