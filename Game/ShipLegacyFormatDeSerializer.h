/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-09
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "MaterialDatabase.h"
#include "ShipDefinition.h"
#include "ShipPreviewData.h"

#include <GameCore/ImageData.h>

#include <cstdint>
#include <filesystem>
#include <optional>

/*
 * All the logic to load and save ships from and to legacy format files.
 */
class ShipLegacyFormatDeSerializer
{
public:

    static ShipDefinition LoadShipFromImageDefinition(
        std::filesystem::path const & shipFilePath,
        MaterialDatabase const & materialDatabase);

    static ShipDefinition LoadShipFromLegacyShpShipDefinition(
        std::filesystem::path const & shipFilePath,
        MaterialDatabase const & materialDatabase);

    static ShipPreviewData LoadShipPreviewDataFromImageDefinition(std::filesystem::path const & shipFilePath);

    static ShipPreviewData LoadShipPreviewDataFromLegacyShpShipDefinition(std::filesystem::path const & shipFilePath);

    static RgbaImageData LoadPreviewImage(
        std::filesystem::path const & previewFilePath,
        ImageSize const & maxSize);

private:

    struct JsonDefinition
    {
        std::filesystem::path StructuralLayerImageFilePath;
        std::optional<std::filesystem::path> ElectricalLayerImageFilePath;
        std::optional<std::filesystem::path> RopesLayerImageFilePath;
        std::optional<std::filesystem::path> TextureLayerImageFilePath;
        ShipMetadata Metadata;
        ShipPhysicsData PhysicsData;
        std::optional<ShipAutoTexturizationSettings> AutoTexturizationSettings;

        JsonDefinition(
            std::filesystem::path const & structuralLayerImageFilePath,
            std::optional<std::filesystem::path> const & electricalLayerImageFilePath,
            std::optional<std::filesystem::path> const & ropesLayerImageFilePath,
            std::optional<std::filesystem::path> const & textureLayerImageFilePath,
            ShipMetadata const & metadata,
            ShipPhysicsData const & physicsData,
            std::optional<ShipAutoTexturizationSettings> autoTexturizationSettings)
            : StructuralLayerImageFilePath(structuralLayerImageFilePath)
            , ElectricalLayerImageFilePath(electricalLayerImageFilePath)
            , RopesLayerImageFilePath(ropesLayerImageFilePath)
            , TextureLayerImageFilePath(textureLayerImageFilePath)
            , Metadata(metadata)
            , PhysicsData(physicsData)
            , AutoTexturizationSettings(autoTexturizationSettings)
        {}
    };

    static JsonDefinition LoadLegacyShpShipDefinitionJson(std::filesystem::path const & shipFilePath);

    static ShipDefinition LoadFromDefinitionImageFilePaths(
        std::filesystem::path const & structuralLayerImageFilePath,
        std::optional<std::filesystem::path> const & electricalLayerImageFilePath,
        std::optional<std::filesystem::path> const & ropesLayerImageFilePath,
        std::optional<std::filesystem::path> const & textureLayerImageFilePath,
        ShipMetadata const & metadata,
        ShipPhysicsData const & physicsData,
        std::optional<ShipAutoTexturizationSettings> const & autoTexturizationSettings,
        MaterialDatabase const & materialDatabase);

    static ShipDefinition LoadFromDefinitionImages(
        RgbImageData && structuralLayerImage,
        std::optional<RgbImageData> && electricalLayerImage,
        std::optional<RgbImageData> && ropesLayerImage,
        std::optional<RgbaImageData> && textureLayerImage,
        ShipMetadata const & metadata,
        ShipPhysicsData const & physicsData,
        std::optional<ShipAutoTexturizationSettings> const & autoTexturizationSettings,
        MaterialDatabase const & materialDatabase);
};
