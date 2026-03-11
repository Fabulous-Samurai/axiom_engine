cmake_minimum_required(VERSION 3.12)

# FORCE NINJA GENERATOR - MAXIMUM SPEED BUILD SYSTEM
set(CMAKE_GENERATOR "Ninja" CACHE STRING "Generator" FORCE)

# Project and Version
project(axiom_engine VERSION 3.1.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SIMD / AVX configuration
option(AXIOM_ENABLE_SIMD_AVX2 "Enable AVX2 SIMD instructions" ON)
option(AXIOM_ENABLE_SIMD_FMA "Enable FMA instructions" ON)
option(AXIOM_ENABLE_SIMD_AVX512 "Enable AVX-512 SIMD instructions" OFF)
option(AXIOM_ENABLE_SIMD_AVX_VNNI "Enable AVX-VNNI integer dot-product instructions (Alder Lake+/Zen4+)" OFF)

# Auto-install optional Python GUI dependencies via pip
option(AXIOM_AUTO_INSTALL_PYTHON_DEPS "Install optional Python GUI dependencies via pip" ON)
option(AXIOM_ENABLE_EMBEDDED_PYTHON_ENGINE "Build embedded PythonEngine/PythonParser/PythonREPL sources" OFF)
option(AXIOM_ENABLE_CXX20_MODULES "Enable incremental C++20 modules migration (experimental)" OFF)
option(AXIOM_ENABLE_NANOBIND "Enable nanobind FFI integration" ON)
option(AXIOM_ENABLE_DOXYGEN "Enable Doxygen API documentation target" OFF)
option(AXIOM_ENABLE_HARMONIC_ARENA "Enable HarmonicArena lock-free fast-path backend" OFF)
option(AXIOM_ENABLE_MOLD_LINKER "Try to use mold linker for faster incremental links" OFF)
option(AXIOM_ENABLE_SIZE_GUARDS "Enable section GC and symbol stripping in Release" ON)
option(AXIOM_AUTO_SIGN_WINDOWS "Automatically Authenticode-sign Windows binaries after build" ON)
set(AXIOM_WINDOWS_SIGNING_SUBJECT "CN=AXIOM Local Dev Code Signing" CACHE STRING "Subject for local Windows code-signing certificate")
set(AXIOM_PY_REQUIREMENTS "${CMAKE_SOURCE_DIR}/requirements-optional.txt")

set(AXIOM_CXX20_MODULES_ACTIVE OFF)
if(AXIOM_ENABLE_CXX20_MODULES)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(STATUS "C++20 modules requested, but GNU modules-ts pipeline is unstable in this profile; skipping module compilation.")
    else()
        set(AXIOM_CXX20_MODULES_ACTIVE ON)
        message(STATUS "C++20 modules migration enabled (incremental mode)")
    endif()
    if(AXIOM_CXX20_MODULES_ACTIVE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-fmodules-ts)
    endif()
endif()

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

if(AXIOM_ENABLE_MOLD_LINKER AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    find_program(AXIOM_MOLD_EXECUTABLE mold)
    if(AXIOM_MOLD_EXECUTABLE)
        add_link_options($<$<CONFIG:Release>:-fuse-ld=mold>)
        message(STATUS "Using mold linker for Release builds")
    else()
        message(STATUS "mold linker requested but not found in PATH")
    endif()
endif()

if(AXIOM_ENABLE_SIZE_GUARDS)
    add_compile_options($<$<CONFIG:Release>:-ffunction-sections> $<$<CONFIG:Release>:-fdata-sections>)
    add_link_options($<$<CONFIG:Release>:-Wl,--gc-sections> $<$<CONFIG:Release>:-s>)
    message(STATUS "Release size guards enabled: function/data sections + gc-sections + strip")
endif()

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

# GoogleTest / GoogleBenchmark integration helper
option(AXIOM_ENABLE_GTEST "Enable GoogleTest integration" OFF)
option(AXIOM_ENABLE_BENCHMARK "Enable GoogleBenchmark integration" OFF)
include(cmake/enable_gtest_benchmark.cmake)

# Find Python for nanobind integration (REQUIRED for nanobind)
find_package(Python COMPONENTS Interpreter Development REQUIRED)
if(Python_FOUND)
    message(STATUS "Python found: ${Python_VERSION}")
    set(ENABLE_PYTHON_FFI ON)
    # Optionally install Python GUI deps automatically (numpy/matplotlib)
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
            message(STATUS "Skipping auto Python deps install: no virtualenv detected (to avoid PEP 668 in MSYS-managed Python). Use your venv's pip manually.")
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
    
    if(AXIOM_ENABLE_NANOBIND)
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
    else()
        set(ENABLE_NANOBIND OFF)
        message(STATUS "Nanobind integration disabled via AXIOM_ENABLE_NANOBIND=OFF")
    endif()
    
    add_definitions(-DENABLE_PYTHON_FFI)
else()
    message(STATUS "Python development libraries not found - building with limited Python support")
    set(ENABLE_PYTHON_FFI OFF)
    set(ENABLE_NANOBIND OFF)
endif()

string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")
configure_file(
    "${CMAKE_SOURCE_DIR}/build/cmake/version.h.in"
    "${CMAKE_BINARY_DIR}/generated/version.h"
)

find_package(Threads REQUIRED)
find_package(OpenMP QUIET)

# Add tests and benchmarks when enabled
if(AXIOM_ENABLE_GTEST)
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests)
        add_subdirectory(tests)
    endif()
endif()
if(AXIOM_ENABLE_BENCHMARK)
    if(EXISTS ${CMAKE_SOURCE_DIR}/benchmarks)
        add_subdirectory(benchmarks)
    endif()
endif()

# ============================================================================
# SIMD Feature Detection — compile-time compiler flag validation
# ============================================================================
include(CheckCXXCompilerFlag)

set(AXIOM_HAS_AVX2 OFF)
set(AXIOM_HAS_FMA OFF)
set(AXIOM_HAS_AVX_VNNI OFF)
set(AXIOM_HAS_AVX512 OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    check_cxx_compiler_flag("-mavx2"     AXIOM_COMPILER_SUPPORTS_AVX2)
    check_cxx_compiler_flag("-mfma"      AXIOM_COMPILER_SUPPORTS_FMA)
    check_cxx_compiler_flag("-mavxvnni"  AXIOM_COMPILER_SUPPORTS_AVX_VNNI)
    check_cxx_compiler_flag("-mavx512f"  AXIOM_COMPILER_SUPPORTS_AVX512)
elseif(MSVC)
    # MSVC always supports /arch:AVX2 on x64; VNNI requires checking __AVX_VNNI__ macro
    set(AXIOM_COMPILER_SUPPORTS_AVX2 ON)
    set(AXIOM_COMPILER_SUPPORTS_FMA ON)
    set(AXIOM_COMPILER_SUPPORTS_AVX_VNNI OFF)  # MSVC lacks a dedicated VNNI flag
    set(AXIOM_COMPILER_SUPPORTS_AVX512 OFF)
endif()

set(AXIOM_SIMD_FLAGS "")
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64|x64|i.86")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        if(AXIOM_ENABLE_SIMD_AVX2 AND AXIOM_COMPILER_SUPPORTS_AVX2)
            list(APPEND AXIOM_SIMD_FLAGS -mavx2)
            set(AXIOM_HAS_AVX2 ON)
        endif()
        if(AXIOM_ENABLE_SIMD_FMA AND AXIOM_COMPILER_SUPPORTS_FMA)
            list(APPEND AXIOM_SIMD_FLAGS -mfma)
            set(AXIOM_HAS_FMA ON)
        endif()
        if(AXIOM_ENABLE_SIMD_AVX_VNNI AND AXIOM_COMPILER_SUPPORTS_AVX_VNNI)
            list(APPEND AXIOM_SIMD_FLAGS -mavxvnni)
            set(AXIOM_HAS_AVX_VNNI ON)
        endif()
        if(AXIOM_ENABLE_SIMD_AVX512 AND AXIOM_COMPILER_SUPPORTS_AVX512)
            list(APPEND AXIOM_SIMD_FLAGS -mavx512f -mavx512dq -mavx512vl)
            set(AXIOM_HAS_AVX512 ON)
        endif()
    elseif(MSVC)
        if(AXIOM_ENABLE_SIMD_AVX512)
            message(WARNING "AXIOM_ENABLE_SIMD_AVX512 requested, but MSVC has no stable AVX-512 /arch switch. Using AVX2.")
        endif()
        if(AXIOM_ENABLE_SIMD_AVX2)
            list(APPEND AXIOM_SIMD_FLAGS /arch:AVX2)
            set(AXIOM_HAS_AVX2 ON)
            set(AXIOM_HAS_FMA ON)  # /arch:AVX2 implies FMA on MSVC
        endif()
    endif()

    # Status report
    message(STATUS "SIMD Feature Detection Results:")
    message(STATUS "  AVX2     : ${AXIOM_HAS_AVX2}")
    message(STATUS "  FMA      : ${AXIOM_HAS_FMA}")
    message(STATUS "  AVX-VNNI : ${AXIOM_HAS_AVX_VNNI}")
    message(STATUS "  AVX-512  : ${AXIOM_HAS_AVX512}")
endif()

function(axiom_apply_simd_flags target_name)
    if(AXIOM_SIMD_FLAGS)
        target_compile_options(${target_name} PRIVATE ${AXIOM_SIMD_FLAGS})
    endif()
    if(AXIOM_HAS_AVX2)
        target_compile_definitions(${target_name} PRIVATE AXIOM_SIMD_AVX2_ENABLED EIGEN_VECTORIZE_AVX2)
    endif()
    if(AXIOM_HAS_FMA)
        target_compile_definitions(${target_name} PRIVATE AXIOM_SIMD_FMA_ENABLED EIGEN_VECTORIZE_FMA)
    endif()
    if(AXIOM_HAS_AVX_VNNI)
        target_compile_definitions(${target_name} PRIVATE AXIOM_SIMD_AVX_VNNI_ENABLED)
    endif()
    if(AXIOM_HAS_AVX512)
        target_compile_definitions(${target_name} PRIVATE AXIOM_SIMD_AVX512_ENABLED EIGEN_VECTORIZE_AVX512)
    endif()
endfunction()

if(WIN32)
    find_program(AXIOM_POWERSHELL_EXECUTABLE NAMES pwsh powershell)
endif()

function(axiom_enable_windows_signing target_name)
    if(NOT WIN32)
        return()
    endif()

    if(NOT AXIOM_AUTO_SIGN_WINDOWS)
        return()
    endif()

    if(NOT AXIOM_POWERSHELL_EXECUTABLE)
        message(WARNING "AXIOM_AUTO_SIGN_WINDOWS is ON, but no PowerShell executable was found")
        return()
    endif()

    add_custom_command(
        TARGET ${target_name}
        POST_BUILD
        COMMAND "${AXIOM_POWERSHELL_EXECUTABLE}"
                -NoProfile
                -ExecutionPolicy
                Bypass
                -File
                "${CMAKE_SOURCE_DIR}/scripts/sign_binary.ps1"
                -FilePath
                "$<TARGET_FILE:${target_name}>"
                -Subject
                "${AXIOM_WINDOWS_SIGNING_SUBJECT}"
                -CreateIfMissing
        COMMENT "Authenticode signing: $<TARGET_FILE_NAME:${target_name}>"
        VERBATIM
    )
endfunction()

if(AXIOM_ENABLE_HARMONIC_ARENA)
    add_compile_definitions(ENABLE_HARMONIC_ARENA)
    message(STATUS "HarmonicArena backend enabled")
endif()


include_directories(
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}/generated"
    "${CMAKE_SOURCE_DIR}/core/dispatch"
)

# Python source files (conditionally compiled)
set(PYTHON_SOURCES "")
set(PYTHON_HEADERS "")
if(ENABLE_PYTHON_FFI AND AXIOM_ENABLE_EMBEDDED_PYTHON_ENGINE)
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
    message(STATUS "Embedded Python engine enabled (PythonEngine/PythonParser/PythonREPL)")
elseif(ENABLE_PYTHON_FFI)
    message(STATUS "Embedded Python engine disabled; using hybrid FFI path (nanobind + external Python GUI)")
endif()


# Enhanced source files with Eigen and parallel computing support
set(ENHANCED_SOURCES
    src/eigen_engine.cpp
    src/cpu_optimization.cpp
    src/mantis_heuristic.cpp
    core/dispatch/selective_dispatcher.cpp
    core/dispatch/selective_dispatcher_capi.cpp
)

if(ENABLE_NANOBIND)
    list(APPEND ENHANCED_SOURCES src/nanobind_interface.cpp)
endif()

set(ENHANCED_HEADERS
    include/eigen_engine.h
    include/cpu_optimization.h
    include/mantis_heuristic.h
    include/mantis_solver.h
    core/dispatch/selective_dispatcher.h
    core/dispatch/selective_dispatcher_capi.h
)

if(ENABLE_NANOBIND)
    list(APPEND ENHANCED_HEADERS include/nanobind_interface.h)
endif()

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
        src/plot_parser.cpp
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
        include/plot_parser.h
        ${ENHANCED_HEADERS}
        ${PYTHON_HEADERS}
)

