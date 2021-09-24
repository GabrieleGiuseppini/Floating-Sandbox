/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ImageFileTools.h"
#include "ShipMetadata.h"

#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/ImageTools.h>

#include <filesystem>

/*
* A partial ship definition, suitable for a preview of the ship.
*/
struct ShipPreview
{
public:

    std::filesystem::path PreviewImageFilePath;
    ShipSpaceSize OriginalSize;
    ShipMetadata Metadata;
    bool IsHD;
    bool HasElectricals;

    ShipPreview(
        std::filesystem::path const & previewImageFilePath,
        ShipSpaceSize const & originalSize,
        ShipMetadata const & metadata,
        bool isHD,
        bool hasElectricals)
        : PreviewImageFilePath(previewImageFilePath)
        , OriginalSize(originalSize)
        , Metadata(metadata)
        , IsHD(isHD)
        , HasElectricals(hasElectricals)
    {
    }

    ShipPreview(ShipPreview && other) = default;
    ShipPreview & operator=(ShipPreview && other) = default;

    RgbaImageData LoadPreviewImage(ImageSize const & maxSize) const
    {
        // Load
        auto previewImage = ImageFileTools::LoadImageRgbaAndResize(
            PreviewImageFilePath,
            maxSize);

        // Trim
        return ImageTools::TrimWhiteOrTransparent(std::move(previewImage));
    }
};
