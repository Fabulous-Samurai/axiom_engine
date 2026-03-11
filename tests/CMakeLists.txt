cmake_minimum_required(VERSION 3.12)

if(NOT AXIOM_ENABLE_GTEST)
    message(STATUS "GTest disabled; skipping tests subdirectory")
    return()
endif()

add_subdirectory(unittest)
