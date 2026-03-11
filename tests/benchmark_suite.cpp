
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
