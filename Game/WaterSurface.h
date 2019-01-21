/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"

#include <GameCore/GameMath.h>
#include <GameCore/RunningAverage.h>

#include <memory>

namespace Physics
{

class WaterSurface
{
public:

    WaterSurface();

    void Update(
        float currentSimulationTime,
        Wind const & wind,
        GameParameters const & gameParameters);

    size_t GetSamplesCount() const
    {
        return SamplesCount;
    }

    float GetWaterHeightAt(float x) const
    {
        // Fractional absolute index in the (infinite) sample array
        float const absoluteSampleIndexF = x / Dx;

        // Integral part
        int64_t absoluteSampleIndexI = FastFloorInt64(absoluteSampleIndexF);

        // Integral part - sample
        int64_t sampleIndexI = absoluteSampleIndexI % SamplesCount;

        // Fractional part within sample index and the next sample index
        float sampleIndexDx= absoluteSampleIndexF - absoluteSampleIndexI;

        if (x < 0.0f)
        {
            // Wrap around and anchor to the left sample
            sampleIndexI += SamplesCount - 1; // Includes shift to left
            sampleIndexDx += 1.0f;
        }

        assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);
        assert(sampleIndexDx >= 0.0f && sampleIndexDx <= 1.0f);

        return mSamples[sampleIndexI].SampleValue
             + mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue * sampleIndexDx;
    }

private:

    // Spatial frequencies of the wave components
    static constexpr float SpatialFrequency1 = 0.1f;
    static constexpr float SpatialFrequency2 = 0.3f;
    static constexpr float SpatialFrequency3 = 0.5f; // Wind component

    // Period of the sum of the spatial frequency components
    static constexpr float Period = 20.0f * Pi<float>;

    // Smoothing of wind incisiveness
    RunningAverage<15> mWindIncisivenessRunningAverage;

    // The number of samples;
    // a higher value means more resolution at the expense of the cost of Update().
    // Power of two's allow the compiler to optimize!
    static constexpr int64_t SamplesCount = 512;

    // The x step of the samples
    static constexpr float Dx = Period / static_cast<float>(SamplesCount);

    // What we store for each sample
    struct Sample
    {
        float SampleValue;
        float SampleValuePlusOneMinusSampleValue;
    };

    // The samples
    std::unique_ptr<Sample[]> mSamples;
};

}
