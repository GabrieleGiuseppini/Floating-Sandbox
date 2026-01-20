/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <cmath>

namespace Physics {

OceanFloor::OceanFloor(OceanFloorHeightMap && heightMap)
    : mBumpProfile(SamplesCount)
    , mHeightMap(std::move(heightMap))
    // Regarding the number of samples:
    //  - The sample index for x==max (HalfMaxWorldWidth) is SamplesCount - 1
    //  - To allow for our "rough check" at x==max, we need an addressable value for sample[SamplesCount].SampleValue
    , mSamples(new Sample[SamplesCount + 1])
    , mIsDirty(false)
    , mIsDirtyForRendering(false)
    , mCurrentSeaDepth(0.0f)
    , mCurrentOceanFloorSiltThickness(0.0f)
    , mCurrentOceanFloorBumpiness(0.0f)
    , mCurrentOceanFloorDetailAmplification(0.0f)
{
    // Initialize constant sample values
    mSamples[SamplesCount - 1].SiltSampleValuePlusOneMinusSampleValue = 0.0f; // Because extra sample is == mSamples[SamplesCount - 1].SampleValue
    mSamples[SamplesCount - 1].BedrockSampleValuePlusOneMinusSampleValue = 0.0f; // Because extra sample is == mSamples[SamplesCount - 1].SampleValue
    mSamples[SamplesCount].SiltSampleValuePlusOneMinusSampleValue = 0.0f; // Just to be neat
    mSamples[SamplesCount].BedrockSampleValuePlusOneMinusSampleValue = 0.0f; // Just to be neat

    // Calculate bump profile
    CalculateBumpProfile();

    // Calculate samples
    CalculateResultantSampleValues();

    // Remember we're dirty
    mIsDirty = true;
    mIsDirtyForRendering = true;
}

void OceanFloor::SetHeightMap(OceanFloorHeightMap const & heightMap)
{
    // Update terrain
    mHeightMap = heightMap;

    // Recalculate samples
    CalculateResultantSampleValues();

    // Remember we're dirty
    mIsDirty = true;
    mIsDirtyForRendering = true;
}

void OceanFloor::Update(SimulationParameters const & simulationParameters)
{
    bool doRecalculateSamples = false;

    // Check whether we need to recalculate the bump profile
    if (simulationParameters.OceanFloorBumpiness != mCurrentOceanFloorBumpiness)
    {
        // Update current game parameters
        mCurrentOceanFloorBumpiness = simulationParameters.OceanFloorBumpiness;

        // Recalculate bump profile
        CalculateBumpProfile();

        doRecalculateSamples = true;
    }

    // Check whether we need to recalculate the samples
    if (doRecalculateSamples
        || simulationParameters.SeaDepth != mCurrentSeaDepth
        || simulationParameters.OceanFloorSiltThickness != mCurrentOceanFloorSiltThickness
        || simulationParameters.OceanFloorDetailAmplification != mCurrentOceanFloorDetailAmplification)
    {
        // Update current game parameters
        mCurrentSeaDepth = simulationParameters.SeaDepth;
        mCurrentOceanFloorSiltThickness = simulationParameters.OceanFloorSiltThickness;
        mCurrentOceanFloorDetailAmplification = simulationParameters.OceanFloorDetailAmplification;

        // Recalculate samples
        CalculateResultantSampleValues();

        // Remember we are dirty
        mIsDirty = true;
        mIsDirtyForRendering = true;
    }
}

void OceanFloor::UpdateEnd()
{
    mIsDirty = false;
}

void OceanFloor::Upload(
    SimulationParameters const & /*simulationParameters*/,
    RenderContext & renderContext) const
{
    if (mIsDirtyForRendering)
    {
        renderContext.UploadLandStart(SamplesCount);

        float sampleIndexX = -SimulationParameters::HalfMaxWorldWidth;

        // We do one extra iteration as the number of slices is the number of quads, and the last vertical
        // quad side must be at the end of its width
        for (size_t s = 0; s <= SamplesCount; ++s, sampleIndexX += Dx)
        {
            renderContext.UploadLand(
                s,
                sampleIndexX,
                mSamples[s].SiltSampleValue,
                mSamples[s].BedrockSampleValue,
                -SimulationParameters::HalfMaxWorldHeight);
        }

        renderContext.UploadLandEnd();

        // We're not anymore dirty wrt rendering
        mIsDirtyForRendering = false;
    }
}

std::optional<bool> OceanFloor::AdjustTo(
    float x1,
    float targetY1,
    float x2,
    float targetY2)
{
    if (mCurrentOceanFloorDetailAmplification == 0.0f)
    {
        // Nothing to do
        return std::nullopt;
    }

    //
    //
    //

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

    float const sampleIndexF = (leftX + SimulationParameters::HalfMaxWorldWidth) / Dx;

    auto const sampleIndex = FastTruncateToArchInt(sampleIndexF + 0.5f);

    assert(sampleIndex >= 0 && sampleIndex < SamplesCount);


    //
    // Update values for all samples in the trajectory
    //

    bool hasAdjusted = false;
    float x = leftX;
    for (auto s = static_cast<size_t>(sampleIndex); x <= rightX && s < SamplesCount; ++s, x += Dx)
    {
        // Calculate new sample value, i.e. trajectory's value
        float const newSampleValue = leftTargetY + slopeY * (x - leftX);

        // Decide whether it's a significant change
        // (consumed by tool)
        hasAdjusted |= std::abs(newSampleValue - mSamples[s].BedrockSampleValue) > 0.2f;

        // Translate sample value into terrain change
        // (inverse of CalculateResultantSampleValue(.))
        assert(mCurrentOceanFloorDetailAmplification != 0.0f);
        float const newTerrainProfileSampleValue =
            (newSampleValue - mBumpProfile[s] + mCurrentSeaDepth)
            / mCurrentOceanFloorDetailAmplification;

        // Update terrain and samples
        SetTerrainHeight(s, newTerrainProfileSampleValue);
    }

    // Remember we're dirty now
    mIsDirty = true;
    mIsDirtyForRendering = true;

    return hasAdjusted;
}

void OceanFloor::DisplaceAt(
    float x,
    float yOffset)
{
    assert(x >= -SimulationParameters::HalfMaxWorldWidth && x <= SimulationParameters::HalfMaxWorldWidth);

    //
    // Find sample index
    //

    // Fractional index in the sample array
    float const sampleIndexF = (x + SimulationParameters::HalfMaxWorldWidth) / Dx;

    // Integral part
    auto const sampleIndexI = FastTruncateToArchInt(sampleIndexF);

    // Fractional part within sample index and the next sample index
    float const sampleIndexDx = sampleIndexF - sampleIndexI;

    assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);
    assert(sampleIndexDx >= 0.0f && sampleIndexDx < 1.0f);

