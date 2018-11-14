/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SysSpecifics.h"
#include "Utils.h"
#include "Vectors.h"

#include <picojson/picojson.h>

#include <filesystem>
#include <optional>
#include <string>

/*
 * The content of a ship definition file (.shp)
 */
struct ShipDefinitionFile
{
public:

    // Absolute or relative path
    std::string const StructuralImageFilePath;

    // Absolute or relative path
    std::optional<std::string> const TextureImageFilePath;

    // The name of the ship
    std::string const ShipName;

    // The name of the ship
    std::optional<std::string> const Author;

    // The offset between the images and the world
    vec2f const Offset;

    static ShipDefinitionFile Create(picojson::object const & definitionJson);

    static bool IsShipDefinitionFile(std::filesystem::path const & filepath)
    {
        return Utils::ToLower(filepath.extension().string()) == ".shp";
    }

    ShipDefinitionFile(
        std::string structuralImageFilePath,
        std::optional<std::string> textureImageFilePath,
        std::string shipName,
        std::optional<std::string> author,
        vec2f offset)
        : StructuralImageFilePath(std::move(structuralImageFilePath))
        , TextureImageFilePath(std::move(textureImageFilePath))
        , ShipName(std::move(shipName))
        , Author(std::move(author))
        , Offset(std::move(offset))
    {
    }
};
