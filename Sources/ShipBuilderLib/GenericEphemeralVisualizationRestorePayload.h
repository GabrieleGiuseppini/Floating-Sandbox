/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-11-06
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <Simulation/Layers.h>

#include <Core/GameTypes.h>

#include <optional>
#include <vector>

namespace ShipBuilder {

/*
 * Generic backup data to restore an ephemeral visualization.
 */

template<CoordinatesSpaceType TCoordinatesSpace>
class GenericEphemeralVisualizationRestorePayload final
{
public:

	class ShipSpaceRegionPayload final
	{
	public:
		ShipSpaceCoordinates Origin;

		std::optional<typename LayerTypeTraits<LayerType::Structural>::buffer_type> StructuralLayerBufferRegion;
		std::optional<typename LayerTypeTraits<LayerType::Electrical>::buffer_type> ElectricalLayerBufferRegion;
		std::optional<typename LayerTypeTraits<LayerType::Ropes>::buffer_type> RopesLayerBuffer; // Whole buffer
		std::optional<typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type> ExteriorTextureLayerBufferRegion;
		std::optional<typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type> InteriorTextureLayerBufferRegion;

		ShipSpaceRegionPayload(ShipSpaceCoordinates const & origin)
			: Origin(origin)
		{
		}

		ShipSpaceRegionPayload(
			ShipSpaceCoordinates const & origin,
			std::optional<typename LayerTypeTraits<LayerType::Structural>::buffer_type> && structuralLayerBufferRegion,
			std::optional<typename LayerTypeTraits<LayerType::Electrical>::buffer_type > && electricalLayerBufferRegion,
			std::optional<typename LayerTypeTraits<LayerType::Ropes>::buffer_type> && ropesLayerBuffer,
			std::optional<typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type> && exteriorTextureLayerBufferRegion,
			std::optional<typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type> && interiorTextureLayerBufferRegion)
			: Origin(origin)
			, StructuralLayerBufferRegion(std::move(structuralLayerBufferRegion))
			, ElectricalLayerBufferRegion(std::move(electricalLayerBufferRegion))
			, RopesLayerBuffer(std::move(ropesLayerBuffer))
			, ExteriorTextureLayerBufferRegion(std::move(exteriorTextureLayerBufferRegion))
			, InteriorTextureLayerBufferRegion(std::move(interiorTextureLayerBufferRegion))
		{
		}

		std::vector<LayerType> GetAffectedLayers() const
		{
			std::vector<LayerType> affectedLayers;

			if (StructuralLayerBufferRegion.has_value())
			{
				affectedLayers.emplace_back(LayerType::Structural);
			}

			if (ElectricalLayerBufferRegion.has_value())
			{
				affectedLayers.emplace_back(LayerType::Electrical);
			}

			if (RopesLayerBuffer.has_value())
			{
				affectedLayers.emplace_back(LayerType::Ropes);
			}

			if (ExteriorTextureLayerBufferRegion.has_value())
			{
				affectedLayers.emplace_back(LayerType::ExteriorTexture);
			}

			if (InteriorTextureLayerBufferRegion.has_value())
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

		std::optional<typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type> ExteriorTextureLayerBufferRegion;
		std::optional<typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type> InteriorTextureLayerBufferRegion;

		TextureSpaceRegionPayload(ImageCoordinates const & origin)
			: Origin(origin)
		{
		}

		TextureSpaceRegionPayload(
			ImageCoordinates const & origin,
			std::optional<typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type> && exteriorTextureLayerBufferRegion,
			std::optional<typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type> && interiorTextureLayerBufferRegion)
			: Origin(origin)
			, ExteriorTextureLayerBufferRegion(std::move(exteriorTextureLayerBufferRegion))
			, InteriorTextureLayerBufferRegion(std::move(interiorTextureLayerBufferRegion))
		{
		}

		size_t GetTotalCost() const
		{
			return
				(ExteriorTextureLayerBufferRegion ? ExteriorTextureLayerBufferRegion->Buffer.GetByteSize() : 0)
				+ (InteriorTextureLayerBufferRegion ? InteriorTextureLayerBufferRegion->Buffer.GetByteSize() : 0);
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
		using integral_coords_tag = ImageSpaceTag;
		using payload_type = TextureSpaceRegionPayload;
	};

	template <>
	struct CoordinatesSpaceTraits<CoordinatesSpaceType::InteriorTexture>
	{
		using integral_coords_tag = ImageSpaceTag;
		using payload_type = TextureSpaceRegionPayload;
	};

	using TPayload = typename CoordinatesSpaceTraits<TCoordinatesSpace>::payload_type;
	using payload_type = TPayload;

public:

	template<typename... TArgs>
	static GenericEphemeralVisualizationRestorePayload Make(TArgs &&... args)
	{
		GenericEphemeralVisualizationRestorePayload payload;
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

private:

	std::vector<TPayload> mPayloads;
};

using GenericShipSpaceEphemeralVisualizationRestorePayload = GenericEphemeralVisualizationRestorePayload<CoordinatesSpaceType::Ship>;
using GenericExteriorTextureSpaceEphemeralVisualizationRestorePayload = GenericEphemeralVisualizationRestorePayload<CoordinatesSpaceType::ExteriorTexture>;
using GenericInteriorTextureSpaceEphemeralVisualizationRestorePayload = GenericEphemeralVisualizationRestorePayload<CoordinatesSpaceType::InteriorTexture>;

}