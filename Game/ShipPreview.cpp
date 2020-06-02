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

ShipPreview ShipPreview::Load(std::filesystem::path const & filepath)
{
    //
    // Load definition and create preview from it
    //

    ShipDefinitionFile sdf = ShipDefinitionFile::Load(filepath);

    std::filesystem::path previewImageFilePath;
    bool isHD = false;
    bool hasElectricals = false;

    if (sdf.TextureLayerImageFilePath.has_value())
    {
        // Use the ship's texture as its preview
        previewImageFilePath = *sdf.TextureLayerImageFilePath;

        // Categorize as HD, unless instructed not to do so
        if (!sdf.DoHideHDInPreview)
            isHD = true;
    }
    else
    {
        // Preview is from structural image
        previewImageFilePath = sdf.StructuralLayerImageFilePath;
    }

    // Check whether it has electricals, unless instructed not to do so
    if (!sdf.DoHideElectricalsInPreview)
        hasElectricals = sdf.ElectricalLayerImageFilePath.has_value();

    return ShipPreview(
        previewImageFilePath,
        ImageFileTools::GetImageSize(sdf.StructuralLayerImageFilePath), // Ship size is from structural image
        sdf.Metadata,
        isHD,
        hasElectricals);
}

RgbaImageData ShipPreview::LoadPreviewImage(ImageSize const & maxSize) const
{
    // Load
    auto previewImage = ImageFileTools::LoadImageRgbaAndResize(
        PreviewImageFilePath,
        maxSize);

    // Trim
    return ImageTools::TrimWhiteOrTransparent(std::move(previewImage));
}