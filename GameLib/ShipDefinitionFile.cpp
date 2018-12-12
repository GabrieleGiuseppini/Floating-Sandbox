/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/

#include "ShipDefinitionFile.h"

#include "Utils.h"

ShipDefinitionFile ShipDefinitionFile::Create(picojson::object const & definitionJson)
{
    std::string structuralLayerImageFilePath = Utils::GetMandatoryJsonMember<std::string>(
        definitionJson,
        "structure_image");

    std::optional<std::string> ropeLayerImageFilePath = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "rope_image");

    std::optional<std::string> electricalLayerImageFilePath = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "electrical_image");

    std::optional<std::string> textureLayerImageFilePath = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "texture_image");

    std::string shipName = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "ship_name",
        "");

    std::optional<std::string> author = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "created_by");

    vec2f offset(0.0f, 0.0f);
    std::optional<picojson::object> offsetObject = Utils::GetOptionalJsonObject(definitionJson, "offset");
    if (!!offsetObject)
    {
        offset.x = static_cast<float>(Utils::GetMandatoryJsonMember<double>(*offsetObject, "x"));
        offset.y = static_cast<float>(Utils::GetMandatoryJsonMember<double>(*offsetObject, "y"));
    }

    return ShipDefinitionFile(
        structuralLayerImageFilePath,
        ropeLayerImageFilePath,
        electricalLayerImageFilePath,
        textureLayerImageFilePath,
        ShipMetadata(
            shipName,
            author,
            offset));
}