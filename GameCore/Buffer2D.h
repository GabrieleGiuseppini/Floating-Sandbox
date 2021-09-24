/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameTypes.h"

#include <algorithm>
#include <memory>

template <typename TElement, typename TIntegralTag>
struct Buffer2D
{
public:

    using element_type = TElement;
    using coordinates_type = _IntegralCoordinates<TIntegralTag>;
    using size_type = _IntegralSize<TIntegralTag>;

public:

    size_type const Size;
    std::unique_ptr<TElement[]> Data;

    Buffer2D(size_type size)
        : Buffer2D(size, TElement())
    {
    }

    Buffer2D(
        size_type size,
        TElement const & defaultValue)
        : Buffer2D(
            size.width,
            size.height,
            defaultValue)
    {
    }

    Buffer2D(
        int width,
        int height,
        TElement defaultValue)
        : Size(width, height)
        , mLinearSize(width * height)
    {
        Data = std::make_unique<TElement[]>(mLinearSize);
        std::fill(
            Data.get(),
            Data.get() + mLinearSize,
            defaultValue);
    }

    Buffer2D(
        size_type size,
        std::unique_ptr<TElement[]> data)
        : Buffer2D(
            size.width,
            size.height,
            std::move(data))
    {
    }

    Buffer2D(
        int width,
        int height,
        std::unique_ptr<TElement[]> data)
        : Size(width, height)
        , Data(std::move(data))
        , mLinearSize(width * height)
    {
    }

    Buffer2D(Buffer2D && other) noexcept
        : Size(other.Size)
        , Data(std::move(other.Data))
        , mLinearSize(other.mLinearSize)
    {
    }

    Buffer2D & operator=(Buffer2D && other) noexcept
    {
        Size = other.Size;
        Data = std::move(other.Data);
        mLinearSize = other.mLinearSize;
    }

    size_t GetByteSize() const
    {
        return mLinearSize * sizeof(TElement);
    }

    std::unique_ptr<Buffer2D> MakeCopy() const
    {
        auto newData = std::make_unique<TElement[]>(mLinearSize);
        std::memcpy(newData.get(), Data.get(), mLinearSize * sizeof(TElement));

        return std::make_unique<Buffer2D>(
            Size,
            std::move(newData));
    }

    std::unique_ptr<Buffer2D> MakeCopy(
        coordinates_type const & regionOrigin,
        size_type const & regionSize)
    {
        auto newData = std::make_unique<TElement[]>(regionSize.width * regionSize.height);
        for (int targetY = 0; targetY < regionSize.height; ++targetY)
        {
            int const sourceLinearIndex = (targetY + regionOrigin.y) * Size.width + regionOrigin.x;
            int const targetLinearIndex = targetY * regionSize.width;

            std::memcpy(
                newData.get() + targetLinearIndex,
                Data.get() + sourceLinearIndex,
                regionSize.width * sizeof(TElement));
        }

        return std::make_unique<Buffer2D>(
            regionSize,
            std::move(newData));
    }

    TElement & operator[](coordinates_type const & index)
    {
        return const_cast<TElement &>((static_cast<Buffer2D const &>(*this))[index]);
    }

    TElement const & operator[](coordinates_type const & index) const
    {
        assert(index.IsInSize(Size));

        size_t const linearIndex = index.y * Size.width + index.x;
        assert(linearIndex < mLinearSize);

        return Data[linearIndex];
    }

private:

    size_t const mLinearSize;
};
