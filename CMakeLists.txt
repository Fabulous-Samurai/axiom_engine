cmake_minimum_required(VERSION 3.12)

# FORCE NINJA GENERATOR - MAXIMUM SPEED BUILD SYSTEM
set(CMAKE_GENERATOR "Ninja" CACHE STRING "Generator" FORCE)

# Project and Version
project(axiom_engine VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Auto-install optional Python GUI dependencies via pip
option(AXIOM_AUTO_INSTALL_PYTHON_DEPS "Install optional Python GUI dependencies via pip" ON)
set(AXIOM_PY_REQUIREMENTS "${CMAKE_SOURCE_DIR}/requirements-optional.txt")

# Enable position independent code for nanobind
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# NINJA PARALLEL BUILD OPTIMIZATION - ALWAYS ENABLED
include(ProcessorCount)
ProcessorCount(CPU_COUNT)
if(NOT CPU_COUNT EQUAL 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${CPU_COUNT})
    message(STATUS "🚀 NINJA PARALLEL BUILD: ${CPU_COUNT} cores")
else()
    set(CMAKE_BUILD_PARALLEL_LEVEL 8)  # Fallback
    message(STATUS "🚀 NINJA PARALLEL BUILD: 8 cores (fallback)")
endif()

# NINJA-ONLY OPTIMIZATION FLAGS - SENNA SPEED
# GCC/Clang: Maximum optimization + native arch + fast math
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -DNDEBUG -ffast-math -flto" CACHE STRING "Release flags" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g" CACHE STRING "Debug flags" FORCE)
# Enable Link Time Optimization  
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto" CACHE STRING "Release linker flags" FORCE)

message(STATUS "🏎️ NINJA OPTIMIZATION: Release mode with native CPU optimizations")

# Force release mode for maximum speed
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    message(STATUS "🏎️ AXIOM: Defaulting to Release mode for Senna Speed")
endif()

# ENTERPRISE FEATURES - ALWAYS ENABLED FOR AXIOM v3.0
set(ENABLE_ADVANCED_FEATURES ON CACHE BOOL "Advanced features" FORCE)
set(ENABLE_PARALLEL_BUILD ON CACHE BOOL "Parallel build" FORCE)
set(ENABLE_FAST_BUILD ON CACHE BOOL "Maximum build speed" FORCE)
set(ENABLE_LTO ON CACHE BOOL "Link Time Optimization" FORCE)

# Find Python for nanobind integration (REQUIRED for nanobind)
find_package(Python COMPONENTS Interpreter Development REQUIRED)
if(Python_FOUND)
    message(STATUS "Python found: ${Python_VERSION}")
    set(ENABLE_PYTHON_FFI ON)
    # Optionally install Python GUI deps automatically (numpy/scipy/matplotlib/pandas/sympy)
    # Skip auto-install if not running inside a virtual environment to avoid PEP 668 errors
    if(AXIOM_AUTO_INSTALL_PYTHON_DEPS AND EXISTS ${AXIOM_PY_REQUIREMENTS})
        if(DEFINED ENV{VIRTUAL_ENV})
            add_custom_target(python_deps ALL
                COMMAND "${Python_EXECUTABLE}" -m pip install -r "${AXIOM_PY_REQUIREMENTS}"
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Installing optional Python GUI dependencies from requirements-optional.txt (venv detected)"
                VERBATIM
            )
        else()
            message(WARNING "Skipping auto Python deps install: no virtualenv detected (to avoid PEP 668 in MSYS-managed Python). Use your venv's pip manually.")
        endif()
    endif()
    
    # Eigen3 for high-performance linear algebra - auto-download via FetchContent
    include(FetchContent)
    find_package(Eigen3 QUIET)
    if(Eigen3_FOUND)
        message(STATUS "Eigen3 found: ${Eigen3_VERSION}")
    else()
        message(STATUS "Eigen3 not found - downloading via FetchContent...")
        FetchContent_Declare(
            Eigen3
            GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
            GIT_TAG 3.4.0
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
        )
        set(EIGEN_BUILD_DOC OFF CACHE BOOL "Build Eigen documentation" FORCE)
        set(BUILD_TESTING OFF CACHE BOOL "Build Eigen tests" FORCE)
        set(EIGEN_BUILD_PKGCONFIG OFF CACHE BOOL "Build Eigen pkg-config" FORCE)
        FetchContent_MakeAvailable(Eigen3)
        message(STATUS "Eigen3 downloaded and configured")
    endif()
    
    # Try to find nanobind
    find_package(nanobind QUIET)
    if(nanobind_FOUND)
        set(ENABLE_NANOBIND ON)
        add_definitions(-DENABLE_NANOBIND)
        message(STATUS "Nanobind found and enabled")
    else()
        # If not found, try to use FetchContent to download it
        message(STATUS "Nanobind not found - attempting to download via FetchContent...")
        FetchContent_Declare(
            nanobind
            GIT_REPOSITORY https://github.com/wjakob/nanobind.git
            GIT_TAG v2.9.2
        )
        FetchContent_MakeAvailable(nanobind)
        set(ENABLE_NANOBIND ON)
        add_definitions(-DENABLE_NANOBIND)
        message(STATUS "Nanobind downloaded and enabled via FetchContent")
    endif()
    
    add_definitions(-DENABLE_PYTHON_FFI)
