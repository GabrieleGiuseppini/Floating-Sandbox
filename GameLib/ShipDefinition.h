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

    enum class TextureOriginType
    {
        // The texture comes from the proper texture layer
        Texture,

        // The texture is a fallback to the structural image
        StructuralImage
    };

    ImageData StructuralImage;

    ImageData TextureImage;

    TextureOriginType TextureOrigin;

    ShipMetadata const Metadata;

    ShipDefinition(
        ImageData structuralImage,
        ImageData textureImage,
        TextureOriginType textureOrigin,
        ShipMetadata const metadata)
        : StructuralImage(std::move(structuralImage))
        , TextureImage(std::move(textureImage))
        , TextureOrigin(textureOrigin)
        , Metadata(std::move(metadata))
    {
    }
};
