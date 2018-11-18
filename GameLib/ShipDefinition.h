/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ImageData.h"
#include "ShipMetadata.h"
#include "Vectors.h"

#include <memory>
#include <optional>
#include <string>

/*
* The complete definition of a ship.
*/
struct ShipDefinition
{
public:

    ImageData StructuralImage;

    std::optional<ImageData> TextureImage;

    ShipMetadata const Metadata;

    ShipDefinition(
        ImageData structuralImage,
        std::optional<ImageData> textureImage,
        ShipMetadata const metadata)
        : StructuralImage(std::move(structuralImage))
        , TextureImage(std::move(textureImage))
        , Metadata(std::move(metadata))
    {
    }
};
