/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-06
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Log.h"

#include <array>
#include <cassert>

/*
 * This class is a fixed-size vector for value elements.
 * Elements can be added up to the specified maximum size, after which an exception occurs.
 *
 * The container is optimized for fast *visit*, so that it can be used to iterate through
 * all elements, and *erase*. Pushes are not optimized.
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

    //
    // Modifiers
    //

    void push_back(TElement const & element)
    {
        if (mCurrentSize < MaxSize)
        {
            assert(false == contains(element));
            mArray[mCurrentSize++] = element;
        }
        else
        {
            throw std::runtime_error("The container is already full");
        }
    }

    void push_back(TElement && element)
    {
        if (mCurrentSize < MaxSize)
        {
            mArray[mCurrentSize++] = std::move(element);
        }
        else
        {
            throw std::runtime_error("The container is already full");
        }
    }

    void erase(size_t index)
    {
        assert(index < mCurrentSize);

        // Shift remaining elements
        for (; index < mCurrentSize - 1; ++index)
        {
            mArray[index] = std::move(mArray[index + 1]);
        }

        --mCurrentSize;
    }

    bool erase_first(TElement const & element)
    {
        for (size_t i = 0; i < mCurrentSize; /* incremented in loop */)
        {
            if (mArray[i] == element)
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

    void clear()
    {
        mCurrentSize = 0u;
    }

private:

    std::array<TElement, MaxSize> mArray;
    size_t mCurrentSize;
};