if(WIN32)
    target_sources(axiom PRIVATE resources/windows/axiom_version.rc)
endif()

target_link_libraries(axiom 
    PRIVATE 
    Threads::Threads
)

# telemetry_shm.hpp uses shm_open/mmap (header-only; no extra sources needed).
# On Linux with glibc < 2.17, shm_open lives in librt; later versions inline
# it into libc.  Linking librt unconditionally on Linux is safe and harmless.
if(UNIX AND NOT APPLE)
    find_library(RT_LIBRARY rt)
    if(RT_LIBRARY)
        target_link_libraries(axiom PRIVATE ${RT_LIBRARY})
        message(STATUS "Linking librt for shm_open (telemetry_shm.hpp)")
    endif()
endif()
axiom_apply_simd_flags(axiom)

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

axiom_enable_windows_signing(axiom)


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
        src/plot_parser.cpp
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
        include/plot_parser.h
        ${PYTHON_HEADERS}
)

target_link_libraries(run_tests 
    PRIVATE 
    Threads::Threads
)
if(AXIOM_ENABLE_GTEST AND TARGET GTest::gtest_main)
    target_link_libraries(run_tests PRIVATE GTest::gtest_main)
    message(STATUS "Integrating GoogleTest for run_tests")
endif()

axiom_apply_simd_flags(run_tests)

