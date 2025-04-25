/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cassert>
#include <cstdint>
#include <memory>

#ifdef _MSC_VER
#include <malloc.h>
#else
#include <cstdlib>
#endif

//
// Architecture and Width
//
// None defined means "no specific code required"
//

#define FS_IS_ARCHITECTURE_ARM_32() 0
#define FS_IS_ARCHITECTURE_ARM_64() 0
#define FS_IS_ARCHITECTURE_X86_32() 0
#define FS_IS_ARCHITECTURE_X86_64() 0

#define FS_IS_REGISTER_WIDTH_32() 0
#define FS_IS_REGISTER_WIDTH_64() 0

#if defined(__arm__) || defined(_ARM) || defined (_M_ARM) || defined(__arm)
#undef FS_IS_ARCHITECTURE_ARM_32
#define FS_IS_ARCHITECTURE_ARM_32() 1
#undef FS_IS_REGISTER_WIDTH_32
#define FS_IS_REGISTER_WIDTH_32() 1
#elif defined(	__aarch64__)
#undef FS_IS_ARCHITECTURE_ARM_64
#define FS_IS_ARCHITECTURE_ARM_64() 1
#undef FS_IS_REGISTER_WIDTH_64
#define FS_IS_REGISTER_WIDTH_64() 1
#elif defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined (_M_AMD64)
#undef FS_IS_ARCHITECTURE_X86_64
#define FS_IS_ARCHITECTURE_X86_64() 1
#undef FS_IS_REGISTER_WIDTH_64
#define FS_IS_REGISTER_WIDTH_64() 1
#elif defined(_M_IX86) || defined(i386) || defined(__i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)
#undef FS_IS_ARCHITECTURE_X86_32
#define FS_IS_ARCHITECTURE_X86_32() 1
#undef FS_IS_REGISTER_WIDTH_32
#define FS_IS_REGISTER_WIDTH_32() 1
#endif

//
// OS
//
// None defined means "no specific code required"
//

#define FS_IS_OS_LINUX() 0
#define FS_IS_OS_MACOS() 0
#define FS_IS_OS_WINDOWS() 0
#define FS_IS_OS_ANDROID() 0

#if defined(macintosh) || defined(Macintosh) || (defined(__APPLE__) && defined(__MACH__))
#undef FS_IS_OS_MACOS
#define FS_IS_OS_MACOS() 1
#elif defined(__ANDROID__)
// Note: also __linux__ is defined, hence here
#undef FS_IS_OS_ANDROID
#define FS_IS_OS_ANDROID() 1
#elif defined(__linux__) || defined(linux) || defined(__linux)
#undef FS_IS_OS_LINUX
#define FS_IS_OS_LINUX() 1
#elif defined(_WIN32) || defined(_WIN64) || defined(__WIN32__)
#undef FS_IS_OS_WINDOWS
#define FS_IS_OS_WINDOWS() 1
#endif

//
// Platform
//

#define FS_IS_PLATFORM_PC() 0
#define FS_IS_PLATFORM_MOBILE() 0

#if defined(__ANDROID__)
#undef FS_IS_PLATFORM_MOBILE
#define FS_IS_PLATFORM_MOBILE() 1
#else
#undef FS_IS_PLATFORM_PC
#define FS_IS_PLATFORM_PC() 1
#endif

//
// ARM NEON
//

// FS_IS_ARM_NEON --implies--> FS_IS_ARCHITECTURE_ARM

#define FS_IS_ARM_NEON() 0

#if FS_IS_ARCHITECTURE_ARM_32() || FS_IS_ARCHITECTURE_ARM_64()
// For now we assume NEON if we are on ARM
#if defined(__ARM_NEON)
#undef FS_IS_ARM_NEON
#define FS_IS_ARM_NEON() 1
#endif
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////

using register_int_32 = std::int32_t;
using register_int_64 = std::int64_t;

#if FS_IS_REGISTER_WIDTH_32()
using register_int = register_int_32;
#elif FS_IS_REGISTER_WIDTH_64()
using register_int = register_int_64;
#endif

#define restrict __restrict

template<typename T>
inline constexpr T ceil_power_of_two(T value)
{
    if (value <= 0)
        return 1;

    // Check if immediately a power of 2
    if (!(value & (value - 1)))
        return value;

    T result = 2;
    while (value >>= 1) result <<= 1;
    return result;
}

