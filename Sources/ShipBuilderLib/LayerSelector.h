/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-11-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/GameTypes.h>

#include <initializer_list>
#include <type_traits>

namespace ShipBuilder {

struct LayerSelector
{
public:

	LayerSelector()
		: mLayerMask(0)
	{ }

	LayerSelector(std::initializer_list<LayerType> layers)
	{
		mLayerMask = 0;
		for (auto l : layers)
		{
			mLayerMask |= (1 << static_cast<integral_type>(l));
		}
	}

	bool IsEmpty() const
	{
		return mLayerMask == 0;
	}

	bool IsSet(LayerType layerType) const
	{
		return (mLayerMask & (1 << static_cast<integral_type>(layerType))) != 0;
	}

	void Add(LayerType layerType)
	{
		mLayerMask |= (1 << static_cast<integral_type>(layerType));
	}

private:

	using integral_type = typename std::underlying_type<LayerType>::type;

	static integral_type MakeMask(std::initializer_list<LayerType> layers)
	{
		integral_type layerMask = 0;
		for (auto l : layers)
		{
			layerMask = layerMask | (1 << static_cast<integral_type>(l));
		}

		return layerMask;
	}

	integral_type mLayerMask;
};

}