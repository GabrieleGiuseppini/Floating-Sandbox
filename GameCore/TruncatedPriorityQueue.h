/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-08-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameTypes.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>

/*
 * This class implements a priority queue of ElementIndex elements,
 * which may only hold a fixed number of elements.
 *
 * The heap property is honored so that:
 *      cmp(parent, child) == false
 *
 * ...so that the root always contains the least priority one.
 */
template <typename PriorityType, typename Compare = std::less_equal<PriorityType>>
class TruncatedPriorityQueue
{
private:

    struct HeapEntry
    {
        PriorityType priority;
        ElementIndex elementIndex;
    };

    using HeapIndex = std::uint32_t;

public:

    TruncatedPriorityQueue(size_t maxSize)
        : mHeap(std::make_unique<HeapEntry[]>(maxSize + 1)) // Entry at zero is sentinel
        , mAllocatedSize(maxSize)
    {
        reset(maxSize);
    }

    inline bool empty() const noexcept
    {
        return mCurrentHeapSize == 0;
    }

    inline size_t size() const noexcept
    {
        return mCurrentHeapSize;
    }

    inline void emplace(ElementIndex e, PriorityType p) noexcept
    {
        Compare cmp;

        assert(mCurrentHeapSize <= mMaxHeapSize);

        if (mCurrentHeapSize == mMaxHeapSize)
        {
            if (mCurrentHeapSize > 0
                && cmp(p, mHeap[1].priority))
            {
                // Replace root
                mHeap[1].priority = p;
                mHeap[1].elementIndex = e;

                // Restore heap
                fix_down(1);
            }
        }
        else
        {
            // Insert at bottom
            ++mCurrentHeapSize;
            auto i = static_cast<HeapIndex>(mCurrentHeapSize);
            mHeap[i].priority = p;
            mHeap[i].elementIndex = e;

            // Restore heap
            fix_up(i);

            assert(mCurrentHeapSize <= mMaxHeapSize);
        }
    }

    inline ElementIndex const & operator[](size_t index) const noexcept
    {
        assert(index <= mCurrentHeapSize);
        return mHeap[index + 1].elementIndex;
    }

    inline void clear() noexcept
    {
        reset(mMaxHeapSize);
    }

    inline void clear(size_t maxSize) noexcept
    {
        reset(maxSize);
    }

    // Mostly for unit tests
    bool verify_heap() const
    {
        if (empty())
            return true;
        else
            return verify_heap_entry(1);
    }

private:

    inline void fix_up(HeapIndex i) noexcept
    {
        Compare cmp;

        while (i > 1 && cmp(mHeap[i / 2].priority, mHeap[i].priority))
        {
            std::swap(mHeap[i], mHeap[i / 2]);

            // Go up
            i = i / 2;
        }
    }

    inline void fix_down(HeapIndex i) noexcept
    {
        Compare cmp;

        auto const currentHeapSize = mCurrentHeapSize;
        for (auto j = 2 * i; j <= currentHeapSize; j = 2 * i)
        {
            // Find largest of two children
            if (j < currentHeapSize && cmp(mHeap[j].priority, mHeap[j + 1].priority))
                ++j;

            // Check whether heap property is satisfed
            if (!cmp(mHeap[i].priority, mHeap[j].priority))
                break;

            // Swap with largest child
            std::swap(mHeap[i], mHeap[j]);

            // Go down
            i = j;
        }
    }

    bool verify_heap_entry(HeapIndex i) const
    {
        Compare cmp;

        // Check left
        HeapIndex l = 2 * i;
        if (l <= mCurrentHeapSize)
        {
            if (cmp(mHeap[i].priority, mHeap[l].priority))
            {
                return false;
            }

            if (!verify_heap_entry(l))
                return false;
        }

        // Check right
        HeapIndex r = l + 1;
        if (r <= mCurrentHeapSize)
        {
            if (cmp(mHeap[i].priority, mHeap[r].priority))
            {
                return false;
            }

            if (!verify_heap_entry(r))
                return false;
        }

        return true;
    }

    void reset(size_t maxHeapSize)
    {
        assert(maxHeapSize <= mAllocatedSize);

        mCurrentHeapSize = 0;
        mMaxHeapSize = maxHeapSize;
    }

private:

    std::unique_ptr<HeapEntry[]> const mHeap; // Entry at zero is unused
    size_t const mAllocatedSize; // Excludes extra entry at zero

    size_t mCurrentHeapSize; // Excludes extra entry at zero
    size_t mMaxHeapSize; // Excludes extra entry at zero
};