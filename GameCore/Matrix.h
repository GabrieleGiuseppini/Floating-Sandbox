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
        mMatrix.reset(new std::unique_ptr<TValue[]>[width]);

        for (int c = 0; c < width; ++c)
        {
            auto col = std::unique_ptr<TValue[]>(new TValue[height]);

            std::fill(
                col.get(),
                col.get() + height,
                defaultValue);

            mMatrix[c] = std::move(col);
        }
    }

    TValue & operator[](vec2i const & index)
    {
        return const_cast<TValue &>((static_cast<Matrix2 const &>(*this))[index]);
    }

    TValue const & operator[](vec2i const & index) const
    {
        assert(index.IsInRect(*this));
        return mMatrix[index.x][index.y];
    }

private:

    std::unique_ptr<std::unique_ptr<TValue[]>[]> mMatrix;
};
