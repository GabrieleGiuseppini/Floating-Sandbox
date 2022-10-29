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
    RopeBuffer()
    {}

    size_t GetSize() const
    {
        return mBuffer.size();
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
        return RopeBuffer(mBuffer);
    }

    /*
     * Copies ropes that have *both* endpoints in the specified region.
     */
    RopeBuffer CloneRegion(ShipSpaceRect const & region) const
    {
        RopeBuffer newBuffer = mBuffer;
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
        RopeBuffer newBuffer;

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

    void Flip(
        DirectionType direction,
        ShipSpaceSize const & size)
    {
        if (direction == DirectionType::Horizontal)
        {
            Flip<true, false>(size);
        }
        else if (direction == DirectionType::Vertical)
        {
            Flip<false, true>(size);
        }
        else if (direction == (DirectionType::Vertical | DirectionType::Horizontal))
        {
            Flip<true, true>(size);
        }
    }

    void Rotate90(
        RotationDirectionType direction,
        ShipSpaceSize const & size)
    {
        if (direction == RotationDirectionType::Clockwise)
        {
            Rotate90<RotationDirectionType::Clockwise>(size);
        }
        else
        {
            assert(direction == RotationDirectionType::CounterClockwise);
            Rotate90<RotationDirectionType::CounterClockwise>(size);
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
    }

private:

    RopeBuffer(std::vector<RopeElement> buffer)
        : mBuffer(std::move(buffer))
    {}

    template<bool H, bool V>
    inline void Flip(ShipSpaceSize const & size)
    {
        for (auto & element : mBuffer)
        {
            auto startCoords = element.StartCoords;
            if constexpr (H)
                startCoords = startCoords.FlipX(size.width);
            if constexpr (V)
                startCoords = startCoords.FlipY(size.height);
            element.StartCoords = startCoords;

            auto endCoords = element.EndCoords;
            if constexpr (H)
                endCoords = endCoords.FlipX(size.width);
            if constexpr (V)
                endCoords = endCoords.FlipY(size.height);
            element.EndCoords = endCoords;
        }
    }

    template<RotationDirectionType TDirection>
    inline void Rotate90(ShipSpaceSize const & size)
    {
        for (auto & element : mBuffer)
        {
            element.StartCoords = element.StartCoords.Rotate90<TDirection>(size);
            element.EndCoords = element.EndCoords.Rotate90<TDirection>(size);
        }
    }

    std::vector<RopeElement> mBuffer;
};
