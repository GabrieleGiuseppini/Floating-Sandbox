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
 * Generic undo payload for multiple regions of the ship. Rules:
 * - Does *not* change the presence of layers
 * - Does *not* change the size of layers
 */

template<CoordinatesSpaceType TCoordinatesSpace>
class GenericUndoPayload final
{
private:

	class ShipSpaceRegionPayload final
	{
	public:

		ShipSpaceCoordinates Origin;

		std::optional<StructuralLayerData> StructuralLayerRegionBackup;
		std::optional<ElectricalLayerData> ElectricalLayerRegionBackup;
		std::optional<RopesLayerData> RopesLayerRegionBackup;
		std::optional<TextureLayerData> ExteriorTextureLayerRegionBackup;
		std::optional<TextureLayerData> InteriorTextureLayerRegionBackup;

		// Futurework: if needed, one day may add other elements, e.g. metadata

		ShipSpaceRegionPayload(ShipSpaceCoordinates const & origin)
			: Origin(origin)
		{
		}

		ShipSpaceRegionPayload(
			ShipSpaceCoordinates const & origin,
			std::optional<StructuralLayerData> && structuralLayerRegionBackup,
			std::optional<ElectricalLayerData > && electricalLayerRegionBackup,
			std::optional<RopesLayerData> && ropesLayerRegionBackup,
			std::optional<TextureLayerData> && exteriorTextureLayerRegionBackup,
			std::optional<TextureLayerData> && interiorTextureLayerRegionBackup)
			: Origin(origin)
			, StructuralLayerRegionBackup(std::move(structuralLayerRegionBackup))
			, ElectricalLayerRegionBackup(std::move(electricalLayerRegionBackup))
			, RopesLayerRegionBackup(std::move(ropesLayerRegionBackup))
			, ExteriorTextureLayerRegionBackup(std::move(exteriorTextureLayerRegionBackup))
			, InteriorTextureLayerRegionBackup(std::move(interiorTextureLayerRegionBackup))
		{
		}

		size_t GetTotalCost() const
		{
			return
				(StructuralLayerRegionBackup ? StructuralLayerRegionBackup->Buffer.GetByteSize() : 0)
				+ (ElectricalLayerRegionBackup ? ElectricalLayerRegionBackup->Buffer.GetByteSize() : 0)
				+ (RopesLayerRegionBackup ? RopesLayerRegionBackup->Buffer.GetByteSize() : 0)
				+ (ExteriorTextureLayerRegionBackup ? ExteriorTextureLayerRegionBackup->Buffer.GetByteSize() : 0)
				+ (InteriorTextureLayerRegionBackup ? InteriorTextureLayerRegionBackup->Buffer.GetByteSize() : 0);
		}

		std::vector<LayerType> GetAffectedLayers() const
		{
			std::vector<LayerType> affectedLayers;

			if (StructuralLayerRegionBackup.has_value())
			{
				affectedLayers.emplace_back(LayerType::Structural);
			}

			if (ElectricalLayerRegionBackup.has_value())
			{
				affectedLayers.emplace_back(LayerType::Electrical);
			}

			if (RopesLayerRegionBackup.has_value())
			{
				affectedLayers.emplace_back(LayerType::Ropes);
			}

			if (ExteriorTextureLayerRegionBackup.has_value())
			{
				affectedLayers.emplace_back(LayerType::ExteriorTexture);
			}

			if (InteriorTextureLayerRegionBackup.has_value())
			{
				affectedLayers.emplace_back(LayerType::InteriorTexture);
			}

			return affectedLayers;
		}
	};

	class TextureSpaceRegionPayload final
	{
	public:

		ImageCoordinates Origin;

		std::optional<TextureLayerData> ExteriorTextureLayerRegionBackup;
		std::optional<TextureLayerData> InteriorTextureLayerRegionBackup;

		TextureSpaceRegionPayload(ImageCoordinates const & origin)
			: Origin(origin)
		{
		}

		TextureSpaceRegionPayload(
			ImageCoordinates const & origin,
			std::optional<TextureLayerData> && exteriorTextureLayerRegionBackup,
			std::optional<TextureLayerData> && interiorTextureLayerRegionBackup)
			: Origin(origin)
			, ExteriorTextureLayerRegionBackup(std::move(exteriorTextureLayerRegionBackup))
			, InteriorTextureLayerRegionBackup(std::move(interiorTextureLayerRegionBackup))
		{
		}

		size_t GetTotalCost() const
		{
			return
				(ExteriorTextureLayerRegionBackup ? ExteriorTextureLayerRegionBackup->Buffer.GetByteSize() : 0)
				+ (InteriorTextureLayerRegionBackup ? InteriorTextureLayerRegionBackup->Buffer.GetByteSize() : 0);
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
		using payload_type = ShipSpaceRegionPayload;
	};

	template <>
	struct CoordinatesSpaceTraits<CoordinatesSpaceType::ExteriorTexture>
	{
		using integral_coords_tag = ImageTag;
		using payload_type = TextureSpaceRegionPayload;
	};

	template <>
	struct CoordinatesSpaceTraits<CoordinatesSpaceType::InteriorTexture>
	{
		using integral_coords_tag = ImageTag;
		using payload_type = TextureSpaceRegionPayload;
	};

	using TPayload = typename CoordinatesSpaceTraits<TCoordinatesSpace>::payload_type;

public:

	template<typename... TArgs>
	static GenericUndoPayload Make(TArgs &&... args)
	{
		GenericUndoPayload payload;
		payload.AddRegion(std::forward<TArgs>(args)...);
		return payload;
	}

	template <typename TFunctor>
	void ConsumeAll(TFunctor f)
	{
		for (size_t i = 0; i < mPayloads.size(); ++i)
		{
			f(std::move(mPayloads[i]));
		}
	}

	template<typename... TArgs>
	void AddRegion(TArgs &&... args)
	{
		mPayloads.emplace_back(std::forward<TArgs>(args)...);
	}

	size_t GetTotalCost() const
	{
		return std::accumulate(
			mPayloads.cbegin(),
			mPayloads.cend(),
			0,
			[](size_t a, TPayload const & b) -> size_t
			{
				return a + b.GetTotalCost();
			});
	}

private:

	std::vector<TPayload> mPayloads;
};

using GenericShipSpaceUndoPayload = GenericUndoPayload<CoordinatesSpaceType::Ship>;
using GenericExteriorTextureSpaceUndoPayload = GenericUndoPayload<CoordinatesSpaceType::ExteriorTexture>;
using GenericInteriorTextureSpaceUndoPayload = GenericUndoPayload<CoordinatesSpaceType::InteriorTexture>;

}