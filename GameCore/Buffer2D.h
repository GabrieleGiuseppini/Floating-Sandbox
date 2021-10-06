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

    explicit Buffer2D(size_type size)
        : Buffer2D(
            size.width,
            size.height)
    {
    }

    Buffer2D(
        int width,
        int height)
        : Size(width, height)
        , mLinearSize(width * height)
    {
        Data = std::make_unique<TElement[]>(mLinearSize);
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

    std::unique_ptr<Buffer2D> MakeCopy() const
    {
        auto newData = std::make_unique<TElement[]>(mLinearSize);
        std::memcpy(newData.get(), Data.get(), mLinearSize * sizeof(TElement));

        return std::make_unique<Buffer2D>(
            Size,
            std::move(newData));
    }

    std::unique_ptr<Buffer2D> MakeCopy(_IntegralRect<TIntegralTag> const & regionRect)
    {
        auto newData = std::make_unique<TElement[]>(regionRect.size.width * regionRect.size.height);
        for (int targetY = 0; targetY < regionRect.size.height; ++targetY)
        {
            int const sourceLinearIndex = (targetY + regionRect.origin.y) * Size.width + regionRect.origin.x;
            int const targetLinearIndex = targetY * regionRect.size.width;

            std::memcpy(
                newData.get() + targetLinearIndex,
                Data.get() + sourceLinearIndex,
                regionRect.size.width * sizeof(TElement));
        }

        return std::make_unique<Buffer2D>(
            regionRect.size,
            std::move(newData));
    }

    void Blit(
        Buffer2D const & sourceRegion,
        _IntegralCoordinates<TIntegralTag> const & origin)
    {
        assert(origin.x + sourceRegion.Size.width <= Size.width);
        assert(origin.y + sourceRegion.Size.height <= Size.height);

        for (int sourceY = 0; sourceY < sourceRegion.Size.height; ++sourceY)
        {
            int const sourceLinearIndex = sourceY * sourceRegion.Size.width;
            int const targetLinearIndex = (origin.y + sourceY) * Size.width + origin.x;

            std::memcpy(
                Data.get() + targetLinearIndex,
                sourceRegion.Data.get() + sourceLinearIndex,
                sourceRegion.Size.width * sizeof(TElement));
        }
    }

private:

    size_t const mLinearSize;
};
