/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipDefinition.h"

#include "ImageFileTools.h"
#include "ShipDefinitionFile.h"

#include <cassert>

ShipDefinition ShipDefinition::Load(std::filesystem::path const & filepath)
{
    std::filesystem::path absoluteStructuralLayerImageFilePath;
    std::optional<RgbImageData> ropesLayerImage;
    std::optional<RgbImageData> electricalLayerImage;
    std::filesystem::path absoluteTextureLayerImageFilePath;
    ShipDefinition::TextureOriginType textureOrigin;
    std::optional<ShipMetadata> shipMetadata;

    if (ShipDefinitionFile::IsShipDefinitionFile(filepath))
    {
        //
        // Load full definition
        //

        ShipDefinitionFile sdf = ShipDefinitionFile::Create(filepath);

        //
        // Make paths absolute
        //

        std::filesystem::path basePath = filepath.parent_path();

        absoluteStructuralLayerImageFilePath =
            basePath / sdf.StructuralLayerImageFilePath;

        if (!!sdf.RopesLayerImageFilePath)
        {
            ropesLayerImage.emplace(
                ImageFileTools::LoadImageRgbUpperLeft(basePath / *sdf.RopesLayerImageFilePath));
        }

        if (!!sdf.ElectricalLayerImageFilePath)
        {
            electricalLayerImage.emplace(
                ImageFileTools::LoadImageRgbUpperLeft(basePath / *sdf.ElectricalLayerImageFilePath));
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

        shipMetadata.emplace(sdf.Metadata);
    }
    else
    {
        //
        // Assume it's just a structural image
        //

        absoluteStructuralLayerImageFilePath = filepath;
        absoluteTextureLayerImageFilePath = absoluteStructuralLayerImageFilePath;
        textureOrigin = ShipDefinition::TextureOriginType::StructuralImage;

        shipMetadata.emplace(std::filesystem::path(filepath).stem().string());
    }

    assert(!!shipMetadata);

    //
    // Load structural image
    //

    ImageData structuralImage = ImageFileTools::LoadImageRgbUpperLeft(absoluteStructuralLayerImageFilePath);

    //
    // Load texture image
    //

    std::optional<RgbaImageData> textureImage;

    switch (textureOrigin)
    {
        case ShipDefinition::TextureOriginType::Texture:
        {
            // Just load as-is

            textureImage.emplace(
                ImageFileTools::LoadImageRgbaLowerLeft(absoluteTextureLayerImageFilePath));

            break;
        }

        case ShipDefinition::TextureOriginType::StructuralImage:
        {
            // Resize it up - ideally by 8, but don't exceed 4096 in any dimension

            int maxDimension = std::max(structuralImage.Size.Width, structuralImage.Size.Height);
            int magnify = 8;
            while (maxDimension * magnify > 4096 && magnify > 1)
                magnify /= 2;

            textureImage.emplace(
                ImageFileTools::LoadImageRgbaLowerLeftAndMagnify(
                    absoluteTextureLayerImageFilePath,
                    magnify));

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