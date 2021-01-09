/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

#include <optional>
#include <string>
#include <map>

/*
 * Metadata for a ship.
 */
struct ShipMetadata
{
public:

    std::string ShipName;

    std::optional<std::string> Author;

    std::optional<std::string> ArtCredits;

    std::optional<std::string> YearBuilt;

    std::optional<std::string> Description;

    vec2f Offset;

    std::map<ElectricalElementInstanceIndex, ElectricalPanelElementMetadata> ElectricalPanelMetadata;

    ShipMetadata(
        std::string shipName,
        std::optional<std::string> author,
        std::optional<std::string> artCredits,
        std::optional<std::string> yearBuilt,
        std::optional<std::string> description,
        vec2f offset,
        std::map<ElectricalElementInstanceIndex, ElectricalPanelElementMetadata> electricalPanelMetadata)
        : ShipName(std::move(shipName))
        , Author(std::move(author))
        , ArtCredits(std::move(artCredits))
        , YearBuilt(std::move(yearBuilt))
        , Description(std::move(description))
        , Offset(std::move(offset))
        , ElectricalPanelMetadata(std::move(electricalPanelMetadata))
    {
    }

    ShipMetadata(std::string shipName)
        : ShipName(std::move(shipName))
        , Author()
        , ArtCredits()
        , YearBuilt()
        , Description()
        , Offset()
        , ElectricalPanelMetadata()
    {
    }

    ShipMetadata(ShipMetadata const & other) = default;
    ShipMetadata(ShipMetadata && other) = default;
};
