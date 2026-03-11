cmake_minimum_required(VERSION 3.12)

if(NOT AXIOM_ENABLE_BENCHMARK)
    message(STATUS "Benchmarking disabled; skipping benchmarks subdirectory")
    return()
endif()

project(axiom_benchmarks)

add_subdirectory(sample)
