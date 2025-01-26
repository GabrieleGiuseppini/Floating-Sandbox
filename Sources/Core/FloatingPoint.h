/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-03
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SysSpecifics.h"

#include <cfloat>
#include <limits>

#if FS_IS_ARCHITECTURE_ARM_32() || FS_IS_ARCHITECTURE_ARM_64()
#include <fenv.h>
#endif

inline void EnableFloatingPointExceptions()
{
    // Enable all floating point exceptions except INEXACT and UNDERFLOW

#if FS_IS_ARCHITECTURE_ARM_32() || FS_IS_ARCHITECTURE_ARM_64()
    fexcept_t excepts;
    fegetexceptflag(&excepts, FE_ALL_EXCEPT);
    fesetexceptflag(&excepts, FE_ALL_EXCEPT & ~(FE_INEXACT | FE_UNDERFLOW));
#elif FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
    _MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() & (_MM_MASK_INEXACT | _MM_MASK_UNDERFLOW));
#else
#pragma message ("WARNING: Unknown architecture - cannot set floating point exception mask")
#endif
}

inline void EnableFloatingPointFlushToZero()
{
#if FS_IS_ARCHITECTURE_ARM_32()
    uint32_t tmpFpscr;
    asm volatile("vmrs %0, fpscr" : "=r" (tmpFpscr));
    tmpFpscr = tmpFpscr | (1 << 24);
#if __has_builtin(__builtin_arm_set_fpscr)
    /* see https://gcc.gnu.org/ml/gcc-patches/2017-04/msg00443.html */
    __builtin_arm_set_fpscr(tmpFpscr);
#else
    asm volatile("vmsr fpscr, %0" : : "r" (tmpFpscr) : "vfpcc", "memory");
#endif
#elif FS_IS_ARCHITECTURE_ARM_64()
    uint64_t tmpFpcr;
    asm volatile("mrs %0, fpcr" : "=r" (tmpFpcr));
    tmpFpcr = tmpFpcr | (1 << 24);
    asm volatile("msr fpcr, %0" : : "ri" (tmpFpcr));
#elif FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#else
#pragma message ("WARNING: Unknown architecture - cannot set floating point flush-to-zero")
#endif
}

