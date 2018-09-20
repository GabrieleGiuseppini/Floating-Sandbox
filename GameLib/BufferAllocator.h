/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-20
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer.h"

#include <cassert>
#include <memory>
#include <vector>

template <typename TElement>
class BufferAllocator
{
public:

    BufferAllocator(size_t bufferSize)
        : mBufferSize(bufferSize)
        , mPool()
    {
    }

    std::shared_ptr<Buffer<TElement>> Allocate()
    {
        Buffer<TElement> * buffer;
        if (!mPool.empty())
        {
            buffer = mPool.back().release();
            mPool.pop_back();
        }
        else
        {
            buffer = new Buffer<TElement>(mBufferSize);
        }

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
        mPool.push_back(std::unique_ptr<Buffer<TElement>>(buffer));
    }

    size_t const mBufferSize;
    std::vector<std::unique_ptr<Buffer<TElement>>> mPool;
};