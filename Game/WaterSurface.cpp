/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

WaterSurface::WaterSurface()
    : mSamples(new Sample[SamplesCount])
{
}

void WaterSurface::Update(
    float currentSimulationTime,
    Wind const & wind,
    GameParameters const & gameParameters)
{
    float const waveSpeed = gameParameters.WindSpeedBase / 8.0f; // Water moves slower than wind
    float const scaledTime = currentSimulationTime * (0.5f + waveSpeed) / 3.0f;

    float const waveHeight = gameParameters.WaveHeight;

    float const windForceAbsoluteMagnitude = wind.GetCurrentWindForce().length();
    float const rawWindNormalizedIncisiveness =
        std::max(0.0f, windForceAbsoluteMagnitude - abs(wind.GetBaseMagnitude()))
        / abs(wind.GetMaxMagnitude() - wind.GetBaseMagnitude());
    float const smoothedWindNormalizedIncisiveness = mWindIncisivenessRunningAverage.Update(rawWindNormalizedIncisiveness);

    float const windRipplesFrequency = 128.0f * gameParameters.WindSpeedBase / 24.0f;
    float const windRipplesWaveHeight = 0.7f * smoothedWindNormalizedIncisiveness;

    // sample index = 0
    float previousSampleValue;
    {
        float const c1 = sinf(scaledTime) * 0.5f;
        float const c2 = sinf(-scaledTime * 1.1f) * 0.3f;
        float const c3 = sinf(-currentSimulationTime * windRipplesFrequency);
        previousSampleValue = (c1 + c2) * waveHeight + c3 * windRipplesWaveHeight;
        mSamples[0].SampleValue = previousSampleValue;
    }

    // sample index = 1...SamplesCount - 1
    float x = Dx;
    for (int64_t i = 1; i < SamplesCount; i++, x += Dx)
    {
        float const c1 = sinf(x * Frequency1 + scaledTime) * 0.5f;
        float const c2 = sinf(x * Frequency2 - scaledTime * 1.1f) * 0.3f;
        float const c3 = sinf(x * Frequency3 - currentSimulationTime * windRipplesFrequency);
        float const sampleValue = (c1 + c2) * waveHeight + c3 * windRipplesWaveHeight;
        mSamples[i].SampleValue = sampleValue;
        mSamples[i - 1].SampleValuePlusOneMinusSampleValue = sampleValue - previousSampleValue;

        previousSampleValue = sampleValue;
    }

    // sample index = SamplesCount
    mSamples[SamplesCount - 1].SampleValuePlusOneMinusSampleValue = mSamples[0].SampleValue - previousSampleValue;
}

}