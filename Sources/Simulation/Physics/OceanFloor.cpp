/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <cmath>

namespace Physics {

// The number of slices we want to render the ocean floor as;
// this is the graphical resolution
template<typename T>
static constexpr T RenderSlices = 500;

OceanFloor::OceanFloor(OceanFloorHeightMap && heightMap)
    : mBumpProfile(SamplesCount)
    , mHeightMap(std::move(heightMap))
    // Regarding the number of samples:
    //  - The sample index for x==max (HalfMaxWorldWidth) is SamplesCount - 1
    //  - To allow for our "rough check" at x==max, we need an addressable value for sample[SamplesCount].SampleValue
    , mSamples(new Sample[SamplesCount + 1])
    , mIsDirty(false)
    , mCurrentSeaDepth(0.0f)
    , mCurrentOceanFloorBumpiness(0.0f)
    , mCurrentOceanFloorDetailAmplification(0.0f)
{
    // Initialize constant sample values
    mSamples[SamplesCount - 1].SampleValuePlusOneMinusSampleValue = 0.0f; // Because extra sample is == mSamples[SamplesCount - 1].SampleValue
    mSamples[SamplesCount].SampleValuePlusOneMinusSampleValue = 0.0f; // Just to be neat

    // Calculate bump profile
    CalculateBumpProfile();

    // Calculate samples
    CalculateResultantSampleValues();

    // Remember we're dirty
    mIsDirty = true;
}

void OceanFloor::SetHeightMap(OceanFloorHeightMap const & heightMap)
{
    // Update terrain
    mHeightMap = heightMap;

    // Recalculate samples
    CalculateResultantSampleValues();

    // Remember we're dirty
    mIsDirty = true;
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
        || simulationParameters.OceanFloorDetailAmplification != mCurrentOceanFloorDetailAmplification)
    {
        // Update current game parameters
        mCurrentSeaDepth = simulationParameters.SeaDepth;
        mCurrentOceanFloorDetailAmplification = simulationParameters.OceanFloorDetailAmplification;

        // Recalculate samples
        CalculateResultantSampleValues();

        // Remember we are dirty
        mIsDirty = true;
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
    //
    // We want to upload at most RenderSlices slices
    //

    // Find index of leftmost sample, and its corresponding world X
    auto const sampleIndex = FastTruncateToArchInt((renderContext.GetVisibleWorld().TopLeft.x + SimulationParameters::HalfMaxWorldWidth) / Dx);
    float sampleIndexX = -SimulationParameters::HalfMaxWorldWidth + (Dx * sampleIndex);

    // Calculate number of samples required to cover screen from leftmost sample
    // up to the visible world right (included)
    float const coverageWidth = renderContext.GetVisibleWorld().BottomRight.x - sampleIndexX;
    size_t const numberOfSamplesToRender = static_cast<size_t>(ceil(coverageWidth / Dx));

    if (numberOfSamplesToRender >= RenderSlices<size_t>)
    {
        //
        // Have to take more than 1 sample per slice
        //

        renderContext.UploadLandStart(RenderSlices<size_t>);

        // Calculate dx between each pair of slices with want to upload
        float const sliceDx = coverageWidth / RenderSlices<float>;

        // We do one extra iteration as the number of slices is the number of quads, and the last vertical
        // quad side must be at the end of the width
        for (size_t s = 0;
            s <= RenderSlices<size_t>;
            ++s, sampleIndexX = std::min(sampleIndexX + sliceDx, SimulationParameters::HalfMaxWorldWidth))
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
        for (size_t s = 0; s <= numberOfSamplesToRender; ++s, sampleIndexX += Dx)
        {
            renderContext.UploadLand(
                sampleIndexX,
                mSamples[s + sampleIndex].SampleValue);
        }
    }

    renderContext.UploadLandEnd();
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
        hasAdjusted |= std::abs(newSampleValue - mSamples[s].SampleValue) > 0.2f;

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
    float const newSampleValue = CalculateResultantSampleValue(sampleIndex);

    // Update sample value
    mSamples[sampleIndex].SampleValue = newSampleValue;

    // Update previous sample's delta
    if (sampleIndex > 0)
        mSamples[sampleIndex - 1].SampleValuePlusOneMinusSampleValue = newSampleValue - mSamples[sampleIndex - 1].SampleValue;

    if (sampleIndex < SamplesCount - 1)
    {
        // Update this sample's delta;
        // no point in updating delta of extra sample, as it's always zero
        mSamples[sampleIndex].SampleValuePlusOneMinusSampleValue = mSamples[sampleIndex + 1].SampleValue - newSampleValue;
    }

    // Make sure extra sample has same value as previous one
    mSamples[SamplesCount].SampleValue = mSamples[SamplesCount - 1].SampleValue;
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
    // sample index = 0
    float previousSampleValue;
    {
        previousSampleValue = CalculateResultantSampleValue(0);

        mSamples[0].SampleValue = previousSampleValue;
    }

    // sample index = 1...SamplesCount-1
    for (size_t i = 1; i < SamplesCount; ++i)
    {
        float const sampleValue = CalculateResultantSampleValue(i);

        mSamples[i].SampleValue = sampleValue;
        mSamples[i - 1].SampleValuePlusOneMinusSampleValue = sampleValue - previousSampleValue;

        previousSampleValue = sampleValue;
    }

    assert(mSamples[SamplesCount - 1].SampleValuePlusOneMinusSampleValue == 0.0f); // From cctor

    // Make sure extra sample has same value as previous one
    assert(previousSampleValue == mSamples[SamplesCount - 1].SampleValue);
    mSamples[SamplesCount].SampleValue = previousSampleValue;

    assert(mSamples[SamplesCount].SampleValuePlusOneMinusSampleValue == 0.0f); // From cctor
}

}