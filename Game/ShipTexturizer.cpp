/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipTexturizer.h"

#include "ImageFileTools.h"

#include <GameCore/Log.h>

#include <algorithm>
#include <chrono>

void ShipTexturizer::VerifyMaterialDatabase(MaterialDatabase const & materialDatabase) const
{
    // TODO
}

RgbaImageData ShipTexturizer::Texturize(
    ImageSize const & structureSize,
    ShipBuildPointIndexMatrix const & pointMatrix, // One more point on each side, to avoid checking for boundaries
    std::vector<ShipBuildPoint> const & points) const
{
    //
    // Calculate target texture size: integral multiple of structure size, but without
    // exceeding 4096 (magic number, also max texture size for low-end gfx cards)
    //

    int const maxDimension = std::max(structureSize.Width, structureSize.Height);
    assert(maxDimension > 0);

    int const magnificationFactor = std::max(1, 4096 / maxDimension);

    ImageSize const textureSize = structureSize * magnificationFactor;

    //
    // Create texture
    //

    auto newImageData = std::make_unique<rgbaColor[]>(textureSize.Width * textureSize.Height);

    auto const startTime = std::chrono::steady_clock::now();

    for (int y = 1; y <= structureSize.Height; ++y)
    {
        for (int x = 1; x <= structureSize.Width; ++x)
        {
            // Get structure pixel color
            rgbaColor const structurePixelColor = pointMatrix[x][y].has_value()
                ? rgbaColor(points[*pointMatrix[x][y]].StructuralMtl.RenderColor)
                : rgbaColor::zero(); // Fully transparent

            // Fill quad
            for (int yy = 0; yy < magnificationFactor; ++yy)
            {
                int const quadOffset =
                    (x - 1) * magnificationFactor
                    + ((y - 1) * magnificationFactor + yy) * textureSize.Width;

                for (int xx = 0; xx < magnificationFactor; ++xx)
                {
                    newImageData[quadOffset + xx] = structurePixelColor;
                }
            }
        }
    }

    auto const endTime = std::chrono::steady_clock::now();
    LogMessage("Ship Auto-Texturization time: ",
        std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count(),
        "us");

    return RgbaImageData(textureSize, std::move(newImageData));
}