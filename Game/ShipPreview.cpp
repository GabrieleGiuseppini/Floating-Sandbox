/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreview.h"

#include "ImageFileTools.h"
#include "ShipDefinitionFile.h"

#include <GameCore/ImageTools.h>

#include <cassert>

ShipPreview ShipPreview::Load(
    std::filesystem::path const & filepath,
    ImageSize const & maxSize)
{
    std::filesystem::path previewImageFilePath;
    std::optional<ImageSize> originalSize;
    std::optional<ShipMetadata> shipMetadata;

    if (ShipDefinitionFile::IsShipDefinitionFile(filepath))
    {
        //
        // Load full definition
        //

        ShipDefinitionFile sdf = ShipDefinitionFile::Create(filepath);

        std::filesystem::path basePath = filepath.parent_path();

        if (!!sdf.TextureLayerImageFilePath)
        {
            // Use texture for preview
            previewImageFilePath = basePath  / *sdf.TextureLayerImageFilePath;
        }
        else
        {
            // Use structural image for preview
            previewImageFilePath = basePath / sdf.StructuralLayerImageFilePath;
        }

        // Original size is from structure
        originalSize = ImageFileTools::GetImageSize(basePath / sdf.StructuralLayerImageFilePath);

        shipMetadata.emplace(sdf.Metadata);
    }
    else
    {
        //
        // Assume it's just a structural image
        //

        previewImageFilePath = filepath;

        originalSize = ImageFileTools::GetImageSize(filepath);

        shipMetadata.emplace(std::filesystem::path(filepath).stem().string());
    }

    assert(!!originalSize);
    assert(!!shipMetadata);

    //
    // Load preview image and trim it
    //

    auto previewImage = ImageFileTools::LoadImageRgbaLowerLeftAndResize(
        previewImageFilePath,
        maxSize);

    auto trimmedPreviewImage = ImageTools::Trim(std::move(previewImage));

    return ShipPreview(
        std::move(trimmedPreviewImage),
        std::move(*originalSize),
        *shipMetadata);
}