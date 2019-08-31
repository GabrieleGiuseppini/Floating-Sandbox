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


////////////////////////////////////////////////////////////////////////////////////////
// Alignment
////////////////////////////////////////////////////////////////////////////////////////

// The number of floats we want to be able to compute in a single vectorization step.
// Dictates alignment of buffers.
// Targeting AVX-256 (though at the moment we might be compiling for a lower fp arch)

template <typename T>
static constexpr T vectorization_float_count = 8; // A.k.a. the vectorization word size

template <typename T>
static constexpr T vectorization_byte_count = vectorization_float_count<T> * sizeof(float);

/*
 * Checks whether the specified pointer is aligned to the vectorization
 * float count.
 */
inline bool is_aligned_to_vectorization_word(void const * ptr) noexcept
{
    auto ui_ptr = reinterpret_cast<std::uintptr_t>(ptr);
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
 * Allocates a buffer of bytes aligned to the vectorization float
 * byte count.
 */
inline void * alloc_aligned_to_vectorization_word(size_t byte_size)
{
    static_assert(vectorization_byte_count<size_t> == ceil_power_of_two(vectorization_byte_count<size_t>));

    // Calculate byte size required to satisfy the aligned_alloc constraints
    auto aligned_byte_size = (byte_size % vectorization_byte_count<size_t>) == 0
        ? byte_size
        : byte_size + vectorization_byte_count<size_t> - (byte_size % vectorization_byte_count<size_t>);

#ifdef _MSC_VER
    return _aligned_malloc(aligned_byte_size, vectorization_byte_count<size_t>);
#else
    return std::aligned_alloc(vectorization_byte_count<size_t>, aligned_byte_size);
#endif
}

inline void free_aligned(void * ptr)
{
    assert(is_aligned_to_vectorization_word(ptr));

#ifdef _MSC_VER
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

struct aligned_buffer_deleter
{
    void operator()(std::uint8_t * ptr)
    {
        free_aligned(reinterpret_cast<void *>(ptr));
    }
};

using unique_aligned_buffer = std::unique_ptr<std::uint8_t, aligned_buffer_deleter>;

inline unique_aligned_buffer make_unique_buffer_aligned_to_vectorization_word(size_t byte_size)
{
    return unique_aligned_buffer(
        reinterpret_cast<uint8_t *>(alloc_aligned_to_vectorization_word(byte_size)),
        aligned_buffer_deleter());
}

using shared_aligned_buffer = std::shared_ptr<std::uint8_t>;

inline shared_aligned_buffer make_shared_buffer_aligned_to_vectorization_word(size_t byte_size)
{
    return shared_aligned_buffer(
        reinterpret_cast<uint8_t *>(alloc_aligned_to_vectorization_word(byte_size)),
        aligned_buffer_deleter());
}



