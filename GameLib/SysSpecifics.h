/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-05-07
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#ifdef _MSC_VER

// At the time of writing, VS 2017 shipped with std::filesystem being still experimental
#include <filesystem>
namespace std {
    namespace filesystem = experimental::filesystem;
}


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