inline void DisableFloatingPointFlushToZero()
{
#if FS_IS_ARCHITECTURE_ARM_32()
    uint32_t tmpFpscr;
    asm volatile("vmrs %0, fpscr" : "=r" (tmpFpscr));
    tmpFpscr = tmpFpscr & (~(1 << 24));
#if __has_builtin(__builtin_arm_set_fpscr)
    /* see https://gcc.gnu.org/ml/gcc-patches/2017-04/msg00443.html */
    __builtin_arm_set_fpscr(tmpFpscr);
#else
    asm volatile("vmsr fpscr, %0" : : "r" (tmpFpscr) : "vfpcc", "memory");
#endif
#elif FS_IS_ARCHITECTURE_ARM_64()
    uint64_t tmpFpcr;
    asm volatile("mrs %0, fpcr" : "=r" (tmpFpcr));
    tmpFpcr = tmpFpcr & (~(1 << 24));
    asm volatile("msr fpcr, %0" : : "ri" (tmpFpcr));
#elif FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_OFF);
#else
#pragma message ("WARNING: Unknown architecture - cannot set floating point flush-to-zero")
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Shamelessly lifted from GTest.
//
// This is only used in tests in GUI applications. So I may be forgiven.
////////////////////////////////////////////////////////////////////////////////////////////////
class FloatingPoint
{
public:
    using Bits = unsigned int;

    // Constants.

    // # of bits in a number.
    static constexpr size_t kBitCount = 8 * sizeof(float);

    // # of fraction bits in a number.
    static constexpr size_t kFractionBitCount =
        std::numeric_limits<float>::digits - 1;

    // # of exponent bits in a number.
    static constexpr size_t kExponentBitCount = kBitCount - 1 - kFractionBitCount;

    // The mask for the sign bit.
    static constexpr Bits kSignBitMask = static_cast<Bits>(1) << (kBitCount - 1);

    // The mask for the fraction bits.
    static constexpr Bits kFractionBitMask =
        ~static_cast<Bits>(0) >> (kExponentBitCount + 1);

    // The mask for the exponent bits.
    static constexpr Bits kExponentBitMask = ~(kSignBitMask | kFractionBitMask);

    // How many ULP's (Units in the Last Place) we want to tolerate when
    // comparing two numbers.  The larger the value, the more error we
    // allow.  A 0 value means that two numbers must be exactly the same
    // to be considered equal.
    //
    // The maximum error of a single floating-point operation is 0.5
    // units in the last place.  On Intel CPU's, all floating-point
    // calculations are done with 80-bit precision, while double has 64
    // bits.  Therefore, 4 should be enough for ordinary use.
    //
    // See the following article for more details on ULP:
    // http://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
    static constexpr size_t kMaxUlps = 4;

    // Constructs a FloatingPoint from a raw floating-point number.
    //
    // On an Intel CPU, passing a non-normalized NAN (Not a Number)
    // around may change its bits, although the new value is guaranteed
    // to be also a NAN.  Therefore, don't expect this constructor to
    // preserve the bits in x when x is a NAN.
    explicit FloatingPoint(float const & x) { u_.value_ = x; }

    // Static methods

    // Reinterprets a bit pattern as a floating-point number.
    //
    // This function is needed to test the AlmostEquals() method.
    static float ReinterpretBits(const Bits bits)
    {
        FloatingPoint fp(0);
        fp.u_.bits_ = bits;
        return fp.u_.value_;
    }

    // Returns the floating-point number that represent positive infinity.
    static float Infinity()
    {
        return ReinterpretBits(kExponentBitMask);
    }

    // Returns the maximum representable finite floating-point number.
    static constexpr float Max()
    {
        return FLT_MAX;
    }

    // Non-static methods

    // Returns the bits that represents this number.
    const Bits & bits() const { return u_.bits_; }

    // Returns the exponent bits of this number.
    Bits exponent_bits() const { return kExponentBitMask & u_.bits_; }

    // Returns the fraction bits of this number.
    Bits fraction_bits() const { return kFractionBitMask & u_.bits_; }

    // Returns the sign bit of this number.
    Bits sign_bit() const { return kSignBitMask & u_.bits_; }

    // Returns true iff this is NAN (not a number).
    bool is_nan() const
    {
        // It's a NAN if the exponent bits are all ones and the fraction
        // bits are not entirely zeros.
        return (exponent_bits() == kExponentBitMask) && (fraction_bits() != 0);
    }

    // Returns true iff this number is at most kMaxUlps ULP's away from
    // rhs.  In particular, this function:
    //
    //   - returns false if either number is (or both are) NAN.
    //   - treats really large numbers as almost equal to infinity.
    //   - thinks +0.0 and -0.0 are 0 DLP's apart.
    bool AlmostEquals(FloatingPoint const & rhs) const
    {
        // The IEEE standard says that any comparison operation involving
        // a NAN must return false.
        if (is_nan() || rhs.is_nan()) return false;

        return DistanceBetweenSignAndMagnitudeNumbers(u_.bits_, rhs.u_.bits_)
            <= kMaxUlps;
    }

private:

    // The data type used to store the actual floating-point number.
    union FloatingPointUnion
    {
        float value_;  // The raw floating-point number.
        Bits bits_;      // The bits that represent the number.
    };

    // Converts an integer from the sign-and-magnitude representation to
    // the biased representation.  More precisely, let N be 2 to the
    // power of (kBitCount - 1), an integer x is represented by the
    // unsigned number x + N.
    //
    // For instance,
    //
    //   -N + 1 (the most negative number representable using
    //          sign-and-magnitude) is represented by 1;
    //   0      is represented by N; and
    //   N - 1  (the biggest number representable using
    //          sign-and-magnitude) is represented by 2N - 1.
    //
    // Read http://en.wikipedia.org/wiki/Signed_number_representations
    // for more details on signed number representations.
    static Bits SignAndMagnitudeToBiased(Bits const & sam)
    {
        if (kSignBitMask & sam)
        {
            // sam represents a negative number.
            return ~sam + 1;
        }
        else
        {
            // sam represents a positive number.
            return kSignBitMask | sam;
        }
    }

    // Given two numbers in the sign-and-magnitude representation,
    // returns the distance between them as an unsigned number.
    static Bits DistanceBetweenSignAndMagnitudeNumbers(
        Bits const & sam1,
        Bits const & sam2)
    {
        Bits const biased1 = SignAndMagnitudeToBiased(sam1);
        Bits const biased2 = SignAndMagnitudeToBiased(sam2);
        return (biased1 >= biased2) ? (biased1 - biased2) : (biased2 - biased1);
    }

    FloatingPointUnion u_;
};