# Link Python libraries for tests if available
if(ENABLE_PYTHON_FFI)
    target_link_libraries(run_tests PRIVATE Python::Python)
    target_include_directories(run_tests PRIVATE ${Python_INCLUDE_DIRS})
endif()

axiom_enable_windows_signing(run_tests)

add_executable(axiom_benchmark
        tests/benchmark_suite.cpp
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
        src/plot_parser.cpp
        src/daemon_engine.cpp
        ${PYTHON_SOURCES}

        include/dynamic_calc.h
        include/dynamic_calc_types.h
        include/iParser.h
        include/daemon_engine.h
)

target_link_libraries(axiom_benchmark
    PRIVATE
    Threads::Threads
)
if(AXIOM_ENABLE_BENCHMARK AND TARGET benchmark::benchmark_main)
    target_link_libraries(axiom_benchmark PRIVATE benchmark::benchmark_main)
    message(STATUS "Integrating GoogleBenchmark for axiom_benchmark")
endif()

axiom_apply_simd_flags(axiom_benchmark)

if(ENABLE_PYTHON_FFI AND AXIOM_ENABLE_EMBEDDED_PYTHON_ENGINE)
    target_link_libraries(axiom_benchmark PRIVATE Python::Python)
    target_include_directories(axiom_benchmark PRIVATE ${Python_INCLUDE_DIRS})
