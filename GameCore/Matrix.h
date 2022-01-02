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

template<typename TValue>
struct Matrix2
{
    int const width;
    int const height;

    Matrix2(
        int _width,
        int _height)
        : Matrix2(
            _width,
            _height,
            TValue())
    {}

    Matrix2(
        int _width,
        int _height,
        TValue defaultValue)
        : width(_width)
        , height(_height)
        , mColumns(nullptr)
        , mMatrixStorage(nullptr)
    {
        mMatrixStorage = std::malloc(sizeof(TValue *) * width + sizeof(TValue) * height * width);

        mColumns = reinterpret_cast<TValue **>(mMatrixStorage);

        for (int c = 0; c < width; ++c)
        {
            TValue * const col = reinterpret_cast<TValue *>(
                reinterpret_cast<uint8_t *>(mMatrixStorage)
                + sizeof(TValue *) * width
                + sizeof(TValue) * height * c);

            mColumns[c] = col;

            for (int r = 0; r < height; ++r)
            {
                new(&(col[r])) TValue(defaultValue);
            }
        }
    }

    ~Matrix2()
    {
        assert(mMatrixStorage != nullptr);
        std::free(mMatrixStorage);
    }

    TValue & operator[](vec2i const & index)
    {
        return const_cast<TValue &>((static_cast<Matrix2 const &>(*this))[index]);
    }

    TValue const & operator[](vec2i const & index) const
    {
        assert(index.IsInSize(*this));
        return mColumns[index.x][index.y];
    }

private:

    TValue ** mColumns;

    // The storage
    void * mMatrixStorage;
};
