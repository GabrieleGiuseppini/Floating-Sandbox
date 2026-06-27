/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipMetadata.h"

picojson::value ShipMetadata::Serialize() const
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

ShipMetadata ShipMetadata::Deserialize(picojson::value const & root)
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

