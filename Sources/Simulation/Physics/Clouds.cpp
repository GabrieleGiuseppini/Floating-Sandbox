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

// The thickness of half of the shadow edges, in buffer elements
static register_int constexpr ShadowEdgeHalfThicknessElementCount = 1;

/////////////////////////////////////////////////////////////////////////////////

Clouds::Clouds()
    : mLastCloudId(0)
    , mClouds()
    , mStormClouds()
    , mShadowBuffer(ShadowBufferSize)
{
}

void Clouds::Update(
    float /*currentSimulationTime*/,
    float baseAndStormSpeedMagnitude,
    Storm::Parameters const & stormParameters,
    SimulationParameters const & simulationParameters)
{
    float const windSign = baseAndStormSpeedMagnitude < 0.0f ? -1.0f : 1.0f;

    //
    // Update normal cloud count
    //

    // Resize clouds vector
    if (mClouds.size() > simulationParameters.NumberOfClouds)
    {
        // Trim off some clouds
        mClouds.resize(simulationParameters.NumberOfClouds);
    }
    else if (mClouds.size() < simulationParameters.NumberOfClouds)
    {
        // Add some clouds
        for (size_t c = mClouds.size(); c < simulationParameters.NumberOfClouds; ++c)
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
                    GameRandomEngine::GetInstance().GenerateUniformReal(0.0f, 100.0f), // VolumetricGrowthProgress
                    linearSpeedX));
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
                    GameRandomEngine::GetInstance().GenerateUniformReal(0.0f, 100.0f), // VolumetricGrowthProgress
                    GameRandomEngine::GetInstance().GenerateUniformReal(0.003f, 0.007f))); // Linear speed X
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
    float const absoluteGlobalCloudSpeed = 0.005f * std::pow(std::abs(baseAndStormSpeedMagnitude), 2.1f);
    float const globalCloudSpeed = windSign * absoluteGlobalCloudSpeed;

    for (auto & cloud : mClouds)
    {
        cloud->Update(globalCloudSpeed);

        // Manage clouds leaving space: rollover when crossing border
        if (baseAndStormSpeedMagnitude >= 0.0f && cloud->X > MaxCloudSpaceX)
        {
            cloud->X -= CloudSpaceWidth;
        }
        else if (baseAndStormSpeedMagnitude < 0.0f && cloud->X < -MaxCloudSpaceX)
        {
            cloud->X += CloudSpaceWidth;
        }

        // Update darkening, as still non-storm clouds should still change their color
        // (or else they remain dark)
        cloud->Darkening = stormParameters.CloudDarkening;
    }

    float const stormGlobalCloudSpeed = windSign * std::max(absoluteGlobalCloudSpeed, 12.0f); // Ensure storm clouds ultimately leave the screen

    for (auto it = mStormClouds.begin(); it != mStormClouds.end();)
    {
        (*it)->Update(stormGlobalCloudSpeed);

        // Update darkening
        (*it)->Darkening = stormParameters.CloudDarkening;

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
                ++it;
            }
        }
        else
        {
            ++it;
        }
    }
}

