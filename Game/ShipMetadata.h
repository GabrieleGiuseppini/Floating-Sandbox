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

    std::optional<std::string> const YearBuilt;

    std::optional<std::string> const Description;

    vec2f const Offset;

    ShipMetadata(
        std::string shipName,
        std::optional<std::string> author,
        std::optional<std::string> yearBuilt,
        std::optional<std::string> description,
        vec2f offset)
        : ShipName(std::move(shipName))
        , Author(std::move(author))
        , YearBuilt(std::move(yearBuilt))
        , Description(std::move(description))
        , Offset(std::move(offset))
    {
    }

    ShipMetadata(std::string shipName)
        : ShipName(std::move(shipName))
        , Author()
        , YearBuilt()
        , Description()
        , Offset()
    {
    }

    ShipMetadata(ShipMetadata const & other) = default;
    ShipMetadata(ShipMetadata && other) = default;
};