template<typename T>
inline constexpr T ceil_square_power_of_two(T value)
{
    // Special cases
    if (value < 2)
        return value;

    T e;
    --value;
    for (e = 0; value > 1; ++e, value >>= 1);
    if (value > 0)
        ++e;

    if ((e % 2) != 0)
        ++e;

    return T(1) << e;
}

////////////////////////////////////////////////////////////////////////////////////////
// Intrinsics
////////////////////////////////////////////////////////////////////////////////////////

#if FS_IS_ARCHITECTURE_X86_64() || FS_IS_ARCHITECTURE_X86_32()
////<mmintrin.h>  MMX
////<xmmintrin.h> SSE
////<emmintrin.h> SSE2
////<pmmintrin.h> SSE3
////<tmmintrin.h> SSSE3
////<smmintrin.h> SSE4.1
////<nmmintrin.h> SSE4.2
////<ammintrin.h> SSE4A
////<wmmintrin.h> AES
////<immintrin.h> AVX, AVX2, FMA
/*
// MSVC
#include <intrin.h>

// GCC/Clang
#include <x86intrin.h>

// MAC
#include <pmmintrin.h>
*/
#include <pmmintrin.h>
#elif (FS_IS_ARCHITECTURE_ARM_32() || FS_IS_ARCHITECTURE_ARM_64()) && FS_IS_ARM_NEON()
#include <arm_neon.h>
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Alignment
////////////////////////////////////////////////////////////////////////////////////////

// The number of floats we want to be able to compute in a single vectorization step.
// Dictates alignment of buffers.
// Targeting SSE, NEON

// A.k.a. the vectorization word size
#if !FS_IS_ARM_NEON()
template <typename T>
static constexpr T vectorization_float_count = 4;
#else
template <typename T>
static constexpr T vectorization_float_count = 4 * 4; // We want to use the 4x4 load/stores
#endif

template <typename T>
static constexpr T vectorization_byte_count = vectorization_float_count<T> * sizeof(float);

#ifdef _MSC_VER
# define FS_ALIGN16_BEG __declspec(align(vectorization_byte_count<int>))
# define FS_ALIGN16_END
#else
# define FS_ALIGN16_BEG
# define FS_ALIGN16_END __attribute__((aligned(vectorization_byte_count<int>)))
#endif

/*
 * Checks whether the specified pointer is aligned to the vectorization
 * float count.
 */
inline bool is_aligned_to_vectorization_word(void const * ptr) noexcept
{
    auto const ui_ptr = reinterpret_cast<std::uintptr_t>(ptr);
    return !(ui_ptr % vectorization_byte_count<std::uintptr_t>);
}

/*
 * Rounds a number of elements up to the next multiple of
 * the vectorization float count.
 * It basically calculates the ideal size of a buffer so that
 * when the element is float, that buffer may be processed
 * efficiently with vectorized instructions that process whole
 * vectorization words.
 * If the element is a multiple of float (e.g. vec2f), the
 * ideal size of the buffer would still be a multiple of the
 * vectorization word:
 *    result*sizeof(f) % word_byte_size == 0 --> result*n*sizeof(f) % word_byte_size == 0
 */
template<typename T>
inline constexpr T make_aligned_float_element_count(T element_count) noexcept
{
    return (element_count % vectorization_float_count<T>) == 0
        ? element_count
        : element_count + vectorization_float_count<T> - (element_count % vectorization_float_count<T>);
}

/*
 * Checks whether the specified number of float elements is aligned
 * with the vectorization float count.
 */
template<typename T>
inline constexpr bool is_aligned_to_float_element_count(T element_count) noexcept
{
    return element_count == make_aligned_float_element_count(element_count);
}

/*
 * Pre-cooked align-as for our vectorization size.
 */

#define aligned_to_vword alignas(vectorization_byte_count<size_t>)

 /*
  * Allocates a buffer of bytes aligned to the vectorization float
  * byte count, without making use of the standard library - Android doesn't
  * have aligned_malloc, unless the SDK is at least 28.
  * *
  * Separate here for unit tests.
  */
