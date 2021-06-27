/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-06-20
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Vectors.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

template<typename TValue>
struct Matrix2
{
    int const Width;
    int const Height;

    Matrix2(
        int width,
        int height)
        : Matrix2(
            width,
            height,
            TValue())
    {}

    Matrix2(
        int width,
        int height,
        TValue defaultValue)
        : Width(width)
        , Height(height)
    {
        mMatrix.reserve(Width);
        for (int c = 0; c < Width; ++c)
        {
            auto col = Column(static_cast<TValue *>(std::malloc(sizeof(TValue) * height)), &std::free);

            std::fill(
                col.get(),
                col.get() + height,
                defaultValue);

            mMatrix.push_back(std::move(col));
        }
    }

    TValue & operator[](vec2i const & index)
    {
        return const_cast<TValue &>((static_cast<Matrix2 const &>(*this))[index]);
    }

    TValue const & operator[](vec2i const & index) const
    {
        assert(index.IsInRect(*this));
        return mMatrix[index.x].get()[index.y];
    }

private:

    using Column = std::unique_ptr<TValue, decltype(&std::free)>;
    std::vector<Column> mMatrix;
};
