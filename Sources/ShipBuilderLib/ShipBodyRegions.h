/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <Simulation/Layers.h>

#include <Core/GameTypes.h>

#include <numeric>
#include <optional>
#include <vector>

namespace ShipBuilder {

/*
 * Container of an extract of a region from the ship.
 */

template<CoordinatesSpaceType TCoordinatesSpace>
class ShipBodyRegions final
{
public:

	class ShipSpaceBodyRegion final
	{
	public:

		ShipSpaceRect OriginatingRegion; // Region in whatever the source of this is, from which we got this extract; size matches size of our regions (when they're not full)

		std::optional<StructuralLayerData> StructuralLayerRegion;
		std::optional<ElectricalLayerData> ElectricalLayerRegion; // Panel is whole
		std::optional<RopesLayerData> RopesLayer; // Whole, for "hanging rope endpoints"
		std::optional<TextureLayerData> ExteriorTextureLayerRegion;
		std::optional<TextureLayerData> InteriorTextureLayerRegion;

		// Futurework: if needed, one day may add other elements, e.g. metadata

		ShipSpaceBodyRegion(ShipSpaceRect const & originatingRegion)
			: OriginatingRegion(originatingRegion)
		{
		}

		ShipSpaceBodyRegion(
			ShipSpaceRect const & originatingRegion,
			std::optional<StructuralLayerData> && structuralLayerRegion,
			std::optional<ElectricalLayerData > && electricalLayerRegion,
			std::optional<RopesLayerData> && ropesLayer,
			std::optional<TextureLayerData> && exteriorTextureLayerRegion,
			std::optional<TextureLayerData> && interiorTextureLayerRegion)
			: OriginatingRegion(originatingRegion)
			, StructuralLayerRegion(std::move(structuralLayerRegion))
			, ElectricalLayerRegion(std::move(electricalLayerRegion))
			, RopesLayer(std::move(ropesLayer))
			, ExteriorTextureLayerRegion(std::move(exteriorTextureLayerRegion))
			, InteriorTextureLayerRegion(std::move(interiorTextureLayerRegion))
		{
		}

		size_t GetTotalCost() const
		{
			return
				(StructuralLayerRegion ? StructuralLayerRegion->Buffer.GetByteSize() : 0)
				+ (ElectricalLayerRegion ? ElectricalLayerRegion->Buffer.GetByteSize() : 0)
				+ (RopesLayer ? RopesLayer->Buffer.GetByteSize() : 0)
				+ (ExteriorTextureLayerRegion ? ExteriorTextureLayerRegion->Buffer.GetByteSize() : 0)
				+ (InteriorTextureLayerRegion ? InteriorTextureLayerRegion->Buffer.GetByteSize() : 0);
		}

		std::vector<LayerType> GetAffectedLayers() const
		{
			std::vector<LayerType> affectedLayers;

			if (StructuralLayerRegion.has_value())
			{
				affectedLayers.emplace_back(LayerType::Structural);
			}

			if (ElectricalLayerRegion.has_value())
			{
				affectedLayers.emplace_back(LayerType::Electrical);
			}

			if (RopesLayer.has_value())
			{
				affectedLayers.emplace_back(LayerType::Ropes);
			}

			if (ExteriorTextureLayerRegion.has_value())
			{
				affectedLayers.emplace_back(LayerType::ExteriorTexture);
			}

			if (InteriorTextureLayerRegion.has_value())
			{
				affectedLayers.emplace_back(LayerType::InteriorTexture);
			}

			return affectedLayers;
		}
	};

	class TextureSpaceBodyRegion final
	{
	public:

		ImageRect OriginatingRegion; // Region in whatever the source of this is, from which we got this extract; size matches size of our regions (when they're not full)

		std::optional<TextureLayerData> ExteriorTextureLayerRegion;
		std::optional<TextureLayerData> InteriorTextureLayerRegion;

		TextureSpaceBodyRegion(ImageRect const & originatingRegion)
			: OriginatingRegion(originatingRegion)
		{
		}

		TextureSpaceBodyRegion(
			ImageRect const & originatingRegion,
			std::optional<TextureLayerData> && exteriorTextureLayerRegion,
			std::optional<TextureLayerData> && interiorTextureLayerRegion)
			: OriginatingRegion(originatingRegion)
			, ExteriorTextureLayerRegion(std::move(exteriorTextureLayerRegion))
			, InteriorTextureLayerRegion(std::move(interiorTextureLayerRegion))
		{
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