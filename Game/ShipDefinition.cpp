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
    // Load definition
    auto sdf = ShipDefinitionFile::Load(filepath);

    //
    // Load images
    //

    ImageData structuralImage = ImageFileTools::LoadImageRgb(sdf.StructuralLayerImageFilePath);

    std::optional<RgbImageData> ropesLayerImage;
    if (sdf.RopesLayerImageFilePath.has_value())
    {
        try
        {
            ropesLayerImage.emplace(
                ImageFileTools::LoadImageRgb(*sdf.RopesLayerImageFilePath));
        }
        catch (GameException const & gex)
        {
            throw GameException("Error loading rope layer image: " + std::string(gex.what()));
        }
    }

    std::optional<RgbImageData> electricalLayerImage;
    if (sdf.ElectricalLayerImageFilePath.has_value())
    {
        try
        {
            electricalLayerImage.emplace(
                ImageFileTools::LoadImageRgb(*sdf.ElectricalLayerImageFilePath));
        }
        catch (GameException const & gex)
        {
            throw GameException("Error loading electrical layer image: " + std::string(gex.what()));
        }
    }

    std::optional<RgbaImageData> textureLayerImage;
    if (sdf.TextureLayerImageFilePath.has_value())
    {
        try
        {
            textureLayerImage.emplace(
                ImageFileTools::LoadImageRgba(*sdf.TextureLayerImageFilePath));
        }
        catch (GameException const & gex)
        {
            throw GameException("Error loading texture layer image: " + std::string(gex.what()));
        }
    }

    //
    // Create definition
    //

    return ShipDefinition(
        std::move(structuralImage),
        std::move(ropesLayerImage),
        std::move(electricalLayerImage),
        std::move(textureLayerImage),
        std::move(sdf.AutoTexturizationSettings),
        sdf.Metadata,
        sdf.PhysicsData);
}