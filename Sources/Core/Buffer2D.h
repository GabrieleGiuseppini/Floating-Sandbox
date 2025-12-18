/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameTypes.h"

#include <algorithm>
#include <cstring>
#include <memory>

template <typename TElement, typename TIntegralTag>
struct Buffer2D
{
public:

    using element_type = TElement;
    using coordinates_type = _IntegralCoordinates<TIntegralTag>;
    using size_type = _IntegralSize<TIntegralTag>;

public:

    size_type Size;
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
        , mLinearSize(static_cast<size_t>(width) * static_cast<size_t>(height))
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
        Fill(defaultValue);
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

        return *this;
    }

    bool operator==(Buffer2D const & other) const noexcept
    {
        if (Size != other.Size)
        {
            return false;
        }

        return std::memcmp(Data.get(), other.Data.get(), GetByteSize()) == 0;
    }

    size_t GetLinearSize() const
    {
        return mLinearSize;
    }

    size_t GetByteSize() const
    {
        return mLinearSize * sizeof(TElement);
    }

    size_t Hash() const
    {
        size_t hash = 0;
        for (size_t i = 0; i < GetByteSize(); ++i)
        {
            hash += static_cast<size_t>((reinterpret_cast<uint8_t const *>(Data.get()))[i]) * 7 + 11;
        }

        return hash;
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

    Buffer2D Clone() const
    {
        auto newData = std::make_unique<TElement[]>(mLinearSize);

        std::memcpy(newData.get(), Data.get(), mLinearSize * sizeof(TElement));

        return Buffer2D(
            Size,
            std::move(newData));
    }

    void CloneFrom(Buffer2D const & other)
    {
        assert(Size == other.Size);

        std::memcpy(Data.get(), other.Data.get(), mLinearSize * sizeof(TElement));
    }

    Buffer2D CloneRegion(_IntegralRect<TIntegralTag> const & regionRect) const
    {
        // The requested region is entirely within this buffer
        assert(regionRect.IsContainedInRect(_IntegralRect<TIntegralTag>(_IntegralCoordinates<TIntegralTag>(0, 0), Size)));

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

        return Buffer2D(
            regionRect.size,
            std::move(newData));
    }

    void Fill(TElement const value)
    {
        std::fill(
            Data.get(),
            Data.get() + mLinearSize,
            value);
    }

    /*
     * In-place shrinking.
     */
    void Trim(_IntegralRect<TIntegralTag> const & rect)
    {
        // The requested region is entirely within this buffer
        assert(rect.IsContainedInRect(_IntegralRect<TIntegralTag>(_IntegralCoordinates<TIntegralTag>(0, 0), Size)));

        if (rect.size != Size)
        {
            for (int targetY = 0; targetY < rect.size.height; ++targetY)
            {
                int const sourceLinearIndex = (targetY + rect.origin.y) * Size.width + rect.origin.x;
                int const targetLinearIndex = targetY * rect.size.width;

                std::memmove(
                    Data.get() + targetLinearIndex,
                    Data.get() + sourceLinearIndex,
                    rect.size.width * sizeof(TElement));
            }

            Size = rect.size;
            mLinearSize = static_cast<size_t>(Size.width) * static_cast<size_t>(Size.height);
        }
        else
        {
            assert (rect.origin == _IntegralCoordinates<TIntegralTag>(0, 0));
        }
    }

    void BlitFromRegion(
        Buffer2D const & source,
        _IntegralRect<TIntegralTag> const & sourceRegion, // Expected to be contained in source buffer
        _IntegralCoordinates<TIntegralTag> const & targetPos) // Might be anywhere
    {
        BlitFromRegion(
            source,
            sourceRegion,
            targetPos,
            [](TElement const & src, TElement const &) -> TElement
            {
                return src;
            });
    }

    template<typename TOperator>
    void BlitFromRegion(
        Buffer2D const & source,
        _IntegralRect<TIntegralTag> const & sourceRegion, // Expected to be contained in source buffer
        _IntegralCoordinates<TIntegralTag> const & targetPos, // Might be anywhere
        TOperator const & elementOperator)
    {
        // The source region is completely contained in the source buffer
        assert(sourceRegion.IsContainedInRect({ {0, 0}, source.Size }));

        int const srcXStart = sourceRegion.origin.x + std::max(-targetPos.x, 0);
        int const tgtXStart = std::max(targetPos.x, 0);
        int const copyW = std::max(
            std::min(
                (sourceRegion.origin.x + sourceRegion.size.width) - srcXStart,
                Size.width - tgtXStart),
            0);

        int const srcYStart = sourceRegion.origin.y + std::max(-targetPos.y, 0);
        int const tgtYStart = std::max(targetPos.y, 0);
        int const copyH = std::max(
            std::min(
                (sourceRegion.origin.y + sourceRegion.size.height) - srcYStart,
                Size.height - tgtYStart),
            0);

        int sourceLinearIndex = srcYStart * source.Size.width + srcXStart;
        int targetLinearIndex = tgtYStart * Size.width + tgtXStart;

        for (int yc = 0; yc < copyH; ++yc)
        {
            for (int xc = 0; xc < copyW; ++xc)
            {
                Data[targetLinearIndex + xc] = elementOperator(source.Data[sourceLinearIndex + xc], Data[targetLinearIndex + xc]);
            }

            sourceLinearIndex += source.Size.width;
            targetLinearIndex += Size.width;
        }
    }

    Buffer2D MakeReframed(
        _IntegralSize<TIntegralTag> const & newSize, // Final size
        _IntegralCoordinates<TIntegralTag> const & originOffset, // Position in final buffer of original {0, 0}
        TElement const & fillerValue) const
    {
        auto newData = std::make_unique<TElement[]>(newSize.width * newSize.height);

        int const leftFillerWRange = originOffset.x;
        int const rightFillerWRange = std::min(originOffset.x + Size.width, newSize.width);

        int const leftFillerHRange = originOffset.y;
        int const rightFillerHRange = std::min(originOffset.y + Size.height, newSize.height);

        for (int ny = 0; ny < newSize.height; ++ny)
        {
            int const oldYOffset = (ny - leftFillerHRange) * Size.width;
            int const newYOffset = ny * newSize.width;

            for (int nx = 0; nx < newSize.width; ++nx)
            {
                _IntegralCoordinates<TIntegralTag> const coords(nx, ny);
                if (nx < leftFillerWRange || nx >= rightFillerWRange
                    || ny < leftFillerHRange || ny >= rightFillerHRange)
                {
                    newData[newYOffset + nx] = fillerValue;
                }
                else
                {
                    newData[newYOffset + nx] = Data[oldYOffset + nx - leftFillerWRange];
                }
            }
        }

        return Buffer2D(
            newSize,
            std::move(newData));
    }

    void Flip(DirectionType direction)
    {
        if (direction == DirectionType::Horizontal)
        {
            Flip<true, false>();
        }
        else if (direction == DirectionType::Vertical)
        {
            Flip<false, true>();
        }
        else if (direction == (DirectionType::Vertical | DirectionType::Horizontal))
        {
            Flip<true, true>();
        }
    }

    void Rotate90(RotationDirectionType direction)
    {
        switch (direction)
        {
            case RotationDirectionType::Clockwise:
            {
                Rotate90<RotationDirectionType::Clockwise>();
                break;
            }

            case RotationDirectionType::CounterClockwise:
            {
                Rotate90<RotationDirectionType::CounterClockwise>();
                break;
            }
        }
    }

    template<typename TNewElement, typename TFunctor>
    Buffer2D<TNewElement, TIntegralTag> Transform(TFunctor const & elementOperator) const
    {
        auto newData = std::make_unique<TNewElement[]>(mLinearSize);

        for (size_t i = 0; i < mLinearSize; ++i)
        {
            newData[i] = elementOperator(Data[i]);
        }

        return Buffer2D<TNewElement, TIntegralTag>(
            Size,
            std::move(newData));
    }

private:

    template<bool H, bool V>
    void Flip()
    {
        int const xMax = (H && !V) ? Size.width / 2 : Size.width;
        int const yMax = V ? Size.height / 2 : Size.height;

        for (int y = 0; y < yMax; ++y)
        {
            for (int x = 0; x < xMax; ++x)
            {
                auto const srcCoords = coordinates_type(x, y);

                auto dstCoords = srcCoords;
                if constexpr (H)
                    dstCoords = dstCoords.FlipX(Size.width);
                if constexpr (V)
                    dstCoords = dstCoords.FlipY(Size.height);

                std::swap(this->operator[](srcCoords), this->operator[](dstCoords));
            }
        }
    }

    template<RotationDirectionType TDirection>
    void Rotate90()
    {
        size_type const newSize(Size.height, Size.width);

        auto newData = std::make_unique<TElement[]>(mLinearSize);

        for (int srcY = 0; srcY < Size.height; ++srcY)
        {
            int const srcYOffset = srcY * Size.width;
            for (int srcX = 0; srcX < Size.width; ++srcX)
            {
                auto const dstCoords = coordinates_type(srcX, srcY).template Rotate90<TDirection>(Size);
                newData[dstCoords.y * newSize.width + dstCoords.x] = Data[srcYOffset + srcX];
            }
        }

        Size = newSize;
        Data = std::move(newData);
    }

    size_t mLinearSize;
};
