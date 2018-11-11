/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameMath.h"
#include "GameParameters.h"
#include "Physics.h"

#include <memory>

namespace Physics
{

class OceanFloor
{
public:

    OceanFloor();

    void Update(GameParameters const & gameParameters);

    float GetFloorHeightAt(float x) const
    {
        // Fractional absolute index in the (infinite) sample array
        float const absoluteSampleIndexF = x / Dx;

        // Integral part
        int32_t absoluteSampleIndexI = FastFloorInt32(absoluteSampleIndexF);

        // Integral part - sample
        int32_t sampleIndexI;

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
    static constexpr float Frequency1 = 0.005f;
    static constexpr float Frequency2 = 0.015f;
    static constexpr float Frequency3 = 0.001f;

    // Period of the sum of the frequency components
    static constexpr float Period = 2000.0f * Pi<float>;

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

    // The sea depth for which we're current
    float mCurrentSeaDepth;
};

}
