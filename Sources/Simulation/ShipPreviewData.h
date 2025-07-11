/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipMetadata.h"

#include <picojson.h>

#include <Core/GameTypes.h>

/*
* A partial ship definition, suitable for a preview of the ship.
*/
struct ShipPreviewData
{
public:

    ShipSpaceSize ShipSize;
    ShipMetadata Metadata;
    bool IsHD;
    bool HasElectricals;

    ShipPreviewData(
        ShipSpaceSize const & shipSize,
        ShipMetadata const & metadata,
        bool isHD,
        bool hasElectricals)
        : ShipSize(shipSize)
        , Metadata(metadata)
        , IsHD(isHD)
        , HasElectricals(hasElectricals)
    {
    }

    ShipPreviewData(ShipPreviewData && other) = default;
    ShipPreviewData & operator=(ShipPreviewData && other) = default;

    picojson::value Serialize() const
    {
        picojson::object root;

        picojson::object sizeObj;
        sizeObj.emplace("width", picojson::value(static_cast<int64_t>(ShipSize.width)));
        sizeObj.emplace("height", picojson::value(static_cast<int64_t>(ShipSize.height)));
        root.emplace("size", picojson::value(sizeObj));

        root.emplace("metadata", Metadata.Serialize());

        root.emplace("is_hd", picojson::value(IsHD));
        root.emplace("has_electricals", picojson::value(HasElectricals));

        return picojson::value(root);
    }

    static ShipPreviewData Deserialize(picojson::value const & root)
    {
        auto const & rootAsObject = Utils::GetJsonValueAsObject(root, "ShipPreviewData");

        auto const sizeObj = Utils::GetMandatoryJsonMember<picojson::object>(rootAsObject, "size");
        int const width = static_cast<int>(Utils::GetMandatoryJsonMember<int64_t>(sizeObj, "width"));
        int const height = static_cast<int>(Utils::GetMandatoryJsonMember<int64_t>(sizeObj, "height"));

        ShipMetadata metadata = ShipMetadata::Deserialize(rootAsObject.at("metadata"));

        bool const isHd = Utils::GetMandatoryJsonMember<bool>(rootAsObject, "is_hd");
        bool const hasElectricals = Utils::GetMandatoryJsonMember<bool>(rootAsObject, "has_electricals");

        return ShipPreviewData(
            ShipSpaceSize(width, height),
            metadata,
            isHd,
            hasElectricals);
    }
};
