/**
 * @file mantis_heuristic.cpp
 * @brief Mantis Heuristic — non-inline utilities and profile loading
 */

#include "mantis_heuristic.h"
#include <cstring>

namespace AXIOM {
namespace Mantis {

// Nothing to put here currently — all hot-path methods are force-inlined
// in the header. This translation unit exists for:
//   1. Future non-inline utility functions
//   2. Ensuring the header compiles as a standalone TU
//   3. Profile loading helpers that don't need inlining

void load_profile_f32(TargetProfileF32& profile,
                      const float* weights,
                      size_t count) noexcept
{
    const size_t n = (count < kFeatureDimFP32) ? count : kFeatureDimFP32;
    std::memcpy(profile.weights, weights, n * sizeof(float));
    // Zero-fill remainder if input is shorter than dimension
    if (n < kFeatureDimFP32) {
        std::memset(profile.weights + n, 0,
                    (kFeatureDimFP32 - n) * sizeof(float));
    }
}

void load_profile_i8(TargetProfileI8& profile,
                     const int8_t* weights,
                     size_t count) noexcept
{
    const size_t n = (count < kFeatureDimINT8) ? count : kFeatureDimINT8;
    std::memcpy(profile.weights, weights, n * sizeof(int8_t));
    if (n < kFeatureDimINT8) {
        std::memset(profile.weights + n, 0,
                    (kFeatureDimINT8 - n) * sizeof(int8_t));
    }
}

} // namespace Mantis
} // namespace AXIOM