    //
    // Distribute offset according to position between two points
    //

    // Left
    float lYOffset = yOffset * (1.0f - sampleIndexDx);
    SetTerrainHeight(sampleIndexI, mHeightMap[sampleIndexI] + lYOffset);

    // Right
    if (static_cast<size_t>(sampleIndexI) < SamplesCount - 1)
    {
        float rYOffset = yOffset * sampleIndexDx;
        SetTerrainHeight(sampleIndexI + 1, mHeightMap[sampleIndexI + 1] + rYOffset);
    }

    // Remember we're dirty now
    mIsDirty = true;
    mIsDirtyForRendering = true;
}

///////////////////////////////////////////////////////////////////////////////////

void OceanFloor::SetTerrainHeight(
    size_t sampleIndex,
    float terrainHeight)
{
    assert(sampleIndex >= 0 && sampleIndex < SamplesCount);

    // Update terrain
    mHeightMap[sampleIndex] = terrainHeight;

    // Recalculate sample value
    float const newBedrockSampleValue = CalculateResultantBedrockSampleValue(sampleIndex);

    // Update sample value
    mSamples[sampleIndex].BedrockSampleValue = newBedrockSampleValue;

    // Update previous sample's delta
    if (sampleIndex > 0)
    {
        mSamples[sampleIndex - 1].BedrockSampleValuePlusOneMinusSampleValue = newBedrockSampleValue - mSamples[sampleIndex - 1].BedrockSampleValue;
    }

    if (sampleIndex < SamplesCount - 1)
    {
        // Update this sample's delta;
        // no point in updating delta of extra sample, as it's always zero
        mSamples[sampleIndex].BedrockSampleValuePlusOneMinusSampleValue = mSamples[sampleIndex + 1].BedrockSampleValue - newBedrockSampleValue;
    }

    // Make sure extra sample has same value as previous one
    mSamples[SamplesCount].BedrockSampleValue = mSamples[SamplesCount - 1].BedrockSampleValue;

    // Now calculate Silt profile, on top of bedrock profile;
    // silt height at i depends on bedrock[i-1] and bedrock[i];
    // since we've changed bedrock[sampleIndex - 1] and bedrock[sampleIndex], we've affected
    // silt at sampleIndex - 1 (bc bedrock[sampleIndex- 1 ]), sampleIndex (bc both), and sampleIndex + 1 (bc bedrock[sampleIndex])
    CalculateSiltSampleValues(sampleIndex > 0 ? sampleIndex - 1 : sampleIndex, sampleIndex < SamplesCount - 1 ? sampleIndex + 1 : sampleIndex);
}