endif()

axiom_enable_windows_signing(axiom_benchmark)


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
        src/statistics_parser.cpp
        src/symbolic_parser.cpp
        src/statistics_engine.cpp
        src/symbolic_engine.cpp
        src/unit_manager.cpp
        src/unit_parser.cpp
        src/plot_engine.cpp
        src/plot_parser.cpp
        src/dynamic_calc.cpp
        src/string_helpers.cpp
        src/arena_allocator.cpp
        ${PYTHON_SOURCES}
    )
    
    target_include_directories(giga_test_suite PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_BINARY_DIR}/generated"
        "${CMAKE_SOURCE_DIR}/core/dispatch"
    )

    if(ENABLE_PYTHON_FFI)
        target_include_directories(giga_test_suite PRIVATE ${Python_INCLUDE_DIRS})
        target_link_libraries(giga_test_suite PRIVATE Python::Python)
    endif()

    if(ENABLE_NANOBIND)
        target_include_directories(giga_test_suite PRIVATE
            "${CMAKE_BINARY_DIR}/_deps/nanobind-src/include")
    endif()

    if(ENABLE_NANOBIND AND TARGET nanobind::nanobind)
        target_link_libraries(giga_test_suite PRIVATE nanobind::nanobind)
    endif()

    target_link_libraries(giga_test_suite PRIVATE Threads::Threads)
    axiom_apply_simd_flags(giga_test_suite)
    
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

    axiom_enable_windows_signing(giga_test_suite)
    
    message(STATUS "🚀 Giga Test Suite configured successfully")
