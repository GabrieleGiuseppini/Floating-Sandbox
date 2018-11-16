/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipMetadata.h"
#include "SysSpecifics.h"
#include "Utils.h"
#include "Vectors.h"

#include <picojson/picojson.h>

#include <filesystem>
#include <optional>
#include <string>

/*
 * The content of a ship definition file (.shp).
 */
struct ShipDefinitionFile
{
public:

    // Absolute or relative path
    std::string const StructuralImageFilePath;

    // Absolute or relative path
    std::optional<std::string> const TextureImageFilePath;

    // The ship's metadata
    ShipMetadata const Metadata;

    static ShipDefinitionFile Create(picojson::object const & definitionJson);

    static bool IsShipDefinitionFile(std::filesystem::path const & filepath)
    {
        return Utils::ToLower(filepath.extension().string()) == ".shp";
    }

    ShipDefinitionFile(
        std::string structuralImageFilePath,
        std::optional<std::string> textureImageFilePath,
        ShipMetadata shipMetadata)
        : StructuralImageFilePath(std::move(structuralImageFilePath))
        , TextureImageFilePath(std::move(textureImageFilePath))
        , Metadata(std::move(shipMetadata))
    {
    }
};
