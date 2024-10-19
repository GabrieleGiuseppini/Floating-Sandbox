/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-11-06
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameTypes.h>

#include <Game/Layers.h>

#include <optional>
#include <vector>

namespace ShipBuilder {

/*
 * Generic backup data to restore an ephemeral visualization.
 */
class GenericEphemeralVisualizationRestorePayload final
{
public:

	ShipSpaceCoordinates Origin;

	std::optional<typename LayerTypeTraits<LayerType::Structural>::buffer_type> StructuralLayerBufferRegion;
	std::optional<typename LayerTypeTraits<LayerType::Electrical>::buffer_type> ElectricalLayerBufferRegion;
	std::optional<typename LayerTypeTraits<LayerType::Ropes>::buffer_type> RopesLayerBuffer; // Whole buffer
	std::optional<typename LayerTypeTraits<LayerType::ExteriorTexture>::buffer_type> ExteriorTextureLayerBufferRegion;
	std::optional<typename LayerTypeTraits<LayerType::InteriorTexture>::buffer_type> InteriorTextureLayerBufferRegion;

	GenericEphemeralVisualizationRestorePayload(ShipSpaceCoordinates const & origin)
		: Origin(origin)
	{}

	GenericEphemeralVisualizationRestorePayload(
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
	{}

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

}