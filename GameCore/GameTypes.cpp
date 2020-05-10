/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-07-18
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameTypes.h"

#include "GameException.h"
#include "Utils.h"

DurationShortLongType StrToDurationShortLongType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Short"))
        return DurationShortLongType::Short;
    else if (Utils::CaseInsensitiveEquals(str, "Long"))
        return DurationShortLongType::Long;
    else
        throw GameException("Unrecognized DurationShortLongType \"" + str + "\"");
}

ShipAutoTexturizationSettings ShipAutoTexturizationSettings::FromJSON(picojson::object const & jsonObject)
{
    auto const modeIt = jsonObject.find("mode");
    if (modeIt == jsonObject.end())
        throw GameException("Error reading ship auto-texturization settings: the 'mode' parameter is missing");
    if (!modeIt->second.is<std::string>())
        throw GameException("Error reading ship auto-texturization settings: the 'mode' parameter must be a string");

    ShipAutoTexturizationMode mode;
    std::string const modeString = modeIt->second.get<std::string>();
    if (modeString == "flat_structure")
        mode = ShipAutoTexturizationMode::FlatStructure;
    else if (modeString == "material_textures")
        mode = ShipAutoTexturizationMode::MaterialTextures;
    else
        throw GameException("Error reading ship auto-texturization settings: the 'mode' parameter is not recognized; it must be 'flat_structure' or 'material_textures'");

    float materialTextureMagnification = Utils::GetOptionalJsonMember<float>(jsonObject, "material_texture_magnification", 1.0f);
    float materialTextureTransparency = Utils::GetOptionalJsonMember<float>(jsonObject, "material_texture_transparency", 0.0f);

    return ShipAutoTexturizationSettings(
        mode,
        materialTextureMagnification,
        materialTextureTransparency);
}

picojson::object ShipAutoTexturizationSettings::ToJSON() const
{
    picojson::object jsonObject;

    switch (Mode)
    {
        case ShipAutoTexturizationMode::FlatStructure:
        {
            jsonObject["mode"] = picojson::value(std::string("flat_structure"));
            break;
        }

        case ShipAutoTexturizationMode::MaterialTextures:
        {
            jsonObject["mode"] = picojson::value(std::string("material_textures"));
            break;
        }
    }

    jsonObject["material_texture_magnification"] = picojson::value(static_cast<double>(MaterialTextureMagnification));
    jsonObject["material_texture_transparency"] = picojson::value(static_cast<double>(MaterialTextureTransparency));

    return jsonObject;
}