/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <algorithm>

namespace Physics {

// The multiplicative factor to transform wind speed into wave speed
constexpr float WindSpeedToWaveSpeedFactor = 1.0f / 6.0f; // Waves move slower than wind

// The number of slices we want to render the water surface as;
// this is the graphical resolution
template<typename T>
constexpr T RenderSlices = 500;

// The height of the height field - indirectly determines velocity
// of waves
float HeightFieldOffset = 30.0f;

// The number of samples we set apart in the SWE buffers for wave generation at each end of a buffer
constexpr size_t SWEWaveGenerationSamples = 2;

// The number of samples we set apart in the SWE buffers for boundary conditions at each end of a buffer
constexpr size_t SWEBoundaryConditionsSamples = 1;

constexpr size_t SWEOuterLayerSamples =
    SWEWaveGenerationSamples
    + SWEBoundaryConditionsSamples;

// The total number of samples in the SWE buffers
constexpr size_t SWETotalSamples =
    SWEOuterLayerSamples
    + OceanSurface::SamplesCount
    + SWEOuterLayerSamples;

namespace TODOTEST {

    static std::optional<size_t> AdjustedSample;

    void LogBuffers(std::string const & header, float const * h, float const * v)
    {
        if (AdjustedSample)
        {
            LogMessage(header);
            size_t const baseIndex = *AdjustedSample;
            for (size_t i = baseIndex - 5; i < baseIndex + 5; ++i)
            {
                LogMessage(i, ": h=", h[i], " v=", v[i]);
            }
        }
    }
}

OceanSurface::OceanSurface(
    float currentSimulationTime,
    Wind const & /*wind*/,
    GameParameters const & gameParameters)
    : mSamples(new Sample[SamplesCount + 1])
    , mHeightFieldBuffer1(new float[SWETotalSamples + 1]) // One extra cell just to ease interpolations
    , mHeightFieldBuffer2(new float[SWETotalSamples + 1]) // One extra cell just to ease interpolations
    , mVelocityFieldBuffer1(new float[SWETotalSamples + 1]) // One extra cell just to ease interpolations
    , mVelocityFieldBuffer2(new float[SWETotalSamples + 1]) // One extra cell just to ease interpolations
{
    //
    // Initialize SWEs
    //

    // Initialize buffer pointers
    mCurrentHeightField = mHeightFieldBuffer1.get();
    mNextHeightField = mHeightFieldBuffer2.get();
    mCurrentVelocityField = mVelocityFieldBuffer1.get();
    mNextVelocityField = mVelocityFieldBuffer2.get();

    // Initialize outer layers, both fields
    // - Boundary condition layers are initialized here once and for all
    for (size_t i = 0; i < SWEOuterLayerSamples; ++i)
    {
        mCurrentHeightField[i] = HeightFieldOffset;
        mCurrentHeightField[SWETotalSamples - i - 1] = HeightFieldOffset;
        mNextHeightField[i] = HeightFieldOffset;
        mNextHeightField[SWETotalSamples - i - 1] = HeightFieldOffset;

        mCurrentVelocityField[i] = 0.0f;
        mCurrentVelocityField[SWETotalSamples - i - 1] = 0.0f;
        mNextVelocityField[i] = 0.0f;
        mNextVelocityField[SWETotalSamples - i - 1] = 0.0f;
    }

    // Initialize middle layer
    float const waveSpeed = gameParameters.WindSpeedBase * WindSpeedToWaveSpeedFactor;
    float const waveTheta = currentSimulationTime * (0.5f + waveSpeed) / 3.0f;
    float const waveHeight = gameParameters.WaveHeight;
    constexpr float SpatialFrequency1 = 0.1f;
    constexpr float SpatialFrequency2 = 0.3f;
    float x = 0.0f;
    for (size_t i = 0; i < SamplesCount; ++i, x += Dx)
    {
        float const c1 = sinf(x * SpatialFrequency1 + waveTheta) * 0.5f;
        float const c2 = sinf(x * SpatialFrequency2 - waveTheta * 1.1f) * 0.3f;
        mCurrentHeightField[SWEOuterLayerSamples + i] = HeightFieldOffset + (c1 + c2) * waveHeight;
        mCurrentVelocityField[SWEOuterLayerSamples + i] = waveSpeed;
    }

    // Initialize extra cell - we won't ever use this as it'll be multiplied with zero,
    // but still...
    mCurrentHeightField[SWETotalSamples] = HeightFieldOffset;
    mCurrentVelocityField[SWETotalSamples] = 0.0f;
}

void OceanSurface::Update(
    float currentSimulationTime,
    Wind const & wind,
    GameParameters const & gameParameters)
{
    //
    // 1. Wave Genesis
    //

    // Set both genesis locii to the same value
    float const waveSpeed = gameParameters.WindSpeedBase * WindSpeedToWaveSpeedFactor;
    float const waveTheta = currentSimulationTime * (0.5f + waveSpeed) / 3.0f;
    float const waveHeight = gameParameters.WaveHeight;
    constexpr float SpatialFrequency1 = 0.1f;
    constexpr float SpatialFrequency2 = 0.3f;
    float const c1 = sinf(waveTheta) * 0.5f;
    float const c2 = sinf(- waveTheta * 1.1f) * 0.3f;
    float const height = HeightFieldOffset + (c1 + c2) * waveHeight;
    mCurrentHeightField[SWEBoundaryConditionsSamples] = height;
    mCurrentHeightField[SWETotalSamples - SWEBoundaryConditionsSamples - 1] = height;
    mCurrentVelocityField[SWEBoundaryConditionsSamples] = waveSpeed;
    mCurrentVelocityField[SWETotalSamples - SWEBoundaryConditionsSamples - 1] = waveSpeed;


    //
    // 2. SWE Update
    //
    // - Current -> Next
    //

    TODOTEST::LogBuffers("START", mCurrentHeightField, mCurrentVelocityField);

    AdvectHeightField();

    TODOTEST::LogBuffers("POST-ADVECT.H", mNextHeightField, mNextVelocityField);

    AdvectVelocityField();

    TODOTEST::LogBuffers("POST-ADVECT.V", mNextHeightField, mNextVelocityField);

    UpdateHeightField();

    TODOTEST::LogBuffers("POST-UPDATE.H", mNextHeightField, mNextVelocityField);

    UpdateVelocityField();

    TODOTEST::LogBuffers("POST-UPDATE.V", mNextHeightField, mNextVelocityField);


    // Calc avg height
    float avgHeight = 0.0f;
    for (size_t i = SWEOuterLayerSamples; i < SWEOuterLayerSamples + SamplesCount; ++i)
    {
        avgHeight += mNextHeightField[i];
    }
    avgHeight /= static_cast<float>(SamplesCount);
    LogMessage("AVG:", avgHeight);



    //
    // 3. Swap Buffers
    //

    std::swap(mCurrentHeightField, mNextHeightField);
    std::swap(mCurrentVelocityField, mNextVelocityField);


    //
    // 4. Generate samples
    //
    // - Sample values are the height field, short of the offset
    // - Wind gust ripples are superimposed
    //

    static constexpr float WindGustRippleSpatialFrequency = 0.5f;

    float const windSpeedAbsoluteMagnitude = wind.GetCurrentWindSpeed().length();
    float const windSpeedGustRelativeAmplitude = wind.GetMaxSpeedMagnitude() - wind.GetBaseSpeedMagnitude();
    float const rawWindNormalizedIncisiveness = (windSpeedGustRelativeAmplitude == 0.0f)
        ? 0.0f
        : std::max(0.0f, windSpeedAbsoluteMagnitude - abs(wind.GetBaseSpeedMagnitude()))
            / abs(windSpeedGustRelativeAmplitude);

    float const windRipplesTimeFrequency = (gameParameters.WindSpeedBase >= 0.0f)
        ? 128.0f
        : -128.0f;

    float const smoothedWindNormalizedIncisiveness = mWindIncisivenessRunningAverage.Update(rawWindNormalizedIncisiveness);
    float const windRipplesWaveHeight = 0.7f * smoothedWindNormalizedIncisiveness;

    // sample index = 0
    float previousSampleValue;
    {
        float const cRipple = sinf(-currentSimulationTime * windRipplesTimeFrequency);
        previousSampleValue =
            (mCurrentHeightField[SWEOuterLayerSamples + 0] - HeightFieldOffset)
            + cRipple * windRipplesWaveHeight;

        mSamples[0].SampleValue = previousSampleValue;
    }

    // sample index = 1...SamplesCount - 1
    float x = Dx;
    for (int64_t i = 1; i < SamplesCount; i++, x += Dx)
    {
        float const cRipple = sinf(x * WindGustRippleSpatialFrequency - currentSimulationTime * windRipplesTimeFrequency);
        float const sampleValue =
            (mCurrentHeightField[SWEOuterLayerSamples + i] - HeightFieldOffset)
            + cRipple * windRipplesWaveHeight;

        mSamples[i].SampleValue = sampleValue;
        mSamples[i - 1].SampleValuePlusOneMinusSampleValue = sampleValue - previousSampleValue;

        previousSampleValue = sampleValue;
    }

    // Populate last delta (extra sample will have same value as this sample)
    mSamples[SamplesCount - 1].SampleValuePlusOneMinusSampleValue = 0.0f;

    // Populate extra sample - same value as last sample
    mSamples[SamplesCount].SampleValue = mSamples[SamplesCount - 1].SampleValue;
    mSamples[SamplesCount].SampleValuePlusOneMinusSampleValue = 0.0f; // Never used
}

// TODOOLD
////void OceanSurface::Update(
////    float currentSimulationTime,
////    Wind const & wind,
////    GameParameters const & gameParameters)
////{
////    // Waves
////
////    float const waveSpeed = gameParameters.WindSpeedBase / 6.0f; // Water moves slower than wind
////    float const waveTheta = currentSimulationTime * (0.5f + waveSpeed) / 3.0f;
////    float const waveHeight = gameParameters.WaveHeight;
////
////    // Ripples
////
////    float const windSpeedAbsoluteMagnitude = wind.GetCurrentWindSpeed().length();
////    float const windSpeedGustRelativeAmplitude = wind.GetMaxSpeedMagnitude() - wind.GetBaseSpeedMagnitude();
////    float const rawWindNormalizedIncisiveness = (windSpeedGustRelativeAmplitude == 0.0f)
////        ? 0.0f
////        : std::max(0.0f, windSpeedAbsoluteMagnitude - abs(wind.GetBaseSpeedMagnitude()))
////          / abs(windSpeedGustRelativeAmplitude);
////
////    float const windRipplesTimeFrequency = (gameParameters.WindSpeedBase >= 0.0f)
////        ? 128.0f
////        : -128.0f;
////
////    float const smoothedWindNormalizedIncisiveness = mWindIncisivenessRunningAverage.Update(rawWindNormalizedIncisiveness);
////    float const windRipplesWaveHeight = 0.7f * smoothedWindNormalizedIncisiveness;
////
////    // Spatial frequencies of the wave components
////
////    static constexpr float SpatialFrequency1 = 0.1f;
////    static constexpr float SpatialFrequency2 = 0.3f;
////    static constexpr float SpatialFrequency3 = 0.5f; // Wind component
////
////    //
////    // Create samples
////    //
////
////    // sample index = 0
////    float previousSampleValue;
////    {
////        float const c1 = sinf(waveTheta) * 0.5f;
////        float const c2 = sinf(-waveTheta * 1.1f) * 0.3f;
////        float const c3 = sinf(-currentSimulationTime * windRipplesTimeFrequency);
////        previousSampleValue = (c1 + c2) * waveHeight + c3 * windRipplesWaveHeight;
////        mSamples[0].SampleValue = previousSampleValue;
////    }
////
////    // sample index = 1...SamplesCount - 1
////    float x = Dx;
////    for (int64_t i = 1; i < SamplesCount; i++, x += Dx)
////    {
////        float const c1 = sinf(x * SpatialFrequency1 + waveTheta) * 0.5f;
////        float const c2 = sinf(x * SpatialFrequency2 - waveTheta * 1.1f) * 0.3f;
////        float const c3 = sinf(x * SpatialFrequency3 - currentSimulationTime * windRipplesTimeFrequency);
////        float const sampleValue = (c1 + c2) * waveHeight + c3 * windRipplesWaveHeight;
////        mSamples[i].SampleValue = sampleValue;
////        mSamples[i - 1].SampleValuePlusOneMinusSampleValue = sampleValue - previousSampleValue;
////
////        previousSampleValue = sampleValue;
////    }
////
////    // Populate last delta (extra sample has same value as this sample)
////    mSamples[SamplesCount - 1].SampleValuePlusOneMinusSampleValue = 0.0f;
////
////    // Populate extra sample - same value as last sample
////    mSamples[SamplesCount].SampleValue = mSamples[SamplesCount - 1].SampleValue;
////    mSamples[SamplesCount].SampleValuePlusOneMinusSampleValue = 0.0f; // Never used
////}

void OceanSurface::Upload(
    GameParameters const & gameParameters,
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

        renderContext.UploadOceanStart(RenderSlices<int>);

        // Calculate dx between each pair of slices with want to upload
        float const sliceDx = coverageWidth / RenderSlices<float>;

        // We do one extra iteration as the number of slices is the number of quads, and the last vertical
        // quad side must be at the end of the width
        for (int64_t s = 0; s <= RenderSlices<int64_t>; ++s, sampleIndexX += sliceDx)
        {
            renderContext.UploadOcean(
                sampleIndexX,
                GetHeightAt(sampleIndexX),
                gameParameters.SeaDepth);
        }
    }
    else
    {
        //
        // We just upload the required number of samples, which is less than
        // the max number of slices we're prepared to upload, and we let OpenGL
        // interpolate on our behalf
        //

        renderContext.UploadOceanStart(numberOfSamplesToRender);

        // We do one extra iteration as the number of slices is the number of quads, and the last vertical
        // quad side must be at the end of the width
        for (std::int64_t s = 0; s <= numberOfSamplesToRender; ++s, sampleIndexX += Dx)
        {
            renderContext.UploadOcean(
                sampleIndexX,
                mSamples[s + sampleIndex].SampleValue,
                gameParameters.SeaDepth);
        }
    }

