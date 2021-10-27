/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameTypes.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace ShipBuilder {

class ElectricalElementInstanceIndexFactory
{
public:

    ElectricalElementInstanceIndexFactory()
        : mFirstFreeInstanceIndex()
    {}

    ElectricalElementInstanceIndex MakeNewIndex()
    {
        size_t freeIndex;
        if (mFirstFreeInstanceIndex.has_value())
        {
            // Use first free index
            assert(*mFirstFreeInstanceIndex < mInstanceIndices.size());
            freeIndex = *mFirstFreeInstanceIndex;

            // Update free index
            mFirstFreeInstanceIndex = FindNextFreeIndex(*mFirstFreeInstanceIndex + 1);
        }
        else
        {
            // Append new index
            freeIndex = mInstanceIndices.size();
            mInstanceIndices.push_back(false);
        }

        // Occupy new index
        assert(mInstanceIndices[freeIndex] == false);
        mInstanceIndices[freeIndex] = true;

        return static_cast<ElectricalElementInstanceIndex>(freeIndex);
    }

    void RegisterIndex(ElectricalElementInstanceIndex instanceIndex)
    {
        size_t const index = static_cast<size_t>(instanceIndex);

        // Make sure there's room
        if (index >= mInstanceIndices.size())
        {
            mInstanceIndices.resize(index + 1, false);
        }

        // Occupy index
        assert(mInstanceIndices[index] == false);
        mInstanceIndices[index] = true;

        mFirstFreeInstanceIndex = FindNextFreeIndex(0);
    }

    void DisposeIndex(ElectricalElementInstanceIndex instanceIndex)
    {
        size_t const index = static_cast<size_t>(instanceIndex);

        // Free index
        assert(index < mInstanceIndices.size());
        mInstanceIndices[index] = false;

        // Update first free index
        if (!mFirstFreeInstanceIndex.has_value()
            || index < *mFirstFreeInstanceIndex)
        {
            mFirstFreeInstanceIndex = index;
        }
    }

    void Reset()
    {
        mInstanceIndices.clear();
        mFirstFreeInstanceIndex.reset();
    }

private:

    std::optional<size_t> FindNextFreeIndex(size_t startValue)
    {
        for (;startValue < mInstanceIndices.size(); ++startValue)
        {
            if (!mInstanceIndices[startValue])
            {
                // Found!
                return startValue;
            }
        }

        return std::nullopt;
    }

    std::vector<bool> mInstanceIndices; // True if in use, false othewise
    std::optional<size_t> mFirstFreeInstanceIndex;
};

}