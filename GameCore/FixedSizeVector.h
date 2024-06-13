/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-06
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SysSpecifics.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>

/*
 * This class is basically an array with a "current size" state member. The maximum size is specified at compile time.
 *
 * Elements can be added up to the specified maximum size, after which the behavior is undefined.
 *
 * The container is optimized for fast *visit*, so that it can be used to iterate through
 * all its elements, and for fast *erase* by index. Pushes are not optimized, unless they are on the back.
 */
template<typename TElement, size_t MaxSize>
class FixedSizeVector
{
private:

    /*
     * Our iterator.
     */
    template <
        typename TIterated,
        typename TIteratedPointer>
    struct _iterator
    {
    public:

        using difference_type = void;
        using value_type = TIterated;
        using pointer = TIteratedPointer;
        using reference = TIterated & ;
        using iterator_category = std::forward_iterator_tag;


    public:

        inline bool operator==(_iterator const & other) const noexcept
        {
            return mCurrent == other.mCurrent;
        }

        inline bool operator!=(_iterator const & other) const noexcept
        {
            return !(mCurrent == other.mCurrent);
        }

        inline void operator++() noexcept
        {
            ++mCurrent;
        }

        inline TIterated & operator*() noexcept
        {
            return *mCurrent;
        }

        inline TIteratedPointer operator->() noexcept
        {
            return mCurrent;
        }

    private:

        friend class FixedSizeVector<TElement, MaxSize>;

        explicit _iterator(TIteratedPointer current) noexcept
            : mCurrent(current)
        {}

        TIteratedPointer mCurrent;
    };

public:

    typedef _iterator<TElement, TElement *> iterator;

    typedef _iterator<TElement const, TElement const *> const_iterator;

public:

    FixedSizeVector()
        : mCurrentSize(0u)
    {
    }

    FixedSizeVector(FixedSizeVector const & other) = default;
    FixedSizeVector & operator=(FixedSizeVector const & other) = default;

    //
    // Visitors
    //

    inline iterator begin() noexcept
    {
        return iterator(mArray.data());
    }

    inline iterator end() noexcept
    {
        return iterator(mArray.data() + mCurrentSize);
    }

    inline const_iterator begin() const noexcept
    {
        return const_iterator(mArray.data());
    }

    inline const_iterator end() const noexcept
    {
        return const_iterator(mArray.data() + mCurrentSize);
    }

    inline const_iterator cbegin() const noexcept
    {
        return const_iterator(mArray.data());
    }

    inline const_iterator cend() const noexcept
    {
        return const_iterator(mArray.data() + mCurrentSize);
    }

    inline TElement & operator[](size_t index) noexcept
    {
        assert(index < mCurrentSize);
        return mArray[index];
    }

    inline TElement const & operator[](size_t index) const noexcept
    {
        assert(index < mCurrentSize);
        return mArray[index];
    }

    inline TElement & back() noexcept
    {
        assert(mCurrentSize > 0);
        return mArray[mCurrentSize - 1];
    }

    inline TElement const & back() const noexcept
    {
        assert(mCurrentSize > 0);
        return mArray[mCurrentSize - 1];
    }

    inline size_t size() const noexcept
    {
        return mCurrentSize;
    }

    inline bool empty() const noexcept
    {
        return mCurrentSize == 0u;
    }

    inline bool contains(TElement const & element) const noexcept
    {
        for (size_t i = 0; i < mCurrentSize; ++i)
        {
            if (mArray[i] == element)
                return true;
        }

        return false;
    }

    template<typename UnaryPredicate>
    inline bool contains(UnaryPredicate p) const noexcept
    {
        for (size_t i = 0; i < mCurrentSize; ++i)
        {
            if (p(mArray[i]))
                return true;
        }

        return false;
    }

    TElement const * data() const
    {
        return mArray.data();
    }

    TElement * data()
    {
        return mArray.data();
    }

    //
    // Modifiers
    //

    void push_front(TElement const & element) noexcept
    {
        assert(mCurrentSize < MaxSize);

        assert(false == contains(element));

        // Shift elements (to the right) first
        for (size_t j = mCurrentSize; j > 0; --j)
        {
            mArray[j] = std::move(mArray[j - 1]);
        }

        // Set new element at front
        mArray[0] = element;

        ++mCurrentSize;
    }

    void push_front_unique(TElement const & element) noexcept
    {
        assert(false == contains(element));

        push_front(element);
    }

    void push_back(TElement const & element) noexcept
    {
        assert(mCurrentSize < MaxSize);

        mArray[mCurrentSize++] = element;
    }

    void push_back(TElement && element) noexcept
    {
        assert(mCurrentSize < MaxSize);

        mArray[mCurrentSize++] = std::move(element);
    }

    void push_back_unique(TElement const & element) noexcept
    {
        assert(false == contains(element));

        push_back(element);
    }

    template<typename ...TArgs>
    void emplace_front(TArgs&&... args) noexcept
    {
        assert(mCurrentSize < MaxSize);

        // Shift elements (to the right) first
        for (size_t j = mCurrentSize; j > 0; --j)
        {
            mArray[j] = std::move(mArray[j - 1]);
        }

        // Set new element at front
        mArray[0] = TElement(std::forward<TArgs>(args)...);

        ++mCurrentSize;
    }

    template <typename ...TArgs>
    TElement & emplace_back(TArgs&&... args) noexcept
    {
        assert(mCurrentSize < MaxSize);

        TElement & newElement = mArray[mCurrentSize++];
        newElement = TElement(std::forward<TArgs>(args)...);
        return newElement;
    }

    void erase(size_t index) noexcept
    {
        assert(index < mCurrentSize);

        // Shift remaining elements
        for (; index < mCurrentSize - 1; ++index)
        {
            mArray[index] = std::move(mArray[index + 1]);
        }

        --mCurrentSize;
    }

    template <typename UnaryPredicate>
    bool erase_first(UnaryPredicate p) noexcept
    {
        for (size_t i = 0; i < mCurrentSize; /* incremented in loop */)
        {
            if (p(mArray[i]))
            {
                // Shift remaining elements
                for (size_t j = i; j < mCurrentSize - 1; ++j)
                {
                    mArray[j] = std::move(mArray[j + 1]);
                }

                --mCurrentSize;

                return true;
            }
            else
            {
                ++i;
            }
        }

        return false;
    }

    bool erase_first(TElement const & element) noexcept
    {
        return erase_first(
            [&element](TElement const & e)
            {
                return e == element;
            });
    }

    void clear() noexcept
    {
        mCurrentSize = 0u;
    }

    void fill(TElement value)
    {
        TElement * restrict const ptr = mArray.data();
        for (size_t i = 0; i < MaxSize; ++i)
            ptr[i] = value;

        mCurrentSize = MaxSize;
    }

    template <typename TCompare>
    void sort(TCompare comp)
    {
        std::sort(
            &(mArray[0]),
            &(mArray[mCurrentSize]),
            comp);
    }

private:

    std::array<TElement, MaxSize> mArray;
    size_t mCurrentSize;
};
