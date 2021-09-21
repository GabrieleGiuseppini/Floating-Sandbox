/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipMetadata.h"
#include "ShipPhysicsData.h"

#include <GameCore/ImageData.h>

#include <filesystem>
#include <optional>

/*
* The complete definition of a ship.
*/
struct ShipDefinition final
{
public:

    RgbImageData StructuralLayerImage;

    std::optional<RgbImageData> RopesLayerImage;

    std::optional<RgbImageData> ElectricalLayerImage;

    std::optional<RgbaImageData> TextureLayerImage;

    std::optional<ShipAutoTexturizationSettings> const AutoTexturizationSettings;

    ShipMetadata Metadata;

    ShipPhysicsData PhysicsData;

    static ShipDefinition Load(std::filesystem::path const & filepath);

private:

    ShipDefinition(
        RgbImageData structuralLayerImage,
        std::optional<RgbImageData> ropesLayerImage,
        std::optional<RgbImageData> electricalLayerImage,
        std::optional<RgbaImageData> textureLayerImage,
        std::optional<ShipAutoTexturizationSettings> autoTexturizationSettings,
        ShipMetadata metadata,
        ShipPhysicsData physicsData)
        : StructuralLayerImage(std::move(structuralLayerImage))
        , RopesLayerImage(std::move(ropesLayerImage))
        , ElectricalLayerImage(std::move(electricalLayerImage))
        , TextureLayerImage(std::move(textureLayerImage))
        , AutoTexturizationSettings(std::move(autoTexturizationSettings))
        , Metadata(std::move(metadata))
        , PhysicsData(std::move(physicsData))
    {
    }
};
