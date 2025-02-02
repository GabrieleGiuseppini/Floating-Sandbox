/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/GameTypes.h>
#include <Core/Vectors.h>

#include <optional>
#include <string>

/*
 * Metadata for a ship.
 */
struct ShipMetadata final
{
public:

    std::string ShipName;

    std::optional<std::string> Author;

    std::optional<std::string> ArtCredits;

    std::optional<std::string> YearBuilt;

    std::optional<std::string> Description;

    ShipSpaceToWorldSpaceCoordsRatio Scale;

    bool DoHideElectricalsInPreview;
    bool DoHideHDInPreview;

    std::optional<PasswordHash> Password;

    explicit ShipMetadata(std::string shipName)
        : ShipName(std::move(shipName))
        , Author()
        , ArtCredits()
        , YearBuilt()
        , Description()
        , Scale(1.0f, 1.0f) // Default is 1:1
        , DoHideElectricalsInPreview(false)
        , DoHideHDInPreview(false)
        , Password()
    {
    }

    ShipMetadata(
        std::string shipName,
        std::optional<std::string> author,
        std::optional<std::string> artCredits,
        std::optional<std::string> yearBuilt,
        std::optional<std::string> description,
        ShipSpaceToWorldSpaceCoordsRatio scale,
        bool doHideElectricalsInPreview,
        bool doHideHDInPreview,
        std::optional<PasswordHash> password)
        : ShipName(std::move(shipName))
        , Author(std::move(author))
        , ArtCredits(std::move(artCredits))
        , YearBuilt(std::move(yearBuilt))
        , Description(std::move(description))
        , Scale(scale)
        , DoHideElectricalsInPreview(doHideElectricalsInPreview)
        , DoHideHDInPreview(doHideHDInPreview)
        , Password(password)
    {
    }

    ShipMetadata(ShipMetadata const & other) = default;
    ShipMetadata(ShipMetadata && other) = default;

    ShipMetadata & operator=(ShipMetadata const & other) = default;
    ShipMetadata & operator=(ShipMetadata && other) = default;
};
