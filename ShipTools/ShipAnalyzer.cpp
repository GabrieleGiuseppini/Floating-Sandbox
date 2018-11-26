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

ShipAnalyzer::AnalysisInfo ShipAnalyzer::Analyze(
    std::string const & inputFile,
    std::string const & materialsFile)
{
    // Load image
    auto image = ResourceLoader::LoadImageRgbUpperLeft(std::filesystem::path(inputFile));

    float const halfWidth = static_cast<float>(image.Size.Width) / 2.0f;

    // Load materials
    auto materials = ResourceLoader::LoadMaterials(materialsFile);

    // Visit all points
    ShipAnalyzer::AnalysisInfo analysisInfo;
    float numPoints = 0.0f;
    float numBuoyantPoints = 0.0f;
    for (int x = 0; x < image.Size.Width; ++x)
    {
        float worldX = static_cast<float>(x) - halfWidth;

        // From bottom to top
        for (int y = 0; y < image.Size.Height; ++y)
        {
            float worldY = static_cast<float>(y);

            auto pixelIndex = (x + (image.Size.Height - y - 1) * image.Size.Width) * 3;

            std::array<uint8_t, 3u> rgbColour = {
                image.Data[pixelIndex + 0],
                image.Data[pixelIndex + 1],
                image.Data[pixelIndex + 2] };

            Material const * material = materials->Find(rgbColour);
            if (nullptr != material)
            {
                analysisInfo.TotalMass += material->Mass;

                numPoints += 1.0f;

                if (!material->IsHull)
                    numBuoyantPoints += 1.0f;

                analysisInfo.BaricentricX += worldX * material->Mass;
                analysisInfo.BaricentricY += worldY * material->Mass;
            }
        }
    }

    if (numPoints != 0.0f)
    {
        analysisInfo.MassPerPoint = analysisInfo.TotalMass / numPoints;
    }

    if (numBuoyantPoints != 0.0f)
    {
        analysisInfo.BuoyantMassPerPoint = analysisInfo.TotalMass / numBuoyantPoints;
    }

    if (analysisInfo.TotalMass != 0.0f)
    {
        analysisInfo.BaricentricX /= analysisInfo.TotalMass;
        analysisInfo.BaricentricY /= analysisInfo.TotalMass;
    }

    return analysisInfo;
}