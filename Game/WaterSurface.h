/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"

#include <GameCore/GameMath.h>

#include <memory>

namespace Physics
{

class WaterSurface
{
public:

    WaterSurface();

    void Update(
        float currentSimulationTime,
        GameParameters const & gameParameters);

   float GetWaterHeightAt(float x) const
    {
        // Fractional absolute index in the (infinite) sample array
        float const absoluteSampleIndexF = x / Dx;

        // Integral part
        int64_t absoluteSampleIndexI = FastFloorInt64(absoluteSampleIndexF);

        // Integral part - sample
        int64_t sampleIndexI;

        // Fractional part within sample index and the next sample index
        float sampleIndexDx;

        if (absoluteSampleIndexI >= 0)
        {
            sampleIndexI = absoluteSampleIndexI % SamplesCount;
            sampleIndexDx = absoluteSampleIndexF - absoluteSampleIndexI;
        }
        else
        {
            sampleIndexI = (SamplesCount - 1) + (absoluteSampleIndexI % SamplesCount);
            sampleIndexDx = 1.0f + absoluteSampleIndexF - absoluteSampleIndexI;
        }

        assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);

        return mSamples[sampleIndexI].SampleValue
             + mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue * sampleIndexDx;
    }

private:

    // Frequencies of the wave components
    static constexpr float Frequency1 = 0.1f;
    static constexpr float Frequency2 = 0.3f;

    // Period of the sum of the frequency components
    static constexpr float Period = 20.0f * Pi<float>;

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
