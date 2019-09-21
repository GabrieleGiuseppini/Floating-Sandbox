/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-11-12
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameTypes.h"

#include <cassert>
#include <iterator>

struct element_index_range_iterator
{
public:

    typedef std::input_iterator_tag iterator_category;

public:

    explicit element_index_range_iterator(ElementIndex current) noexcept
        : mCurrent(current)
    {}

    inline bool operator==(element_index_range_iterator const & other) const noexcept
    {
        return mCurrent == other.mCurrent;
    }

    inline bool operator!=(element_index_range_iterator const & other) const noexcept
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

    ElementIndex mCurrent;
};


class ElementIndexRangeIterable
{
public:

    ElementIndexRangeIterable(
        ElementIndex startIndex,
        ElementIndex endIndex /*excluded*/)
        : mStartIndex(startIndex)
        , mEndIndex(endIndex)
    {}

    inline element_index_range_iterator begin() const noexcept
    {
        return element_index_range_iterator(mStartIndex);
    }

    inline element_index_range_iterator end() const noexcept
    {
        return element_index_range_iterator(mEndIndex);
    }

private:

    ElementIndex const mStartIndex;
    ElementIndex const mEndIndex;
};

struct element_index_reverse_range_iterator
{
public:

    typedef std::input_iterator_tag iterator_category;

public:

    explicit element_index_reverse_range_iterator(ElementIndex current) noexcept
        : mCurrent(current)
    {}

    inline bool operator==(element_index_reverse_range_iterator const & other) const noexcept
    {
        return mCurrent == other.mCurrent;
    }

    inline bool operator!=(element_index_reverse_range_iterator const & other) const noexcept
    {
        return !(mCurrent == other.mCurrent);
    }

    inline void operator++() noexcept
    {
        --mCurrent;
    }

    inline ElementIndex operator*() noexcept
    {
        return mCurrent;
    }

private:

    ElementIndex mCurrent;
};

class ElementIndexReverseRangeIterable
{
public:

    ElementIndexReverseRangeIterable(
        ElementIndex startIndex,
        ElementIndex endIndex /*excluded*/)
        : mStartIndex(startIndex)
        , mEndIndex(endIndex)
    {}

    inline element_index_reverse_range_iterator begin() const noexcept
    {
        return element_index_reverse_range_iterator(mEndIndex - 1);
    }

    inline element_index_reverse_range_iterator end() const noexcept
    {
        return element_index_reverse_range_iterator(mStartIndex - 1);
    }

private:

    ElementIndex const mStartIndex;
    ElementIndex const mEndIndex;
};