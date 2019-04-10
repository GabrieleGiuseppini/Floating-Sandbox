/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDefinitionFile.h"

#include <GameCore/Utils.h>

ShipDefinitionFile ShipDefinitionFile::Create(std::filesystem::path definitionFilePath)
{
    // Load JSON file
    picojson::value root = Utils::ParseJSONFile(definitionFilePath.string());
    if (!root.is<picojson::object>())
    {
        throw GameException("Ship definition file \"" + definitionFilePath.string() + "\" does not contain a JSON object");
    }

    // Parse definition
    return Create(
        root.get<picojson::object>(),
        definitionFilePath.stem().string());
}

ShipDefinitionFile ShipDefinitionFile::Create(
    picojson::object const & definitionJson,
    std::string const & defaultShipName)
{
    std::string structuralLayerImageFilePath = Utils::GetMandatoryJsonMember<std::string>(
        definitionJson,
        "structure_image");

    std::optional<std::string> ropesLayerImageFilePath = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "ropes_image");

    std::optional<std::string> electricalLayerImageFilePath = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "electrical_image");

    std::optional<std::string> textureLayerImageFilePath = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "texture_image");

    std::string shipName = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "ship_name",
        defaultShipName);

    std::optional<std::string> author = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "created_by");

    std::optional<std::string> yearBuilt = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "year_built");

    std::optional<std::string> description = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "description");

    vec2f offset(0.0f, 0.0f);
    std::optional<picojson::object> offsetObject = Utils::GetOptionalJsonObject(definitionJson, "offset");
    if (!!offsetObject)
    {
        offset.x = Utils::GetMandatoryJsonMember<float>(*offsetObject, "x");
        offset.y = Utils::GetMandatoryJsonMember<float>(*offsetObject, "y");
    }

    return ShipDefinitionFile(
        structuralLayerImageFilePath,
        ropesLayerImageFilePath,
        electricalLayerImageFilePath,
        textureLayerImageFilePath,
        ShipMetadata(
            shipName,
            author,
            yearBuilt,
            description,
            offset));
}