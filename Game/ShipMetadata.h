/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/Vectors.h>

#include <optional>
#include <string>

/*
 * Metadata for a ship.
 */
struct ShipMetadata
{
public:

    std::string const ShipName;

    std::optional<std::string> const Author;

    vec2f const Offset;

    ShipMetadata(
        std::string shipName,
        std::optional<std::string> author,
        vec2f offset)
        : ShipName(std::move(shipName))
        , Author(std::move(author))
        , Offset(std::move(offset))
    {
    }

    ShipMetadata(std::string shipName)
        : ShipName(std::move(shipName))
        , Author()
        , Offset()
    {
    }
};
