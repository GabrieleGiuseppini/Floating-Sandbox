/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDefinitionFile.h"

#include <GameCore/GameException.h>
#include <GameCore/Utils.h>

ShipDefinitionFile ShipDefinitionFile::Load(std::filesystem::path definitionFilePath)
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
    std::string const structuralLayerImageFilePath = Utils::GetMandatoryJsonMember<std::string>(
        definitionJson,
        "structure_image");

    std::optional<std::string> const ropesLayerImageFilePath = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "ropes_image");

    std::optional<std::string> const electricalLayerImageFilePath = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "electrical_image");

    std::optional<std::string> const textureLayerImageFilePath = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "texture_image");

    std::optional<ShipAutoTexturizationSettings> autoTexturizationSettings;
    if (auto const memberIt = definitionJson.find("auto_texturization"); memberIt != definitionJson.end())
    {
        // Check constraints
        if (textureLayerImageFilePath.has_value())
        {
            throw GameException("Ship definition cannot contain an \"auto_texturization\" directive when it also contains a \"texture_image\" directive");
        }

        if (!memberIt->second.is<picojson::object>())
        {
            throw GameException("Invalid syntax of \"auto_texturization\" directive in ship definition.");
        }

        // Parse
        autoTexturizationSettings = ShipAutoTexturizationSettings::FromJSON(memberIt->second.get<picojson::object>());
    }

    bool const doHideElectricalsInPreview = Utils::GetOptionalJsonMember<bool>(
        definitionJson,
        "do_hide_electricals_in_preview",
        false);

    bool const doHideHDInPreview = Utils::GetOptionalJsonMember<bool>(
        definitionJson,
        "do_hide_hd_in_preview",
        false);

    //
    // Metadata
    //

    std::string const shipName = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "ship_name",
        defaultShipName);

    std::optional<std::string> const author = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "created_by");

    std::optional<std::string> const yearBuilt = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "year_built");

    std::optional<std::string> const description = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "description");

    vec2f offset(0.0f, 0.0f);
    std::optional<picojson::object> const offsetObject = Utils::GetOptionalJsonObject(definitionJson, "offset");
    if (!!offsetObject)
    {
        offset.x = Utils::GetMandatoryJsonMember<float>(*offsetObject, "x");
        offset.y = Utils::GetMandatoryJsonMember<float>(*offsetObject, "y");
    }

    //
    // Electrical panel metadata
    //

    std::map<ElectricalElementInstanceIndex, ElectricalPanelElementMetadata> electricalPanelMetadata;
    std::optional<picojson::object> const electricalPanelMetadataObject = Utils::GetOptionalJsonObject(definitionJson, "electrical_panel");
    if (!!electricalPanelMetadataObject)
    {
        for (auto const & it : *electricalPanelMetadataObject)
        {
            ElectricalElementInstanceIndex instanceIndex;
            if (!Utils::LexicalCast(it.first, &instanceIndex))
                throw GameException("Key of electrical panel element '" + it.first + "' is not a valid integer");

            picojson::object const & elementMetadataObject = Utils::GetJsonValueAs<picojson::object>(it.second, it.first);
            auto const panelX = Utils::GetMandatoryJsonMember<std::int64_t>(elementMetadataObject, "panel_x");
            auto const panelY = Utils::GetMandatoryJsonMember<std::int64_t>(elementMetadataObject, "panel_y");
            auto const label = Utils::GetMandatoryJsonMember<std::string>(elementMetadataObject, "label");
            auto const isHidden = Utils::GetOptionalJsonMember<bool>(elementMetadataObject, "is_hidden", false);

            auto const res = electricalPanelMetadata.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(instanceIndex),
                std::forward_as_tuple(IntegralPoint(int(panelX), int(panelY)), label, isHidden));

            if (!res.second)
                throw GameException("Electrical element with ID '" + it.first + "' is specified more than twice in the electrical panel");
        }
    }


    return ShipDefinitionFile(
        structuralLayerImageFilePath,
        ropesLayerImageFilePath,
        electricalLayerImageFilePath,
        textureLayerImageFilePath,
        autoTexturizationSettings,
        doHideElectricalsInPreview,
        doHideHDInPreview,
        ShipMetadata(
            shipName,
            author,
            yearBuilt,
            description,
            offset,
            electricalPanelMetadata));
}