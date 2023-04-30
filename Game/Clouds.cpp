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

//
// Shadows: we map the entire X range of the clouds onto the shadow buffer
//

static size_t constexpr ShadowBufferSize = 64;

// ndc X (visible part only) -> index
static float constexpr ShadowBufferDNdcx = static_cast<float>(ShadowBufferSize);

// The thickness of the shadow edges, NDC
static float constexpr ShadowEdgeHalfThicknessNdc = 0.05f;


Clouds::Clouds()
    : mLastCloudId(0)
    , mClouds()
    , mStormClouds()
    , mShadowBuffer(ShadowBufferSize)
{}

void Clouds::Update(
    float /*currentSimulationTime*/,
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
        // Trim off some clouds
        mClouds.resize(gameParameters.NumberOfClouds);
    }
    else if (mClouds.size() < gameParameters.NumberOfClouds)
    {
        // Add some clouds
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
                GameRandomEngine::GetInstance().GenerateUniformReal(0.004f, 0.007f)
                / (1.2f * z2 + 1.0f);

            mClouds.emplace_back(
                new Cloud(
                cloudId,
                GameRandomEngine::GetInstance().GenerateUniformReal(-MaxCloudSpaceX, MaxCloudSpaceX), // Initial X
                y,
                z2,
                scale,
                1.0f, // Darkening
                linearSpeedX,
                GameRandomEngine::GetInstance().GenerateNormalizedUniformReal())); // Initial growth phase
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
            mStormClouds.emplace_back(
                new Cloud(
                    mLastCloudId++,
                    -MaxCloudSpaceX * windSign, // Initial X
                    GameRandomEngine::GetInstance().GenerateUniformReal(-1.0f, 1.0f), // Y [-1.0 -> 1.0]
                    0.0f, // Z
                    stormParameters.CloudsSize,
                    stormParameters.CloudDarkening, // Darkening
                    GameRandomEngine::GetInstance().GenerateUniformReal(0.003f, 0.007f), // Linear speed X
                    GameRandomEngine::GetInstance().GenerateNormalizedUniformReal())); // Initial growth phase
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
        cloud->Update(globalCloudSpeed);

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
        (*it)->Update(globalCloudSpeed);

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

    //
    // Update shadows
    //

    mShadowBuffer.fill(1.0f);

    UpdateShadows(mClouds);
    UpdateShadows(mStormClouds);
}

void Clouds::Upload(Render::RenderContext & renderContext) const
{
    //
    // Upload clouds
    //

    renderContext.UploadCloudsStart(mClouds.size() + mStormClouds.size());

    for (auto const & cloud : mClouds)
    {
        renderContext.UploadCloud(
            cloud->Id,
            cloud->X,
            cloud->Y,
            cloud->Z,
            cloud->Scale,
            cloud->Darkening,
            cloud->GrowthProgress);
    }

    for (auto const & cloud : mStormClouds)
    {
        renderContext.UploadCloud(
            cloud->Id,
            cloud->X,
            cloud->Y,
            cloud->Z,
            cloud->Scale,
            cloud->Darkening,
            cloud->GrowthProgress);
    }

    renderContext.UploadCloudsEnd();

    //
    // Upload shadows
    //

    renderContext.UploadCloudShadows(
        mShadowBuffer.data(),
        mShadowBuffer.GetSize());
}

/////////////////////////////////////////////////////////////////////////////////////////////

namespace {

    register_int NdcXToShadowBufferSampleIndex(float ndcX) // -> [0, ShadowBufferSize]
    {
        // Fractional index in the sample array
        float const sampleIndexF = std::max((ndcX + 0.5f) * ShadowBufferDNdcx, 0.0f);

        // Integral part
        register_int const sampleIndexI = FastTruncateToArchInt(sampleIndexF);

        return std::min(sampleIndexI, static_cast<register_int>(ShadowBufferSize));
    }
}

void Clouds::UpdateShadows(std::vector<std::unique_ptr<Cloud>> const & clouds)
{
    for (auto const & c : clouds)
    {
        // Check if cloud intesects screen

        // TODO: solve cloud size (if needed)
        float constexpr cloudSizeNdc = 0.4f;
        float const leftEdgeX = c->X - cloudSizeNdc / 2.0f;
        float const rightEdgeX = c->X + cloudSizeNdc / 2.0f;
        if (leftEdgeX < 0.5f && rightEdgeX > -0.5f)
        {
            // Left edge
            for (register_int i = NdcXToShadowBufferSampleIndex(leftEdgeX - ShadowEdgeHalfThicknessNdc);
                i < NdcXToShadowBufferSampleIndex(leftEdgeX + ShadowEdgeHalfThicknessNdc);
                ++i)
            {
                mShadowBuffer[i] *= 0.8f;
            }

            // Middle
            for (register_int i = NdcXToShadowBufferSampleIndex(leftEdgeX + ShadowEdgeHalfThicknessNdc);
                i < NdcXToShadowBufferSampleIndex(rightEdgeX - ShadowEdgeHalfThicknessNdc);
                ++i)
            {
                // TODOTEST
                //mShadowBuffer[i] *= 0.01f;
                mShadowBuffer[i] *= 0.35f;
            }

            // Right edge
            for (register_int i = NdcXToShadowBufferSampleIndex(rightEdgeX - ShadowEdgeHalfThicknessNdc);
                i < NdcXToShadowBufferSampleIndex(rightEdgeX + ShadowEdgeHalfThicknessNdc);
                ++i)
            {
                mShadowBuffer[i] *= 0.8f;
            }
        }
    }
}

}