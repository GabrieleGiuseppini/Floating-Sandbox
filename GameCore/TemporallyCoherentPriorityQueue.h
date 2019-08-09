/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-07-12
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
 * which is expected to expose a high degree of temporally coherency.
 *
 * The queue allows for insertions, updates, and removals of individual
 * elements, addressed by their ElementIndex.
 *
 * The heap property is honored so that:
 *      cmp(parent, child) == true
 */
template <typename PriorityType, typename Compare = std::less_equal<PriorityType>>
class TemporallyCoherentPriorityQueue
{
private:

    struct HeapEntry
    {
        PriorityType priority;
        ElementIndex elementIndex;
    };

    using HeapIndex = std::uint32_t;
    static HeapIndex constexpr HeapIndexNone = std::numeric_limits<HeapIndex>::max();

public:

    TemporallyCoherentPriorityQueue(size_t size)
        : mMaxSize(size)
        , mHeap(std::make_unique<HeapEntry[]>(mMaxSize + 1)) // Entry at zero is sentinel
        , mHeapIndices(std::make_unique<HeapIndex[]>(mMaxSize))
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
        mHeapIndices[e] = HeapIndexNone;

        // Move smallest to root
        mHeap[1] = mHeap[mHeapSize];
        mHeapIndices[mHeap[mHeapSize].elementIndex] = 1;
        --mHeapSize;

        fix_down(1);

        return e;
    }

    inline void add_or_update(ElementIndex e, PriorityType p) noexcept
    {
        assert(e < mMaxSize);

        auto i = mHeapIndices[e];

        if (i == HeapIndexNone)
        {
            // New

            // Insert at bottom
            ++mHeapSize;
            i = static_cast<HeapIndex>(mHeapSize);
            mHeap[i].priority = p;
            mHeap[i].elementIndex = e;
            mHeapIndices[e] = i;

            // Restore heap
            fix_up(i);
        }
        else
        {
            // Old

            auto const oldP = mHeap[i].priority;

            mHeap[i].priority = p;
            assert(mHeap[i].elementIndex == e);

            // Fix heap
            Compare cmp;
            if (cmp(p, oldP))
            {
                if (p != oldP)
                {
                    // Decreased priority
                    fix_up(i);
                }
            }
            else
            {
                // Increased priority
                fix_down(i);
            }
        }
    }

    inline void remove_if_in(ElementIndex e) noexcept
    {
        assert(e < mMaxSize);

        auto const i = mHeapIndices[e];

        if (i != HeapIndexNone)
        {
            // Remove element
            mHeapIndices[e] = HeapIndexNone;

            // Move bottom here
            auto oldP = mHeap[i].priority;
            mHeap[i] = mHeap[mHeapSize];
            mHeapIndices[mHeap[mHeapSize].elementIndex] = i;
            --mHeapSize;

            // Fix heap
            Compare cmp;
            if (cmp(mHeap[i].priority, oldP))
            {
                // Decreased priority
                fix_up(i);
            }
            else
            {
                // Increased priority
                fix_down(i);
            }
        }
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
            std::swap(mHeapIndices[mHeap[i].elementIndex], mHeapIndices[mHeap[i / 2].elementIndex]);

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

            // Check whether heap property is satisfed
            if (cmp(mHeap[i].priority, mHeap[j].priority))
                break;

            std::swap(mHeap[i], mHeap[j]);
            std::swap(mHeapIndices[mHeap[i].elementIndex], mHeapIndices[mHeap[j].elementIndex]);

            // Go down
            i = j;
        }
    }

    bool verify_heap_entry(HeapIndex i) const
    {
        // Verify index
        if (mHeapIndices[mHeap[i].elementIndex] != i)
            return false;

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

        std::fill(
            mHeapIndices.get(),
            mHeapIndices.get() + mMaxSize,
            HeapIndexNone);
    }

private:

    size_t const mMaxSize;

    std::unique_ptr<HeapEntry[]> mHeap; // Entry at zero is unused
    size_t mHeapSize;

    std::unique_ptr<HeapIndex[]> mHeapIndices;
};