void OceanFloor::CalculateBumpProfile()
{
    static constexpr float BumpFrequency1 = 0.005f;
    static constexpr float BumpFrequency2 = 0.015f;
    static constexpr float BumpFrequency3 = 0.001f;

    float x = -SimulationParameters::HalfMaxWorldWidth;
    for (size_t i = 0; i < SamplesCount; ++i, x += Dx)
    {
        float const c1 = sinf(x * BumpFrequency1) * 10.f;
        float const c2 = sinf(x * BumpFrequency2) * 6.f;
        float const c3 = sinf(x * BumpFrequency3) * 45.f;
        mBumpProfile[i] = (c1 + c2 - c3) * mCurrentOceanFloorBumpiness;
    }
}

void OceanFloor::CalculateResultantSampleValues()
{
    //
    // Populate bedrock profile
    //

    // sample index = 0
    float previousBedrockSampleValue;
    {
        previousBedrockSampleValue = CalculateResultantBedrockSampleValue(0);

        mSamples[0].BedrockSampleValue = previousBedrockSampleValue;
    }

    // sample index = 1...SamplesCount-1
    for (size_t i = 1; i < SamplesCount; ++i)
    {
        float const bedrockSampleValue = CalculateResultantBedrockSampleValue(i);

        mSamples[i].BedrockSampleValue = bedrockSampleValue;
        mSamples[i - 1].BedrockSampleValuePlusOneMinusSampleValue = bedrockSampleValue - previousBedrockSampleValue;

        previousBedrockSampleValue = bedrockSampleValue;
    }

    assert(mSamples[SamplesCount - 1].BedrockSampleValuePlusOneMinusSampleValue == 0.0f); // From cctor

    // Make sure extra sample has same value as previous one
    assert(previousBedrockSampleValue == mSamples[SamplesCount - 1].BedrockSampleValue);
    mSamples[SamplesCount].BedrockSampleValue = previousBedrockSampleValue;
    assert(mSamples[SamplesCount].BedrockSampleValuePlusOneMinusSampleValue == 0.0f); // From cctor

    // Now calculate Silt profile, on top of bedrock profile
    CalculateSiltSampleValues(0, SamplesCount - 1);

    // Verify invariants
    assert(mSamples[SamplesCount - 1].SiltSampleValuePlusOneMinusSampleValue == 0.0f); // From cctor
    assert(mSamples[SamplesCount].SiltSampleValue == mSamples[SamplesCount - 1].SiltSampleValue);
    assert(mSamples[SamplesCount].SiltSampleValuePlusOneMinusSampleValue == 0.0f); // From cctor
}

