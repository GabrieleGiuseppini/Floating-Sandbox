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
    std::vector<std::string> electricalElementLabels;
    std::optional<ShipMetadata> shipMetadata;

    if (ShipDefinitionFile::IsShipDefinitionFile(filepath))
    {
        std::filesystem::path const basePath = filepath.parent_path();

        //
        // Load full definition
        //

        ShipDefinitionFile sdf = ShipDefinitionFile::Load(filepath);

        //
        // Make structure layer image path absolute
        //

        absoluteStructuralLayerImageFilePath =
            basePath / sdf.StructuralLayerImageFilePath;

        //
        // Load ropes layer image
        //

        if (!!sdf.RopesLayerImageFilePath)
        {
            try
            {
                ropesLayerImage.emplace(
                    ImageFileTools::LoadImageRgbUpperLeft(basePath / *sdf.RopesLayerImageFilePath));
            }
            catch (GameException const & gex)
            {
                throw GameException("Error loading rope layer image: " + std::string(gex.what()));
            }
        }

        //
        // Load electrical layer image
        //

        if (!!sdf.ElectricalLayerImageFilePath)
        {
            try
            {
                electricalLayerImage.emplace(
                    ImageFileTools::LoadImageRgbUpperLeft(basePath / *sdf.ElectricalLayerImageFilePath));
            }
            catch (GameException const & gex)
            {
                throw GameException("Error loading electrical layer image: " + std::string(gex.what()));
            }
        }

        //
        // Make texture layer image path
        //

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
    // Load structural layer image
    //

    ImageData structuralImage = ImageFileTools::LoadImageRgbUpperLeft(absoluteStructuralLayerImageFilePath);

    //
    // Make texture layer image
    //

    std::optional<RgbaImageData> textureImage;

    switch (textureOrigin)
    {
        case ShipDefinition::TextureOriginType::Texture:
        {
            // Just load as-is

            try
            {
                textureImage.emplace(
                    ImageFileTools::LoadImageRgbaLowerLeft(absoluteTextureLayerImageFilePath));
            }
            catch (GameException const & gex)
            {
                throw GameException("Error loading texture layer image: " + std::string(gex.what()));
            }

            break;
        }

        case ShipDefinition::TextureOriginType::StructuralImage:
        {
            // Resize it up - ideally by 8, but don't exceed 4096 (magic number) in any dimension

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