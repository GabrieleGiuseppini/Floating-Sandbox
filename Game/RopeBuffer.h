/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-11-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "LayerElements.h"

#include <GameCore/GameTypes.h>

#include <vector>

struct RopeBuffer
{
    explicit RopeBuffer(ShipSpaceSize const & size)
        : mSize(size)
        , mBuffer()
    {}

    ShipSpaceSize GetSize() const
    {
        return mSize;
    }

    size_t GetElementCount() const
    {
        return mBuffer.size();
    }

    size_t GetByteSize() const
    {
        return mBuffer.size() * sizeof(RopeElement);
    }

    auto begin() const
    {
        return mBuffer.begin();
    }

    auto end() const
    {
        return mBuffer.end();
    }

    auto cbegin() const
    {
        return mBuffer.cbegin();
    }

    auto cend() const
    {
        return mBuffer.cend();
    }

    RopeElement const & operator[](size_t index) const noexcept
    {
        return mBuffer[index];
    }

    RopeElement & operator[](size_t index) noexcept
    {
        return mBuffer[index];
    }

    StructuralMaterial const * SampleMaterialEndpointAt(ShipSpaceCoordinates const & endpointCoords) const noexcept
    {
        for (auto const & ropeElement : mBuffer)
        {
            if (ropeElement.StartCoords == endpointCoords || ropeElement.EndCoords == endpointCoords)
            {
                return ropeElement.Material;
            }
        }

        return nullptr;
    }

    template<typename ... TArgs>
    void EmplaceBack(TArgs ... args)
    {
        mBuffer.emplace_back(std::forward<TArgs>(args) ...);
    }

    template<typename TIt>
    void Erase(TIt const & it)
    {
        mBuffer.erase(it);
    }

    void Clear()
    {
        mBuffer.clear();
    }

    RopeBuffer Clone() const
    {
        return RopeBuffer(mSize, mBuffer);
    }

    /*
     * Copies ropes that have *both* endpoints in the specified region.
     */
    RopeBuffer CloneRegion(ShipSpaceRect const & region) const
    {
        RopeBuffer newBuffer(mSize, mBuffer);
        newBuffer.Reframe(
            region.size,
            ShipSpaceCoordinates(
                -region.origin.x,
                -region.origin.y));

        return newBuffer;
    }

    /*
     * Copies ropes that have *at least* one endpoint in the specified region.
     */
    RopeBuffer CopyRegion(ShipSpaceRect const & region) const
    {
        RopeBuffer newBuffer(region.size);

        ShipSpaceSize const offset(region.origin.x, region.origin.y);

        for (auto const & r : mBuffer)
        {
            if (r.StartCoords.IsInRect(region) || r.EndCoords.IsInRect(region))
            {
                newBuffer.EmplaceBack(
                    r.StartCoords - offset,
                    r.EndCoords - offset,
                    r.Material,
                    r.RenderColor);
            }
        }

        return newBuffer;
    }

    void BlitFromRegion(
        RopeBuffer const & source,
        ShipSpaceRect const & sourceRegion,
        ShipSpaceCoordinates const & targetPos,
        bool isTransparent)
    {
        // Clear affected region first, if requested
        if (!isTransparent)
        {
            ShipSpaceRect const targetPasteRegion(
                targetPos,
                sourceRegion.size);

            for (auto tgtIt = mBuffer.begin(); tgtIt != mBuffer.end(); )
            {
                if (tgtIt->StartCoords.IsInRect(targetPasteRegion) || tgtIt->EndCoords.IsInRect(targetPasteRegion))
                {
                    tgtIt = mBuffer.erase(tgtIt);
                }
                else
                {
                    ++tgtIt;
                }
            }
        }

        // Now copy
        for (auto const & r : source.mBuffer)
        {
            // Only copy source ropes that have at least one endpoint in source region
            if (r.StartCoords.IsInRect(sourceRegion) || r.EndCoords.IsInRect(sourceRegion))
            {
                // Translate coords
                ShipSpaceCoordinates startCoordsInTarget = targetPos + (r.StartCoords - sourceRegion.origin);
                ShipSpaceCoordinates endCoordsInTarget = targetPos + (r.EndCoords - sourceRegion.origin);

                // Make sure translated coords are inside our size
                if (startCoordsInTarget.IsInSize(mSize) && endCoordsInTarget.IsInSize(mSize))
                {
                    // Remove all ropes in target that share an endpoint with this rope
                    for (auto tgtIt = mBuffer.begin(); tgtIt != mBuffer.end(); )
                    {
                        if (tgtIt->StartCoords == startCoordsInTarget
                            || tgtIt->StartCoords == endCoordsInTarget
                            || tgtIt->EndCoords == startCoordsInTarget
                            || tgtIt->EndCoords == endCoordsInTarget)
                        {
                            tgtIt = mBuffer.erase(tgtIt);
                        }
                        else
                        {
                            ++tgtIt;
                        }
                    }

                    // Store
                    mBuffer.emplace_back(
                        startCoordsInTarget,
                        endCoordsInTarget,
                        r.Material,
                        r.RenderColor);
                }
            }
        }
    }

