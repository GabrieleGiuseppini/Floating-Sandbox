/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

//
// We keep clouds in a virtual 3.0 x 1.0 space, of which only
// the central [-0.5, 0.5] x [-0.5, 0.5] slice will be visible
//

float constexpr CloudSpaceWidth = 3.0f;
float constexpr MaxCloudSpaceX = 1.5f;
float constexpr MaxCloudSpaceY = 0.5f;

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
                    ++mLastCloudId,
                    GameRandomEngine::GetInstance().GenerateUniformReal(-MaxCloudSpaceX, MaxCloudSpaceX),
                    GameRandomEngine::GetInstance().GenerateUniformReal(-MaxCloudSpaceY, MaxCloudSpaceY),
                    GameRandomEngine::GetInstance().GenerateUniformReal(1.0f, 1.3f), // Size
                    1.0f));
        }
    }


    //
    // Fill up to storm cloud count
    //

    if (mStormClouds.size() < stormParameters.NumberOfClouds)
    {
        // Add a cloud if the last cloud (arbitrary) is already enough ahead
        if (mStormClouds.empty()
            || (baseAndStormSpeedMagnitude >= 0.0f && mStormClouds.back()->X >= -MaxCloudSpaceX + CloudSpaceWidth / static_cast<float>(stormParameters.NumberOfClouds))
            || (baseAndStormSpeedMagnitude < 0.0f && mStormClouds.back()->X <= MaxCloudSpaceX - CloudSpaceWidth / static_cast<float>(stormParameters.NumberOfClouds)))
        {
            mStormClouds.emplace_back(
                new Cloud(
                    ++mLastCloudId,
                    -MaxCloudSpaceX * windSign,
                    GameRandomEngine::GetInstance().GenerateUniformReal(-MaxCloudSpaceY, MaxCloudSpaceY),
                    stormParameters.CloudsSize,
                    stormParameters.CloudDarkening));
        }
    }


    //
    // Update clouds
    //

    // Convert wind speed into cloud speed.
    //
    // We do not take variable wind speed into account, otherwise clouds would move with gusts
    // and we don't want that. We do take storm wind into account though.
    // Also, higher winds should make clouds move over-linearly faster.
    //
    // A linear factor of 1.0/8.0 worked fine at low wind speeds.
    float const cloudSpeed = windSign * 0.03f * std::pow(std::abs(baseAndStormSpeedMagnitude), 1.7f);

    for (auto & cloud : mClouds)
    {
        cloud->Update(
            currentSimulationTime,
            cloudSpeed);

        // Manage clouds leaving space: rollover and update darkening when crossing border
        if (baseAndStormSpeedMagnitude >= 0.0f && cloud->X > MaxCloudSpaceX)
        {
            cloud->X -= CloudSpaceWidth;
            cloud->Darkening = stormParameters.CloudDarkening;
        }
        else if (baseAndStormSpeedMagnitude < 0.0f && cloud->X < -MaxCloudSpaceX)
        {
            cloud->X += CloudSpaceWidth;
            cloud->Darkening = stormParameters.CloudDarkening;
        }
    }

    for (auto it = mStormClouds.begin(); it != mStormClouds.end();)
    {
        (*it)->Update(
            currentSimulationTime,
            cloudSpeed);

        // Manage clouds leaving space: retire when cross border if too many, else rollover
        if (baseAndStormSpeedMagnitude >= 0.0f && (*it)->X > MaxCloudSpaceX)
        {
            if (mStormClouds.size() > stormParameters.NumberOfClouds)
            {
                it = mStormClouds.erase(it);
            }
            else
            {
                // Rollover and catch up
                (*it)->X -= CloudSpaceWidth;
                (*it)->Scale = stormParameters.CloudsSize;
                (*it)->Darkening = stormParameters.CloudDarkening;
                ++it;
            }
        }
        else if (baseAndStormSpeedMagnitude < 0.0f && (*it)->X < -MaxCloudSpaceX)
        {
            if (mStormClouds.size() > stormParameters.NumberOfClouds)
            {
                it = mStormClouds.erase(it);
            }
            else
            {
                // Rollover and catch up
                (*it)->X += CloudSpaceWidth;
                (*it)->Scale = stormParameters.CloudsSize;
                (*it)->Darkening = stormParameters.CloudDarkening;
                ++it;
            }
        }
        else
        {
            ++it;
        }
    }
}

}