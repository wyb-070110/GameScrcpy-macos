#include "gs/vision/Simd.h"

#include "Kernels.h"

#if defined(_M_X64) || defined(__x86_64__)
#define GS_VISION_X86 1
#include <immintrin.h>
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif

namespace gs::vision {

namespace {

#if defined(GS_VISION_X86) && defined(_MSC_VER)
constexpr int kOsSavesState = 1 << 27;
constexpr int kAvx = 1 << 28;
constexpr int kAvx2 = 1 << 5;
constexpr unsigned long long kVectorState = 0x6;
#endif

#if defined(GS_VISION_X86)
bool detectAvx2() noexcept
{
#if defined(_MSC_VER)
    int registers[4] = {};
    __cpuid(registers, 0);
    if (registers[0] < 7) {
        return false;
    }
    __cpuid(registers, 1);
    if ((registers[2] & kOsSavesState) == 0 || (registers[2] & kAvx) == 0) {
        return false;
    }
    if ((_xgetbv(0) & kVectorState) != kVectorState) {
        return false;
    }
    __cpuidex(registers, 7, 0);
    return (registers[1] & kAvx2) != 0;
#else
    // GCC/Clang: the builtin checks CPUID and the OS-saved AVX state.
    return __builtin_cpu_supports("avx2");
#endif
}
#else
bool detectAvx2() noexcept { return false; }
#endif

}

bool avx2Available() noexcept
{
    static const bool available = detectAvx2();
    return available;
}

Simd resolvedSimd(Simd requested) noexcept
{
    return requested != Simd::Scalar && avx2Available() ? Simd::Avx2 : Simd::Scalar;
}

namespace detail {

const Kernels& kernels(Simd simd) noexcept
{
    static constexpr Kernels kScalarKernels{&scalar::halve, &scalar::correlate, &scalar::accumulateColumns};
#if defined(GS_VISION_X86)
    static constexpr Kernels kAvx2Kernels{&avx2::halve, &avx2::correlate, &avx2::accumulateColumns};
    return resolvedSimd(simd) == Simd::Avx2 ? kAvx2Kernels : kScalarKernels;
#else
    (void)simd;
    return kScalarKernels;
#endif
}

}

}
