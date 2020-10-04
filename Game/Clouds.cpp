/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <algorithm>

namespace Physics {

//
// We keep clouds in a virtual 3.0 x 1.0 x 1.0 space, mapped as follows:
//  X: only the central [-0.5, 0.5] is visible, the remaining 1.0 on either side is to allow clouds to disappear
//  Y: 0.0 @ horizon, 1.0 @ top
//  Z: 0.0 closest, 1.0 furthest
//

float constexpr CloudSpaceWidth = 3.0f;
float constexpr MaxCloudSpaceX = CloudSpaceWidth / 2.0f;

void Clouds::Update(
    float currentSimulationTime,
    float baseAndStormSpeedMagnitude,
    Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters)
{
    float const windSign = baseAndStormSpeedMagnitude < 0.0f ? -1.0f : 1.0f;

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
            uint32_t const cloudId = mLastCloudId++;

            // Choose z stratum, between 0.0 and 1.0, starting from middle
            uint32_t constexpr NumZStrata = 5;
            float const z = static_cast<float>((cloudId + NumZStrata / 2) % NumZStrata) * 1.0f / static_cast<float>(NumZStrata - 1);
            float const z2 = z * z; // Augment density at lower Z values

            // Choose y stratum, between 0.3 and 0.9, starting from middle
            uint32_t constexpr NumYStrata = 3;
            float const y = 0.3f + static_cast<float>((cloudId + NumYStrata / 2) % NumYStrata) * 0.6f / static_cast<float>(NumYStrata - 1);

            // Calculate scale == random, but obeying perspective
            float const scale =
                GameRandomEngine::GetInstance().GenerateUniformReal(1.0f, 1.2f)
                / (0.66f * z2 + 1.0f);

            // Calculate X speed == random, but obeying perspective
            float const linearSpeedX =
                GameRandomEngine::GetInstance().GenerateUniformReal(0.003f, 0.007f)
                / (1.2f * z2 + 1.0f);

            mClouds.emplace_back(
                new Cloud(
                    cloudId,
                    GameRandomEngine::GetInstance().GenerateUniformReal(-MaxCloudSpaceX, MaxCloudSpaceX), // InitialX
                    y,
                    z2,
                    scale,
                    1.0f, // Darkening
                    linearSpeedX,
                    0.0f)); // TODO
        }

        // Sort by Z, so that we upload the furthest clouds first
        std::sort(
            mClouds.begin(),
            mClouds.end(),
            [](auto const & c1, auto const & c2)
            {
                return c1->Z > c2->Z;
            });
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
            // TODOHERE
            mStormClouds.emplace_back(
                new Cloud(
                    mLastCloudId++,
                    -MaxCloudSpaceX * windSign, // InitialX
                    1.0f, // Y, TODO
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal(), // TODO: z=0?
                    stormParameters.CloudsSize,
                    stormParameters.CloudDarkening, // Darkening
                    GameRandomEngine::GetInstance().GenerateUniformReal(0.003f, 0.007f),
                    0.0f)); // TODO
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
    float const globalCloudSpeed = windSign * 0.03f * std::pow(std::abs(baseAndStormSpeedMagnitude), 1.7f);

    for (auto & cloud : mClouds)
    {
        cloud->Update(currentSimulationTime, globalCloudSpeed);

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
        (*it)->Update(currentSimulationTime, globalCloudSpeed);

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