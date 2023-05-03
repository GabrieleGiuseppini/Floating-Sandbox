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
// Shadows: we map the entire X range of the clouds onto the shadow buffer,
// conceptually divided into three blocks
//

static size_t constexpr ShadowBufferSize = 64 * 3;

// cloud X [-1.5, 1.5] -> index, or width of an element
static float constexpr ShadowBufferDx = CloudSpaceWidth / static_cast<float>(ShadowBufferSize);

// The thickness of the shadow edges, in buffer elements
static register_int constexpr ShadowEdgeHalfThicknessElementCount = 1;


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

    // Offset shadow values so that TODO
    //
    // Note: we only sample the visible (central) slice, so that we
    // do not undergo non-linearities when clouds disappear
    // at the edges of the cloud space

    ////// TODOTEST: Option 1: mean

    ////float sum = 0.0f;
    ////float count = 0.0f;
    ////for (size_t i = ShadowBufferSize / 3; i < ShadowBufferSize * 2 / 3; ++i)
    ////{
    ////    sum += mShadowBuffer[i];
    ////    count += 1.0f;
    ////}

    ////float const adjustment = 1.0f - sum / count;
    ////for (size_t i = 0; i < ShadowBufferSize; ++i)
    ////{
    ////    mShadowBuffer[i] += adjustment;
    ////}

    // TODOTEST: Option 2: min shifted to 1.0

    float minValue = 1.0f;
    for (size_t i = ShadowBufferSize / 3; i < ShadowBufferSize * 2 / 3; ++i)
    {
        minValue = std::min(minValue, mShadowBuffer[i]);
    }

    float const adjustment = 1.0f - minValue;
    for (size_t i = 0; i < ShadowBufferSize; ++i)
    {
        mShadowBuffer[i] += adjustment;
    }
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

void Clouds::UpdateShadows(std::vector<std::unique_ptr<Cloud>> const & clouds)
{
    float constexpr CloudSize = 0.3f; // In cloud X space
    register_int const ClouseSizeElementCount = static_cast<register_int>(std::round(CloudSize / ShadowBufferDx));

    for (auto const & c : clouds)
    {
        float const leftEdgeX = c->X - CloudSize / 2.0f;

        // Fractional index in the sample array
        float const leftEdgeIndexF = (leftEdgeX + CloudSpaceWidth / 2.0f) / ShadowBufferDx;

        // Integral part
        register_int const leftEdgeIndexI = FastTruncateToArchInt(leftEdgeIndexF);
        assert(leftEdgeIndexI < static_cast<register_int>(ShadowBufferSize));

        // Fractional part within sample index and the next sample index
        float const sampleIndexDx = leftEdgeIndexF - leftEdgeIndexI;

        // Edge indices
        register_int const iLeftEdgeLeft = Clamp(leftEdgeIndexI - ShadowEdgeHalfThicknessElementCount, register_int(0), static_cast<register_int>(ShadowBufferSize - 2));
        register_int const iLeftEdgeRight = Clamp(leftEdgeIndexI + ShadowEdgeHalfThicknessElementCount, register_int(0), static_cast<register_int>(ShadowBufferSize - 2));
        register_int const iRightEdgeLeft = Clamp(leftEdgeIndexI + ClouseSizeElementCount - ShadowEdgeHalfThicknessElementCount, register_int(0), static_cast<register_int>(ShadowBufferSize - 2));
        register_int const iRightEdgeRight = Clamp(leftEdgeIndexI + ClouseSizeElementCount + ShadowEdgeHalfThicknessElementCount, register_int(0), static_cast<register_int>(ShadowBufferSize - 2));
        
        register_int i;

        float constexpr EdgeShadow = 0.8f;
        float constexpr FullShadow = 0.5f;

        // Left edge
        for (i = iLeftEdgeLeft; i < iLeftEdgeRight; ++i)
        {
            mShadowBuffer[i] *= 1.0f - (1.0f - EdgeShadow) * (1.0f - sampleIndexDx);
            mShadowBuffer[i + 1] *= 1.0f - (1.0f - EdgeShadow) * (sampleIndexDx);
        }

        // Middle
        for (; i < iRightEdgeLeft; ++i)
        {
            mShadowBuffer[i] *= 1.0f - (1.0f - FullShadow) * (1.0f - sampleIndexDx);
            mShadowBuffer[i + 1] *= 1.0f - (1.0f - FullShadow) * (sampleIndexDx);
        }

        // Right edge
        for (; i < iRightEdgeRight; ++i)
        {
            mShadowBuffer[i] *= 1.0f - (1.0f - EdgeShadow) * (1.0f - sampleIndexDx);
            mShadowBuffer[i + 1] *= 1.0f - (1.0f - EdgeShadow) * (sampleIndexDx);
        }
    }
}

}