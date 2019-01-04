/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDefinition.h"

#include "ShipDefinitionFile.h"

#include <GameCore/ResourceLoader.h>
#include <GameCore/Utils.h>

#include <cassert>

ShipDefinition ShipDefinition::Load(std::filesystem::path const & filepath)
{
    std::filesystem::path absoluteStructuralLayerImageFilePath;
    std::optional<ImageData> ropesLayerImage;
    std::optional<ImageData> electricalLayerImage;
    std::filesystem::path absoluteTextureLayerImageFilePath;
    ShipDefinition::TextureOriginType textureOrigin;
    std::optional<ShipMetadata> shipMetadata;

    if (ShipDefinitionFile::IsShipDefinitionFile(filepath))
    {
        //
        // Load full definition
        //

        picojson::value root = Utils::ParseJSONFile(filepath.string());
        if (!root.is<picojson::object>())
        {
            throw GameException("File \"" + filepath.string() + "\" does not contain a JSON object");
        }

        ShipDefinitionFile sdf = ShipDefinitionFile::Create(root.get<picojson::object>());

        //
        // Make paths absolute
        //

        std::filesystem::path basePath = filepath.parent_path();

        absoluteStructuralLayerImageFilePath =
            basePath / sdf.StructuralLayerImageFilePath;

        if (!!sdf.RopesLayerImageFilePath)
        {
            ropesLayerImage.emplace(
                ResourceLoader::LoadImageRgbUpperLeft(
                    (basePath / *sdf.RopesLayerImageFilePath).string()));
        }

        if (!!sdf.ElectricalLayerImageFilePath)
        {
            electricalLayerImage.emplace(
                ResourceLoader::LoadImageRgbUpperLeft(
                    (basePath / *sdf.ElectricalLayerImageFilePath).string()));
        }

        if (!!sdf.TextureLayerImageFilePath)
        {
            // Texture image is as specified

            absoluteTextureLayerImageFilePath = basePath / *sdf.TextureLayerImageFilePath;

            textureOrigin = ShipDefinition::TextureOriginType::Texture;
        }
        else
        {
            // Texture image is the structural image

            absoluteTextureLayerImageFilePath = absoluteStructuralLayerImageFilePath;

            textureOrigin = ShipDefinition::TextureOriginType::StructuralImage;
        }


        //
        // Make metadata
        //

        std::string shipName = sdf.Metadata.ShipName.empty()
            ? std::filesystem::path(filepath).stem().string()
            : sdf.Metadata.ShipName;

        shipMetadata.emplace(
            shipName,
            sdf.Metadata.Author,
            sdf.Metadata.Offset);
    }
    else
    {
        //
        // Assume it's just a structural image
        //

        absoluteStructuralLayerImageFilePath = filepath;
        absoluteTextureLayerImageFilePath = absoluteStructuralLayerImageFilePath;
        textureOrigin = ShipDefinition::TextureOriginType::StructuralImage;

        shipMetadata.emplace(
            std::filesystem::path(filepath).stem().string(),
            std::nullopt,
            vec2f(0.0f, 0.0f));
    }

    assert(!!shipMetadata);

    //
    // Load structural image
    //

    ImageData structuralImage = ResourceLoader::LoadImageRgbUpperLeft(
        absoluteStructuralLayerImageFilePath.string());

    //
    // Load texture image
    //

    std::optional<ImageData> textureImage;

    switch (textureOrigin)
    {
        case ShipDefinition::TextureOriginType::Texture:
        {
            // Just load as-is

            textureImage.emplace(
                ResourceLoader::LoadImageRgbaLowerLeft(
                    absoluteTextureLayerImageFilePath.string()));

            break;
        }

        case ShipDefinition::TextureOriginType::StructuralImage:
        {
            // Resize it up - ideally by 8, but don't exceed 4096 in any dimension

            int maxDimension = std::max(structuralImage.Size.Width, structuralImage.Size.Height);
            int resize = 8;
            while (maxDimension * resize > 4096 && resize > 1)
                resize /= 2;

            textureImage.emplace(
                ResourceLoader::LoadImageRgbaLowerLeft(
                    absoluteTextureLayerImageFilePath.string(),
                    resize));

            break;
        }
    }

    assert(!!textureImage);

    return ShipDefinition(
        std::move(structuralImage),
        std::move(ropesLayerImage),
        std::move(electricalLayerImage),
        std::move(*textureImage),
        textureOrigin,
        *shipMetadata);
}