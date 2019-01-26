/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipMetadata.h"

#include <GameCore/ImageData.h>
#include <GameCore/Vectors.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

/*
* A partial ship definition, suitable for a preview of the ship.
*/
struct ShipPreview
{
public:

    ImageData PreviewImage;

    ShipMetadata const Metadata;

    static ShipPreview Load(
        std::filesystem::path const & filepath,
        int previewWidth);

private:

    ShipPreview(
        ImageData previewImage,
        ShipMetadata const metadata)
        : PreviewImage(std::move(previewImage))
        , Metadata(std::move(metadata))
    {
    }
};
