/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

// The number of slices we want to render the water surface as;
// this is the graphical resolution
template<typename T>
static constexpr T RenderSlices = 500;

WaterSurface::WaterSurface()
    : mSamples(new Sample[SamplesCount + 1])
{
}

void WaterSurface::Update(
    float currentSimulationTime,
    Wind const & wind,
    GameParameters const & gameParameters)
{
    // Waves

    float const waveSpeed = gameParameters.WindSpeedBase / 6.0f; // Water moves slower than wind
    float const waveTheta = currentSimulationTime * (0.5f + waveSpeed) / 3.0f;
    float const waveHeight = gameParameters.WaveHeight;

    // Ripples

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

    // Spatial frequencies of the wave components

    static constexpr float SpatialFrequency1 = 0.1f;
    static constexpr float SpatialFrequency2 = 0.3f;
    static constexpr float SpatialFrequency3 = 0.5f; // Wind component

    //
    // Create samples
    //

    // sample index = 0
    float previousSampleValue;
    {
        float const c1 = sinf(waveTheta) * 0.5f;
        float const c2 = sinf(-waveTheta * 1.1f) * 0.3f;
        float const c3 = sinf(-currentSimulationTime * windRipplesTimeFrequency);
        previousSampleValue = (c1 + c2) * waveHeight + c3 * windRipplesWaveHeight;
        mSamples[0].SampleValue = previousSampleValue;
    }

    // sample index = 1...SamplesCount - 1
    float x = Dx;
    for (int64_t i = 1; i < SamplesCount; i++, x += Dx)
    {
        float const c1 = sinf(x * SpatialFrequency1 + waveTheta) * 0.5f;
        float const c2 = sinf(x * SpatialFrequency2 - waveTheta * 1.1f) * 0.3f;
        float const c3 = sinf(x * SpatialFrequency3 - currentSimulationTime * windRipplesTimeFrequency);
        float const sampleValue = (c1 + c2) * waveHeight + c3 * windRipplesWaveHeight;
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

void WaterSurface::Upload(
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
                GetWaterHeightAt(sampleIndexX),
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