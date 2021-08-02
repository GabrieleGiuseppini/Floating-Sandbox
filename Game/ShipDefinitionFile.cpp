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
    if (Utils::CaseInsensitiveEquals(definitionFilePath.extension().string(), ".shp"))
    {
        //
        // Load JSON file
        //

        std::filesystem::path const basePath = definitionFilePath.parent_path();

        picojson::value root = Utils::ParseJSONFile(definitionFilePath.string());
        if (!root.is<picojson::object>())
        {
            throw GameException("Ship definition file \"" + definitionFilePath.string() + "\" does not contain a JSON object");
        }

        picojson::object const & definitionJson = root.get<picojson::object>();

        std::string structuralLayerImageFilePathStr = Utils::GetMandatoryJsonMember<std::string>(
            definitionJson,
            "structure_image");

        std::optional<std::string> const ropesLayerImageFilePathStr = Utils::GetOptionalJsonMember<std::string>(
            definitionJson,
            "ropes_image");

        std::optional<std::string> const electricalLayerImageFilePathStr = Utils::GetOptionalJsonMember<std::string>(
            definitionJson,
            "electrical_image");

        std::optional<std::string> const textureLayerImageFilePathStr = Utils::GetOptionalJsonMember<std::string>(
            definitionJson,
            "texture_image");

        std::optional<ShipAutoTexturizationSettings> autoTexturizationSettings;
        if (auto const memberIt = definitionJson.find("auto_texturization"); memberIt != definitionJson.end())
        {
            // Check constraints
            if (textureLayerImageFilePathStr.has_value())
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
            definitionFilePath.stem().string());

        std::optional<std::string> author = Utils::GetOptionalJsonMember<std::string>(
            definitionJson,
            "created_by");

        std::optional<std::string> artCredits = Utils::GetOptionalJsonMember<std::string>(
            definitionJson,
            "art_credits");

        if (!artCredits.has_value())
        {
            // See if may populate art credits from author (legacy mode)
            if (author.has_value())
            {
                // Split at ';'
                auto const separator = author->find(';');
                if (separator != std::string::npos)
                {
                    // ArtCredits
                    if (separator < author->length() - 1)
                        artCredits = Utils::Trim(author->substr(separator + 1));

                    // Cleanse Author
                    if (separator > 0)
                        author = author->substr(0, separator);
                    else
                        author.reset();
                }
            }
        }

        std::optional<std::string> const yearBuilt = Utils::GetOptionalJsonMember<std::string>(
            definitionJson,
            "year_built");

        std::optional<std::string> const description = Utils::GetOptionalJsonMember<std::string>(
            definitionJson,
            "description");

        //
        // Physics data
        //

        vec2f offset(0.0f, 0.0f);
        std::optional<picojson::object> const offsetObject = Utils::GetOptionalJsonObject(definitionJson, "offset");
        if (!!offsetObject)
        {
            offset.x = Utils::GetMandatoryJsonMember<float>(*offsetObject, "x");
            offset.y = Utils::GetMandatoryJsonMember<float>(*offsetObject, "y");
        }

        std::optional<float> const internalPressure = Utils::GetOptionalJsonMember<float>(definitionJson, "internal_pressure");

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
                auto const panelX = Utils::GetOptionalJsonMember<std::int64_t>(elementMetadataObject, "panel_x");
                auto const panelY = Utils::GetOptionalJsonMember<std::int64_t>(elementMetadataObject, "panel_y");
                if (panelX.has_value() != panelY.has_value())
                    throw GameException("Found only one of 'panel_x' or 'panel_y' in the electrical panel; either none of both of them must be specified");
                auto const label = Utils::GetOptionalJsonMember<std::string>(elementMetadataObject, "label");
                auto const isHidden = Utils::GetOptionalJsonMember<bool>(elementMetadataObject, "is_hidden", false);

                auto const res = electricalPanelMetadata.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(instanceIndex),
                    std::forward_as_tuple(
                        panelX.has_value() ? IntegralPoint(int(*panelX), int(*panelY)) : std::optional<IntegralPoint>(),
                        label,
                        isHidden));

                if (!res.second)
                    throw GameException("Electrical element with ID '" + it.first + "' is specified more than twice in the electrical panel");
            }
        }

        return ShipDefinitionFile(
            basePath / std::filesystem::path(structuralLayerImageFilePathStr),
            ropesLayerImageFilePathStr.has_value()
                ? basePath / std::filesystem::path(*ropesLayerImageFilePathStr)
                : std::optional<std::filesystem::path>(std::nullopt),
            electricalLayerImageFilePathStr.has_value()
                ? basePath / std::filesystem::path(*electricalLayerImageFilePathStr)
                : std::optional<std::filesystem::path>(std::nullopt),
            textureLayerImageFilePathStr.has_value()
                ? basePath / std::filesystem::path(*textureLayerImageFilePathStr)
                : std::optional<std::filesystem::path>(std::nullopt),
            autoTexturizationSettings,
            doHideElectricalsInPreview,
            doHideHDInPreview,
            ShipMetadata(
                shipName,
                author,
                artCredits,
                yearBuilt,
                description,
                electricalPanelMetadata),
            ShipPhysicsData(
                offset,
                internalPressure));
    }
    else if (Utils::CaseInsensitiveEquals(definitionFilePath.extension().string(), ".png"))
    {
        //
        // From structural image file
        //

        return ShipDefinitionFile(
            definitionFilePath,
            std::nullopt, // Ropes
            std::nullopt, // Electrical
            std::nullopt, // Texture
            std::nullopt, // AutoTexturizationSettings
            false, // HideElectricalsInPreview
            false, // HideHDInPreview
            ShipMetadata(definitionFilePath.stem().string()),
            ShipPhysicsData());
    }
    else
    {
        throw GameException("File type \"" + definitionFilePath.extension().string() + "\" is not recognized as a ship file");
    }
}