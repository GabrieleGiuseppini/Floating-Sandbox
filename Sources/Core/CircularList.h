/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <algorithm>
#include <array>
#include <cassert>

/*
 * This class implements a circular list of elements.
 *
 * Elements can be added up to the specified maximum size, after which older elements
 * start being overwritten.
 *
 * The list keeps elements in the order of their insertion, and allows for visits
 * in insert order.
 */
template<typename TElement, size_t MaxSize>
class CircularList
{
private:

    /*
     * Our iterator.
     */
    template <typename TIterated, typename TList>
    struct _iterator
    {
    public:
        using value_type = TIterated;
        using difference_type = std::size_t;
        using pointer = TIterated *;
        using reference = TIterated &;
        using iterator_category = std::input_iterator_tag;

    public:

        _iterator(_iterator const & other) = default;
        _iterator & operator=(_iterator const & other) = default;

        inline bool operator==(_iterator const & other) const noexcept
        {
            return mCurrentHead == other.mCurrentHead;
        }

        inline bool operator!=(_iterator const & other) const noexcept
        {
            return !(mCurrentHead == other.mCurrentHead);
        }

        inline void operator++() noexcept
        {
            if (mCurrentHead > 0)
                mCurrentHead--;
            else
                mCurrentHead = MaxSize;
        }

        inline reference operator*() noexcept
        {
            return mList->mArray[mCurrentHead > 0 ? mCurrentHead - 1u : MaxSize];
        }

        inline pointer operator->() noexcept
        {
            return mList->mArray + (mCurrentHead > 0 ? mCurrentHead - 1u : MaxSize);
        }

    private:

        friend class CircularList<TElement, MaxSize>;

        explicit _iterator(
            size_t head,
            TList * list) noexcept
            : mCurrentHead(head)
            , mList(list)
        {}

        size_t mCurrentHead;
        TList * mList;
    };

public:

    typedef _iterator<TElement, CircularList> iterator;

    typedef _iterator<TElement const, CircularList const> const_iterator;

public:

    CircularList()
        : mHead(0u)
        , mTail(0u)
    {
        static_assert(MaxSize > 0);
    }

    // Make sure we don't introduce unnecessary copies inadvertently
    CircularList(CircularList const & other) = delete;
    CircularList & operator=(CircularList const & other) = delete;


    //
    // Visitors
    //

    // Returns an iterator to the beginning (most recently added) element
    inline iterator begin() noexcept
    {
        return iterator(
            mHead,
            this);
    }

    inline iterator end() noexcept
    {
        return iterator(
            mTail,
            this);
    }

    // Returns an iterator to the beginning (most recently added) element
    inline const_iterator begin() const noexcept
    {
        return const_iterator(
            mHead,
            this);
    }

    inline const_iterator end() const noexcept
    {
        return const_iterator(
            mTail,
            this);
    }

    inline const_iterator cbegin() const noexcept
    {
        return const_iterator(
            mHead,
            this);
    }

    inline const_iterator cend() const noexcept
    {
        return const_iterator(
            mTail,
            this);
    }

    inline size_t size() const noexcept
    {
        if (mHead >= mTail)
            return mHead - mTail;
        else
            return mHead + (MaxSize + 1) - mTail;
    }

    inline bool empty() const noexcept
    {
        return mHead == mTail;
    }


    //
    // Modifiers
    //

    //
    // Adds an element to the list.
    // If the list is full, purges the oldest element from the list,
    // invoking the specified handler.
    //
    template <typename ElementPurgedHandler, typename... Args>
    TElement & emplace(
        ElementPurgedHandler onElementPurged,
        Args&&... args)
    {
        TElement & res = *new (&(mArray[mHead])) TElement(std::forward<Args>(args)...);

        mHead = (mHead + 1) % (MaxSize + 1);

        if (mHead == mTail)
        {
            // Make room
            onElementPurged(mArray[mTail]);
            mTail = (mTail + 1) % (MaxSize + 1);
        }

        return res;
    }

    template <typename Iterator>
    Iterator erase(Iterator const & it)
    {
        //
        // it.mCurrentHead points to one after the to-be-deleted point
        //

        assert((mTail < mHead && (mTail < it.mCurrentHead && it.mCurrentHead <= mHead))
            || (mHead < mTail && (it.mCurrentHead <= mHead || mTail < it.mCurrentHead)));

        // Shift all elements towards tail
        for (size_t i = it.mCurrentHead; i != mHead; i = (i + 1) % (MaxSize + 1))
        {
            if (i == 0)
                mArray[MaxSize] = std::move(mArray[i]);
            else
                mArray[i - 1] = std::move(mArray[i]);
        }

        // Adjust head
        if (mHead == 0)
            mHead = MaxSize;
        else
            --mHead;

        // Return iterator to prev element
        return iterator(
            it.mCurrentHead == 0 ? MaxSize : it.mCurrentHead - 1,
            this);
    }

    void erase(TElement const & element)
    {
        auto it = std::find(this->begin(), this->end(), element);

        assert(it != this->end());
        erase(it);
    }

    void clear()
    {
        mHead = mTail = 0;
    }

private:

    std::array<TElement, MaxSize + 1> mArray;

    // Index where the next added element goes;
    // not allowed to go beyond tail - 1
    size_t mHead;

    // Index of the oldest element
    size_t mTail;
};
