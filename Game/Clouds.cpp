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
    float baseAndStormSpeedMagnitude,
    Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters)
{
    //
    // Update cloud count
    //

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
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 100.0f,    // OffsetX
                    GameRandomEngine::GetInstance().GenerateUniformReal(0.003f, 0.007f),         // SpeedX1
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.00006f,  // AmpX
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.01f,     // SpeedX2
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 100.0f,    // OffsetY
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.00007f,  // AmpY
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.005f,    // SpeedY
                    0.27f + static_cast<float>(c) / static_cast<float>(c + 3), // OffsetScale - the earlier clouds are smaller
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.0005f,   // AmpScale
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.002f));  // SpeedScale 
        }
    }


    //
    // Update clouds
    //

    // We do not take variable wind speed into account, otherwise clouds would move with gusts
    // and we don't want that. We do take storm wind into account though.
    // Also, higher winds should make clouds move over-linearly faster
    float const cloudSpeed = baseAndStormSpeedMagnitude / 8.0f; // Clouds move slower than wind

    // Update all clouds
    for (auto & cloud : mClouds)
    {
        cloud->Update(
            currentSimulationTime,
            cloudSpeed);
    }


    //
    // Update cloud darkening
    //

    mCloudDarkening = stormParameters.CloudDarkening;
}

}