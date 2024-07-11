/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-06-25
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Quantizer.h"

#include <Game/MaterialDatabase.h>

#include <GameCore/Vectors.h>

#include <IL/il.h>
#include <IL/ilu.h>

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

    ILuint image;
    ilGenImages(1, &image);
    ilBindImage(image);

    if (!ilLoadImage(inputFile.c_str()))
    {
        ILint devilError = ilGetError();
        std::string devilErrorMessage(iluErrorString(devilError));
        throw std::runtime_error("Could not load image '" + inputFile + "': " + devilErrorMessage);
    }

    // Convert format
    if (ilGetInteger(IL_IMAGE_FORMAT) != IL_RGBA
        || ilGetInteger(IL_IMAGE_TYPE) != IL_UNSIGNED_BYTE)
    {
        if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE))
        {
            ILint devilError = ilGetError();
            std::string devilErrorMessage(iluErrorString(devilError));
            throw std::runtime_error("Could not convert image '" + inputFile + "': " + devilErrorMessage);
        }
    }

    ILubyte * imageData = ilGetData();
    int const width = ilGetInteger(IL_IMAGE_WIDTH);
    int const height = ilGetInteger(IL_IMAGE_HEIGHT);

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

    for (int r = 0; r < height; ++r)
    {
        size_t index = r * width * 4;

        for (int c = 0; c < width; ++c, index += 4)
        {
            vec3f imgColor = vec3f(
                static_cast<float>(imageData[index]) / 255.0f,
                static_cast<float>(imageData[index + 1]) / 255.0f,
                static_cast<float>(imageData[index + 2]) / 255.0f);

            std::optional<rgbColor> bestColor;

            if (!targetFixedColor)
            {
                // Find closest color
                std::optional<size_t> bestGameColorIndex;
                float bestColorSquareDistance = std::numeric_limits<float>::max();
                for (size_t gameColor = 0; gameColor < gameColors.size(); ++gameColor)
                {
                    float colorSquareDistance = (imgColor - gameColors[gameColor].first).squareLength();
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
                if (imageData[index + 3] != 0)
                {
                    bestColor = *targetFixedColor;
                }
            }

            if (!!bestColor)
            {
                imageData[index] = bestColor->r;
                imageData[index + 1] = bestColor->g;
                imageData[index + 2] = bestColor->b;
                imageData[index + 3] = 255;
            }
            else
            {
                // Full white
                imageData[index] = PureWhite.r;
                imageData[index + 1] = PureWhite.g;
                imageData[index + 2] = PureWhite.b;
                imageData[index + 3] = 255;
            }
        }
    }


    //
    // Save image
    //

    ilEnable(IL_FILE_OVERWRITE);
    if (!ilSave(IL_PNG, outputFile.c_str()))
    {
        ILint devilError = ilGetError();
        std::string devilErrorMessage(iluErrorString(devilError));
        throw std::runtime_error("Could not save image '" + outputFile + "': " + devilErrorMessage);
    }
}