void OceanFloor::CalculateSiltSampleValues(size_t startIndex, size_t endIndex)
{
    assert(startIndex >= 0 && endIndex < SamplesCount);

    float const perturbationSinAmplitude = mCurrentOceanFloorSiltThickness / 20.0f;

    SegmentDirection previousSegmentDirection = startIndex > 0
        ? GetSegmentDirection(startIndex - 1)
        : SegmentDirection::Horizontal;

    float previousSiltSampleValue = startIndex > 0
        ? mSamples[startIndex - 1].SiltSampleValue
        : 0.0f; // Unused

    for (size_t i = startIndex; i <= endIndex; ++i)
    {
        // Calculate multiplier based on sequence: we want to avoid silt on either end of high-slope segments

        SegmentDirection const segmentDirection = GetSegmentDirection(i);

        float segmentSequenceMultiplier;
        if (previousSegmentDirection != SegmentDirection::Horizontal
            || segmentDirection != SegmentDirection::Horizontal)
        {
            segmentSequenceMultiplier = 0.0f;
        }
        else
        {
            segmentSequenceMultiplier = 1.0f;
        }

        float const siltSampleValue =
            mSamples[i].BedrockSampleValue
            + (mCurrentOceanFloorSiltThickness + perturbationSinAmplitude * std::sinf(static_cast<float>(i) / 8.0f * 2.0f * Pi<float>))
            * segmentSequenceMultiplier;

        mSamples[i].SiltSampleValue = siltSampleValue;

        if (i > 0)
        {
            mSamples[i - 1].SiltSampleValuePlusOneMinusSampleValue = siltSampleValue - previousSiltSampleValue;
        }

        previousSiltSampleValue = siltSampleValue;
        previousSegmentDirection = segmentDirection;
    }

    //
    // Make sure extra sample has same value as previous one
    //

    if (endIndex == SamplesCount - 1)
    {
        assert(previousSiltSampleValue == mSamples[SamplesCount - 1].SiltSampleValue);
        mSamples[SamplesCount].SiltSampleValue = previousSiltSampleValue;
    }

    //// TODOOLD

    ////
    //// Sample index = 0
    ////

    //float previousSiltSampleValue = mSamples[0].BedrockSampleValue + mCurrentOceanFloorSiltThickness; // Sin component is zero here
    //mSamples[0].SiltSampleValue = previousSiltSampleValue;

    ////
    //// Sample index = 1...SamplesCount-1
    ////

    //SegmentDirection previousSegmentDirection = GetSegmentDirection(0);

    //for (size_t i = 1; i < SamplesCount; ++i)
    //{
    //    // Calculate multiplier based on sequence: we want to avoid silt on either end of high-slope segments

    //    SegmentDirection const segmentDirection = GetSegmentDirection(i);

    //    float segmentSequenceMultiplier;
    //    if (previousSegmentDirection != SegmentDirection::Horizontal
    //        || segmentDirection != SegmentDirection::Horizontal)
    //    {
    //        segmentSequenceMultiplier = 0.0f;
    //    }
    //    else
    //    {
    //        segmentSequenceMultiplier = 1.0f;
    //    }

    //    float const siltSampleValue =
    //        mSamples[i].BedrockSampleValue
    //        + (mCurrentOceanFloorSiltThickness + perturbationSinAmplitude * std::sinf(static_cast<float>(i) / 8.0f * 2.0f * Pi<float>))
    //        * segmentSequenceMultiplier;

    //    mSamples[i].SiltSampleValue = siltSampleValue;
    //    mSamples[i - 1].SiltSampleValuePlusOneMinusSampleValue = siltSampleValue - previousSiltSampleValue;

    //    previousSiltSampleValue = siltSampleValue;
    //    previousSegmentDirection = segmentDirection;
    //}

    //assert(mSamples[SamplesCount - 1].SiltSampleValuePlusOneMinusSampleValue == 0.0f); // From cctor

    ////
    //// Make sure extra sample has same value as previous one
    ////

    //assert(previousSiltSampleValue == mSamples[SamplesCount - 1].SiltSampleValue);
    //mSamples[SamplesCount].SiltSampleValue = previousSiltSampleValue;
    //assert(mSamples[SamplesCount].SiltSampleValuePlusOneMinusSampleValue == 0.0f); // From cctor
}

OceanFloor::SegmentDirection OceanFloor::GetSegmentDirection(size_t sampleIndex) const
{
    assert(sampleIndex <= SamplesCount);

    float const deltaY = mSamples[sampleIndex].BedrockSampleValuePlusOneMinusSampleValue;
    float constexpr HighSlopeThreshold = 5.0f;
    if (deltaY > Dx * HighSlopeThreshold)
        return SegmentDirection::Upward;
    else if (deltaY < -Dx * HighSlopeThreshold)
        return SegmentDirection::Downward;
    else
        return SegmentDirection::Horizontal;
}

}