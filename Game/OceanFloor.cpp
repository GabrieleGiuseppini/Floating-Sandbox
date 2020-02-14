/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

// The number of slices we want to render the ocean floor as;
// this is the graphical resolution
template<typename T>
static constexpr T RenderSlices = 500;

OceanFloor::OceanFloor(OceanFloorTerrain && terrain)
    : mBumpProfile(SamplesCount)
    , mTerrain(std::move(terrain))
    , mSamples(new Sample[SamplesCount + 1])
    , mCurrentSeaDepth(0.0f)
    , mCurrentOceanFloorBumpiness(0.0f)
    , mCurrentOceanFloorDetailAmplification(0.0f)
{
    // Calculate bump profile
    CalculateBumpProfile();

    // Calculate samples
    CalculateResultantSampleValues();
}

void OceanFloor::SetTerrain(OceanFloorTerrain const & terrain)
{
    // Update terrain
    mTerrain = terrain;

    // Recalculate samples
    CalculateResultantSampleValues();
}

void OceanFloor::Update(GameParameters const & gameParameters)
{
    bool doRecalculateSamples = false;

    // Check whether we need to recalculate the bump profile
    if (gameParameters.OceanFloorBumpiness != mCurrentOceanFloorBumpiness)
    {
        // Update current game parameters
        mCurrentOceanFloorBumpiness = gameParameters.OceanFloorBumpiness;

        // Recalculate bump profile
        CalculateBumpProfile();

        doRecalculateSamples = true;
    }

    // Check whether we need to recalculate the samples
    if (doRecalculateSamples
        || gameParameters.SeaDepth != mCurrentSeaDepth
        || gameParameters.OceanFloorDetailAmplification != mCurrentOceanFloorDetailAmplification)
    {
        // Update current game parameters
        mCurrentSeaDepth = gameParameters.SeaDepth;
        mCurrentOceanFloorDetailAmplification = gameParameters.OceanFloorDetailAmplification;

        // Recalculate samples
        CalculateResultantSampleValues();
    }
}

void OceanFloor::Upload(
    GameParameters const & /*gameParameters*/,
    Render::RenderContext & renderContext) const
{
    //
    // We want to upload at most RenderSlices slices
    //

    // Find index of leftmost sample, and its corresponding world X
    auto sampleIndex = FastTruncateInt64((renderContext.GetVisibleWorldLeft() + GameParameters::HalfMaxWorldWidth) / Dx);
    float sampleIndexX = -GameParameters::HalfMaxWorldWidth + (Dx * sampleIndex);

    // Calculate number of samples required to cover screen from leftmost sample
    // up to the visible world right (included)
    float const coverageWidth = renderContext.GetVisibleWorldRight() - sampleIndexX;
    auto const numberOfSamplesToRender = static_cast<int64_t>(ceil(coverageWidth / Dx));

    if (numberOfSamplesToRender >= RenderSlices<int64_t>)
    {
        //
        // Have to take more than 1 sample per slice
        //

        renderContext.UploadLandStart(RenderSlices<int>);

        // Calculate dx between each pair of slices with want to upload
        float const sliceDx = coverageWidth / RenderSlices<float>;

        // We do one extra iteration as the number of slices is the number of quads, and the last vertical
        // quad side must be at the end of the width
        for (int64_t s = 0; s <= RenderSlices<int64_t>; ++s, sampleIndexX += sliceDx)
        {
            renderContext.UploadLand(
                sampleIndexX,
                GetHeightAt(sampleIndexX));
        }
    }
    else
    {
        //
        // We just upload the required number of samples, which is less than
        // the max number of slices we're prepared to upload, and we let OpenGL
        // interpolate on our behalf
        //

        renderContext.UploadLandStart(numberOfSamplesToRender);

        // We do one extra iteration as the number of slices is the number of quads, and the last vertical
        // quad side must be at the end of the width
        for (std::int64_t s = 0; s <= numberOfSamplesToRender; ++s, sampleIndexX += Dx)
        {
            renderContext.UploadLand(
                sampleIndexX,
                mSamples[s + sampleIndex].SampleValue);
        }
    }

    renderContext.UploadLandEnd();
}

bool OceanFloor::AdjustTo(
    float x1,
    float targetY1,
    float x2,
    float targetY2)
{
    float leftX, leftTargetY;
    float rightX, rightTargetY;
    if (x1 <= x2)
    {
        leftX = x1;
        leftTargetY = targetY1;
        rightX = x2;
        rightTargetY = targetY2;
    }
    else
    {
        leftX = x2;
        leftTargetY = targetY2;
        rightX = x1;
        rightTargetY = targetY1;
    }

    float slopeY = (leftX != rightX)
        ? (rightTargetY - leftTargetY) / (rightX - leftX)
        : 1.0f;

    //
    // Calculate left first sample index, minimizing error
    //

    float const sampleIndexF = (leftX + GameParameters::HalfMaxWorldWidth) / Dx;

    int64_t sampleIndex = FastTruncateInt64(sampleIndexF + 0.5f);

    assert(sampleIndex >= 0 && sampleIndex <= SamplesCount);


    //
    // Update values for all samples in the trajectory
    //

    bool hasAdjusted = false;
    float x = leftX;
    for (int64_t s = sampleIndex; x <= rightX && s < SamplesCount; ++s, x += Dx)
    {
        // Calculate new sample value, i.e. trajectory's value
        float newSampleValue = leftTargetY + slopeY * (x - leftX);

        // Decide whether it's a significant change
        hasAdjusted |= abs(newSampleValue - mSamples[s].SampleValue) > 0.2f;

        // Translate sample value into terrain change
        float newTerrainProfileSampleValue =
            (newSampleValue - mBumpProfile[s] + mCurrentSeaDepth)
            / (mCurrentOceanFloorDetailAmplification != 0.0f ? mCurrentOceanFloorDetailAmplification : 1.0f);

        // Update terrain and samples
        SetTerrainHeight(s, newTerrainProfileSampleValue);
    }

    return hasAdjusted;
}

