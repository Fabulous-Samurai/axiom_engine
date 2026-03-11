#include <benchmark/benchmark.h>

static void BM_SimpleAdd(benchmark::State& state) {
    for (auto _ : state) {
        volatile int x = 0;
        for (int i = 0; i < 1000; ++i) x += i;
        benchmark::DoNotOptimize(x);
    }
}
BENCHMARK(BM_SimpleAdd);

BENCHMARK_MAIN();
