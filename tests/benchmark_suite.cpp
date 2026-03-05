/**
 * @file benchmark_suite.cpp
 * @brief Zero-overhead benchmark suite for AXIOM v3.1 Engine validation.
 * * Validates:
 * 1. Static Polymorphism (std::visit) throughput.
 * 2. Move Semantics (Zero-copy) on Vectors/Matrices.
 * 3. SPSC Lock-Free Queue IPC latency.
 */

#include "../include/dynamic_calc.h"
#include "../include/daemon_engine.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>
#include <fstream>
#include <string>

using namespace AXIOM;

struct BenchmarkResult
{
    std::string name;
    size_t iterations = 0;
    double elapsed_ms = 0.0;
    double ops_per_sec = 0.0;
    double latency_us = 0.0;
};

// High-resolution timer utility for accurate sub-millisecond profiling.
class Profiler
{
    std::chrono::high_resolution_clock::time_point start_;

public:
    void start() { start_ = std::chrono::high_resolution_clock::now(); }
    double get_elapsed_ms() const
    {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }
};

BenchmarkResult RunScalarThroughputTest(DynamicCalc &engine, size_t iterations)
{
    Profiler profiler;
    profiler.start();

    // Benchmarking vtable miss elimination (Static Dispatch)
    for (size_t i = 0; i < iterations; ++i)
    {
        // Assume "2+2" hits the AlgebraicParser via std::visit
        volatile auto res = engine.Evaluate("2+2");
    }

    double ms = profiler.get_elapsed_ms();
    BenchmarkResult result{"scalar_throughput", iterations, ms, (iterations / ms) * 1000.0, (ms * 1000.0) / static_cast<double>(iterations)};
    std::cout << "[Test 1] Scalar Throughput (" << iterations << " ops): "
              << result.elapsed_ms << " ms (" << result.ops_per_sec << " ops/sec)\n";
    return result;
}

BenchmarkResult RunTypedFastPathTest(DynamicCalc &engine, size_t iterations)
{
    Profiler profiler;
    profiler.start();

    volatile double sink = 0.0;
    for (size_t i = 0; i < iterations; ++i)
    {
        const double lhs = static_cast<double>(i);
        const double rhs = static_cast<double>((i % 97) + 1);
        const auto result = engine.EvaluateFast(lhs, rhs, FastArithmeticOp::Multiply);
        if (result.HasResult())
        {
            auto val = result.GetDouble();
            if (val.has_value())
            {
                sink += *val;
            }
        }
    }

    double ms = profiler.get_elapsed_ms();
    BenchmarkResult result{"typed_fast_path", iterations, ms, (iterations / ms) * 1000.0, (ms * 1000.0) / static_cast<double>(iterations)};
    std::cout << "[Test 1b] Typed Fast-Path Throughput (" << iterations << " ops): "
              << result.elapsed_ms << " ms (" << result.ops_per_sec << " ops/sec)\n";

    if (sink < 0)
    {
        std::cout << "";
    }

    return result;
}

BenchmarkResult RunZeroCopyVectorTest(size_t vector_size, size_t iterations)
{
    Profiler profiler;
    profiler.start();

    for (size_t i = 0; i < iterations; ++i)
    {
        std::vector<double> large_vec(vector_size, 1.0);
        // Benchmarking Rvalue reference transfer. No heap allocation should occur here.
        EngineResult res = CreateSuccessResult(std::move(large_vec));
        volatile bool ready = res.HasResult();
    }

    double ms = profiler.get_elapsed_ms();
    BenchmarkResult result{"zero_copy_vector_transfer", iterations, ms, (iterations / ms) * 1000.0, (ms * 1000.0) / static_cast<double>(iterations)};
    std::cout << "[Test 2] Zero-Copy Vector Transfer (" << vector_size << " elements, "
              << iterations << " iters): " << result.elapsed_ms << " ms\n";
    return result;
}

BenchmarkResult RunLockFreeQueueLatencyTest(size_t iterations)
{
    LockFreeRingBuffer<DaemonEngine::Request, 1024> queue;
    Profiler profiler;
    profiler.start();

    for (size_t i = 0; i < iterations; ++i)
    {
        DaemonEngine::Request req;
        req.request_id = i;
        req.command = "benchmark_cmd";

        // Push Lvalue (moves internally if std::move is used in call)
        while (!queue.push(std::move(req)))
        {
        }

        DaemonEngine::Request popped_req;
        while (!queue.pop(popped_req))
        {
        }
    }

    double ms = profiler.get_elapsed_ms();
    BenchmarkResult result{"lock_free_queue_ipc", iterations, ms, (iterations / ms) * 1000.0, (ms * 1000.0) / static_cast<double>(iterations)};
    std::cout << "[Test 3] Lock-Free Queue IPC Latency (" << iterations << " cycles): "
              << result.elapsed_ms << " ms (" << result.latency_us << " microseconds/cycle)\n";
    return result;
}

void WriteBenchmarkCsv(const std::vector<BenchmarkResult> &results, const std::string &path)
{
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open())
    {
        std::cerr << "[WARN] Could not write CSV benchmark report: " << path << "\n";
        return;
    }

    out << "name,iterations,elapsed_ms,ops_per_sec,latency_us\n";
    for (const auto &r : results)
    {
        out << r.name << ","
            << r.iterations << ","
            << r.elapsed_ms << ","
            << r.ops_per_sec << ","
            << r.latency_us << "\n";
    }
}

void WriteBenchmarkJson(const std::vector<BenchmarkResult> &results, const std::string &path)
{
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open())
    {
        std::cerr << "[WARN] Could not write JSON benchmark report: " << path << "\n";
        return;
    }

    out << "{\n  \"benchmarks\": [\n";
    for (size_t i = 0; i < results.size(); ++i)
    {
        const auto &r = results[i];
        out << "    {\n"
            << "      \"name\": \"" << r.name << "\",\n"
            << "      \"iterations\": " << r.iterations << ",\n"
            << "      \"elapsed_ms\": " << r.elapsed_ms << ",\n"
            << "      \"ops_per_sec\": " << r.ops_per_sec << ",\n"
            << "      \"latency_us\": " << r.latency_us << "\n"
            << "    }";
        if (i + 1 < results.size())
        {
            out << ",";
        }
        out << "\n";
    }
    out << "  ]\n}\n";
}

int main()
{
    std::cout << "--- AXIOM v3.1 HARDWARE BENCHMARK ---\n";

    DynamicCalc engine;
    engine.SetMode(CalculationMode::ALGEBRAIC);

    std::vector<BenchmarkResult> results;
    results.reserve(4);

    // Warm-up to populate CPU L1/L2 Cache
    RunScalarThroughputTest(engine, 1000);

    std::cout << "\nRunning production benchmarks...\n";

    results.push_back(RunScalarThroughputTest(engine, 1000000)); // 1 Million scalar evaluations
    results.push_back(RunTypedFastPathTest(engine, 1000000));    // 1 Million typed fast-path evaluations
    results.push_back(RunZeroCopyVectorTest(100000, 10000));     // 10K transfers of 100K-element vectors
    results.push_back(RunLockFreeQueueLatencyTest(1000000));     // 1 Million IPC ping-pongs

    const std::string csv_path = "benchmark_results.csv";
    const std::string json_path = "benchmark_results.json";
    WriteBenchmarkCsv(results, csv_path);
    WriteBenchmarkJson(results, json_path);

    std::cout << "\nSaved benchmark reports:\n";
    std::cout << " - " << csv_path << "\n";
    std::cout << " - " << json_path << "\n";

    return 0;
}