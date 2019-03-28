/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#ifdef _MSC_VER

#include <malloc.h>

inline void * aligned_alloc(
    size_t alignment,
    size_t size)
{
    return _aligned_malloc(size, alignment);
}

inline void aligned_free(void * ptr)
{
    _aligned_free(ptr);
}

#define restrict __restrict

#else

#include <cstdlib>

inline void * aligned_alloc(
    size_t alignment,
    size_t size)
{
    return std::aligned_alloc(alignment, size);
}

inline void aligned_free(void * ptr)
{
    std::free(ptr);
}

#define restrict __restrict

#endif

// Targeting AVX-512
static constexpr size_t VectorizationWordSize = 8; // Number of elements, not bytes

/*
 * Rounds a number of elements up to the next multiple of the
 * vectorization word size, to facilitate loops with vectorized code.
 */
inline size_t make_aligned_element_count(size_t element_count)
{
    return (element_count % VectorizationWordSize) == 0
        ? element_count
        : element_count + VectorizationWordSize - (element_count % VectorizationWordSize);
}
