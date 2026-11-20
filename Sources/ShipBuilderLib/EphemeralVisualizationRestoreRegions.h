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
 * Backup data to restore an ephemeral visualization.
 */

template<CoordinatesSpaceType TCoordinatesSpace>
class EphemeralVisualizationRestoreRegions final
{
public:

	class ShipSpaceRestoreRegion final
	{
	public:
		ShipSpaceRect OriginatingRegion; // Region in whatever the source of this is, from which we got this extract; size matches size of our regions (when they're not full)

		std::optional<typename LayerTypeTraits<LayerType::Structural>::buffer_type> StructuralLayerBufferRegion;
		std::optional<typename LayerTypeTraits<LayerType::Electrical>::buffer_type> ElectricalLayerBufferRegion;
		std::optional<typename LayerTypeTraits<LayerType::Ropes>::buffer_type> RopesLayerBuffer; // Whole, for "hanging rope endpoints"
		std::optional<typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type> ExteriorTextureLayerBufferRegion;
		std::optional<typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type> InteriorTextureLayerBufferRegion;

		ShipSpaceRestoreRegion() // Empty
			: OriginatingRegion(ShipSpaceSize(0, 0))
		{
		}

		ShipSpaceRestoreRegion(
			ShipSpaceRect const & originatingRegion,
			std::optional<typename LayerTypeTraits<LayerType::Structural>::buffer_type> && structuralLayerBufferRegion,
			std::optional<typename LayerTypeTraits<LayerType::Electrical>::buffer_type > && electricalLayerBufferRegion,
			std::optional<typename LayerTypeTraits<LayerType::Ropes>::buffer_type> && ropesLayerBuffer,
			std::optional<typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type> && exteriorTextureLayerBufferRegion,
			std::optional<typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type> && interiorTextureLayerBufferRegion)
			: OriginatingRegion(originatingRegion)
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

	class TextureSpaceRestoreRegion final
	{
	public:

		ImageRect OriginatingRegion; // Region in whatever the source of this is, from which we got this extract; size matches size of our regions (when they're not full)

		std::optional<typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type> ExteriorTextureLayerBufferRegion;
		std::optional<typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type> InteriorTextureLayerBufferRegion;

		TextureSpaceRestoreRegion(ImageRect const & originatingRegion)
			: OriginatingRegion(originatingRegion)
		{
		}

		TextureSpaceRestoreRegion(
			ImageRect const & originatingRegion,
			std::optional<typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type> && exteriorTextureLayerBufferRegion,
			std::optional<typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type> && interiorTextureLayerBufferRegion)
			: OriginatingRegion(originatingRegion)
			, ExteriorTextureLayerBufferRegion(std::move(exteriorTextureLayerBufferRegion))
			, InteriorTextureLayerBufferRegion(std::move(interiorTextureLayerBufferRegion))
		{
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
		using payload_type = ShipSpaceRestoreRegion;
	};

	template <>
	struct CoordinatesSpaceTraits<CoordinatesSpaceType::ExteriorTexture>
	{
		using integral_coords_tag = ImageSpaceTag;
		using payload_type = TextureSpaceRestoreRegion;
	};

	template <>
	struct CoordinatesSpaceTraits<CoordinatesSpaceType::InteriorTexture>
	{
		using integral_coords_tag = ImageSpaceTag;
		using payload_type = TextureSpaceRestoreRegion;
	};

	using TPayload = typename CoordinatesSpaceTraits<TCoordinatesSpace>::payload_type;
	using payload_type = TPayload;

public:

	template<typename... TArgs>
	static EphemeralVisualizationRestoreRegions Make(TArgs &&... args)
	{
		EphemeralVisualizationRestoreRegions regions;
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

private:

	std::vector<TPayload> mRegions;
};

using ShipSpaceEphemeralVisualizationRestoreRegions = EphemeralVisualizationRestoreRegions<CoordinatesSpaceType::Ship>;
using ExteriorTextureSpaceEphemeralVisualizationRestoreRegions = EphemeralVisualizationRestoreRegions<CoordinatesSpaceType::ExteriorTexture>;
using InteriorTextureSpaceEphemeralVisualizationRestoreRegions = EphemeralVisualizationRestoreRegions<CoordinatesSpaceType::InteriorTexture>;

}