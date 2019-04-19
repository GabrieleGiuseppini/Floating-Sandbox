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

// The number of samples we set apart in the SWE buffers for wave generation at each end of a buffer
constexpr size_t SWEWaveGenerationSamples = 2;

// The number of samples we set apart in the SWE buffers for boundary conditions at each end of a buffer
constexpr size_t SWEBoundaryConditionsSamples = 1;

constexpr size_t SWEOuterLayerSamples =
    SWEWaveGenerationSamples
    + SWEBoundaryConditionsSamples;

// The total number of samples in the SWE buffers
constexpr size_t SWEBufferSize =
    SWEOuterLayerSamples
    + OceanSurface::SamplesCount
    + SWEOuterLayerSamples;

OceanSurface::OceanSurface(
    float currentSimulationTime,
    Wind const & /*wind*/,
    GameParameters const & gameParameters)
    : mSamples(new Sample[SamplesCount + 1])
    , mHeight1Buffer(new float[SWEBufferSize])
    , mHeight2Buffer(new float[SWEBufferSize])
    , mVelocity1Buffer(new float[SWEBufferSize])
    , mVelocity2Buffer(new float[SWEBufferSize])
{
    //
    // Initialize SWEs
    //

    // Initialize buffer pointers
    mCurrentHeightBuffer = mHeight1Buffer.get();
    mNextHeightBuffer = mHeight2Buffer.get();
    mCurrentVelocityBuffer = mVelocity1Buffer.get();
    mNextVelocityBuffer = mVelocity2Buffer.get();

    // Initialize outer layers
    for (size_t i = 0; i < SWEOuterLayerSamples; ++i)
    {
        mCurrentHeightBuffer[i] = 0.0f;
        mCurrentHeightBuffer[SWEBufferSize - i - 1] = 0.0f;

        mCurrentVelocityBuffer[i] = 0.0f;
        mCurrentVelocityBuffer[SWEBufferSize - i - 1] = 0.0f;
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
        mCurrentHeightBuffer[SWEOuterLayerSamples + i] = (c1 + c2) * waveHeight;
        mCurrentVelocityBuffer[SWEOuterLayerSamples + i] = waveSpeed;
    }
}

void OceanSurface::Update(
    float currentSimulationTime,
    Wind const & wind,
    GameParameters const & gameParameters)
{
    //
    // 1. SWE Update
    //

    // TODOHERE
    std::memcpy(mNextHeightBuffer, mCurrentHeightBuffer, SWEBufferSize * sizeof(float));
    std::memcpy(mNextVelocityBuffer, mCurrentVelocityBuffer, SWEBufferSize * sizeof(float));

    //
    // 2. Swap Buffers
    //

    std::swap(mCurrentHeightBuffer, mNextHeightBuffer);
    std::swap(mCurrentVelocityBuffer, mNextVelocityBuffer);

    //
    // 3. Generate samples
    //
    // - Here we simply superimpose wind gust ripples onto the SWE height field.
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
            mCurrentHeightBuffer[SWEOuterLayerSamples + 0]
            + cRipple * windRipplesWaveHeight;

        mSamples[0].SampleValue = previousSampleValue;
    }

    // sample index = 1...SamplesCount - 1
    float x = Dx;
    for (int64_t i = 1; i < SamplesCount; i++, x += Dx)
    {
        float const cRipple = sinf(x * WindGustRippleSpatialFrequency - currentSimulationTime * windRipplesTimeFrequency);
        float const sampleValue =
            mCurrentHeightBuffer[SWEOuterLayerSamples + i]
            + cRipple * windRipplesWaveHeight;

        mSamples[i].SampleValue = sampleValue;
        mSamples[i - 1].SampleValuePlusOneMinusSampleValue = sampleValue - previousSampleValue;

        previousSampleValue = sampleValue;
    }

    // Populate last delta (extra sample has same value as this sample)
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

}