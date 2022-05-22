/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-10-03
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>

/*
 * This class is exactly like a unique_ptr<TValue[]>, but it also
 * carries the size of the buffer, and implements some mathematical operators.
 */
template<typename TValue>
class unique_buffer
{
public:

    explicit unique_buffer(size_t size)
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

    unique_buffer(unique_buffer<TValue> const & other)
        : mBuffer()
        , mSize(0)
    {
        *this = other;
    }

    unique_buffer(unique_buffer<TValue> && other) noexcept
    {
        mBuffer = std::move(other.mBuffer);
        mSize = other.mSize;
        other.mSize = 0;
    }

    unique_buffer<TValue> & operator=(unique_buffer<TValue> const & other)
    {
        if (mSize != other.mSize)
        {
            mBuffer = std::make_unique<TValue[]>(other.mSize);
            mSize = other.mSize;
        }

        std::memcpy(mBuffer.get(), other.mBuffer.get(), other.mSize * sizeof(TValue));

        return *this;
    }

    unique_buffer<TValue> & operator=(unique_buffer<TValue> && other)
    {
        mBuffer = std::move(other.mBuffer);
        mSize = other.mSize;
        other.mSize = 0;

        return *this;
    }

    bool operator==(unique_buffer<TValue> const & other) const
    {
        // Shortcut
        if (mBuffer.get() == other.mBuffer.get())
            return true;

        if ((!!mBuffer) != (!!other.mBuffer))
            return false;

        assert(!!mBuffer && !!other.mBuffer);

        return mSize == other.mSize
            && 0 == std::memcmp(mBuffer.get(), other.mBuffer.get(), mSize * sizeof(TValue));
    }

    bool operator!=(unique_buffer<TValue> const & other) const
    {
        return !(*this == other);
    }

	unique_buffer<TValue> & operator+=(unique_buffer<TValue> const & rhs)
	{
		assert(mSize == rhs.mSize);

		for (size_t i = 0; i < mSize; ++i)
			mBuffer[i] += rhs.mBuffer[i];

		return *this;
	}

	unique_buffer<TValue> & operator-=(unique_buffer<TValue> const & rhs)
	{
		assert(mSize == rhs.mSize);

		for (size_t i = 0; i < mSize; ++i)
			mBuffer[i] -= rhs.mBuffer[i];

		return *this;
	}

	unique_buffer<TValue> & operator*=(float rhs)
	{
		for (size_t i = 0; i < mSize; ++i)
			mBuffer[i] *= rhs;

		return *this;
	}

	unique_buffer<TValue> & operator/=(float rhs)
	{
		for (size_t i = 0; i < mSize; ++i)
			mBuffer[i] /= rhs;

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
        std::memcpy(reinterpret_cast<void *>(newBuffer.get()), reinterpret_cast<void *>(mBuffer.get()), mSize * sizeof(TValue));

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

    void fill(TValue const & value)
    {
        std::fill(
            mBuffer.get(),
            mBuffer.get() + mSize,
            value);
    }

private:

    std::unique_ptr<TValue[]> mBuffer;
    size_t mSize;
};