void Clouds::Upload(RenderContext & renderContext)
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
            cloud->VolumetricGrowthProgress);
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
            cloud->VolumetricGrowthProgress);
    }

    renderContext.UploadCloudsEnd();

    //
    // Upload shadows
    //

    if (renderContext.GetOceanRenderDetail() == OceanRenderDetailType::Detailed)
    {
        // Update shadows
        {
            mShadowBuffer.fill<ShadowBufferSize>(1.0f);

            UpdateShadows(mClouds, renderContext.GetViewModel());
            UpdateShadows(mStormClouds, renderContext.GetViewModel());

            OffsetShadowsBuffer_Min();
        }

        // Upload shadows
        renderContext.UploadCloudShadows(
            mShadowBuffer.data(),
            mShadowBuffer.GetSize());
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Clouds::UpdateShadows(
    std::vector<std::unique_ptr<Cloud>> const & clouds,
    ViewModel const & viewModel)
{
    float constexpr BaseCloudSize = 0.3f; // In cloud X space

    float * restrict shadowBuffer = mShadowBuffer.data();

    for (auto const & c : clouds)
    {
        float const cloudSize = BaseCloudSize * c->Scale;
        register_int const cloudSizeElementCount = FastTruncateToArchInt(cloudSize / ShadowBufferDx);

        // Apply the same perspective that we apply for clouds, so that shadows are correct
        // (a bit hacky having to deal with perspective here, but that's in the nature of cloud shadows)
        // Note: when we're paused and panning, this won't update - a bit buggy
        vec2f const perspectivedCloudPos = viewModel.ApplyCloudPerspectiveTransformation(c->X, c->Y, c->Z);

        float const leftEdgeX = perspectivedCloudPos.x - cloudSize / 2.0f;

        // Fractional index in the sample array - might be negative
        float const leftEdgeIndexF = (leftEdgeX + CloudSpaceWidth / 2.0f) / ShadowBufferDx;

        // Integral part
        // Note: leftEdgeIndexF might be negative now, and we want I(-7.6)==-8 (because of left-right interpolation)
        register_int const leftEdgeIndexI = FastTruncateToArchIntTowardsNInfinity(leftEdgeIndexF);
        assert(leftEdgeIndexI < static_cast<register_int>(ShadowBufferSize));

        // Fractional part within sample index and the next sample index
        float const sampleIndexDx = leftEdgeIndexF - leftEdgeIndexI;

        // Coeffs:
        // i-1 (n1): fraction of total shadow onto i-1; depends on fraction of buffer cell covered
        // i (z): fraction of total shadow onto i; independent on buffer cell (always 0.5)
        // i+1 (p1): fraction of total shadow onto i+1; depends on fraction of buffer cell covered

        float constexpr EdgeShadow = 0.75f;
        float const edgeN1Coeff = 1.0f - (1.0f - EdgeShadow) * (1.0f - sampleIndexDx) / 2.0f;
        float const edgeZCoeff = 1.0f - (1.0f - EdgeShadow) * 1.0f / 2.0f;
        float const edgeP1Coeff = 1.0f - (1.0f - EdgeShadow) * sampleIndexDx / 2.0f;

        float constexpr FullShadow = 0.6f;
        float const fullN1Coeff = 1.0f - (1.0f - FullShadow) * (1.0f - sampleIndexDx) / 2.0f;
        float const fullZCoeff = 1.0f - (1.0f - FullShadow) * 1.0f / 2.0f;
        float const fullP1Coeff = 1.0f - (1.0f - FullShadow) * sampleIndexDx / 2.0f;

        assert(edgeN1Coeff <= 1.0f && edgeZCoeff <= 1.0f && edgeP1Coeff <= 1.0f);
        assert(fullN1Coeff <= 1.0f && fullZCoeff <= 1.0f && fullP1Coeff <= 1.0f);

        // Edge indices
        register_int const iLeftEdgeLeft = Clamp(leftEdgeIndexI - ShadowEdgeHalfThicknessElementCount, register_int(1), static_cast<register_int>(ShadowBufferSize - 2));
        register_int const iLeftEdgeRight = std::min(leftEdgeIndexI + ShadowEdgeHalfThicknessElementCount, static_cast<register_int>(ShadowBufferSize - 2));
        register_int const iRightEdgeLeft = std::min(leftEdgeIndexI + cloudSizeElementCount - ShadowEdgeHalfThicknessElementCount, static_cast<register_int>(ShadowBufferSize - 2));
        register_int const iRightEdgeRight = std::min(leftEdgeIndexI + cloudSizeElementCount + ShadowEdgeHalfThicknessElementCount, static_cast<register_int>(ShadowBufferSize - 2));

        register_int i = iLeftEdgeLeft;

        //
        // Left edge
        //

        for (; i < iLeftEdgeRight; ++i)
        {
            assert(i >= 1 && i <= static_cast<register_int>(ShadowBufferSize - 2));

            shadowBuffer[i - 1] *= edgeN1Coeff;
            shadowBuffer[i] *= edgeZCoeff;
            shadowBuffer[i + 1] *= edgeP1Coeff;
        }

        //
        // Middle
        //
        //     i         i
        //   N Z P
        //     N Z P
        //       N Z P
        //         N Z P
        //           N Z P
        //             N Z P
        //

        if (i < iRightEdgeLeft - 1)
        {
            assert(i >= 1 && i <= static_cast<register_int>(ShadowBufferSize - 2));

            shadowBuffer[i - 1] *= fullN1Coeff;
            shadowBuffer[i] *= fullN1Coeff * fullZCoeff;

            ++i;
        }

        for (; i < iRightEdgeLeft - 1; ++i)
        {
            assert(i >= 1 && i <= static_cast<register_int>(ShadowBufferSize - 2));

            shadowBuffer[i] *= FullShadow;
        }

        if (i < iRightEdgeLeft)
        {
            assert(i >= 1 && i <= static_cast<register_int>(ShadowBufferSize - 2));

            shadowBuffer[i] *= fullZCoeff * fullP1Coeff;
            shadowBuffer[i + 1] *= fullP1Coeff;

            ++i;
        }

        //
        // Right edge
        //

        for (; i < iRightEdgeRight; ++i)
        {
            assert(i >= 1 && i <= static_cast<register_int>(ShadowBufferSize - 2));

            shadowBuffer[i - 1] *= edgeN1Coeff;
            shadowBuffer[i] *= edgeZCoeff;
            shadowBuffer[i + 1] *= edgeP1Coeff;
        }
    }
}

void Clouds::OffsetShadowsBuffer_Mean()
{
    // Offset shadow values so that the mean is 1.0
    //
    // Note: we only sample the visible (central) slice, so that we
    // do not undergo non-linearities when clouds disappear
    // at the edges of the cloud space

    float sum = 0.0f;
    float count = 0.0f;
    for (size_t i = ShadowBufferSize / 3; i < ShadowBufferSize * 2 / 3; ++i)
    {
        sum += mShadowBuffer[i];
        count += 1.0f;
    }

    float const adjustment = 1.0f - sum / count;
    for (size_t i = 0; i < ShadowBufferSize; ++i)
    {
        mShadowBuffer[i] += adjustment;
    }
}

void Clouds::OffsetShadowsBuffer_Min()
{
    // Offset shadow values so that the min is 1.0
    //
    // Note: we only sample the visible (central) slice, so that we
    // do not undergo non-linearities when clouds disappear
    // at the edges of the cloud space

    float minShadow = 1.0f;
    for (size_t i = ShadowBufferSize / 3; i < ShadowBufferSize * 2 / 3; ++i)
    {
        minShadow = std::min(minShadow, mShadowBuffer[i]);
    }

    float const adjustment = 1.0f - minShadow;
    assert(adjustment >= 0.0f && adjustment <= 1.0f);
    for (size_t i = 0; i < ShadowBufferSize; ++i)
    {
        mShadowBuffer[i] += adjustment;
        assert(mShadowBuffer[i] >= 0.0f && mShadowBuffer[i] <= 2.0f);
    }
}

}