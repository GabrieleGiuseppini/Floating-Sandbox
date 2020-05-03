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

    //
    // Resize it up - ideally by 8, but don't exceed 4096 (magic number) in any dimension
    //

    int const maxDimension = std::max(structureSize.Width, structureSize.Height);
    assert(maxDimension > 0);

    int const magnificationFactor = std::max(1, 4096 / maxDimension);

    ImageSize const textureSize = structureSize * magnificationFactor;

    auto newImageData = std::make_unique<rgbaColor[]>(textureSize.Width * textureSize.Height);

    // TODO: optimize (precalc'd indices)
    for (int y = 1; y <= structureSize.Height; ++y)
    {
        for (int x = 1; x <= structureSize.Width; ++x)
        {
            rgbaColor pixelColor;
            if (pointMatrix[x][y].has_value())
            {
                // TODOTEST: We're using RenderColor here, rather than color key, just because
                // material doesn't carry color key
                pixelColor = rgbaColor(points[*pointMatrix[x][y]].StructuralMtl.RenderColor);
            }
            else
            {
                pixelColor = rgbaColor::zero();
            }

            // Fill quad
            for (int yy = 0; yy < magnificationFactor; ++yy)
            {
                for (int xx = 0; xx < magnificationFactor; ++xx)
                {
                    int const destIndex =
                        (x - 1) * magnificationFactor + xx
                        + ((y - 1) * magnificationFactor + yy) * textureSize.Width;

                    newImageData[destIndex] = pixelColor;
                }
            }
        }
    }

    return RgbaImageData(textureSize, std::move(newImageData));
}