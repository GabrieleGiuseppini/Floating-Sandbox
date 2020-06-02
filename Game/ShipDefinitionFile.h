/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipMetadata.h"

#include <GameCore/GameTypes.h>
#include <GameCore/SysSpecifics.h>
#include <GameCore/Utils.h>
#include <GameCore/Vectors.h>

#include <picojson.h>

#include <filesystem>
#include <optional>
#include <string>

/*
 * A ship definition as extracted from its file (.shp or .png).
 */
struct ShipDefinitionFile
{
public:

    std::filesystem::path const StructuralLayerImageFilePath;

    std::optional<std::filesystem::path> const RopesLayerImageFilePath;

    std::optional<std::filesystem::path> const ElectricalLayerImageFilePath;

    std::optional<std::filesystem::path> const TextureLayerImageFilePath;

    std::optional<ShipAutoTexturizationSettings> const AutoTexturizationSettings;

    bool const DoHideElectricalsInPreview;
    bool const DoHideHDInPreview;

    ShipMetadata const Metadata;

public:

    static ShipDefinitionFile Load(std::filesystem::path definitionFilePath);

    static bool IsShipDefinitionFile(std::filesystem::path const & filepath)
    {
        return Utils::CaseInsensitiveEquals(filepath.extension().string(), ".shp")
            || Utils::CaseInsensitiveEquals(filepath.extension().string(), ".png");
    }

private:

    ShipDefinitionFile(
        std::filesystem::path const & structuralLayerImageFilePath,
        std::optional<std::filesystem::path> const & ropesLayerImageFilePath,
        std::optional<std::filesystem::path> const & electricalLayerImageFilePath,
        std::optional<std::filesystem::path> const & textureLayerImageFilePath,
        std::optional<ShipAutoTexturizationSettings> const & autoTexturizationSettings,
        bool doHideElectricalsInPreview,
        bool doHideHDInPreview,
        ShipMetadata && shipMetadata)
        : StructuralLayerImageFilePath(structuralLayerImageFilePath)
        , RopesLayerImageFilePath(ropesLayerImageFilePath)
        , ElectricalLayerImageFilePath(electricalLayerImageFilePath)
        , TextureLayerImageFilePath(textureLayerImageFilePath)
        , AutoTexturizationSettings(autoTexturizationSettings)
        , DoHideElectricalsInPreview(doHideElectricalsInPreview)
        , DoHideHDInPreview(doHideHDInPreview)
        , Metadata(std::move(shipMetadata))
    {
    }
};
