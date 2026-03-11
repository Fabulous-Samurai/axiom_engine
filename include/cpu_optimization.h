/**
 * @file cpu_optimization.h
 * @brief CPU-specific optimizations and performance tuning
 */

#pragma once

#ifndef CPU_OPTIMIZATION_H
#define CPU_OPTIMIZATION_H

#include <string>
#include <memory>

#if defined(_MSC_VER)
    #include <intrin.h>
    #define AXIOM_FORCE_INLINE __forceinline
    #define AXIOM_NO_INLINE __declspec(noinline)
    #define AXIOM_HOT
    #define AXIOM_YIELD_PROCESSOR _mm_pause()
#elif defined(__GNUC__) || defined(__clang__)
    #define AXIOM_FORCE_INLINE inline __attribute__((always_inline))
    #define AXIOM_NO_INLINE __attribute__((noinline))
    #define AXIOM_HOT __attribute__((hot))
    #if defined(__i386__) || defined(__x86_64__)
        #define AXIOM_YIELD_PROCESSOR __builtin_ia32_pause()
    #elif defined(__aarch64__)
        #define AXIOM_YIELD_PROCESSOR __asm__ volatile("yield" ::: "memory")
    #else
        #define AXIOM_YIELD_PROCESSOR
    #endif
#else
    #define AXIOM_FORCE_INLINE inline
    #define AXIOM_NO_INLINE
    #define AXIOM_HOT
    #define AXIOM_YIELD_PROCESSOR
#endif

namespace AXIOM {

// ============================================================================
// Alignment for AVX 256-bit vectors (32 bytes)
// ============================================================================
#define AXIOM_ALIGN_AVX alignas(32)

// ============================================================================
// Function target attribute for AVX2+FMA multi-versioning
// ============================================================================
#if defined(__GNUC__) || defined(__clang__)
    #define AXIOM_TARGET_AVX2_FMA __attribute__((target("avx2,fma")))
#else
    #define AXIOM_TARGET_AVX2_FMA
#endif

// ============================================================================
// Compile-time CPU Feature Guard
// Reads CMake-propagated defines (AXIOM_SIMD_*_ENABLED) to determine
// which instruction sets are available at build time.
// ============================================================================
struct CpuFeatureGuard {
    static constexpr bool has_avx2 =
#ifdef AXIOM_SIMD_AVX2_ENABLED
        true;
#else
        false;
#endif

    static constexpr bool has_fma =
#ifdef AXIOM_SIMD_FMA_ENABLED
        true;
#else
        false;
#endif

    static constexpr bool has_avx_vnni =
#ifdef AXIOM_SIMD_AVX_VNNI_ENABLED
        true;
#else
        false;
#endif

    static constexpr bool has_avx512 =
#ifdef AXIOM_SIMD_AVX512_ENABLED
        true;
#else
        false;
#endif

    /// True when both AVX2 and FMA are available (FMA3 dot-product kernel)
    static constexpr bool can_fma3_dot = has_avx2 && has_fma;

    /// True when VNNI is available (INT8 dot-product kernel)
    static constexpr bool can_vnni_dot = has_avx_vnni;
};

/**
 * @brief CPU optimization utilities
 */
class CPUOptimization {
public:
    static void Initialize();
    static std::string GetCPUInfo();
    static void OptimizeForCurrentCPU();
    
private:
    static bool DetectSSE();
    static bool DetectAVX();
    static bool DetectAVX2();
};

} // namespace AXIOM

#endif // CPU_OPTIMIZATION_H