else()
    message(STATUS "Python development libraries not found - building with limited Python support")
    set(ENABLE_PYTHON_FFI ON)  # Still enable for basic FFI
    set(ENABLE_NANOBIND OFF)
endif()

string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")
configure_file(
    "${CMAKE_SOURCE_DIR}/build/cmake/version.h.in"
    "${CMAKE_BINARY_DIR}/generated/version.h"
)

find_package(Threads REQUIRED)


include_directories(
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}/generated"
)

# Python source files (conditionally compiled)
set(PYTHON_SOURCES "")
set(PYTHON_HEADERS "")
if(ENABLE_PYTHON_FFI)
    set(PYTHON_SOURCES
        src/python_engine.cpp
        src/python_parser.cpp
        src/python_repl.cpp
    )
    set(PYTHON_HEADERS
        include/python_engine.h
        include/python_parser.h
        include/python_repl.h
    )
    include_directories(${Python_INCLUDE_DIRS})
endif()


# Enhanced source files with Eigen and parallel computing support
set(ENHANCED_SOURCES
    src/eigen_engine.cpp
    src/cpu_optimization.cpp
    # Temporarily disabled until Eigen3 is available
    # core/engine/eigen_engine.cpp
    core/dispatch/selective_dispatcher.cpp
)

# Temporarily disable nanobind until headers are resolved
# Only include nanobind interface if nanobind is enabled
#if(ENABLE_NANOBIND)
#    list(APPEND ENHANCED_SOURCES src/nanobind_interface.cpp)
#endif()

set(ENHANCED_HEADERS
    include/eigen_engine.h
    include/cpu_optimization.h
    # Temporarily disabled until Eigen3 is available
    # core/engine/eigen_engine.h
    core/dispatch/selective_dispatcher.h
)

# Temporarily disable nanobind until headers are resolved
# Only include nanobind interface header if nanobind is enabled
#if(ENABLE_NANOBIND)
#    list(APPEND ENHANCED_HEADERS include/nanobind_interface.h)
#endif()

add_executable(axiom
        src/main.cpp
        src/dynamic_calc.cpp
        src/algebraic_parser.cpp
        src/linear_system_parser.cpp
    src/statistics_parser.cpp
    src/symbolic_parser.cpp
        src/string_helpers.cpp
        src/unit_manager.cpp
        src/unit_parser.cpp
        src/statistics_engine.cpp
        src/symbolic_engine.cpp
        src/plot_engine.cpp
        ${ENHANCED_SOURCES}
        ${PYTHON_SOURCES}
        
        include/dynamic_calc.h
        include/dynamic_calc_types.h
        include/iParser.h
        include/algebraic_parser.h
        include/linear_system_parser.h
    include/statistics_parser.h
    include/symbolic_parser.h
        include/string_helpers.h
        include/unit_manager.h
        include/unit_parser.h
        include/statistics_engine.h
        include/symbolic_engine.h
        include/plot_engine.h
        ${ENHANCED_HEADERS}
        ${PYTHON_HEADERS}
)

target_link_libraries(axiom 
    PRIVATE 
    Threads::Threads
)

