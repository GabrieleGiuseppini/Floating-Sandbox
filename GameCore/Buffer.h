/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameMath.h"
#include "SysSpecifics.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

/*
* This class implements a simple buffer of "things". The buffer is fixed-size and cannot
* grow more than the size that it is initially constructed with.
*
* The buffer is mem-aligned and takes care of deallocating itself at destruction time.
* The number of elements is assumed to be rounded to a multiple of the word size.
*/
template <typename TElement>
class Buffer
{
public:

    Buffer(size_t size)
        : mSize(size)
        , mCurrentPopulatedSize(0)
    {
        assert(make_aligned_element_count(size) == size);

        mBuffer = static_cast<TElement *>(aligned_alloc(CeilPowerOfTwo(sizeof(TElement)), size * sizeof(TElement)));
        assert(nullptr != mBuffer);
    }

    Buffer(
        size_t size,
        size_t fillStart,
        TElement fillValue)
        : Buffer(size)
    {
        assert(fillStart <= mSize);

        // Fill-in values
        for (size_t i = fillStart; i < mSize; ++i)
            mBuffer[i] = fillValue;
    }

    Buffer(Buffer && other)
        : mBuffer(other.mBuffer)
        , mSize(other.mSize)
        , mCurrentPopulatedSize(other.mCurrentPopulatedSize)
    {
        other.mBuffer = nullptr;
    }

    ~Buffer()
    {
        if (nullptr != mBuffer)
        {
            aligned_free(reinterpret_cast<void *>(mBuffer));
        }
    }

    /*
     * Gets the current number of elements populated in the buffer via emplace_back();
     * less than or equal the declared buffer size.
     */
    size_t GetCurrentPopulatedSize() const
    {
        return mCurrentPopulatedSize;
    }

    /*
     * Adds an element to the buffer. Assumed to be invoked only at initialization time.
     *
     * Cannot add more elements than the size specified at constructor time.
     */
    template <typename... Args>
    TElement & emplace_back(Args&&... args)
    {
        if (mCurrentPopulatedSize < mSize)
        {
            return *new(&(mBuffer[mCurrentPopulatedSize++])) TElement(std::forward<Args>(args)...);
        }
        else
        {
            throw std::runtime_error("The buffer is already full");
        }
    }

    /*
     * Fills the buffer with a value.
     */
    void fill(TElement value)
    {
        for (size_t i = 0; i < mSize; ++i)
            mBuffer[i] = value;
    }

    /*
     * Copies a buffer into this buffer.
     *
     * The sizes of the buffers must match.
     */
    void copy_from(Buffer<TElement> const & other)
    {
        assert(mSize == other.mSize);

        std::memcpy(mBuffer, other.mBuffer, mSize * sizeof(TElement));
    }

    /*
     * Gets an element.
     */

    inline TElement const & operator[](size_t index) const noexcept
    {
#ifndef NDEBUG
        // Ugly trick to allow setting breakpoints on assert failures
        if (index >= mSize)
        {
            assert(index < mSize);
        }
#endif

        return mBuffer[index];
    }

    inline TElement & operator[](size_t index) noexcept
    {
        assert(index < mSize);

        return mBuffer[index];
    }

    /*
     * Gets the buffer.
     */

    inline TElement const * restrict data() const
    {
        return mBuffer;
    }

    inline TElement * data()
    {
        return mBuffer;
    }

private:

    TElement * restrict mBuffer;
    size_t const mSize;
    size_t mCurrentPopulatedSize;
};