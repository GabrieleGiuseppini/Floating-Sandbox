/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-11-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "LayerElements.h"

#include <Core/GameTypes.h>

#include <algorithm>
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

    auto begin()
    {
        return mBuffer.begin();
    }

    auto end()
    {
        return mBuffer.end();
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

    bool HasEndpointAt(ShipSpaceCoordinates const & coords) const noexcept
    {
        return std::find_if(
            mBuffer.cbegin(),
            mBuffer.cend(),
            [&coords](RopeElement const & element) -> bool
            {
                return element.StartCoords == coords || element.EndCoords == coords;
            }) != mBuffer.cend();
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
    auto Erase(TIt const & it)
    {
        return mBuffer.erase(it);
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
     * Copies ropes that have *at least* one endpoint in the specified region,
     * re-origing coords of endpoints within the region.
     * Endpoints outside the region are mainted on their original coords.
     */
    RopeBuffer CloneRegion(ShipSpaceRect const & region) const
    {
        RopeBuffer newBuffer(region.size);

        ShipSpaceSize const offset(region.origin.x, region.origin.y);

        for (auto const & r : mBuffer)
        {
            if (r.StartCoords.IsInRect(region) || r.EndCoords.IsInRect(region))
            {
                newBuffer.EmplaceBack(
                    r.StartCoords.IsInRect(region) ? r.StartCoords - offset : r.StartCoords,
                    r.EndCoords.IsInRect(region) ? r.EndCoords - offset : r.EndCoords,
                    r.Material,
                    r.RenderColor);
            }
        }

        return newBuffer;
    }

    /*
     * Maintains coords of endpoints outside the blitted region.
     */
    void BlitFromRegion(
        RopeBuffer const & source,
        ShipSpaceRect const & sourceRegion, // Region in source that we want to blit; fully contained in source
        ShipSpaceCoordinates const & targetPos, // Absolute, might lay outside of ship space
        bool isTransparent)
    {
        assert(sourceRegion.IsContainedInRect(ShipSpaceRect(source.GetSize())));

        // Clear affected region first, if requested
        if (!isTransparent)
        {
            auto const targetPasteRegion =
                ShipSpaceRect(
                    targetPos,
                    sourceRegion.size)
                .MakeIntersectionWith(ShipSpaceRect(mSize));

            if (targetPasteRegion)
            {
                for (auto tgtIt = mBuffer.begin(); tgtIt != mBuffer.end(); )
                {
                    if (tgtIt->StartCoords.IsInRect(*targetPasteRegion) || tgtIt->EndCoords.IsInRect(*targetPasteRegion))
                    {
                        tgtIt = mBuffer.erase(tgtIt);
                    }
                    else
                    {
                        ++tgtIt;
                    }
                }
            }
        }

        // Now copy
        for (auto const & r : source.mBuffer)
        {
            // Only copy source ropes that have at least one endpoint in source region
            if (r.StartCoords.IsInRect(sourceRegion) || r.EndCoords.IsInRect(sourceRegion))
            {
                // Translate coords:
                //  - Endpoint inside source region: translate to new pos, and clamp to our space
                //  - Endpoint outside source region: maintain their value, but clamp to our space

                ShipSpaceCoordinates const startCoordsInTarget = (
                    r.StartCoords.IsInRect(sourceRegion)
                    ? (r.StartCoords - ShipSpaceSize(sourceRegion.origin.x, sourceRegion.origin.y) + ShipSpaceSize(targetPos.x, targetPos.y))
                    : r.StartCoords).Clamp(mSize);

                ShipSpaceCoordinates const endCoordsInTarget = (
                    r.EndCoords.IsInRect(sourceRegion)
                    ? (r.EndCoords - ShipSpaceSize(sourceRegion.origin.x, sourceRegion.origin.y) + ShipSpaceSize(targetPos.x, targetPos.y))
                    : r.EndCoords).Clamp(mSize);

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
