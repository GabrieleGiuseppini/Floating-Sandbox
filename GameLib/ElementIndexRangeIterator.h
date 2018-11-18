/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-11-12
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cassert>

class ElementIndexRangeIterator
{
public:

    ElementIndexRangeIterator(
        ElementIndex startIndex,
        ElementIndex endIndex /*excluded*/)
        : mStartIndex(startIndex)
        , mEndIndex(endIndex)
    {}

    struct iterator
    {
    public:

        typedef std::input_iterator_tag iterator_category;

    public:

        inline bool operator==(iterator const & other) const noexcept
        {
            return mCurrent == other.mCurrent;
        }

        inline bool operator!=(iterator const & other) const noexcept
        {
            return !(mCurrent == other.mCurrent);
        }

        inline void operator++() noexcept
        {
            ++mCurrent;
        }

        inline ElementIndex operator*() noexcept
        {
            return mCurrent;
        }

    private:

        friend class ElementIndexRangeIterator;

        explicit iterator(ElementIndex current) noexcept
            : mCurrent(current)
        {}

        ElementIndex mCurrent;
    };

    inline iterator begin() const noexcept
    {
        return iterator(mStartIndex);
    }

    inline iterator end() const noexcept
    {
        return iterator(mEndIndex);
    }

private:

    ElementIndex const mStartIndex;
    ElementIndex const mEndIndex;
};