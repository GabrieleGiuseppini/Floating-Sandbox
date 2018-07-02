/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-06-30
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ShipAnalyzer.h"

#include <GameLib/MaterialDatabase.h>
#include <GameLib/ResourceLoader.h>
#include <GameLib/Vectors.h>

#include <IL/il.h>
#include <IL/ilu.h>

#include <array>
#include <limits>
#include <stdexcept>
#include <vector>

ShipAnalyzer::WeightInfo ShipAnalyzer::Weight(
    std::string const & inputFile,
    std::string const & materialsFile)
{
    // Load image
    auto image = ResourceLoader::LoadImage(std::filesystem::path(inputFile), IL_RGB, IL_ORIGIN_UPPER_LEFT);

    // Load materials
    MaterialDatabase materials = ResourceLoader::LoadMaterials(materialsFile);

    // Visit all points
    ShipAnalyzer::WeightInfo weightInfo;
    float numPoints = 0.0f;
    for (int x = 0; x < image.Size.Width; ++x)
    {
        // From bottom to top
        for (int y = 0; y < image.Size.Height; ++y)
        {
            auto index = (x + (image.Size.Height - y - 1) * image.Size.Width) * 3;

            std::array<uint8_t, 3u> rgbColour = {
                image.Data[index + 0],
                image.Data[index + 1],
                image.Data[index + 2] };

            Material const * material = materials.Get(rgbColour);
            if (nullptr != material)
            {
                weightInfo.TotalMass += material->Mass;
                numPoints += 1.0f;
            }
        }
    }

    if (numPoints != 0.0f)
    {
        weightInfo.MassPerPoint = weightInfo.TotalMass / numPoints;
    }

    return weightInfo;
}