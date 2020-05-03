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
    std::optional<RgbaImageData> textureLayerImage;
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
                    ImageFileTools::LoadImageRgb(basePath / *sdf.RopesLayerImageFilePath));
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
                    ImageFileTools::LoadImageRgb(basePath / *sdf.ElectricalLayerImageFilePath));
            }
            catch (GameException const & gex)
            {
                throw GameException("Error loading electrical layer image: " + std::string(gex.what()));
            }
        }

        //
        // Load texture layer image
        //

        if (!!sdf.TextureLayerImageFilePath)
        {
            try
            {
                textureLayerImage.emplace(
                    ImageFileTools::LoadImageRgba(basePath / *sdf.TextureLayerImageFilePath));
            }
            catch (GameException const & gex)
            {
                throw GameException("Error loading texture layer image: " + std::string(gex.what()));
            }
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

        shipMetadata.emplace(std::filesystem::path(filepath).stem().string());
    }

    assert(!!shipMetadata);

    //
    // Load structural layer image
    //

    ImageData structuralImage = ImageFileTools::LoadImageRgb(absoluteStructuralLayerImageFilePath);

    //
    // Create definition
    //

    return ShipDefinition(
        std::move(structuralImage),
        std::move(ropesLayerImage),
        std::move(electricalLayerImage),
        std::move(textureLayerImage),
        *shipMetadata);
}