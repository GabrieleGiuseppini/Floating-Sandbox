/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

namespace Physics {

//
// We keep clouds in a virtual 3.0 x 1.0 space, of which only
// the central 1.0-wide slice is visible
//

float constexpr CloudSpaceWidth = 3.0f;
float constexpr MinCloudSpaceX = -1.5f;
float constexpr MaxCloudSpaceX = 1.5f;

void Clouds::Update(
    float currentSimulationTime,
    float baseAndStormSpeedMagnitude,
    Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters)
{
    float windSign = baseAndStormSpeedMagnitude < 0.0f ? -1.0f : 1.0f;

    //
    // Update normal cloud count
    //

    // Resize clouds vector
    if (mClouds.size() > gameParameters.NumberOfClouds)
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
    // Update storm cloud count
    //

    if (mStormClouds.size() < stormParameters.NumberOfClouds)
    {
        // Add a cloud if the last cloud (arbitrary) is already enough ahead
        if (mStormClouds.empty()
            || MaxCloudSpaceX - std::abs(mStormClouds.back()->GetX()) >= 1.0f / static_cast<float>(stormParameters.NumberOfClouds))
        {
            // TODOTEST: not good yet, bigger!
            //float sizeMedian = 0.85f + 1.20f * stormParameters.CloudsSize;
            float sizeMedian = 1.50f;
            float sizeDev = 0.25f + 0.45f * stormParameters.CloudsSize;

            // TODO: move most to cctor of Cloud
            mStormClouds.emplace_back(
                new Cloud(
                    MaxCloudSpaceX * windSign,    // OffsetX
                    GameRandomEngine::GetInstance().GenerateUniformReal(0.003f, 0.007f),         // SpeedX1
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.00006f,  // AmpX
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.01f,     // SpeedX2
                    GameRandomEngine::GetInstance().GenerateUniformReal(-0.5, 0.5f),    // OffsetY
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.00007f,  // AmpY
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.005f,    // SpeedY
                    GameRandomEngine::GetInstance().GenerateUniformReal(
                        sizeMedian - sizeDev,
                        sizeMedian + sizeDev), // OffsetScale
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.0005f,   // AmpScale
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.002f));  // SpeedScale 

        }
    }


    //
    // Update clouds
    //

    // We do not take variable wind speed into account, otherwise clouds would move with gusts
    // and we don't want that. We do take storm wind into account though.
    // Also, higher winds should make clouds move over-linearly faster.
    //
    // A linear factor of 1.0/8.0 worked fine at low wind speeds.    
    // TODO: seems broken for negative wind
    float const cloudSpeed = 
        windSign 
        * (- 7.0f * std::abs(baseAndStormSpeedMagnitude) / 8.0f + std::pow(std::abs(baseAndStormSpeedMagnitude), 1.04f));

    for (auto & cloud : mClouds)
    {
        cloud->Update(
            currentSimulationTime,
            cloudSpeed);

        // Manage clouds leaving space:
        // - Normal clouds: rollover when cross border
        // TODOHERE
    }

    for (auto & cloud : mStormClouds)
    {
        cloud->Update(
            currentSimulationTime,
            cloudSpeed);

        // Manage clouds leaving space:
        // - Storm clouds: retire when cross border if too many, else rollover
        // TODOHERE
    }


    //
    // Update cloud darkening
    //

    mCloudDarkening = stormParameters.CloudDarkening;
}

}