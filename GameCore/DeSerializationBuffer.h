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

        Endian<T, TEndianess>::Write(value, mBuffer.get() + index);

        return sizeof(T);
    }

    /*
     * Appends the specified value to the end of the buffer, growing the buffer.
     * Returns the number of bytes written.
     */
    template<typename T>
    size_t Append(T const & value)
    {
        // Make sure it fits
        size_t const requiredSize = sizeof(T);
        EnsureMayAppend(mSize + requiredSize);

        // Append
        Endian<T, TEndianess>::Write(value, mBuffer.get() + mSize);

        // Advance
        mSize += requiredSize;

        return requiredSize;
    }

    /*
     * Appends the specified string to the end of the buffer, growing the buffer.
     * Returns the number of bytes written.
     */
    template<>
    size_t Append(std::string const & value)
    {
        // Make sure it fits
        size_t const requiredSize = sizeof(std::uint32_t) + value.length();
        EnsureMayAppend(mSize + requiredSize);

        // Append len
        std::uint32_t const length = static_cast<std::uint32_t>(value.length());
        Endian<std::uint32_t, TEndianess>::Write(length, mBuffer.get() + mSize);

        // Serialize chars
        std::memcpy(mBuffer.get() + mSize + sizeof(std::uint32_t), value.data(), length);

        return requiredSize;
    }

    /*
     * Appends the specified data, and advances.
     */
    void Append(unsigned char const * data, size_t size)
    {
        EnsureMayAppend(mSize + size);

        // Append
        std::memcpy(mBuffer.get() + mSize, data, size);

        // Advance
        mSize += size;
    }

    /*
     * Reads a value from the specified index.
     */
    template<typename T>
    T ReadAt(size_t index)
    {
        assert(index + sizeof(T) <= mAllocatedSize);

        return Endian<T, TEndianess>::Read(mBuffer.get() + index);
    }

    /*
     * Reads a string at the specified index.
     */
    template<>
    std::string ReadAt<std::string>(size_t index)
    {
        // Read length
        assert(index + sizeof(std::uint32_t) <= mAllocatedSize);
        std::uint32_t length = Endian<std::uint32_t, TEndianess>::Read(mBuffer.get() + index);

        // Read bytes
        assert(index + sizeof(std::uint32_t) + length <= mAllocatedSize);
        std::string retVal = std::string(reinterpret_cast<char const *>(mBuffer.get()) + index + sizeof(std::uint32_t), length);

        return retVal;
    }

    void Reset()
    {
        mSize = 0;
    }

private:

    void EnsureMayAppend(size_t newSize)
    {
        size_t requiredAllocatedSize = mSize + newSize;
        if (requiredAllocatedSize > mAllocatedSize)
        {
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