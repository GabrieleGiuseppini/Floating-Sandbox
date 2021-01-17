/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipMetadata.h"

#include <GameCore/ImageData.h>

#include <filesystem>

/*
* A partial ship definition, suitable for a preview of the ship.
*/
struct ShipPreview
{
public:

    std::filesystem::path PreviewImageFilePath;
    ImageSize OriginalSize;
    ShipMetadata Metadata;
    bool IsHD;
    bool HasElectricals;

    static ShipPreview Load(std::filesystem::path const & filepath);

    ShipPreview (ShipPreview && other) = default;
    ShipPreview & operator=(ShipPreview && other) = default;

    RgbaImageData LoadPreviewImage(ImageSize const & maxSize) const;

private:

    ShipPreview(
        std::filesystem::path const & previewImageFilePath,
        ImageSize const & originalSize,
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
};
