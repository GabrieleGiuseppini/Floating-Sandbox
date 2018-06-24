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
        float const absoluteSampleIndex = floorf(x / Dx);

        int64_t index = static_cast<int64_t>(absoluteSampleIndex) % SamplesCount;
        if (index < 0)
            index += SamplesCount;

        assert(index >= 0 && index < SamplesCount);

        return mSamples[index]
            + (mSamples[index + 1] - mSamples[index]) * ((x / Dx) - absoluteSampleIndex);
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

    // The samples
    std::unique_ptr<float[]> mSamples;    
};

}
