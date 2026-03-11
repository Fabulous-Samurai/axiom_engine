cmake_minimum_required(VERSION 3.12)

project(sample_bench)

set(BENCH_SOURCES
    sample_benchmark.cpp
)

axiom_add_benchmark(axiom_sample_bench "${BENCH_SOURCES}")
