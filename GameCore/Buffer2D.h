/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameTypes.h"
#include "Vectors.h"

#include <algorithm>
#include <memory>

template <typename TElement>
struct Buffer2D
{
public:

    Integral2DSize const Size;
    std::unique_ptr<TElement[]> Data;

    Buffer2D(
        int width,
        int height,
        TElement defaultValue)
        : Size(width, height)
    {
        Data = std::make_unique<TElement[]>(width * height);
        std::fill(
            Data.get(),
            Data.get() + (width * height),
            defaultValue);
    }

    Buffer2D(
        int width,
        int height,
        std::unique_ptr<TElement[]> data)
        : Size(width, height)
        , Data(std::move(data))
    {
    }

    Buffer2D(
        Integral2DSize size,
        std::unique_ptr<TElement[]> data)
        : Size(size)
        , Data(std::move(data))
    {
    }

    Buffer2D(Buffer2D && other) noexcept
        : Size(other.Size)
        , Data(std::move(other.Data))
    {
    }

    Buffer2D & operator=(Buffer2D && other) noexcept
    {
        Size = other.Size;
        Data = std::move(other.Data);
    }

    size_t GetByteSize() const
    {
        return static_cast<size_t>(Size.Width * Size.Height) * sizeof(TElement);
    }

    TElement & operator[](vec2i const & index)
    {
        return const_cast<TElement &>((static_cast<Buffer2D const &>(*this))[index]);
    }

    TElement const & operator[](vec2i const & index) const
    {
        assert(index.IsInRect(Size));
        return Data[index.y * Size.Width + index.x];
    }
};
