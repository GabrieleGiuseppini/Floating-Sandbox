/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-20
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer.h"

#include <cassert>
#include <memory>
#include <mutex>
#include <vector>

template <typename TElement>
class BufferAllocator
{
public:

    BufferAllocator(size_t bufferSize)
        : mBufferSize(bufferSize)
        , mPool()
        , mLock()
    {
    }

    BufferAllocator(BufferAllocator && other) noexcept
        : mBufferSize(other.mBufferSize)
        , mPool(std::move(other.mPool))
        , mLock() // We make our own, new lock - assuming we're not moving synchronization state here
    {
    }

    std::shared_ptr<Buffer<TElement>> Allocate()
    {
        Buffer<TElement> * buffer;

        if (std::lock_guard lock{ mLock }; !mPool.empty())
        {
            buffer = mPool.back().release();
            mPool.pop_back();
        }
        else
        {
            buffer = new Buffer<TElement>(mBufferSize);
        }

        assert(nullptr != buffer);

        return std::shared_ptr<Buffer<TElement>>(
            buffer,
            [this](auto p)
            {
                this->Release(p);
            });
    }

private:

    void Release(Buffer<TElement> * buffer)
    {
        std::lock_guard lock{ mLock };

        mPool.push_back(std::unique_ptr<Buffer<TElement>>(buffer));
    }

    size_t const mBufferSize;
    std::vector<std::unique_ptr<Buffer<TElement>>> mPool;

    // The mutex guarding concurrency-sensitive operations
    std::mutex mLock;
};