/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-03-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Log.h"

#include <cassert>
#include <cstdlib>
#include <memory>

/*
 * This class is a vector whose size is specified at runtime, and which cannot grow
 * more than that specified size.
 *
 * The container is optimized for fast *pushes* of POD types.
 */
template<typename TElement>
class BoundedVector
{
public:

    BoundedVector()
        : mBuffer(nullptr, &std::free)
        , mAllocatedSize(0u)
        , mSize(0u)
    {
    }

    BoundedVector(size_t maxSize)
        : mBuffer(static_cast<TElement *>(std::malloc(sizeof(TElement) * maxSize)), &std::free)
        , mAllocatedSize(maxSize)
        , mSize(0u)
    {
    }

    inline size_t size() const noexcept
    {
        return mSize;
    }

    inline size_t max_size() const noexcept
    {
        return mAllocatedSize;
    }

    inline TElement const * data() const noexcept
    {
        return mBuffer.get();
    }

    inline TElement const & operator[](size_t index) const noexcept
    {
        assert(index < mSize);
        return mBuffer[index];
    }

    inline TElement const & back() const noexcept
    {
        assert(mSize > 0);
        return mBuffer[mSize - 1];
    }

    inline bool empty() const noexcept
    {
        return mSize == 0u;
    }

    inline void clear() noexcept
    {
        mSize = 0u;
    }

    inline void reset(size_t maxSize)
    {
        mBuffer.reset(static_cast<TElement *>(std::malloc(sizeof(TElement) * maxSize)));
        mAllocatedSize = maxSize;
        mSize = 0u;
    }

    template<typename... TArgs>
    inline TElement & emplace_back(TArgs&&... args)
    {
        assert(mSize < mAllocatedSize);
        return *new(&(mBuffer[mSize++])) TElement(std::forward<TArgs>(args)...);
    }

private:

    std::unique_ptr<TElement[], decltype(&std::free)> mBuffer;
    size_t mAllocatedSize;
    size_t mSize;
};