inline void * _poor_mans_alloc_aligned_to_vectorization_word(size_t byte_size)
{
    // We store the original pointer in the word immediately preceding the calculated beginning
    // of the buffer

    //
    // 1. Align size
    //

    size_t const enhanced_byte_size = byte_size + sizeof(uintptr_t);

    // Calculate extra bytes needed for size alignment
    size_t extra_bytes_for_size_alignment = 0;
    if ((enhanced_byte_size % vectorization_byte_count<size_t>) != 0)
    {
        extra_bytes_for_size_alignment = vectorization_byte_count<size_t> - (enhanced_byte_size % vectorization_byte_count<size_t>);
    }

    size_t const aligned_size = enhanced_byte_size + extra_bytes_for_size_alignment;

    //
    // 2. Align pointer

    size_t const total_to_alloc = aligned_size + vectorization_byte_count<size_t> - 1; // Worst case scenario, enough fluff to allow for alignment
    void * const ptr = malloc(total_to_alloc);
    assert(ptr != nullptr);

    // Calculate extra bytes needed for return pointer alignment
    uintptr_t extra_bytes_for_alignment = 0;
    uintptr_t const ptr_integral = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t const ptr_integral_including_free_pointer = ptr_integral + static_cast<uintptr_t>(sizeof(uintptr_t));
    if ((ptr_integral_including_free_pointer % vectorization_byte_count<uintptr_t>) != 0)
    {
        extra_bytes_for_alignment += vectorization_byte_count<uintptr_t> - (ptr_integral_including_free_pointer % vectorization_byte_count<uintptr_t>);
    }

    // Store free pointer
    std::uint8_t * const ptr_aligned_before_free_pointer = reinterpret_cast<std::uint8_t *>(ptr) + extra_bytes_for_alignment;
    *(reinterpret_cast<uintptr_t *>(ptr_aligned_before_free_pointer)) = reinterpret_cast<intptr_t>(ptr);

    // Skip free pointer
    void * const return_ptr = reinterpret_cast<void *>(ptr_aligned_before_free_pointer + sizeof(uintptr_t));

    assert(reinterpret_cast<uintptr_t>(return_ptr) + byte_size <= reinterpret_cast<uintptr_t>(ptr) + total_to_alloc);
    assert(is_aligned_to_vectorization_word(return_ptr));

    return return_ptr;
}

/*
 * free() for above manually-aligned pointer.
 */
inline void _poor_mans_free_aligned(void * ptr)
{
    // We store the original pointer in the word immediately preceding the calculated beginning
    // of the buffer

    uintptr_t const orig_ptr = *(reinterpret_cast<uintptr_t *>(ptr) - 1);
    std::free(reinterpret_cast<void *>(orig_ptr));
}

/*
 * Allocates a buffer of bytes aligned to the vectorization float
 * byte count.
 */
inline void * alloc_aligned_to_vectorization_word(size_t byte_size)
{
    // The vectorization byte count is itself a power of two
    static_assert(vectorization_byte_count<size_t> == ceil_power_of_two(vectorization_byte_count<size_t>));

#if FS_IS_OS_ANDROID()

    return _poor_mans_alloc_aligned_to_vectorization_word(byte_size);

#else

    // Calculate byte size required to satisfy the aligned_alloc constraints
    size_t const aligned_byte_size = (byte_size % vectorization_byte_count<size_t>) == 0
        ? byte_size
        : byte_size + vectorization_byte_count<size_t> - (byte_size % vectorization_byte_count<size_t>);

    void * ptr;
#ifdef _MSC_VER
    ptr = _aligned_malloc(aligned_byte_size, vectorization_byte_count<size_t>);
#else
    ptr = aligned_alloc(vectorization_byte_count<size_t>, aligned_byte_size);
#endif

    assert(is_aligned_to_vectorization_word(ptr));

    return ptr;

#endif
}

inline void free_aligned(void * ptr)
{
    assert(is_aligned_to_vectorization_word(ptr));

#if FS_IS_OS_ANDROID()
    _poor_mans_free_aligned(ptr);
#else
#ifdef _MSC_VER
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
#endif
}

template<typename TElement>
struct aligned_buffer_deleter
{
    void operator()(TElement * ptr)
    {
        free_aligned(reinterpret_cast<void *>(ptr));
    }
};

template<typename TElement>
using unique_aligned_buffer = std::unique_ptr<TElement[], aligned_buffer_deleter<TElement>>;

template<typename TElement>
inline unique_aligned_buffer<TElement> make_unique_buffer_aligned_to_vectorization_word(size_t elementCount)
{
    return unique_aligned_buffer<TElement>(
        reinterpret_cast<TElement *>(alloc_aligned_to_vectorization_word(elementCount * sizeof(TElement))),
        aligned_buffer_deleter<TElement>());
}