void OceanFloor::DisplaceAt(
    float x,
    float yOffset)
{
    assert(x >= -GameParameters::HalfMaxWorldWidth && x <= GameParameters::HalfMaxWorldWidth);

    //
    // Find sample index
    //

    // Fractional index in the sample array
    float const sampleIndexF = (x + GameParameters::HalfMaxWorldWidth) / Dx;

    // Integral part
    int64_t const sampleIndexI = FastTruncateInt64(sampleIndexF);

    // Fractional part within sample index and the next sample index
    float const sampleIndexDx = sampleIndexF - sampleIndexI;

    assert(sampleIndexI >= 0 && sampleIndexI <= SamplesCount);
    assert(sampleIndexDx >= 0.0f && sampleIndexDx <= 1.0f);

    //
    // Distribute offset according to position between two points
    //

    if (sampleIndexI < SamplesCount)
    {
        // Left
        float lYOffset = yOffset * (1.0f - sampleIndexDx);
        SetTerrainHeight(sampleIndexI, mTerrain[sampleIndexI] + lYOffset);

        // Right
        if (sampleIndexI < SamplesCount - 1)
        {
            float rYOffset = yOffset * sampleIndexDx;
            SetTerrainHeight(sampleIndexI + 1, mTerrain[sampleIndexI + 1] + rYOffset);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////

void OceanFloor::SetTerrainHeight(
    size_t sampleIndex,
    float terrainHeight)
{
    assert(sampleIndex >= 0 && sampleIndex < SamplesCount);

    // Update terrain
    mTerrain[sampleIndex] = terrainHeight;

    // Recalculate sample value
    float const newSampleValue = CalculateResultantSampleValue(sampleIndex);

    // Update sample value
    mSamples[sampleIndex].SampleValue = newSampleValue;

    // Update previous sample's delta
    if (sampleIndex > 0)
        mSamples[sampleIndex - 1].SampleValuePlusOneMinusSampleValue = newSampleValue - mSamples[sampleIndex - 1].SampleValue;

    if (sampleIndex < SamplesCount - 1)
    {
        // Update this sample's delta;
        // no point in updating delta of extra sample, as it's always zero,
        // and no point in updating delta of last sample, as it's always zero
        mSamples[sampleIndex].SampleValuePlusOneMinusSampleValue = mSamples[sampleIndex + 1].SampleValue - newSampleValue;
    }
    else if (sampleIndex == SamplesCount - 1)
    {
        // Make sure the final extra sample has the same value as the last sample
        mSamples[SamplesCount].SampleValue = mSamples[SamplesCount - 1].SampleValue;
    }
}

void OceanFloor::CalculateBumpProfile()
{
    static constexpr float BumpFrequency1 = 0.005f;
    static constexpr float BumpFrequency2 = 0.015f;
    static constexpr float BumpFrequency3 = 0.001f;

    float x = 0.0;
    for (int64_t i = 0; i < SamplesCount; ++i, x += Dx)
    {
        float const c1 = sinf(x * BumpFrequency1) * 10.f;
        float const c2 = sinf(x * BumpFrequency2) * 6.f;
        float const c3 = sinf(x * BumpFrequency3) * 45.f;
        mBumpProfile[i] = (c1 + c2 - c3) * mCurrentOceanFloorBumpiness;
    }
}

void OceanFloor::CalculateResultantSampleValues()
{
    // sample index = 0
    float previousSampleValue;
    {
        previousSampleValue = CalculateResultantSampleValue(0);

        mSamples[0].SampleValue = previousSampleValue;
    }

    // sample index = 1...SamplesCount-1
    for (int64_t i = 1; i < SamplesCount; ++i)
    {
        float const sampleValue = CalculateResultantSampleValue(i);

        mSamples[i].SampleValue = sampleValue;
        mSamples[i - 1].SampleValuePlusOneMinusSampleValue = sampleValue - previousSampleValue;

        previousSampleValue = sampleValue;
    }

    // Populate last delta (extra sample has same value as this sample)
    mSamples[SamplesCount - 1].SampleValuePlusOneMinusSampleValue = 0.0f;

    // Populate extra sample - same value as last sample
    mSamples[SamplesCount].SampleValue = mSamples[SamplesCount - 1].SampleValue;
    mSamples[SamplesCount].SampleValuePlusOneMinusSampleValue = 0.0f; // Accessed only for derivative at x=MaxWorldWidth
}

}