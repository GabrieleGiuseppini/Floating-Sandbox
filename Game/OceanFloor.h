/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"
#include "ResourceLoader.h"

#include "GameParameters.h"

#include <GameCore/GameMath.h>

#include <memory>

namespace Physics
{

class OceanFloor
{
public:

    OceanFloor(ResourceLoader & resourceLoader);

    bool AdjustTo(
        float x,
        float targetY);

    void Update(GameParameters const & gameParameters);

    size_t GetSamplesCount() const
    {
        return SamplesCount;
    }

    float GetFloorHeightAt(float x) const
    {
        // Fractional absolute index in the (infinite) sample array
        float const absoluteSampleIndexF = x / Dx;

        // Integral part
        int64_t absoluteSampleIndexI = FastFloorInt64(absoluteSampleIndexF);

        // Integral part - sample
        int64_t sampleIndexI = absoluteSampleIndexI % SamplesCount;

        // Fractional part within sample index and the next sample index
        float sampleIndexDx = absoluteSampleIndexF - absoluteSampleIndexI;

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

    // The current samples
    std::unique_ptr<Sample[]> mSamples;

    // The bump map samples - between -H/2 and H/2
    std::unique_ptr<float[]> const mBumpMapSamples;

    // The game parameters for which we're current
    float mCurrentSeaDepth;
    float mCurrentOceanFloorBumpiness;
    float mCurrentOceanFloorDetailAmplification;
};

}
