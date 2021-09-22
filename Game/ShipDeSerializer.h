/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "MaterialDatabase.h"
#include "ShipMaterialization.h"
#include "ShipPreview.h"

#include <GameCore/ImageData.h>
#include <GameCore/Utils.h>

#include <filesystem>
#include <optional>

/*
 * All the logic to load and save ships from and to files.
 */
class ShipDeSerializer
{
public:

    /* TODOHERE
    static bool IsShipDefinitionFile(std::filesystem::path const & filepath)
    {
        return Utils::CaseInsensitiveEquals(filepath.extension().string(), ".shp")
            || Utils::CaseInsensitiveEquals(filepath.extension().string(), ".png");
    }
    */

    static ShipMaterialization LoadShip(
        std::filesystem::path const & shipFilePath,
        MaterialDatabase const & materialDatabase);

    static ShipPreview LoadShipPreview(std::filesystem::path const & shipFilePath);

    static void SaveShip(
        ShipMaterialization const & shipDefinition,
        std::filesystem::path const & shipFilePath);

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

    static bool IsPngFileType(std::filesystem::path const & shipFilePath);

    static bool IsShpFileType(std::filesystem::path const & shipFilePath);

    static ShipMaterialization LoadShipPng(
        std::filesystem::path const & shipFilePath,
        MaterialDatabase const & materialDatabase);

    static ShipPreview LoadShipPreviewPng(std::filesystem::path const & shipFilePath);

    static ShipMaterialization LoadShipShp(
        std::filesystem::path const & shipFilePath,
        MaterialDatabase const & materialDatabase);

    static ShipPreview LoadShipPreviewShp(std::filesystem::path const & shipFilePath);

    static JsonDefinition LoadJsonDefinitionShp(std::filesystem::path const & shipFilePath);

    static ShipMaterialization LoadFromDefinitionImageFilePaths(
        std::filesystem::path const & structuralLayerImageFilePath,
        std::optional<std::filesystem::path> const & electricalLayerImageFilePath,
        std::optional<std::filesystem::path> const & ropesLayerImageFilePath,
        std::optional<std::filesystem::path> const & textureLayerImageFilePath,
        ShipMetadata const & metadata,
        ShipPhysicsData const & physicsData,
        std::optional<ShipAutoTexturizationSettings> const & autoTexturizationSettings,
        MaterialDatabase const & materialDatabase);

    static ShipMaterialization LoadFromDefinitionImages(
        RgbImageData && structuralLayerImage,
        std::optional<RgbImageData> && electricalLayerImage,
        std::optional<RgbImageData> && ropesLayerImage,
        std::optional<RgbaImageData> && textureLayerImage,
        ShipMetadata const & metadata,
        ShipPhysicsData const & physicsData,
        std::optional<ShipAutoTexturizationSettings> const & autoTexturizationSettings,
        MaterialDatabase const & materialDatabase);
};
