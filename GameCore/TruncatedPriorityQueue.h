/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-08-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameTypes.h"

#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>

/*
 * This class implements a priority queue of ElementIndex elements,
 * which may only hold a fixed number of elements.
 *
 * The heap property is honored so that:
 *      cmp(parent, child) == true
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

    TruncatedPriorityQueue(size_t size)
        : mMaxSize(size)
        , mHeap(std::make_unique<HeapEntry[]>(mMaxSize + 1)) // Entry at zero is sentinel
    {
        reset();
    }

    inline bool empty() const noexcept
    {
        return mHeapSize == 0;
    }

    inline size_t size() const noexcept
    {
        return mHeapSize;
    }

    inline ElementIndex pop() noexcept
    {
        assert(!empty());

        // Remove root
        auto const e = mHeap[1].elementIndex;

        // Move smallest to root
        mHeap[1] = mHeap[mHeapSize];
        --mHeapSize;

        fix_down(1);

        return e;
    }

    inline void emplace(ElementIndex e, PriorityType p) noexcept
    {
        // Insert at bottom
        ++mHeapSize;
        auto i = static_cast<HeapIndex>(mHeapSize);
        mHeap[i].priority = p;
        mHeap[i].elementIndex = e;

        // Restore heap
        fix_up(i);
    }

    inline void clear() noexcept
    {
        reset();
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

        while (i > 1 && !cmp(mHeap[i / 2].priority, mHeap[i].priority))
        {
            std::swap(mHeap[i], mHeap[i / 2]);

            // Go up
            i = i / 2;
        }
    }

    inline void fix_down(HeapIndex i) noexcept
    {
        Compare cmp;

        while (2 * i <= mHeapSize)
        {
            auto j = 2 * i;

            // Find largest of two
            if (j < mHeapSize && !cmp(mHeap[j].priority, mHeap[j + 1].priority))
                ++j;

            // Check whether heap property is statisfed
            if (cmp(mHeap[i].priority, mHeap[j].priority))
                break;

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
        if (l <= mHeapSize)
        {
            if (!cmp(mHeap[i].priority, mHeap[l].priority))
            {
                return false;
            }

            if (!verify_heap_entry(l))
                return false;
        }

        // Check right
        HeapIndex r = l + 1;
        if (r <= mHeapSize)
        {
            if (!cmp(mHeap[i].priority, mHeap[r].priority))
            {
                return false;
            }

            if (!verify_heap_entry(r))
                return false;
        }

        return true;
    }

    void reset()
    {
        mHeapSize = 0;
    }

private:

    size_t const mMaxSize;

    std::unique_ptr<HeapEntry[]> mHeap; // Entry at zero is unused
    size_t mHeapSize;
};