    renderContext.UploadOceanEnd();
}

void OceanSurface::AdjustTo(
    float x,
    float targetY)
{
    //
    // Calculate sample index, minimizing error
    //

    float const sampleIndexF = (x + GameParameters::HalfMaxWorldWidth) / Dx;

    int32_t sampleIndexI = FastTruncateInt32(sampleIndexF + 0.5f);
    assert(sampleIndexI >= 0 && sampleIndexI <= SamplesCount);


    //
    // Update height field
    //

    mCurrentHeightField[SWEOuterLayerSamples + sampleIndexI] = targetY + HeightFieldOffset;

    LogMessage("TODOTEST: Adjusted:", SWEOuterLayerSamples + sampleIndexI, "=", mCurrentHeightField[SWEOuterLayerSamples + sampleIndexI]);

    TODOTEST::AdjustedSample = SWEOuterLayerSamples + sampleIndexI;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void OceanSurface::AdvectHeightField()
{
    //
    // Semi-Lagrangian method
    //

    // Process all height samples, except for boundary condition samples
    for (size_t i = SWEBoundaryConditionsSamples; i < SWETotalSamples - SWEBoundaryConditionsSamples; ++i)
    {
        // The height field values are at the center of the cell,
        // while velocities are at the edges - hence we need to take
        // the two neighboring velocities
        float const v = (mCurrentVelocityField[i] + mCurrentVelocityField[i + 1]) / 2.0f;

        // Calculate the (fractional) index that this height sample had one time step ago
        float const prevCellIndex =
            static_cast<float>(i) + 0.5f
            - v * GameParameters::SimulationStepTimeDuration<float> / Dx;

        // Transform index to ease interpolations, constraining the cell
        // to our grid at the same time
        float const prevCellIndex2 = std::min(
            std::max(0.0f, prevCellIndex - 0.5f),
            static_cast<float>(SWETotalSamples - 1));

        // Calculate integral and fractional parts of the index
        int32_t const prevCellIndexI = FastTruncateInt32(prevCellIndex2);
        float const prevCellIndexF = prevCellIndex2 - prevCellIndexI;
        assert(prevCellIndexF >= 0.0f && prevCellIndexF < 1.0f);

        // Set this height field sample as the previous (in time) sample,
        // interpolated between its two neighbors
        mNextHeightField[i] =
            (1.0f - prevCellIndexF) * mCurrentHeightField[prevCellIndexI]
            + prevCellIndexF * mCurrentHeightField[prevCellIndexI + 1];
    }
}

void OceanSurface::AdvectVelocityField()
{
    //
    // Semi-Lagrangian method
    //

    // Process all velocity samples, except for boundary condition samples
    for (size_t i = SWEBoundaryConditionsSamples; i < SWETotalSamples - SWEBoundaryConditionsSamples; ++i)
    {
        // Velocity values are at the edges of the cell
        float const v = mCurrentVelocityField[i];

        // Calculate the (fractional) index that this velocity sample had one time step ago
        float const prevCellIndex =
            static_cast<float>(i)
            - v * GameParameters::SimulationStepTimeDuration<float> / Dx;

        // Transform index to ease interpolations, constraining the cell
        // to our grid at the same time
        float const prevCellIndex2 = std::min(
            std::max(0.0f, prevCellIndex),
            static_cast<float>(SWETotalSamples - 1));

        // Calculate integral and fractional parts of the index
        int32_t const prevCellIndexI = FastTruncateInt32(prevCellIndex2);
        float const prevCellIndexF = prevCellIndex2 - prevCellIndexI;
        assert(prevCellIndexF >= 0.0f && prevCellIndexF < 1.0f);

        // Set this velocity field sample as the previous (in time) sample,
        // interpolated between its two neighbors
        mNextVelocityField[i] =
            (1.0f - prevCellIndexF) * mCurrentVelocityField[prevCellIndexI]
            + prevCellIndexF * mCurrentVelocityField[prevCellIndexI + 1];
    }
}

void OceanSurface::UpdateHeightField()
{
    // Process all samples, except for boundary condition samples
    for (size_t i = SWEBoundaryConditionsSamples; i < SWETotalSamples - SWEBoundaryConditionsSamples; ++i)
    {
        mNextHeightField[i] -=
            mNextHeightField[i]
            * (mNextVelocityField[i + 1] - mNextVelocityField[i]) / Dx
            * GameParameters::SimulationStepTimeDuration<float>;
    }
}

void OceanSurface::UpdateVelocityField()
{
    // Process all samples, except for boundary condition samples
    for (size_t i = SWEBoundaryConditionsSamples; i < SWETotalSamples - SWEBoundaryConditionsSamples; ++i)
    {
        mNextVelocityField[i] +=
            GameParameters::GravityMagnitude
            * (mNextHeightField[i - 1] - mNextHeightField[i]) / Dx
            * GameParameters::SimulationStepTimeDuration<float>;
    }
}

}