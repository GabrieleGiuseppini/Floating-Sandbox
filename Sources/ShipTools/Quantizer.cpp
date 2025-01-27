/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-06-25
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Quantizer.h"

#include <Game/MaterialDatabase.h>
#include <Game/PngImageFileTools.h>

#include <Core/ImageData.h>
#include <Core/Vectors.h>

#include <limits>
#include <stdexcept>
#include <vector>

void Quantizer::Quantize(
    std::string const & inputFile,
    std::string const & outputFile,
    std::string const & materialsDir,
    bool doKeepRopes,
    bool doKeepGlass,
    std::optional<rgbColor> targetFixedColor)
{
    //
    // Load image
    //

    RgbaImageData image = PngImageFileTools::LoadImageRgba(inputFile);

    //
    // Create set of colors to quantize to
    //

    auto materials = MaterialDatabase::Load(materialsDir);

    std::vector<std::pair<vec3f, rgbColor>> gameColors;

    for (auto const & entry : materials.GetStructuralMaterialColorMap())
    {
        if ( (!entry.second.IsUniqueType(StructuralMaterial::MaterialUniqueType::Rope) || doKeepRopes)
            && (entry.second.Name != "Glass" || doKeepGlass))
        {
            gameColors.emplace_back(
                std::make_pair(
                    entry.first.toVec3f(),
                    rgbColor(entry.first)));
        }
    }

    // Add pure white
    static rgbColor PureWhite = { 255, 255, 255 };
    gameColors.emplace_back(
        std::make_pair(
            vec3f(1.0f, 1.0f, 1.0f),
            PureWhite));


    //
    // Quantize image
    //

    for (int r = 0; r < image.Size.height; ++r)
    {
        for (int c = 0; c < image.Size.width; ++c)
        {
            rgbaColor & imgColor = image[{c, r}];
            vec3f imgColorF = imgColor.toVec3f();

            std::optional<rgbColor> bestColor;

            if (!targetFixedColor)
            {
                // Find closest color
                std::optional<size_t> bestGameColorIndex;
                float bestColorSquareDistance = std::numeric_limits<float>::max();
                for (size_t gameColor = 0; gameColor < gameColors.size(); ++gameColor)
                {
                    float colorSquareDistance = (imgColorF - gameColors[gameColor].first).squareLength();
                    if (colorSquareDistance < bestColorSquareDistance)
                    {
                        bestGameColorIndex = gameColor;
                        bestColorSquareDistance = colorSquareDistance;
                    }
                }

                // Store color
                assert(!!bestGameColorIndex);
                bestColor = gameColors[*bestGameColorIndex].second;
            }
            else
            {
                // Assign a color only if not transparent
                if (imgColor.a != 0)
                {
                    bestColor = *targetFixedColor;
                }
            }

            if (!!bestColor)
            {
                imgColor = rgbaColor(*bestColor, 255);
            }
            else
            {
                // Full white
                imgColor = rgbaColor(PureWhite, 255);
            }
        }
    }


    //
    // Save image
    //

    PngImageFileTools::SavePngImage(image, outputFile);
}