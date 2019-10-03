/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-10-03
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>

/*
 * This class is exactly like a unique_ptr<TValue[]>, but it also
 * carries the size ofthe buffer.
 */
template<typename TValue>
class unique_buffer
{
public:

    unique_buffer(size_t size)
        : mBuffer(new TValue[size])
        , mSize(size)
    {
    }

    unique_buffer(
        TValue * ptr,
        size_t size)
        : mBuffer(ptr)
        , mSize(size)
    {}

    unique_buffer(unique_buffer const & other)
    {
        *this = other;
    }

    unique_buffer(unique_buffer && other)
    {
        mBuffer = std::move(other.mBuffer);
        mSize = other.mSize;
        other.mSize = 0;
    }

    unique_buffer & operator=(unique_buffer const & other)
    {
        mBuffer = std::make_unique<TValue[]>(other.mSize);
        std::memcpy(mBuffer.get(), other.mBuffer.get(), other.mSize * sizeof(TValue));
        mSize = other.mSize;

        return *this;
    }

    unique_buffer & operator=(unique_buffer && other)
    {
        mBuffer = std::move(other.mBuffer);
        mSize = other.mSize;
        other.mSize = 0;

        return *this;
    }

    inline size_t size() const noexcept
    {
        return mSize;
    }

    inline TValue const * get() const noexcept
    {
        return mBuffer.get();
    }

    inline TValue * get() noexcept
    {
        return mBuffer.get();
    }

    inline TValue const & operator[](size_t index) const noexcept
    {
        assert(index < mSize);
        return mBuffer[index];
    }

    inline TValue & operator[](size_t index) noexcept
    {
        assert(index < mSize);
        return mBuffer[index];
    }

    template<typename TValue2>
    unique_buffer<TValue2> convert_copy() const
    {
        assert((mSize * sizeof(TValue)) % sizeof(TValue2) == 0);

        auto const newSize = mSize * sizeof(TValue) / sizeof(TValue2);
        unique_buffer<TValue2> newBuffer(newSize);
        std::memcpy(reinterpret_cast<void *>(newBuffer.get()), reinterpret_cast<void *>(mBuffer.get()), newSize * sizeof(TValue2));
        
        return newBuffer;
    }

    template<typename TValue2>
    unique_buffer<TValue2> convert_move()
    {
        assert((mSize * sizeof(TValue)) % sizeof(TValue2) == 0);
        
        TValue2 * const newPtr = reinterpret_cast<TValue2 *>(mBuffer.get());
        auto const newSize = mSize * sizeof(TValue) / sizeof(TValue2);

        mBuffer.release();
        mSize = 0;

        return unique_buffer<TValue2>(newPtr, newSize);
    }

private:

    std::unique_ptr<TValue[]> mBuffer;
    size_t mSize;
};