# Link OpenMP if available
if(OpenMP_CXX_FOUND)
    target_link_libraries(axiom PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(axiom PRIVATE ENABLE_OPENMP)
    message(STATUS "Linking with OpenMP for parallel computing")
endif()

# Link Eigen if available
if(TARGET Eigen3::Eigen)
    target_link_libraries(axiom PRIVATE Eigen3::Eigen)
    target_compile_definitions(axiom PRIVATE ENABLE_EIGEN)
    message(STATUS "Linking with Eigen3")
endif()

# Link Python libraries if available
if(ENABLE_PYTHON_FFI AND Python_FOUND)
    target_link_libraries(axiom PRIVATE ${Python_LIBRARIES})
    target_include_directories(axiom PRIVATE ${Python_INCLUDE_DIRS})
    message(STATUS "Linking with Python libraries")
endif()

# Link nanobind if available
if(ENABLE_NANOBIND AND TARGET nanobind::nanobind)
    target_link_libraries(axiom PRIVATE nanobind::nanobind)
    message(STATUS "Linking with nanobind")
endif()

# Add Python and nanobind include paths for all targets
if(ENABLE_PYTHON_FFI)
    target_include_directories(axiom PRIVATE ${Python_INCLUDE_DIRS})
endif()

if(ENABLE_NANOBIND)
    # nanobind includes should be available after FetchContent
    target_include_directories(axiom PRIVATE 
        ${CMAKE_BINARY_DIR}/_deps/nanobind-src/include)
endif()

# Enable parallel computing flags
if(ENABLE_PARALLEL_BUILD)
    target_compile_definitions(axiom PRIVATE ENABLE_PARALLEL_COMPUTING)
    message(STATUS "Parallel computing support enabled")
endif()


add_executable(run_tests
        tests/tests.cpp
        src/dynamic_calc.cpp
        src/algebraic_parser.cpp
        src/linear_system_parser.cpp
    src/statistics_parser.cpp
    src/symbolic_parser.cpp
        src/string_helpers.cpp
        src/unit_manager.cpp
        src/unit_parser.cpp
        src/statistics_engine.cpp
        src/symbolic_engine.cpp
        src/plot_engine.cpp
        ${PYTHON_SOURCES}
        
        include/dynamic_calc.h
        include/string_helpers.h
    include/statistics_parser.h
    include/symbolic_parser.h
        include/unit_manager.h
        include/unit_parser.h
        include/statistics_engine.h
        include/symbolic_engine.h
        include/plot_engine.h
        ${PYTHON_HEADERS}
)

target_link_libraries(run_tests 
    PRIVATE 
    Threads::Threads
)

# Link Python libraries for tests if available
if(ENABLE_PYTHON_FFI)
    target_link_libraries(run_tests PRIVATE Python::Python)
    target_include_directories(run_tests PRIVATE ${Python_INCLUDE_DIRS})
endif()


# ============================================================================
# GIGA TEST SUITE - Monolithic Production-Grade Test Suite
# ============================================================================
option(BUILD_GIGA_TESTS "Build comprehensive giga test suite" ON)
if(BUILD_GIGA_TESTS)
    message(STATUS "🧪 Configuring Giga Test Suite...")
    
    add_executable(giga_test_suite
        tests/giga_test_suite.cpp
        # Core sources needed for testing
        src/algebraic_parser.cpp
        src/linear_system_parser.cpp
        src/statistics_engine.cpp
        src/symbolic_engine.cpp
        src/unit_manager.cpp
        src/unit_parser.cpp
        src/plot_engine.cpp
        src/dynamic_calc.cpp
        src/string_helpers.cpp
        src/arena_allocator.cpp
    )
    
    target_include_directories(giga_test_suite PRIVATE 
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_BINARY_DIR}/generated"
    )
    
    target_link_libraries(giga_test_suite PRIVATE Threads::Threads)
    
    # Link Eigen if available
    if(TARGET Eigen3::Eigen)
        target_link_libraries(giga_test_suite PRIVATE Eigen3::Eigen)
        target_compile_definitions(giga_test_suite PRIVATE ENABLE_EIGEN)
        message(STATUS "  ✓ Linking Eigen3 to giga test suite")
    endif()
    
    # Link OpenMP if available
    if(OpenMP_CXX_FOUND)
        target_link_libraries(giga_test_suite PRIVATE OpenMP::OpenMP_CXX)
        target_compile_definitions(giga_test_suite PRIVATE ENABLE_OPENMP)
        message(STATUS "  ✓ Linking OpenMP to giga test suite")
    endif()
    
    # Add Eigen sources if enabled
    if(TARGET Eigen3::Eigen)
        target_sources(giga_test_suite PRIVATE
            src/eigen_engine.cpp
            src/cpu_optimization.cpp
            core/dispatch/selective_dispatcher.cpp
        )
    endif()
    
    message(STATUS "🚀 Giga Test Suite configured successfully")
endif()

add_executable(ast_drills tests/ast_drills.cpp)
target_include_directories(ast_drills PRIVATE "${CMAKE_SOURCE_DIR}/include")