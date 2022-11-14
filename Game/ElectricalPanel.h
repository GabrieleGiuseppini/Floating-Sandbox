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
#include <optional>

class ElectricalPanel final
{
public:

	struct ElementMetadata
	{
		std::optional<IntegralCoordinates> PanelCoordinates;
		std::optional<std::string> Label;
		bool IsHidden;

		ElementMetadata(
			std::optional<IntegralCoordinates> panelCoordinates,
			std::optional<std::string> label,
			bool isHidden)
			: PanelCoordinates(std::move(panelCoordinates))
			, Label(std::move(label))
			, IsHidden(isHidden)
		{}
	};

	auto begin() const
	{
		return mMap.begin();
	}

	auto cbegin() const
	{
		return mMap.cbegin();
	}

	auto end() const
	{
		return mMap.end();
	}

	auto cend() const
	{
		return mMap.cend();
	}

	auto find(ElectricalElementInstanceIndex const instanceIndex) const
	{
		return mMap.find(instanceIndex);
	}

	bool Contains(ElectricalElementInstanceIndex const instanceIndex) const
	{
		return mMap.find(instanceIndex) != mMap.end();
	}

	ElementMetadata const & operator[](ElectricalElementInstanceIndex instanceIndex) const
	{
		assert(mMap.find(instanceIndex) != mMap.cend());
		return mMap.at(instanceIndex);
	}

	ElementMetadata & operator[](ElectricalElementInstanceIndex instanceIndex)
	{
		assert(mMap.find(instanceIndex) != mMap.end());
		return mMap.at(instanceIndex);
	}

	bool IsEmpty() const
	{
		return mMap.empty();
	}

	size_t GetSize() const
	{
		return mMap.size();
	}

	bool Add(
		ElectricalElementInstanceIndex instanceIndex,
		ElementMetadata metadata)
	{
		auto const [_, isInserted] = mMap.emplace(
			instanceIndex,
			std::move(metadata));

		return isInserted;
	}

	/*
	 * Adds the specified element, eventually clearing position information if it conflicts
	 * with the position of another element.
	 */
	void SafeAdd(
		ElectricalElementInstanceIndex instanceIndex,
		ElementMetadata metadata)
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

		bool const isInserted = Add(instanceIndex, std::move(metadata));
		assert(isInserted);
		(void)isInserted;
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

	std::map<ElectricalElementInstanceIndex, ElementMetadata> mMap;
};
