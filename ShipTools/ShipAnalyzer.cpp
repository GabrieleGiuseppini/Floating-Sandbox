/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-06-30
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ShipAnalyzer.h"

#include <Game/GameParameters.h>
#include <Game/ImageFileTools.h>
#include <Game/MaterialDatabase.h>

#include <GameCore/Vectors.h>

#include <IL/il.h>
#include <IL/ilu.h>

#include <array>
#include <limits>
#include <stdexcept>
#include <vector>

ShipAnalyzer::AnalysisInfo ShipAnalyzer::Analyze(
    std::string const & inputFile,
    std::string const & materialsDir)
{
    // Load image
    auto image = ImageFileTools::LoadImageRgbUpperLeft(std::filesystem::path(inputFile));

    float const halfWidth = static_cast<float>(image.Size.Width) / 2.0f;

    // Load materials
    auto materials = MaterialDatabase::Load(materialsDir);

    // Visit all points
    ShipAnalyzer::AnalysisInfo analysisInfo;
    float totalMass = 0.0f;
    float buoyantMass = 0.0f;
    float numPoints = 0.0f;
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

            StructuralMaterial const * structuralMaterial = materials.FindStructuralMaterial(rgbColour);
            if (nullptr != structuralMaterial)
            {
                numPoints += 1.0f;

                totalMass += structuralMaterial->Mass;

                buoyantMass +=
                    structuralMaterial->Mass
                    - (structuralMaterial->WaterVolumeFill * GameParameters::WaterMass);

                // Update center of mass
                analysisInfo.BaricentricX += worldX * structuralMaterial->Mass;
                analysisInfo.BaricentricY += worldY * structuralMaterial->Mass;
            }
        }
    }

    analysisInfo.TotalMass = totalMass;

    if (numPoints != 0.0f)
    {
        analysisInfo.MassPerPoint = totalMass / numPoints;
        analysisInfo.BuoyantMassPerPoint = buoyantMass / numPoints;
    }

    if (analysisInfo.TotalMass != 0.0f)
    {
        analysisInfo.BaricentricX /= analysisInfo.TotalMass;
        analysisInfo.BaricentricY /= analysisInfo.TotalMass;
    }

    return analysisInfo;
}