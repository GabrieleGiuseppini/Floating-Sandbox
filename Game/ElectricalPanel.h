/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-11-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameTypes.h>

#include <algorithm>
#include <cassert>
#include <map>

class ElectricalPanel final
{
public:

	auto cbegin() const
	{
		return mMap.cbegin();
	}

	auto cend() const
	{
		return mMap.cend();
	}

	size_t GetSize() const
	{
		return mMap.size();
	}

	/*
	 * Adds the specified element, eventually clearing position information if it conflicts
	 * with the position of another element.
	 */
	void Add(
		ElectricalElementInstanceIndex instanceIndex,
		ElectricalPanelElementMetadata metadata)
	{
		// Find if position is occupied
		auto const it = std::find_if(
			mMap.begin(),
			mMap.end(),
			[&metadata](auto const & entry) -> bool
			{
				return entry.second.PanelCoordinates.has_value()
				&& metadata.PanelCoordinates.has_value()
				&& *entry.second.PanelCoordinates == metadata.PanelCoordinates;
			});

		if (it != mMap.end())
		{
			metadata.PanelCoordinates.reset();
		}

		auto const res = mMap.emplace(
			instanceIndex,
			std::move(metadata));

		assert(res.second);
	}

	void Remove(ElectricalElementInstanceIndex instanceIndex)
	{
		auto const it = mMap.find(instanceIndex);
		if (it != mMap.end())
		{
			mMap.erase(it);
		}
	}

	void Clear()
	{
		mMap.clear();
	}

private:

	std::map<ElectricalElementInstanceIndex, ElectricalPanelElementMetadata> mMap;
};
