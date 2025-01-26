/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-09-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Endian.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>

template<typename TEndianess>
class DeSerializationBuffer
{
public:

    DeSerializationBuffer(size_t capacity)
        : mSize(0)
        , mAllocatedSize(capacity)
    {
        mBuffer = std::make_unique<unsigned char[]>(mAllocatedSize);
    }

    size_t GetSize() const
    {
        return mSize;
    }

    unsigned char const * GetData() const
    {
        return mBuffer.get();
    }

    /*
     * Appends undefined data for the specified amount of data, advances by that much,
     * and returns the index to the append position.
     */
    template<typename TType>
    size_t ReserveAndAdvance()
    {
        EnsureMayAppend(mSize + sizeof(TType));

        // Advance
        size_t startIndex = mSize;
        mSize += sizeof(TType);
        return startIndex;
    }

    /*
     * Appends undefined data for the specified amount of data, advances by that much,
     * and returns the index to the append position.
     */
    size_t ReserveAndAdvance(size_t size)
    {
        EnsureMayAppend(mSize + size);

        // Advance
        size_t startIndex = mSize;
        mSize += size;
        return startIndex;
    }

    /*
     * Appends undefined data for the specified amount of data, advances by that much,
     * and returns the pointer to the append position, which should be used right away.
     */
    unsigned char * Receive(size_t size)
    {
        EnsureMayAppend(mSize + size);

        // Advance
        size_t startIndex = mSize;
        mSize += size;
        return mBuffer.get() + startIndex;
    }

    /*
     * Writes the specified value at the specified index, without growing the buffer.
     * Returns the number of bytes written.
     */
    template<typename T>
    size_t WriteAt(T const & value, size_t index)
    {
        assert(index + sizeof(T) < mAllocatedSize);

        return Endian<T, TEndianess>::Write(value, mBuffer.get() + index);
    }

    /*
     * Appends the specified value to the end of the buffer, growing the buffer.
     * Returns the number of bytes written.
     */
    template<typename T, typename std::enable_if_t<!std::is_same_v<T, std::string>, int> = 0>
    size_t Append(T const & value)
    {
        // Make sure it fits
        size_t const requiredSize = sizeof(T);
        EnsureMayAppend(mSize + requiredSize);

        // Append
        size_t const sz = Endian<T, TEndianess>::Write(value, mBuffer.get() + mSize);
        assert(sz <= requiredSize);

        // Advance
        mSize += sz;

        return sz;
    }

    /*
     * Appends the specified string to the end of the buffer, growing the buffer.
     * Returns the number of bytes written.
     */
    template<typename T, typename std::enable_if_t<std::is_same_v<T, std::string>, int> = 0>
    size_t Append(T const & value)
    {
        // Make sure it fits
        size_t const requiredSize = sizeof(std::uint32_t) + value.length();
        EnsureMayAppend(mSize + requiredSize);

        // Append len
        std::uint32_t const length = static_cast<std::uint32_t>(value.length());
        size_t const sz1 = Endian<std::uint32_t, TEndianess>::Write(length, mBuffer.get() + mSize);
        assert(sz1 <= sizeof(std::uint32_t));

        // Serialize chars
        std::memcpy(mBuffer.get() + mSize + sz1, value.data(), length);
        assert(sz1 + length <= requiredSize);

        // Advance
        mSize += sz1 + length;

        return sz1 + length;
    }

    /*
     * Appends the specified data, and advances.
     * Returns the number of bytes written.
     */
    size_t Append(unsigned char const * data, size_t size)
    {
        EnsureMayAppend(mSize + size);

        // Append
        std::memcpy(mBuffer.get() + mSize, data, size);

        // Advance
        mSize += size;

        return size;
    }

    /*
     * Reads a value from the specified index.
     * Returns the number of bytes read.
     */
    template<typename T, typename std::enable_if_t<!std::is_same_v<T, var_uint16_t> && !std::is_same_v<T, std::string>, int> = 0>
    size_t ReadAt(size_t index, T & value) const
    {
        assert(index + sizeof(T) <= mAllocatedSize);

        return Endian<T, TEndianess>::Read(mBuffer.get() + index, value);
    }

    /*
     * Reads a var_uint16_t value from the specified index.
     * Returns the number of bytes read.
     */
    template<typename T, typename std::enable_if_t<std::is_same_v<T, var_uint16_t>, int> = 0>
    size_t ReadAt(size_t index, T & value) const
    {
        assert(index + 1 <= mAllocatedSize);

        return Endian<var_uint16_t, TEndianess>::Read(mBuffer.get() + index, value);
    }

    /*
     * Reads a string at the specified index.
     * Returns the number of bytes read.
     */
    template<typename T, typename std::enable_if_t<std::is_same_v<T, std::string>, int> = 0>
    size_t ReadAt(size_t index, T & value) const
    {
        // Read length
        assert(index + sizeof(std::uint32_t) <= mAllocatedSize);
        std::uint32_t length;
        size_t const sz1 = Endian<std::uint32_t, TEndianess>::Read(mBuffer.get() + index, length);
        assert(sz1 == sizeof(std::uint32_t));

        // Read bytes
        assert(index + sizeof(std::uint32_t) + length <= mAllocatedSize);
        value = std::string(reinterpret_cast<char const *>(mBuffer.get()) + index + sz1, length);

        return sz1 + length;
    }

    /*
     * Reads bytes at the specified index.
     * Returns the number of bytes read.
     */
    size_t ReadAt(size_t index, unsigned char * ptr, size_t count) const
    {
        assert(index + count <= mAllocatedSize);

        std::memcpy(ptr, mBuffer.get() + index, count);

        return count;
    }

    void Reset()
    {
        mSize = 0;
    }

private:

    void EnsureMayAppend(size_t additionalSize)
    {
        size_t requiredAllocatedSize = mSize + additionalSize;
        if (requiredAllocatedSize > mAllocatedSize)
        {
            if (requiredAllocatedSize < 128 * 1024
                && requiredAllocatedSize < mAllocatedSize * 2)
            {
                requiredAllocatedSize = mAllocatedSize * 2;
            }

            unsigned char * newBuffer = new unsigned char[requiredAllocatedSize];
            std::memcpy(newBuffer, mBuffer.get(), mSize);
            mBuffer.reset(newBuffer);
            mAllocatedSize = requiredAllocatedSize;
        }
    }

private:

    std::unique_ptr<unsigned char[]> mBuffer;
    size_t mSize; // Current pointer
    size_t mAllocatedSize;
};