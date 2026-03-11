cmake_minimum_required(VERSION 3.12)

project(axiom_unittest)

set(TEST_SOURCES
    sample_test.cpp
)

axiom_add_gtest(axiom_unit_tests "${TEST_SOURCES}")
