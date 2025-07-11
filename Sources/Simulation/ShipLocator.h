/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-04-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/Utils.h>

#include <picojson.h>

#include <string>

/*
 * Abstracts out the location of a ship.
 *
 * Note: this is used exclusively by the Android port; it's here as support to ShipDatabase.
 */

struct ShipLocator final
{
    std::string RelativeFilePath;

    ShipLocator() = default;

    explicit ShipLocator(std::string const & relativeFilePath)
        : RelativeFilePath(relativeFilePath)
    {}

    picojson::value Serialize() const
    {
        picojson::object locatorRoot;

		locatorRoot.emplace("relative_file_path", picojson::value(RelativeFilePath));

		return picojson::value(locatorRoot);
    }

    static ShipLocator Deserialize(picojson::value const & locatorRoot)
    {
        return ShipLocator(Utils::GetMandatoryJsonMember<std::string>(Utils::GetJsonValueAsObject(locatorRoot, "ShipLocator"), "relative_file_path"));
    }
};