endif()

# ============================================================================
# C++20 MODULES PILOT (INCREMENTAL MIGRATION)
# ============================================================================
if(AXIOM_CXX20_MODULES_ACTIVE)
    add_library(axiom_modules_pilot OBJECT
        modules/axiom_dynamic_calc_types_module.ixx
        modules/axiom_fast_math_module.ixx
        modules/axiom_signal_exec_module.ixx
    )

    target_include_directories(axiom_modules_pilot PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
    )

    if(TARGET Eigen3::Eigen)
        target_link_libraries(axiom_modules_pilot PRIVATE Eigen3::Eigen)
    endif()

    message(STATUS "🧩 C++20 modules pilot target enabled: axiom_modules_pilot")
elseif(AXIOM_ENABLE_CXX20_MODULES)
    message(STATUS "🧩 C++20 modules pilot target skipped for current compiler")
    add_custom_target(axiom_modules_pilot
        COMMAND ${CMAKE_COMMAND} -E echo "C++20 modules pilot skipped for compiler: ${CMAKE_CXX_COMPILER_ID}"
    )
endif()

add_executable(ast_drills tests/ast_drills.cpp)
target_include_directories(ast_drills PRIVATE "${CMAKE_SOURCE_DIR}/include")
axiom_apply_simd_flags(ast_drills)
axiom_enable_windows_signing(ast_drills)

if(AXIOM_SIMD_FLAGS)
    string(JOIN " " AXIOM_SIMD_FLAGS_STR ${AXIOM_SIMD_FLAGS})
    message(STATUS "SIMD compile flags active: ${AXIOM_SIMD_FLAGS_STR}")
else()
    message(STATUS "SIMD compile flags active: compiler defaults")
endif()

# ============================================================================
# DOXYGEN API DOCUMENTATION
# ============================================================================
if(AXIOM_ENABLE_DOXYGEN)
    find_package(Doxygen QUIET)

    set(AXIOM_DOXYGEN_IN "${CMAKE_SOURCE_DIR}/docs/Doxyfile.in")
    set(AXIOM_DOXYGEN_OUT "${CMAKE_BINARY_DIR}/Doxyfile")

    if(EXISTS "${AXIOM_DOXYGEN_IN}")
        configure_file("${AXIOM_DOXYGEN_IN}" "${AXIOM_DOXYGEN_OUT}" @ONLY)

        add_custom_target(docs
            COMMAND "${DOXYGEN_EXECUTABLE}" "${AXIOM_DOXYGEN_OUT}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            COMMENT "Generating API documentation in docs/api"
            VERBATIM
        )

        message(STATUS "Doxygen docs target enabled: 'docs' -> ${AXIOM_DOXYGEN_OUTPUT_DIR}")
    else()
        message(WARNING "AXIOM_ENABLE_DOXYGEN is ON but Doxyfile.in was not found at ${AXIOM_DOXYGEN_IN}")
    endif()

    if(NOT DOXYGEN_EXECUTABLE)
        message(WARNING "AXIOM_ENABLE_DOXYGEN is ON but Doxygen was not found")
    endif()
