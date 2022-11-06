/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-09
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipLegacyFormatDeSerializer.h"
#include "ImageFileTools.h"

#include <GameCore/GameException.h>
#include <GameCore/ImageTools.h>
#include <GameCore/Utils.h>

#include <memory>

ShipDefinition ShipLegacyFormatDeSerializer::LoadShipFromImageDefinition(
    std::filesystem::path const & shipFilePath,
    MaterialDatabase const & materialDatabase)
{
    return LoadFromDefinitionImageFilePaths(
        shipFilePath,
        std::nullopt, // Electrical
        ElectricalPanelMetadata(),
        std::nullopt, // Ropes
        std::nullopt, // Texture
        ShipMetadata(shipFilePath.stem().string()),
        ShipPhysicsData(),
        std::nullopt, // AutoTexturizationSettings
        materialDatabase);
}

ShipDefinition ShipLegacyFormatDeSerializer::LoadShipFromLegacyShpShipDefinition(
    std::filesystem::path const & shipFilePath,
    MaterialDatabase const & materialDatabase)
{
    JsonDefinition jsonDefinition = LoadLegacyShpShipDefinitionJson(shipFilePath);

    return LoadFromDefinitionImageFilePaths(
        jsonDefinition.StructuralLayerImageFilePath,
        jsonDefinition.ElectricalLayerImageFilePath,
        std::move(jsonDefinition.ElectricalPanel),
        jsonDefinition.RopesLayerImageFilePath,
        jsonDefinition.TextureLayerImageFilePath,
        jsonDefinition.Metadata,
        jsonDefinition.PhysicsData,
        jsonDefinition.AutoTexturizationSettings,
        materialDatabase);
}

ShipPreviewData ShipLegacyFormatDeSerializer::LoadShipPreviewDataFromImageDefinition(std::filesystem::path const & imageDefinitionFilePath)
{
    auto const imageSize = ImageFileTools::GetImageSize(imageDefinitionFilePath);

    return ShipPreviewData(
        imageDefinitionFilePath,
        ShipSpaceSize(imageSize.width, imageSize.height),
        ShipMetadata(imageDefinitionFilePath.stem().string()),
        false, // isHD
        false, // hasElectricals
        PortableTimepoint::FromLastWriteTime(imageDefinitionFilePath));
}

ShipPreviewData ShipLegacyFormatDeSerializer::LoadShipPreviewDataFromLegacyShpShipDefinition(std::filesystem::path const & shipFilePath)
{
    JsonDefinition const & jsonDefinition = LoadLegacyShpShipDefinitionJson(shipFilePath);

    std::filesystem::path previewImageFilePath;
    bool isHD = false;
    bool hasElectricals = false;
    if (jsonDefinition.TextureLayerImageFilePath.has_value())
    {
        // Use the ship's texture as its preview
        previewImageFilePath = *(jsonDefinition.TextureLayerImageFilePath);

        // Categorize as HD, unless instructed not to do so
        if (!jsonDefinition.Metadata.DoHideHDInPreview)
            isHD = true;
    }
    else
    {
        // Preview is from structural image
        previewImageFilePath = jsonDefinition.StructuralLayerImageFilePath;
    }

    // Check whether it has electricals, unless instructed not to do so
    if (!jsonDefinition.Metadata.DoHideElectricalsInPreview)
        hasElectricals = jsonDefinition.ElectricalLayerImageFilePath.has_value();

    ImageSize const structuralImageSize = ImageFileTools::GetImageSize(jsonDefinition.StructuralLayerImageFilePath);

    return ShipPreviewData(
        previewImageFilePath,
        ShipSpaceSize(structuralImageSize.width, structuralImageSize.height), // Ship size is from structural image
        jsonDefinition.Metadata,
        isHD,
        hasElectricals,
        PortableTimepoint::FromLastWriteTime(shipFilePath));
}

RgbaImageData ShipLegacyFormatDeSerializer::LoadPreviewImage(
    std::filesystem::path const & previewFilePath,
    ImageSize const & maxSize)
{
    RgbaImageData previewImage = ImageFileTools::LoadImageRgbaAndResize(
        previewFilePath,
        maxSize);

    // Trim
    return ImageTools::TrimWhiteOrTransparent(std::move(previewImage));
}

///////////////////////////////////////////////////////

