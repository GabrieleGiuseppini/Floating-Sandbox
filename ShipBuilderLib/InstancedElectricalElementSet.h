/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/Materials.h>

#include <GameCore/GameTypes.h>

#include <cstdint>
#include <map>

namespace ShipBuilder {

class InstancedElectricalElementSet
{
public:

    InstancedElectricalElementSet()
        : mFirstFreeInstanceIndex(0)
    {}

    auto const & GetElements() const
    {
        return mInstanceMap;
    }

    bool IsRegistered(ElectricalElementInstanceIndex instanceIndex) const
    {
        return mInstanceMap.count(instanceIndex) != 0;
    }

    ElectricalElementInstanceIndex Add(ElectricalMaterial const * material)
    {
        assert(nullptr != material);
        assert(material->IsInstanced);

        // Assign instance index
        ElectricalElementInstanceIndex newIndex = mFirstFreeInstanceIndex;

        // Store instance
        assert(mInstanceMap.count(newIndex) == 0);
        auto const [insertIt, isInserted] = mInstanceMap.try_emplace(newIndex, material);
        assert(isInserted);

        // Find next index
        RecalculateNextFreeIndex(insertIt);

        return newIndex;
    }

    ElectricalElementInstanceIndex Register(ElectricalElementInstanceIndex instanceIndex, ElectricalMaterial const * material)
    {
        assert(mInstanceMap.count(instanceIndex) == 0);
        assert(nullptr != material);
        assert(material->IsInstanced);

        // Store instance
        auto const [insertIt, isInserted] = mInstanceMap.try_emplace(instanceIndex, material);
        assert(isInserted);

        // Find next index
        if (instanceIndex <= mFirstFreeInstanceIndex)
        {
            assert(instanceIndex == mFirstFreeInstanceIndex);
            RecalculateNextFreeIndex(insertIt);
        }

        return instanceIndex;
    }

    void Remove(ElectricalElementInstanceIndex instanceIndex)
    {
        auto it = mInstanceMap.find(instanceIndex);
        assert(it != mInstanceMap.end());

        auto const freedIndex = it->first;

        mInstanceMap.erase(it);

        if (freedIndex < mFirstFreeInstanceIndex)
        {
            mFirstFreeInstanceIndex = freedIndex;
        }
    }

    void Reset()
    {
        mInstanceMap.clear();
        mFirstFreeInstanceIndex = 0;
    }

private:

    template<typename TIterator>
    void RecalculateNextFreeIndex(TIterator it)
    {
        // Assumption: all indices before the instance index at the iterator are in-use;
        // so we just search ahead
        auto highestKnownUsedIndex = it->first;
        for (++it; it != mInstanceMap.end(); ++it)
        {
            assert(it->first > highestKnownUsedIndex);

            if (it->first > highestKnownUsedIndex + 1)
            {
                // Found a gap here
                break;
            }
            else
            {
                assert(it->first == highestKnownUsedIndex + 1);
                ++highestKnownUsedIndex;
            }
        }

        mFirstFreeInstanceIndex = highestKnownUsedIndex + 1;
    }

    std::map<ElectricalElementInstanceIndex, ElectricalMaterial const *> mInstanceMap;
    ElectricalElementInstanceIndex mFirstFreeInstanceIndex;
};

}