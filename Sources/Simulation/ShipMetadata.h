/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/GameTypes.h>
#include <Core/Utils.h>
#include <Core/Vectors.h>

#include <picojson.h>

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

    std::optional<ShipCategoryType> Category;

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
        , Category()
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
        std::optional<ShipCategoryType> category,
        std::optional<std::string> description,
        ShipSpaceToWorldSpaceCoordsRatio scale,
        bool doHideElectricalsInPreview,
        bool doHideHDInPreview,
        std::optional<PasswordHash> password)
        : ShipName(std::move(shipName))
        , Author(std::move(author))
        , ArtCredits(std::move(artCredits))
        , YearBuilt(std::move(yearBuilt))
        , Category(std::move(category))
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

    picojson::value Serialize() const
    {
        picojson::object root;

        root.emplace("ship_name", picojson::value(ShipName));

        if (Author.has_value())
        {
            root.emplace("created_by", picojson::value(*Author));
        }

        if (ArtCredits.has_value())
        {
            root.emplace("art_credits", picojson::value(*ArtCredits));
        }

        if (YearBuilt.has_value())
        {
            root.emplace("year_built", picojson::value(*YearBuilt));
        }

        if (Category.has_value())
        {
            root.emplace("category", picojson::value(static_cast<std::int64_t>(*Category)));
        }

        if (Description.has_value())
        {
            root.emplace("description", picojson::value(*Description));
        }

        picojson::object scaleObj;
        scaleObj.emplace("input_units", picojson::value(static_cast<double>(Scale.inputUnits)));
        scaleObj.emplace("output_units", picojson::value(static_cast<double>(Scale.outputUnits)));
        root.emplace("scale", picojson::value(scaleObj));

        root.emplace("do_hide_electricals_in_preview", picojson::value(DoHideElectricalsInPreview));
        root.emplace("do_hide_hd_in_preview", picojson::value(DoHideHDInPreview));

        if (Password.has_value())
        {
            root.emplace("password", picojson::value(static_cast<int64_t>(*Password)));
        }

        return picojson::value(root);
    }

    static ShipMetadata Deserialize(picojson::value const & root)
    {
        auto const & rootAsObject = Utils::GetJsonValueAsObject(root, "ShipMetadata");

        auto const shipName = Utils::GetMandatoryJsonMember<std::string>(rootAsObject, "ship_name");
        auto const author = Utils::GetOptionalJsonMember<std::string>(rootAsObject, "created_by");
        auto const artCredits = Utils::GetOptionalJsonMember<std::string>(rootAsObject, "art_credits");
        auto const yearBuilt = Utils::GetOptionalJsonMember<std::string>(rootAsObject, "year_built");
        auto const categoryInt = Utils::GetOptionalJsonMember<std::int64_t>(rootAsObject, "category");
        auto const description = Utils::GetOptionalJsonMember<std::string>(rootAsObject, "description");

        auto const scaleObj = Utils::GetMandatoryJsonMember<picojson::object>(rootAsObject, "scale");
        float const scaleInputUnits = static_cast<float>(Utils::GetMandatoryJsonMember<double>(scaleObj, "input_units"));
        float const scaleOutputUnits = static_cast<float>(Utils::GetMandatoryJsonMember<double>(scaleObj, "output_units"));

        bool const doHideElectricalsInPreview = Utils::GetMandatoryJsonMember<bool>(rootAsObject, "do_hide_electricals_in_preview");
        bool const doHideHDInPreview = Utils::GetMandatoryJsonMember<bool>(rootAsObject, "do_hide_hd_in_preview");

        auto const password = Utils::GetOptionalJsonMember<int64_t>(rootAsObject, "password");

        return ShipMetadata(
            shipName,
            author,
            artCredits,
            yearBuilt,
            categoryInt.has_value() ? static_cast<ShipCategoryType>(*categoryInt) : std::optional<ShipCategoryType>(),
            description,
            ShipSpaceToWorldSpaceCoordsRatio(scaleInputUnits, scaleOutputUnits),
            doHideElectricalsInPreview,
            doHideHDInPreview,
            password.has_value() ? static_cast<PasswordHash>(*password) : std::optional<PasswordHash>());
    }
};
