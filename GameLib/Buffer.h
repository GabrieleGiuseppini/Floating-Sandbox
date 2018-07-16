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
#include <stdexcept>

/*
* This class implements a simple buffer of "things". The buffer is fixed-size and cannot
* grow more than the size that it is initially constructed with.
*
* The buffer is mem-aligned and takes care of deallocating itself at destruction time.
*/
template <typename TElement>
class Buffer
{
public:

    Buffer(size_t size)
        : mSize(size)
        , mCurrentSize(0)
    {
        mBuffer = static_cast<TElement *>(aligned_alloc(CeilPowerOfTwo(sizeof(TElement)), size * sizeof(TElement)));
        assert(nullptr != mBuffer);
    }

    Buffer(Buffer && other)
        : mBuffer(other.mBuffer)
        , mSize(other.mSize)
        , mCurrentSize(other.mCurrentSize)
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
     * Gets the current number of elements in the buffer; less than or equal the declared buffer size.
     */

    size_t GetCurrentSize() const
    {
        return mCurrentSize;
    }

    /*
     * Adds an element to the buffer. Assumed to be invoked only at initialization time.
     *
     * Cannot add more elements than the size specified at constructor time.
     */
    template <typename... Args>
    TElement & emplace_back(Args&&... args)
    {
        if (mCurrentSize < mSize)
        {
            return *new(&(mBuffer[mCurrentSize++])) TElement(std::forward<Args>(args)...);
        }
        else
        {
            throw std::runtime_error("The repository is already full");
        }
    }

    /*
     * Gets an element.
     */

    inline TElement const & operator[](size_t index) const noexcept
    {
#ifndef NDEBUG
        // Ugly trick to allows setting breakpoints
        if (index >= mCurrentSize)
        {
            assert(index < mCurrentSize);
        }
#endif

        return mBuffer[index];
    }

    inline TElement & operator[](size_t index) noexcept
    {
        assert(index < mCurrentSize);

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
    size_t mCurrentSize;
};