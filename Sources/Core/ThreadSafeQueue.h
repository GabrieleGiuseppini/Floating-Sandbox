/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-12-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <atomic>
#include <cassert>
#include <mutex>
#include <optional>
#include <queue>

template<typename T>
class ThreadSafeQueue
{
public:

    ThreadSafeQueue()
        : mQueue()
        , mLock()
        , mSize(0)
    {}

    void Push(T && value)
    {
        std::unique_lock const lock{ mLock };

        mQueue.push(value);
        ++mSize;
        assert(mSize == mQueue.size());
    }

    std::optional<T> TryPop()
    {
        if (mSize.load(std::memory_order::memory_order_relaxed) > 0)
        {
            std::unique_lock const lock{ mLock };

            assert(mQueue.size() > 0);

            auto val = mQueue.front();

            mQueue.pop();
            --mSize;
            assert(mSize == mQueue.size());

            return val;
        }
        else
        {
            return std::nullopt;
        }
    }

private:

    std::queue<T> mQueue;

    // Our thread lock
    std::mutex mLock;

    // Our size, reflecting the deque's size as atomic
    std::atomic<size_t> mSize;
};
