/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

namespace Physics {

void Clouds::Update(
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    // Resize clouds vector
    if (gameParameters.NumberOfClouds < mClouds.size())
    {
        mClouds.resize(gameParameters.NumberOfClouds);
    }
    else
    {
        for (size_t c = mClouds.size(); c < gameParameters.NumberOfClouds; ++c)
        {
            mClouds.emplace_back(
                new Cloud(
                    GameRandomEngine::GetInstance().GenerateRandomNormalizedReal() * 100.0f,    // OffsetX
                    GameRandomEngine::GetInstance().GenerateRandomNormalizedReal() * 0.01f,     // SpeedX1
                    GameRandomEngine::GetInstance().GenerateRandomNormalizedReal() * 0.04f,     // AmpX
                    GameRandomEngine::GetInstance().GenerateRandomNormalizedReal() * 0.01f,     // SpeedX2
                    GameRandomEngine::GetInstance().GenerateRandomNormalizedReal() * 100.0f,    // OffsetY
                    GameRandomEngine::GetInstance().GenerateRandomNormalizedReal() * 0.001f,    // AmpY
                    GameRandomEngine::GetInstance().GenerateRandomNormalizedReal() * 0.005f,    // SpeedY
                    0.2f + static_cast<float>(c) / static_cast<float>(c + 3), // OffsetScale - the earlier clouds are smaller
                    GameRandomEngine::GetInstance().GenerateRandomNormalizedReal() * 0.05f,     // AmpScale
                    GameRandomEngine::GetInstance().GenerateRandomNormalizedReal() * 0.005f));  // SpeedScale
        }
    }

    // Update clouds
    float const cloudSpeed = gameParameters.WindSpeedBase / 8.0f; // Clouds move slower than wind
    for (auto & cloud : mClouds)
    {
        cloud->Update(
            currentSimulationTime,
            cloudSpeed);
    }

}

}