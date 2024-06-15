/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameTypes.h>

#include <Game/Layers.h>

#include <optional>
#include <vector>

namespace ShipBuilder {

/*
 * Generic undo payload for a region of the ship. Rules:
 * - Does *not* change the presence of layers
 * - Does *not* change the size of layers
 */
class GenericUndoPayload final
{
public:

	ShipSpaceCoordinates Origin;

	std::optional<StructuralLayerData> StructuralLayerRegionBackup;
	std::optional<ElectricalLayerData> ElectricalLayerRegionBackup;
	std::optional<RopesLayerData> RopesLayerRegionBackup;
	std::optional<TextureLayerData> ExteriorTextureLayerRegionBackup;
	std::optional<TextureLayerData> InteriorTextureLayerRegionBackup;

	// Futurework: if needed, one day may add other elements, e.g. metadata

	GenericUndoPayload(ShipSpaceCoordinates const & origin)
		: Origin(origin)
	{}

	GenericUndoPayload(
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
	{}

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

}