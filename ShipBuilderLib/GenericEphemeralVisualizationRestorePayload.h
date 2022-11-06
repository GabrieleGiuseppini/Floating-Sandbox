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
	std::optional<typename LayerTypeTraits<LayerType::Texture>::buffer_type> TextureLayerBufferRegion;

	GenericEphemeralVisualizationRestorePayload(ShipSpaceCoordinates const & origin)
		: Origin(origin)
	{}

	GenericEphemeralVisualizationRestorePayload(
		ShipSpaceCoordinates const & origin,
		std::optional<typename LayerTypeTraits<LayerType::Structural>::buffer_type> && structuralLayerBufferRegion,
		std::optional<typename LayerTypeTraits<LayerType::Electrical>::buffer_type > && electricalLayerBufferRegion,
		std::optional<typename LayerTypeTraits<LayerType::Ropes>::buffer_type> && ropesLayerBuffer,
		std::optional<typename LayerTypeTraits<LayerType::Texture>::buffer_type> && textureLayerBufferRegion)
		: Origin(origin)
		, StructuralLayerBufferRegion(std::move(structuralLayerBufferRegion))
		, ElectricalLayerBufferRegion(std::move(electricalLayerBufferRegion))
		, RopesLayerBuffer(std::move(ropesLayerBuffer))
		, TextureLayerBufferRegion(std::move(textureLayerBufferRegion))
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

		if (TextureLayerBufferRegion.has_value())
		{
			affectedLayers.emplace_back(LayerType::Texture);
		}

		return affectedLayers;
	}

};

}