/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-09-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>

class SerializationBuffer
{
public:

    SerializationBuffer(size_t capacity)
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

    unsigned char & operator[](size_t index)
    {
        assert(index < mSize);

        return mBuffer[index];
    }

    template<typename TType>
    TType & GetAs(size_t index)
    {
        return *reinterpret_cast<TType *>(mBuffer.get() + index);
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
     * Appends the specified data, and advances.
     */
    template<typename TType>
    void Append(TType const & data)
    {
        EnsureMayAppend(mSize + sizeof(TType));

        // Append
        std::memcpy(mBuffer.get() + mSize, &data, sizeof(TType));

        // Advance
        mSize += sizeof(TType);
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