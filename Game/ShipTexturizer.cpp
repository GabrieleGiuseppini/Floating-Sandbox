/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipTexturizer.h"

#include "ImageFileTools.h"

#include <algorithm>

void ShipTexturizer::VerifyMaterialDatabase(MaterialDatabase const & materialDatabase) const
{
    // TODO
}

RgbaImageData ShipTexturizer::Texturize(
    ImageSize const & structureSize,
    ShipBuildPointIndexMatrix const & pointMatrix, // One more point on each side, to avoid checking for boundaries
    std::vector<ShipBuildPoint> const & points) const
{
    // TODOTEST: for the time being we always do it with structure, as we used to do
    /*
    // Resize it up - ideally by 8, but don't exceed 4096 (magic number) in any dimension

    int maxDimension = std::max(shipDefinition.StructuralLayerImage.Size.Width, shipDefinition.StructuralLayerImage.Size.Height);
    int magnify = 8;
    while (maxDimension * magnify > 4096 && magnify > 1)
        magnify /= 2;

    return ImageFileTools::LoadImageRgbaAndMagnify(
        shipDefinition.
            absoluteTextureLayerImageFilePath,
            magnify));
    */
    std::unique_ptr<rgbaColor[]> newImageData;
    return RgbaImageData(1, 1, std::move(newImageData));
}