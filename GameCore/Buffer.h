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
#include <functional>
#include <memory>

/*
 * This class is the base of a hierarchy implementing a simple buffer of "things".
 * The buffer is fixed-size and cannot grow more than the size that it is initially
 * constructed with.
 *
 * The buffer is assumed to be mem-aligned so that if TElement is float,
 * then the buffer is aligned to the vectorization number of floats.
 */
template <typename TElement>
class BaseBuffer
{
public:

    static constexpr size_t CalculateByteSize(size_t element_count) noexcept
    {
        return sizeof(TElement) * element_count;
    }

public:

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
    void copy_from(BaseBuffer<TElement> const & other)
    {
        assert(mSize == other.mSize);
        std::memcpy(mBuffer, other.mBuffer, mSize * sizeof(TElement));

        mCurrentPopulatedSize = other.mCurrentPopulatedSize;
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

protected:

    BaseBuffer(
        TElement * buffer,
        size_t size)
        : mBuffer(buffer)
        , mSize(size)
        , mCurrentPopulatedSize(0)
    {
        assert(nullptr != mBuffer);
        assert(is_aligned_to_vectorization_word(buffer));
    }

    BaseBuffer(
        TElement * buffer,
        size_t size,
        size_t fillStart,
        TElement fillValue)
        : BaseBuffer(
            buffer,
            size)
    {
        assert(fillStart <= mSize);

        // Fill-in values
        std::fill(
            mBuffer + fillStart,
            mBuffer + mSize,
            fillValue);
    }

    BaseBuffer(BaseBuffer && other) noexcept
        : mBuffer(other.mBuffer)
        , mSize(other.mSize)
        , mCurrentPopulatedSize(other.mCurrentPopulatedSize)
    {
        other.mBuffer = nullptr; // Just to faciliate debugging
    }

    inline void swap(BaseBuffer & other) noexcept
    {
        std::swap(mBuffer, other.mBuffer);
        std::swap(mSize, other.mSize);
        std::swap(mCurrentPopulatedSize, other.mCurrentPopulatedSize);
    }

    TElement * restrict mBuffer;
    size_t mSize;
    size_t mCurrentPopulatedSize;
};

/*
 * A buffer that owns its memory buffer.
 *
 * The buffer is mem-aligned so that if TElement is float, then the buffer
 * is aligned to the vectorization number of floats.
 */
template <typename TElement>
class Buffer : public BaseBuffer<TElement>
{
public:

    Buffer(size_t size)
        : Buffer(
            make_unique_buffer_aligned_to_vectorization_word<TElement>(size),
            size)
    {
    }

    Buffer(
        size_t size,
        size_t fillStart,
        TElement fillValue)
        : Buffer(
            make_unique_buffer_aligned_to_vectorization_word<TElement>(size),
            size,
            fillStart,
            fillValue)
    {
    }

    Buffer(Buffer && other) noexcept
        : BaseBuffer<TElement>(std::move(other))
        , mAllocatedBuffer(std::move(other.mAllocatedBuffer))
    {
    }

    inline void swap(Buffer & other) noexcept
    {
        BaseBuffer<TElement>::swap(other);
        std::swap(mAllocatedBuffer, other.mAllocatedBuffer);
    }

private:

    Buffer(
        unique_aligned_buffer<TElement> allocatedBuffer,
        size_t size)
    : BaseBuffer<TElement>(
        reinterpret_cast<TElement *>(allocatedBuffer.get()),
        size)
    , mAllocatedBuffer(std::move(allocatedBuffer))
    {
    }

    Buffer(
        unique_aligned_buffer<TElement> allocatedBuffer,
        size_t size,
        size_t fillStart,
        TElement fillValue)
        : BaseBuffer<TElement>(
            reinterpret_cast<TElement *>(allocatedBuffer.get()),
            size,
            fillStart,
            fillValue)
        , mAllocatedBuffer(std::move(allocatedBuffer))
    {
    }

    // The buffer owned by us
    unique_aligned_buffer<TElement> mAllocatedBuffer;
};

/*
 * A buffer that sees a segment of another buffer, with external ownership.
 *
 * The buffer's segment is assumed to be mem-aligned so that if TElement is float,
 * then the buffer is aligned to the vectorization number of floats.
 */
template <typename TElement>
class BufferSegment : public BaseBuffer<TElement>
{
public:

    BufferSegment(
        shared_aligned_buffer<std::uint8_t> allocatedBuffer,
        size_t startByteCount,
        size_t size)
        : BaseBuffer<TElement>(
            reinterpret_cast<TElement *>(allocatedBuffer.get() + startByteCount),
            size)
        , mAllocatedBuffer(std::move(allocatedBuffer))
    {
    }

    BufferSegment(
        shared_aligned_buffer<std::uint8_t> allocatedBuffer,
        size_t startByteCount,
        size_t size,
        size_t fillStart,
        TElement fillValue)
        : BaseBuffer<TElement>(
            reinterpret_cast<TElement *>(allocatedBuffer.get() + startByteCount),
            size,
            fillStart,
            fillValue)
        , mAllocatedBuffer(std::move(allocatedBuffer))
    {
    }

    BufferSegment(BufferSegment && other) noexcept
        : BaseBuffer<TElement>(std::move(other))
        , mAllocatedBuffer(std::move(other.mAllocatedBuffer))
    {
    }

private:

    // The buffer not owned by us
    shared_aligned_buffer<std::uint8_t> mAllocatedBuffer;
};