endif()

# ============================================================================
# AXIOM VULKAN EXPRESSWAY — offscreen GPU renderer shared library
# Requires: Vulkan SDK (find_package Vulkan), glslc shader compiler.
# Python bridge (gui/vulkan/vulkan_bridge.py) falls back to QOpenGLWidget
# when this library is absent.
# ============================================================================
option(AXIOM_ENABLE_VULKAN "Build Vulkan Expressway offscreen GPU renderer" ON)

if(AXIOM_ENABLE_VULKAN)
    find_package(Vulkan QUIET)

    if(Vulkan_FOUND)
        message(STATUS "🔥 Vulkan found: ${Vulkan_VERSION}")

        # Locate glslc shader compiler
        find_program(GLSLC_EXECUTABLE
            NAMES glslc
            HINTS
                ENV VULKAN_SDK PATH_SUFFIXES Bin bin
                "${Vulkan_INCLUDE_DIRS}/../Bin"
                "${Vulkan_INCLUDE_DIRS}/../bin"
        )

        if(NOT GLSLC_EXECUTABLE)
            message(STATUS "  glslc not found — skipping SPIR-V compilation")
            message(STATUS "  axiom_vulkan library will NOT be built until glslc is available")
        else()
            message(STATUS "  glslc: ${GLSLC_EXECUTABLE}")

            # Output directory for compiled SPIR-V
            set(AXIOM_SHADER_OUT "${CMAKE_BINARY_DIR}/shaders")
            file(MAKE_DIRECTORY "${AXIOM_SHADER_OUT}")

            # Compile GLSL → SPIR-V
            set(AXIOM_SPIRV_FILES "")
            foreach(_shader axiom3d.vert axiom3d.frag)
                set(_src "${CMAKE_SOURCE_DIR}/shaders/${_shader}")
                set(_spv "${AXIOM_SHADER_OUT}/${_shader}.spv")
                add_custom_command(
                    OUTPUT  "${_spv}"
                    COMMAND "${GLSLC_EXECUTABLE}" "${_src}" -o "${_spv}"
                    DEPENDS "${_src}"
                    COMMENT "glslc: compiling ${_shader} → ${_shader}.spv"
                    VERBATIM
                )
                list(APPEND AXIOM_SPIRV_FILES "${_spv}")
            endforeach()

            add_custom_target(axiom_shaders DEPENDS ${AXIOM_SPIRV_FILES})

            # Build shared library
            add_library(axiom_vulkan SHARED
                src/vulkan_expressway.cpp
                include/vulkan_expressway.h
            )
            add_dependencies(axiom_vulkan axiom_shaders)

            target_include_directories(axiom_vulkan PRIVATE
                "${CMAKE_SOURCE_DIR}/include"
                ${Vulkan_INCLUDE_DIRS}
            )
            target_compile_definitions(axiom_vulkan PRIVATE AXIOM_VULKAN_BUILD)
            target_link_libraries(axiom_vulkan PRIVATE ${Vulkan_LIBRARIES})

            if(WIN32)
                # Place DLL alongside the main executable
                set_target_properties(axiom_vulkan PROPERTIES
                    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
                )
            endif()

            axiom_enable_windows_signing(axiom_vulkan)
            message(STATUS "  ✓ axiom_vulkan shared library target enabled")
            message(STATUS "  ✓ SPIR-V output: ${AXIOM_SHADER_OUT}/")
        endif()
    else()
        message(STATUS "Vulkan SDK not found — axiom_vulkan GPU renderer skipped")
        message(STATUS "  Install Vulkan SDK from https://vulkan.lunarg.com/")
        message(STATUS "  Python falls back to QOpenGLWidget hardware renderer")
    endif()
else()
    message(STATUS "AXIOM_ENABLE_VULKAN=OFF — Vulkan Expressway disabled")
endif()