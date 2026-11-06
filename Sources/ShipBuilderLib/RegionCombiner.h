/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-11-06
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Core/GameTypes.h>

#include <cassert>
#include <vector>

namespace ShipBuilder {

/*
 * Guarantees non-overlapping regions, trying to minimize waste.
 */
template<typename TCoordsTag>
class RegionCombiner final
{
public:

	using TCoords = typename _IntegralCoordinates<TCoordsTag>;
	using TRect = typename _IntegralRect<TCoordsTag>;

	void AddRegion(TRect const & region)
	{
		InternalAddRegion(region, 0);
	}

	size_t GetRegionCount() const
	{
		return mRegionInfos.size();
	}

	TRect const & GetRegionAt(size_t index) const
	{
		assert(index < mRegionInfos.size());
		return mRegionInfos[index].Region;
	}

private:

	void InternalAddRegion(TRect const & region, typename TCoords::integral_type preWastedArea)
	{
		// Check whether it overlaps with any
		for (auto it = mRegionInfos.begin(); it != mRegionInfos.end(); ++it)
		{
			auto intersection = it->Region.MakeIntersectionWith(region);
			if (intersection)
			{
				// Extract
				auto otherRegionInfo = *it;
				mRegionInfos.erase(it);

				// Combine
				InternalAddRegion(otherRegionInfo.Region.MakeUnionWith(region), otherRegionInfo.WastedArea + preWastedArea);

				// Done
				return;
			}
		}

		// Non-adjacent, combine if makes sense
		for (auto it = mRegionInfos.begin(); it != mRegionInfos.end(); ++it)
		{
			assert(!it->Region.MakeIntersectionWith(region));

			auto const sumWastedArea = it->WastedArea + preWastedArea;

			auto const unionRegion = it->Region.MakeUnionWith(region);
			auto const combinedWasteArea = sumWastedArea + (unionRegion.CalculateArea() - it->Region.CalculateArea() - region.CalculateArea());

			if (combinedWasteArea <= unionRegion.CalculateArea() / 2) // combinedWasteArea <= useful
			{
				// Extract
				auto otherRegionInfo = *it;
				mRegionInfos.erase(it);

				// Combine
				InternalAddRegion(otherRegionInfo.Region.MakeUnionWith(region), combinedWasteArea);

				// Done
				return;
			}
		}

		// Just add
		mRegionInfos.emplace_back(RegionInfo{ region, preWastedArea });
	}

	struct RegionInfo
	{
		TRect Region;
		typename TCoords::integral_type WastedArea;
	};

	std::vector<RegionInfo> mRegionInfos;
};

}