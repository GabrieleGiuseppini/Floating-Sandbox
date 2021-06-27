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
        , mMatrix(static_cast<TValue *>(std::malloc(sizeof(TValue) * width * height)), &std::free)
    {
        std::fill(
            mMatrix.get(),
            mMatrix.get() + width * height,
            defaultValue);
    }

    TValue & operator[](vec2i const & index)
    {
        return const_cast<TValue &>((static_cast<Matrix2 const &>(*this))[index]);
    }

    TValue const & operator[](vec2i const & index) const
    {
        assert(index.IsInRect(*this));
        return mMatrix.get()[index.x * Height + index.y];
    }

private:

    std::unique_ptr<TValue, decltype(&std::free)> mMatrix;
};