ShipLegacyFormatDeSerializer::JsonDefinition ShipLegacyFormatDeSerializer::LoadLegacyShpShipDefinitionJson(std::filesystem::path const & shipFilePath)
{
    std::filesystem::path const basePath = shipFilePath.parent_path();

    picojson::value root = Utils::ParseJSONFile(shipFilePath.string());
    if (!root.is<picojson::object>())
    {
        throw GameException("Ship definition file \"" + shipFilePath.string() + "\" does not contain a JSON object");
    }

    picojson::object const & definitionJson = root.get<picojson::object>();

    std::string structuralLayerImageFilePathStr = Utils::GetMandatoryJsonMember<std::string>(
        definitionJson,
        "structure_image");

    std::optional<std::string> const electricalLayerImageFilePathStr = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "electrical_image");

    std::optional<std::string> const ropesLayerImageFilePathStr = Utils::GetOptionalJsonMember<std::string>(
        definitionJson,
        "ropes_image");

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
        shipFilePath.stem().string());

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

    float const internalPressure = Utils::GetOptionalJsonMember<float>(definitionJson, "internal_pressure", 1.0f);

    //
    // Electrical panel metadata
    //

    ElectricalPanelMetadata electricalPanel;
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

            auto const res = electricalPanel.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(instanceIndex),
                std::forward_as_tuple(
                    panelX.has_value() ? IntegralCoordinates(int(*panelX), int(*panelY)) : std::optional<IntegralCoordinates>(),
                    label,
                    isHidden));

            if (!res.second)
                throw GameException("Electrical element with ID '" + it.first + "' is specified more than twice in the electrical panel");
        }
    }

    return JsonDefinition(
        basePath / std::filesystem::path(structuralLayerImageFilePathStr),
        electricalLayerImageFilePathStr.has_value()
            ? basePath / std::filesystem::path(*electricalLayerImageFilePathStr)
            : std::optional<std::filesystem::path>(std::nullopt),
        std::move(electricalPanel),
        ropesLayerImageFilePathStr.has_value()
            ? basePath / std::filesystem::path(*ropesLayerImageFilePathStr)
            : std::optional<std::filesystem::path>(std::nullopt),
        textureLayerImageFilePathStr.has_value()
            ? basePath / std::filesystem::path(*textureLayerImageFilePathStr)
            : std::optional<std::filesystem::path>(std::nullopt),
        ShipMetadata(
            shipName,
            author,
            artCredits,
            yearBuilt,
            description,
            ShipSpaceToWorldSpaceCoordsRatio(1.0f, 1.0f), // When loading legacy, scale is always 1:1
            doHideElectricalsInPreview,
            doHideHDInPreview,
            std::nullopt), // Password
        ShipPhysicsData(
            offset,
            internalPressure),
        autoTexturizationSettings);
}

ShipDefinition ShipLegacyFormatDeSerializer::LoadFromDefinitionImageFilePaths(
    std::filesystem::path const & structuralLayerImageFilePath,
    std::optional<std::filesystem::path> const & electricalLayerImageFilePath,
    ElectricalPanelMetadata && electricalPanel,
    std::optional<std::filesystem::path> const & ropesLayerImageFilePath,
    std::optional<std::filesystem::path> const & textureLayerImageFilePath,
    ShipMetadata const & metadata,
    ShipPhysicsData const & physicsData,
    std::optional<ShipAutoTexturizationSettings> const & autoTexturizationSettings,
    MaterialDatabase const & materialDatabase)
{
    //
    // Load images
    //

    RgbImageData structuralLayerImage = ImageFileTools::LoadImageRgb(structuralLayerImageFilePath);

    std::optional<RgbImageData> electricalLayerImage;
    if (electricalLayerImageFilePath.has_value())
    {
        try
        {
            electricalLayerImage.emplace(
                ImageFileTools::LoadImageRgb(*electricalLayerImageFilePath));
        }
        catch (GameException const & gex)
        {
            throw GameException("Error loading electrical layer image: " + std::string(gex.what()));
        }
    }

    std::optional<RgbImageData> ropesLayerImage;
    if (ropesLayerImageFilePath.has_value())
    {
        try
        {
            ropesLayerImage.emplace(
                ImageFileTools::LoadImageRgb(*ropesLayerImageFilePath));
        }
        catch (GameException const & gex)
        {
            throw GameException("Error loading rope layer image: " + std::string(gex.what()));
        }
    }

    std::optional<RgbaImageData> textureLayerImage;
    if (textureLayerImageFilePath.has_value())
    {
        try
        {
            textureLayerImage.emplace(
                ImageFileTools::LoadImageRgba(*textureLayerImageFilePath));
        }
        catch (GameException const & gex)
        {
            throw GameException("Error loading texture layer image: " + std::string(gex.what()));
        }
    }

    //
    // Materialize ship
    //

    return LoadFromDefinitionImages(
        std::move(structuralLayerImage),
        std::move(electricalLayerImage),
        std::move(electricalPanel),
        std::move(ropesLayerImage),
        std::move(textureLayerImage),
        metadata,
        physicsData,
        autoTexturizationSettings,
        materialDatabase);
}

