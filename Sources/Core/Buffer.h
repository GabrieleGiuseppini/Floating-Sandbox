/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameMath.h"
#include "SysSpecifics.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>

/*
 * A fixed-size, mem-aligned buffer which cannot grow more than the size that it is initially
 * constructed with.
 *
 * The buffer is mem-aligned so that if TElement is float,
 * then the buffer is aligned to the vectorization number of floats.
 */
template <typename TElement>
class Buffer final
{
public:

    static constexpr size_t CalculateByteSize(size_t element_count)
    {
        return sizeof(TElement) * element_count;
    }

public:

    explicit Buffer(size_t size)
        : mBuffer(make_unique_buffer_aligned_to_vectorization_word<TElement>(size))
        , mSize(size)
        , mCurrentPopulatedSize(0)
    {
    }

    Buffer(
        size_t size,
        size_t fillStart,
        TElement fillValue)
        : Buffer(size)
    {
        assert(fillStart <= mSize);

        // Fill-in values
        std::fill(
            mBuffer.get() + fillStart,
            mBuffer.get() + mSize,
            fillValue);
    }

    Buffer(
        size_t size,
        size_t fillStart,
        std::function<TElement(size_t)> fillFunction)
        : Buffer(size)
    {
        assert(fillStart <= mSize);

        for (size_t i = fillStart; i < mSize; ++i)
            mBuffer[i] = fillFunction(i);
    }

    Buffer(
        size_t size,
        TElement fillValue)
        : Buffer(size)
    {
        // Fill-in values
        fill(fillValue);
    }

    Buffer(Buffer && other) noexcept
        : mBuffer(std::move(other.mBuffer))
        , mSize(other.mSize)
        , mCurrentPopulatedSize(other.mCurrentPopulatedSize)
    {
    }

    /*
     * Gets the size of the buffer, including the extra room allocated to make the buffer aligned;
     * greater than or equal the currently-populated size.
     */
    size_t GetSize() const
    {
        return mSize;
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
     * Gets the current number of bytes populated in the buffer via emplace_back();
     * less than or equal the declared buffer size.
     */
    size_t GetCurrentPopulatedByteSize() const
    {
        return mCurrentPopulatedSize * sizeof(TElement);
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
     * Appends undefined data for the specified amount of data, advances by that much,
     * and returns the pointer to the append position, which should be used right away.
     */
    inline TElement * receive(size_t size)
    {
        if (mCurrentPopulatedSize + size <= mSize)
        {
            // Advance
            size_t startIndex = mCurrentPopulatedSize;
            mCurrentPopulatedSize += size;
            return &(mBuffer[startIndex]);
        }
        else
        {
            throw std::runtime_error("The buffer does not have enough free space");
        }
    }

    /*
     * Fills the buffer with a value.
     */
    inline void fill(TElement value)
    {
        TElement * restrict const ptr = mBuffer.get();
        for (size_t i = 0; i < mSize; ++i)
            ptr[i] = value;

        mCurrentPopulatedSize = mSize;
    }

    /*
     * Fills the buffer with a value.
     * This overload is when the caller has the buffer size at compile time;
     * it's faster than the other overload.
     */
    template<size_t Size>
    inline void fill(TElement value)
    {
        assert(mSize == Size);

        TElement * restrict const ptr = mBuffer.get();
        for (size_t i = 0; i < Size; ++i)
            ptr[i] = value;

        mCurrentPopulatedSize = Size;
    }

    /*
     * Clears the buffer, by reducing its currently-populated
     * element count to zero, so that it is ready for being re-populated.
     */
    void clear()
    {
        mCurrentPopulatedSize = 0;
    }

    /*
     * Copies a buffer into this buffer.
     *
     * The sizes of the buffers must match.
     */
    void copy_from(Buffer<TElement> const & other)
    {
        assert(mSize == other.mSize);
        std::memcpy(mBuffer.get(), other.mBuffer.get(), mSize * sizeof(TElement));

        mCurrentPopulatedSize = other.mCurrentPopulatedSize;
    }

    inline void swap(Buffer & other) noexcept
    {
        std::swap(mBuffer, other.mBuffer);
        std::swap(mSize, other.mSize);
        std::swap(mCurrentPopulatedSize, other.mCurrentPopulatedSize);
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
        return mBuffer.get();
    }

    inline TElement * data()
    {
        return mBuffer.get();
    }

    unique_aligned_buffer<TElement> mBuffer;
    size_t mSize;
    size_t mCurrentPopulatedSize;
};