    void EraseRegion(ShipSpaceRect const & region)
    {
        for (auto it = mBuffer.begin(); it != mBuffer.end(); /* incremented in loop */)
        {
            if (it->StartCoords.IsInRect(region) || it->EndCoords.IsInRect(region))
            {
                it = mBuffer.erase(it);
            }
            else
            {
                ++it;
            }
        }
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
        if (direction == RotationDirectionType::Clockwise)
        {
            Rotate90<RotationDirectionType::Clockwise>();
        }
        else
        {
            assert(direction == RotationDirectionType::CounterClockwise);
            Rotate90<RotationDirectionType::CounterClockwise>();
        }
    }

    void Trim(
        ShipSpaceCoordinates const & origin,
        ShipSpaceSize const & size)
    {
        Reframe(
            size,
            ShipSpaceCoordinates(-origin.x, -origin.y));
    }

    void Reframe(
        ShipSpaceSize const & newSize, // Final size
        ShipSpaceCoordinates const & originOffset) // Position in final buffer of original {0, 0}
    {
        ShipSpaceSize coordsOffset(originOffset.x, originOffset.y);

        for (auto it = mBuffer.begin(); it != mBuffer.end(); /* incremented in loop */)
        {
            // Shift
            auto const newStartCoords = it->StartCoords + coordsOffset;
            auto const newEndCoords = it->EndCoords + coordsOffset;
            if (!newStartCoords.IsInSize(newSize)
                || !newEndCoords.IsInSize(newSize))
            {
                it = mBuffer.erase(it);
            }
            else
            {
                it->StartCoords = newStartCoords;
                it->EndCoords = newEndCoords;

                ++it;
            }
        }

        mSize = newSize;
    }

private:

    RopeBuffer(
        ShipSpaceSize const & size,
        std::vector<RopeElement> buffer)
        : mSize(size)
        , mBuffer(std::move(buffer))
    {}

    template<bool H, bool V>
    inline void Flip()
    {
        for (auto & element : mBuffer)
        {
            auto startCoords = element.StartCoords;
            if constexpr (H)
                startCoords = startCoords.FlipX(mSize.width);
            if constexpr (V)
                startCoords = startCoords.FlipY(mSize.height);
            element.StartCoords = startCoords;

            auto endCoords = element.EndCoords;
            if constexpr (H)
                endCoords = endCoords.FlipX(mSize.width);
            if constexpr (V)
                endCoords = endCoords.FlipY(mSize.height);
            element.EndCoords = endCoords;
        }
    }

    template<RotationDirectionType TDirection>
    inline void Rotate90()
    {
        for (auto & element : mBuffer)
        {
            element.StartCoords = element.StartCoords.Rotate90<TDirection>(mSize);
            element.EndCoords = element.EndCoords.Rotate90<TDirection>(mSize);
        }

        mSize = ShipSpaceSize(mSize.height, mSize.width);
    }

    ShipSpaceSize mSize;
    std::vector<RopeElement> mBuffer;
};