ShipDefinition ShipLegacyFormatDeSerializer::LoadFromDefinitionImages(
    RgbImageData && structuralLayerImage,
    std::optional<RgbImageData> && electricalLayerImage,
    ElectricalPanelMetadata && electricalPanel,
    std::optional<RgbImageData> && ropesLayerImage,
    std::optional<RgbaImageData> && textureLayerImage,
    ShipMetadata const & metadata,
    ShipPhysicsData const & physicsData,
    std::optional<ShipAutoTexturizationSettings> const & autoTexturizationSettings,
    MaterialDatabase const & materialDatabase)
{
    ShipSpaceSize const shipSize(
        structuralLayerImage.Size.width,
        structuralLayerImage.Size.height);

    // Create layers in any case - even though we might not need some

    StructuralLayerData structuralLayer(shipSize);
    bool hasStructuralElements = false;

    ElectricalLayerData electricalLayer(shipSize, std::move(electricalPanel));
    bool hasElectricalElements = false;

    RopesLayerData ropesLayer(shipSize);
    bool hasRopeElements = false;

    std::unique_ptr<TextureLayerData> textureLayer = textureLayerImage.has_value()
        ? std::make_unique<TextureLayerData>(std::move(*textureLayerImage))
        : nullptr;

    // Table remembering rope endpoints - three values:
    // - Entry not in map: haven't seen color key
    // - Entry in map: have seen first endpoint, corods is value
    // - Entry in map without value: have seen second endpoint
    std::map<MaterialColorKey, std::optional<ShipSpaceCoordinates>> ropeFirstEndpointCoordsByColorKey;

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // 1. Process structural layer, eventually creating electrical and rope elements from legacy
    //    specifications
    //////////////////////////////////////////////////////////////////////////////////////////////////

    ropeFirstEndpointCoordsByColorKey.clear();

    // Visit all columns
    for (int x = 0; x < shipSize.width; ++x)
    {
        // From bottom to top
        for (int y = 0; y < shipSize.height; ++y)
        {
            ImageCoordinates const imageCoords(x, y);

            // Lookup structural material
            MaterialColorKey const colorKey = structuralLayerImage[imageCoords];
            StructuralMaterial const * structuralMaterial = materialDatabase.FindStructuralMaterial(colorKey);
            if (nullptr != structuralMaterial)
            {
                ShipSpaceCoordinates const coords = ShipSpaceCoordinates(x, y);

                // Store structural element
                structuralLayer.Buffer[coords] = StructuralElement(structuralMaterial);

                //
                // Check if it's also a legacy electrical element
                //

                ElectricalMaterial const * const electricalMaterial = materialDatabase.FindElectricalMaterial(colorKey);
                if (nullptr != electricalMaterial)
                {
                    // Cannot have instanced elements in legacy mode
                    assert(!electricalMaterial->IsInstanced);

                    // Store electrical element
                    electricalLayer.Buffer[coords] = ElectricalElement(
                        electricalMaterial,
                        NoneElectricalElementInstanceIndex);

                    // Remember we have seen at least one electrical element
                    hasElectricalElements = true;
                }

                //
                // Check if it's a legacy rope endpoint
                //

                if (structuralMaterial->IsUniqueType(StructuralMaterial::MaterialUniqueType::Rope)
                    && !materialDatabase.IsUniqueStructuralMaterialColorKey(StructuralMaterial::MaterialUniqueType::Rope, colorKey))
                {
                    // Check if it's the first or the second endpoint for the rope
                    auto searchIt = ropeFirstEndpointCoordsByColorKey.find(colorKey);
                    if (searchIt == ropeFirstEndpointCoordsByColorKey.end())
                    {
                        // First time we see the rope color key
                        ropeFirstEndpointCoordsByColorKey[colorKey] = coords;
                    }
                    else if (searchIt->second.has_value())
                    {
                        // Second time we see the rope color key

                        // Store rope element
                        rgbaColor const ropeColor = rgbaColor(colorKey, 255);
                        ropesLayer.Buffer.EmplaceBack(
                            *(searchIt->second),
                            coords,
                            structuralMaterial,
                            ropeColor);

                        // Remember we have seen at least one rope element
                        hasRopeElements = true;

                        // Mark as "complete"
                        searchIt->second.reset();
                    }
                    else
                    {
                        // Too many rope endpoints for this color key
                        throw GameException(
                            "More than two rope endpoints for rope color \"" + colorKey.toString() + "\", detected at "
                            + imageCoords.FlipY(shipSize.height).ToString());
                    }
                }

                // Remember we have seen at least one structural element
                hasStructuralElements = true;
            }
        }
    }

    // Make sure all rope endpoints are matched
    {
        auto const unmatchedSrchIt = std::find_if(
            ropeFirstEndpointCoordsByColorKey.cbegin(),
            ropeFirstEndpointCoordsByColorKey.cend(),
            [](auto const & entry)
            {
                return entry.second.has_value();
            });

        if (unmatchedSrchIt != ropeFirstEndpointCoordsByColorKey.cend())
        {
            throw GameException("Rope endpoint with color key \"" + unmatchedSrchIt->first.toString() + "\" is unmatched");
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // 2. Process ropes layer - if any - creating rope elements
    //////////////////////////////////////////////////////////////////////////////////////////////////

    if (ropesLayerImage.has_value())
    {
        // Make sure dimensions match
        if (ropesLayerImage->Size != structuralLayerImage.Size)
        {
            throw GameException("The size of the image used for the ropes layer must match the size of the image used for the structural layer");
        }

        StructuralMaterial const & standardRopeMaterial = materialDatabase.GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType::Rope);

        ropeFirstEndpointCoordsByColorKey.clear();

        // Visit all columns
        for (int x = 0; x < shipSize.width; ++x)
        {
            // From bottom to top
            for (int y = 0; y < shipSize.height; ++y)
            {
                // Check if it's a rope endpoint: iff different than background
                ImageCoordinates const imageCoords(x, y);
                MaterialColorKey colorKey = (*ropesLayerImage)[imageCoords];
                if (colorKey != EmptyMaterialColorKey)
                {
                    //
                    // It's a rope endpoint
                    //

                    ShipSpaceCoordinates const coords = ShipSpaceCoordinates(x, y);

                    rgbaColor const ropeColor = rgbaColor(colorKey, 255);

                    // Make sure we don't have a rope already with an endpoint here
                    if (std::find_if(
                            ropesLayer.Buffer.cbegin(),
                            ropesLayer.Buffer.cend(),
                            [&coords](RopeElement const & e)
                            {
                                return e.StartCoords == coords
                                    || e.EndCoords == coords;
                            }) != ropesLayer.Buffer.cend())
                    {
                        throw GameException("There is already a rope endpoint at " + imageCoords.FlipY(shipSize.height).ToString());
                    }

                    // Check if it's the first or the second endpoint for the rope
                    auto searchIt = ropeFirstEndpointCoordsByColorKey.find(colorKey);
                    if (searchIt == ropeFirstEndpointCoordsByColorKey.end())
                    {
                        // First time we see the rope color key
                        ropeFirstEndpointCoordsByColorKey[colorKey] = coords;
                    }
                    else if (searchIt->second.has_value())
                    {
                        // Second time we see the rope color key

                        // Store rope element
                        ropesLayer.Buffer.EmplaceBack(
                            *(searchIt->second),
                            coords,
                            &standardRopeMaterial,
                            ropeColor);

                        // Remember we have seen at least one rope element
                        hasRopeElements = true;

                        // Mark as "complete"
                        searchIt->second.reset();
                    }
                    else
                    {
                        // Too many rope endpoints for this color key
                        throw GameException(
                            "More than two rope endpoints for rope color \"" + colorKey.toString() + "\", detected at "
                            + imageCoords.FlipY(shipSize.height).ToString());
                    }
                }
            }
        }

        // Make sure all rope endpoints are matched
        {
            auto const unmatchedSrchIt = std::find_if(
                ropeFirstEndpointCoordsByColorKey.cbegin(),
                ropeFirstEndpointCoordsByColorKey.cend(),
                [](auto const & entry)
                {
                    return entry.second.has_value();
                });

            if (unmatchedSrchIt != ropeFirstEndpointCoordsByColorKey.cend())
            {
                throw GameException("Rope endpoint with color key \"" + unmatchedSrchIt->first.toString() + "\" is unmatched");
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // 3. Process electrical layer - if any
    //////////////////////////////////////////////////////////////////////////////////////////////////

    if (electricalLayerImage.has_value())
    {
        // Make sure dimensions match
        if (electricalLayerImage->Size != structuralLayerImage.Size)
        {
            throw GameException("The size of the image used for the electrical layer must match the size of the image used for the structural layer");
        }

        std::map<ElectricalElementInstanceIndex, ImageCoordinates> seenInstanceIndicesToImageCoords;

        // Visit all columns
        for (int x = 0; x < shipSize.width; ++x)
        {
            // From bottom to top
            for (int y = 0; y < shipSize.height; ++y)
            {
                // Check if it's an electrical material: iff different than background
                ImageCoordinates const imageCoords(x, y);
                MaterialColorKey colorKey = (*electricalLayerImage)[imageCoords];
                if (colorKey != EmptyMaterialColorKey)
                {
                    //
                    // It's an electrical material
                    //

                    ShipSpaceCoordinates const coords = ShipSpaceCoordinates(x, y);

                    // Get material (matching instanced elements on r and g only)
                    ElectricalMaterial const * const electricalMaterial = materialDatabase.FindElectricalMaterialLegacy(colorKey);
                    if (electricalMaterial == nullptr)
                    {
                        throw GameException(
                            "Color key \"" + Utils::RgbColor2Hex(colorKey)
                            + "\" of pixel found at " + imageCoords.FlipY(shipSize.height).ToString()
                            + " in the electrical layer image");
                    }

                    // Make sure we have a structural point here, or a rope endpoint
                    if (structuralLayer.Buffer[coords].Material == nullptr
                        && std::find_if(
                                ropesLayer.Buffer.cbegin(),
                                ropesLayer.Buffer.cend(),
                                [&coords](RopeElement const & e)
                                {
                                    return e.StartCoords == coords
                                        || e.EndCoords == coords;
                                }) == ropesLayer.Buffer.cend())
                    {
                        throw GameException(
                            "The electrical layer image specifies an electrical material at "
                            + imageCoords.FlipY(shipSize.height).ToString()
                            + ", but no pixel may be found at those coordinates in either the structural or the ropes layer image");
                    }

                    // Extract instance index, if material requires one
                    ElectricalElementInstanceIndex instanceIndex;
                    if (electricalMaterial->IsInstanced)
                    {
                        instanceIndex = MaterialDatabase::ExtractElectricalElementInstanceIndex(colorKey);

                        // Make sure instance ID is not dupe
                        auto const searchIt = seenInstanceIndicesToImageCoords.find(instanceIndex);
                        if (searchIt != seenInstanceIndicesToImageCoords.end())
                        {
                            throw GameException(
                                "Found two electrical elements with instance ID \""
                                + std::to_string(instanceIndex)
                                + "\" in the electrical layer image, at " + searchIt->second.FlipY(shipSize.height).ToString()
                                + " and at " + imageCoords.FlipY(shipSize.height).ToString() + "; "
                                + " make sure that all instanced elements"
                                + " have unique values for the blue component of their color codes!");
                        }
                        else
                        {
                            // First time we see it
                            seenInstanceIndicesToImageCoords.emplace(
                                instanceIndex,
                                imageCoords);
                        }
                    }
                    else
                    {
                        instanceIndex = NoneElectricalElementInstanceIndex;
                    }

                    // Store electrical element
                    electricalLayer.Buffer[coords] = ElectricalElement(
                        electricalMaterial,
                        instanceIndex);

                    // Remember we have seen at least one electrical element
                    hasElectricalElements = true;
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////

    // Make sure we have at least one structural or rope element
    if (!hasStructuralElements)
    {
        throw GameException("The ship structure contains no pixels that may be recognized as structural material");
    }

    // Bake definition
    return ShipDefinition(
        ShipLayers(
            shipSize,
            hasStructuralElements ? std::make_unique<StructuralLayerData>(std::move(structuralLayer)) : nullptr,
            hasElectricalElements ? std::make_unique<ElectricalLayerData>(std::move(electricalLayer)) : nullptr,
            hasRopeElements ? std::make_unique<RopesLayerData>(std::move(ropesLayer)) : nullptr,
            std::move(textureLayer)),
        metadata,
        physicsData,
        autoTexturizationSettings);
}