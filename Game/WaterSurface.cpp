/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

WaterSurface::WaterSurface()
    : mSamples(new Sample[SamplesCount + 1])
{
}

void WaterSurface::Update(
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    float const waveSpeed = gameParameters.WindSpeedBase / 8.0f; // Water moves slower than clouds
    float const scaledTime = currentSimulationTime * (0.5f + waveSpeed) / 3.0f;

    //
    // We fill-in an extra sample, so we can avoid having to wrap around
    //

    // sample index = 0
    float previousSampleValue;
    {
        float const c1 = sinf(scaledTime) * 0.5f;
        float const c2 = sinf(-scaledTime * 1.1f) * 0.3f;
        previousSampleValue = (c1 + c2) * gameParameters.WaveHeight;
        mSamples[0].SampleValue = previousSampleValue;
    }

    // sample index = 1...SamplesCount
    float x = Dx;
    for (int64_t i = 1; i <= SamplesCount; i++, x += Dx)
    {
        float const c1 = sinf(x * Frequency1 + scaledTime) * 0.5f;
        float const c2 = sinf(x * Frequency2 - scaledTime * 1.1f) * 0.3f;
        float const sampleValue = (c1 + c2) * gameParameters.WaveHeight;
        mSamples[i].SampleValue = sampleValue;
        mSamples[i - 1].SampleValuePlusOneMinusSampleValue = sampleValue - previousSampleValue;

        previousSampleValue = sampleValue;
    }

    // sample index = SamplesCount
    mSamples[SamplesCount].SampleValuePlusOneMinusSampleValue = mSamples[0].SampleValue - previousSampleValue;
}

}