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

    RopeBuffer Clone() const
    {
        return RopeBuffer(mBuffer);
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

    std::vector<RopeElement> mBuffer;
};
