/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <Simulation/Layers.h>

#include <Core/GameTypes.h>

#include <cassert>
#include <memory>
#include <numeric>
#include <vector>

namespace ShipBuilder {

/*
 * Container of an extract of a region from the ship.
 *
 * Suitable for full restore:
 * - Contains whole original ElectricalPanel
 * - Contains whole RopesBuffer, for "hangin rope endpoint"
 */

template<CoordinatesSpaceType TCoordinatesSpace>
class ShipBodyRegions final
{
public:

	class ShipSpaceBodyRegion final
	{
	public:

		ShipSpaceRect OriginatingRegion; // Region in whatever the source of this is, from which we got this extract; size matches size of our regions (when they're not full)

		ShipLayers LayerRegions; // ElectricalPanel is whole; RopeLayer is whole

		// Futurework: if needed, one day may add other elements, e.g. metadata

		ShipSpaceBodyRegion() // Empty
			: OriginatingRegion(ShipSpaceSize(0, 0))
			, LayerRegions(ShipSpaceSize(0, 0))
		{ }

		ShipSpaceBodyRegion(
			ShipSpaceRect const & originatingRegion,
			ShipLayers && layerRegions)
			: OriginatingRegion(originatingRegion)
			, LayerRegions(std::move(layerRegions))
		{
		}

		ShipSpaceBodyRegion(
			ShipSpaceRect const & originatingRegion,
			std::unique_ptr<StructuralLayerData> && structuralLayerRegion,
			std::unique_ptr<ElectricalLayerData > && electricalLayerRegion,
			std::unique_ptr<RopesLayerData> && ropesLayer,
			std::unique_ptr<TextureLayerData> && exteriorTextureLayerRegion,
			std::unique_ptr<TextureLayerData> && interiorTextureLayerRegion)
			: OriginatingRegion(originatingRegion)
			, LayerRegions(
				originatingRegion.size,
				std::move(structuralLayerRegion),
				std::move(electricalLayerRegion),
				std::move(ropesLayer),
				std::move(exteriorTextureLayerRegion),
				std::move(interiorTextureLayerRegion))
		{
		}

		ShipSpaceSize GetSize() const
		{
			assert(OriginatingRegion.size == LayerRegions.Size);
			return OriginatingRegion.size;
		}

		size_t GetTotalCost() const
		{
			return
				(LayerRegions.StructuralLayer ? LayerRegions.StructuralLayer->Buffer.GetByteSize() : 0)
				+ (LayerRegions.ElectricalLayer ? LayerRegions.ElectricalLayer->Buffer.GetByteSize() : 0)
				+ (LayerRegions.RopesLayer ? LayerRegions.RopesLayer->Buffer.GetByteSize() : 0)
				+ (LayerRegions.ExteriorTextureLayer ? LayerRegions.ExteriorTextureLayer->Buffer.GetByteSize() : 0)
				+ (LayerRegions.InteriorTextureLayer ? LayerRegions.InteriorTextureLayer->Buffer.GetByteSize() : 0);
		}

		std::vector<LayerType> GetAffectedLayers() const
		{
			std::vector<LayerType> affectedLayers;

			if (LayerRegions.StructuralLayer)
			{
				affectedLayers.emplace_back(LayerType::Structural);
			}

			if (LayerRegions.ElectricalLayer)
			{
				affectedLayers.emplace_back(LayerType::Electrical);
			}

			if (LayerRegions.RopesLayer)
			{
				affectedLayers.emplace_back(LayerType::Ropes);
			}

			if (LayerRegions.ExteriorTextureLayer)
			{
				affectedLayers.emplace_back(LayerType::ExteriorTexture);
			}

			if (LayerRegions.InteriorTextureLayer)
			{
				affectedLayers.emplace_back(LayerType::InteriorTexture);
			}

			return affectedLayers;
		}

		ShipSpaceBodyRegion Clone() const
		{
			return ShipSpaceBodyRegion(
				OriginatingRegion,
				LayerRegions.Clone());
		}
	};

	class TextureSpaceBodyRegion final
	{
	public:

		ImageRect OriginatingRegion; // Region in whatever the source of this is, from which we got this extract; size matches size of our regions (when they're not full)

		std::unique_ptr<TextureLayerData> ExteriorTextureLayerRegion;
		std::unique_ptr<TextureLayerData> InteriorTextureLayerRegion;

		TextureSpaceBodyRegion(
			ImageRect const & originatingRegion,
			std::unique_ptr<TextureLayerData> && exteriorTextureLayerRegion,
			std::unique_ptr<TextureLayerData> && interiorTextureLayerRegion)
			: OriginatingRegion(originatingRegion)
			, ExteriorTextureLayerRegion(std::move(exteriorTextureLayerRegion))
			, InteriorTextureLayerRegion(std::move(interiorTextureLayerRegion))
		{
		}

		ImageSize GetSize() const
		{
			return OriginatingRegion.size;
		}

		size_t GetTotalCost() const
		{
			return
				(ExteriorTextureLayerRegion ? ExteriorTextureLayerRegion->Buffer.GetByteSize() : 0)
				+ (InteriorTextureLayerRegion ? InteriorTextureLayerRegion->Buffer.GetByteSize() : 0);
		}
	};

	template <CoordinatesSpaceType _TCoordinatesSpace>
	struct CoordinatesSpaceTraits
	{
	};

	template <>
	struct CoordinatesSpaceTraits<CoordinatesSpaceType::Ship>
	{
		using integral_coords_tag = ShipSpaceTag;
		using payload_type = ShipSpaceBodyRegion;
	};

	template <>
	struct CoordinatesSpaceTraits<CoordinatesSpaceType::ExteriorTexture>
	{
		using integral_coords_tag = ImageSpaceTag;
		using payload_type = TextureSpaceBodyRegion;
	};

	template <>
	struct CoordinatesSpaceTraits<CoordinatesSpaceType::InteriorTexture>
	{
		using integral_coords_tag = ImageSpaceTag;
		using payload_type = TextureSpaceBodyRegion;
	};

	using TPayload = typename CoordinatesSpaceTraits<TCoordinatesSpace>::payload_type;
	using payload_type = TPayload;

public:

	template<typename... TArgs>
	static ShipBodyRegions Make(TArgs &&... args)
	{
		ShipBodyRegions regions;
		regions.AddRegion(std::forward<TArgs>(args)...);
		return regions;
	}

	template <typename TFunctor>
	void ConsumeAll(TFunctor f)
	{
		for (size_t i = 0; i < mRegions.size(); ++i)
		{
			f(std::move(mRegions[i]));
		}
	}

	template<typename... TArgs>
	void AddRegion(TArgs &&... args)
	{
		mRegions.emplace_back(std::forward<TArgs>(args)...);
	}

	size_t GetTotalCost() const
	{
		return std::accumulate(
			mRegions.cbegin(),
			mRegions.cend(),
			0,
			[](size_t a, TPayload const & b) -> size_t
			{
				return a + b.GetTotalCost();
			});
	}

private:

	std::vector<TPayload> mRegions;
};

using ShipSpaceBodyRegions = ShipBodyRegions<CoordinatesSpaceType::Ship>;
using ExteriorTextureSpaceBodyRegions = ShipBodyRegions<CoordinatesSpaceType::ExteriorTexture>;
using InteriorTextureSpaceBodyRegions = ShipBodyRegions<CoordinatesSpaceType::InteriorTexture>;

}