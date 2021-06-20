/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-06-20
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

struct Matrix2Index
{
    int X;
    int Y;

    Matrix2Index()
        : Matrix2Index(0, 0)
    {}

    constexpr Matrix2Index(
        int x,
        int y)
        : X(x)
        , Y(y)
    {}

    Matrix2Index operator+(Matrix2Index const & other) const
    {
        return Matrix2Index(X + other.X, Y + other.Y);
    }

    Matrix2Index operator-(Matrix2Index const & other) const
    {
        return Matrix2Index(X - other.X, Y - other.Y);
    }

    template<typename TRect>
    bool IsInRect(TRect const & rect) const
    {
        return X >= 0 && X < rect.Width && Y >= 0 && Y < rect.Height;
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "(" << X << ", " << Y << ")";
        return ss.str();
    }
};

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, Matrix2Index const & is)
{
    os << is.ToString();
    return os;
}

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

    TValue & operator[](Matrix2Index const & index)
    {
        return const_cast<TValue &>((static_cast<Matrix2 const &>(*this))[index]);
    }

    TValue const & operator[](Matrix2Index const & index) const
    {
        assert(index.IsInRect(*this));

        assert(index.X >= 0 && index.X < Width);
        assert(index.Y >= 0 && index.Y < Height);
        return mMatrix[index.X][index.Y];
    }

private:

    std::unique_ptr<std::unique_ptr<TValue[]>[]> mMatrix;
};
