/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipMetadata.h"

#include <GameCore/SysSpecifics.h>
#include <GameCore/Utils.h>
#include <GameCore/Vectors.h>

#include <picojson.h>

#include <filesystem>
#include <optional>
#include <string>

/*
 * The content of a ship definition file (.shp).
 */
struct ShipDefinitionFile
{
public:

    std::filesystem::path const StructuralLayerImageFilePath;

    std::optional<std::filesystem::path> const RopesLayerImageFilePath;

    std::optional<std::filesystem::path> const ElectricalLayerImageFilePath;

    std::optional<std::filesystem::path> const TextureLayerImageFilePath;

    // The ship's metadata
    ShipMetadata const Metadata;

    static ShipDefinitionFile Load(std::filesystem::path definitionFilePath);

    static ShipDefinitionFile Create(
        picojson::object const & definitionJson,
        std::string const & defaultShipName);

    static bool IsShipDefinitionFile(std::filesystem::path const & filepath)
    {
        return Utils::ToLower(filepath.extension().string()) == ".shp";
    }

    ShipDefinitionFile(
        std::filesystem::path const & structuralLayerImageFilePath,
        std::optional<std::filesystem::path> const & ropesLayerImageFilePath,
        std::optional<std::filesystem::path> const & electricalLayerImageFilePath,
        std::optional<std::filesystem::path> const & textureLayerImageFilePath,
        ShipMetadata && shipMetadata)
        : StructuralLayerImageFilePath(structuralLayerImageFilePath)
        , RopesLayerImageFilePath(ropesLayerImageFilePath)
        , ElectricalLayerImageFilePath(electricalLayerImageFilePath)
        , TextureLayerImageFilePath(std::move(textureLayerImageFilePath))
        , Metadata(std::move(shipMetadata))
    {
    }
};
