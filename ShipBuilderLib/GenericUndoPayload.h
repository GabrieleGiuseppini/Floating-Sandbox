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

	std::optional<StructuralLayerData> StructuralLayerRegion;
	std::optional<ElectricalLayerData> ElectricalLayerRegion; // Panel is whole
	std::optional<RopesLayerData> RopesLayerWhole;
	std::optional<TextureLayerData> TextureLayerRegion;

	// Futurework: if needed, one day may add other elements, e.g. metadata

	GenericUndoPayload(
		ShipSpaceCoordinates const & origin,
		std::optional<StructuralLayerData> && structuralLayerRegion,
		std::optional<ElectricalLayerData > && electricalLayerRegion,
		std::optional<RopesLayerData> && ropesLayerWhole,
		std::optional<TextureLayerData> && textureLayerRegion)
		: Origin(origin)
		, StructuralLayerRegion(std::move(structuralLayerRegion))
		, ElectricalLayerRegion(std::move(electricalLayerRegion))
		, RopesLayerWhole(std::move(ropesLayerWhole))
		, TextureLayerRegion(std::move(textureLayerRegion))
	{}

	size_t GetTotalCost() const
	{
		return
			(StructuralLayerRegion ? StructuralLayerRegion->Buffer.GetByteSize() : 0)
			+ (ElectricalLayerRegion ? ElectricalLayerRegion->Buffer.GetByteSize() : 0)
			+ (RopesLayerWhole ? RopesLayerWhole->Buffer.GetByteSize() : 0)
			+ (TextureLayerRegion ? TextureLayerRegion->Buffer.GetByteSize() : 0);
	}
};

}