/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameTypes.h>

#include <Game/Layers.h>

#include <optional>

namespace ShipBuilder {

/*
 * Generic undo payload for a region of the ship. Rules:
 * - Does *not* change the presence of layers
 */
class GenericUndoPayload final
{
public:

	ShipSpaceCoordinates Origin;

	std::optional<StructuralLayerData> StructuralLayerRegionBackup;
	std::optional<ElectricalLayerData> ElectricalLayerRegionBackup;
	std::optional<RopesLayerData> RopesLayerRegionBackup;
	std::optional<TextureLayerData> TextureLayerRegionBackup;

	// Futurework: if needed, one day may add other elements, e.g. metadata

	GenericUndoPayload(ShipSpaceCoordinates const & origin)
		: Origin(origin)
	{}

	GenericUndoPayload(
		ShipSpaceCoordinates const & origin,
		std::optional<StructuralLayerData> && structuralLayerRegionBackup,
		std::optional<ElectricalLayerData > && electricalLayerRegionBackup,
		std::optional<RopesLayerData> && ropesLayerRegionBackup,
		std::optional<TextureLayerData> && textureLayerRegionBackup)
		: Origin(origin)
		, StructuralLayerRegionBackup(std::move(structuralLayerRegionBackup))
		, ElectricalLayerRegionBackup(std::move(electricalLayerRegionBackup))
		, RopesLayerRegionBackup(std::move(ropesLayerRegionBackup))
		, TextureLayerRegionBackup(std::move(textureLayerRegionBackup))
	{}

	size_t GetTotalCost() const
	{
		return
			(StructuralLayerRegionBackup ? StructuralLayerRegionBackup->Buffer.GetByteSize() : 0)
			+ (ElectricalLayerRegionBackup ? ElectricalLayerRegionBackup->Buffer.GetByteSize() : 0)
			+ (RopesLayerRegionBackup ? RopesLayerRegionBackup->Buffer.GetByteSize() : 0)
			+ (TextureLayerRegionBackup ? TextureLayerRegionBackup->Buffer.GetByteSize() : 0);
	}
};

}