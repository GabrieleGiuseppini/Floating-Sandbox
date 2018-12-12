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

    ImageData StructuralLayerImage;

    std::optional<ImageData> RopeLayerImage;

    std::optional<ImageData> ElectricalLayerImage;

    ImageData TextureLayerImage;

    enum class TextureOriginType
    {
        // The texture comes from the proper texture layer
        Texture,

        // The texture is a fallback to the structural image
        StructuralImage
    };

    TextureOriginType TextureOrigin;

    ShipMetadata const Metadata;

    ShipDefinition(
        ImageData structuralLayerImage,
        std::optional<ImageData> ropeLayerImage,
        std::optional<ImageData> electricalLayerImage,
        ImageData textureLayerImage,
        TextureOriginType textureOrigin,
        ShipMetadata const metadata)
        : StructuralLayerImage(std::move(structuralLayerImage))
        , RopeLayerImage(std::move(ropeLayerImage))
        , ElectricalLayerImage(std::move(electricalLayerImage))
        , TextureLayerImage(std::move(textureLayerImage))
        , TextureOrigin(textureOrigin)
        , Metadata(std::move(metadata))
    {
    }
};
