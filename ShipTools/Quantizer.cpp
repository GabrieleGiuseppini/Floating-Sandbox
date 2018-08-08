/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-06-25
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Quantizer.h"

#include <GameLib/MaterialDatabase.h>
#include <GameLib/ResourceLoader.h>
#include <GameLib/Vectors.h>

#include <IL/il.h>
#include <IL/ilu.h>

#include <limits>
#include <stdexcept>
#include <vector>

void Quantizer::Quantize(
    std::string const & inputFile,
    std::string const & outputFile,
    std::string const & materialsFile,
    bool doKeepRopes,
    bool doKeepGlass,
    std::optional<std::array<uint8_t, 3u>> targetFixedColor)
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
    if (ilGetInteger(IL_IMAGE_FORMAT) != IL_RGB 
        || ilGetInteger(IL_IMAGE_TYPE) != IL_UNSIGNED_BYTE)
    {
        if (!ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE))
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

    MaterialDatabase materials = ResourceLoader::LoadMaterials(materialsFile);

    std::vector<std::pair<vec3f, std::array<uint8_t, 3u>>> gameColors;

    for (size_t m = 0; m < materials.GetMaterialCount(); ++m)
    {
        Material const & material = materials.GetMaterialAt(m);

        if ( (!material.IsRope || doKeepRopes)
            && (material.Name != "Glass" || doKeepGlass))
        {
            if (!targetFixedColor)
            {
                gameColors.emplace_back(
                    std::make_pair(
                        material.StructuralColour,
                        material.StructuralColourRgb));
            }
            else
            {
                gameColors.emplace_back(
                    std::make_pair(
                        material.StructuralColour,
                        *targetFixedColor));
            }
        }
    }

    // Add pure white
    static std::array<uint8_t, 3u> PureWhite = { 255, 255, 255 };
    gameColors.emplace_back(
        std::make_pair(
            vec3f(1.0f, 1.0f, 1.0f),
            PureWhite));


    //
    // Quantize image
    //
    
    for (int r = 0; r < height; ++r)
    {
        size_t index = r * width * 3;

        for (int c = 0; c < width; ++c, index += 3)
        {
            vec3f imgColor = vec3f(
                static_cast<float>(imageData[index]) / 255.0f,
                static_cast<float>(imageData[index + 1]) / 255.0f,
                static_cast<float>(imageData[index + 2]) / 255.0f);

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
            auto const & bestGameColor = gameColors[*bestGameColorIndex].second;
            imageData[index] = bestGameColor[0];
            imageData[index + 1] = bestGameColor[1];
            imageData[index + 2] = bestGameColor[2];
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