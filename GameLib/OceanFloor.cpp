/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

OceanFloor::OceanFloor()
    : mSamples(new Sample[SamplesCount + 1])
    , mCurrentSeaDepth(std::numeric_limits<float>::lowest())
{
}

void OceanFloor::Update(GameParameters const & gameParameters)
{
    if (gameParameters.SeaDepth != mCurrentSeaDepth)
    {
        //
        // We fill-in an extra sample, so we can avoid having to wrap around
        //

        // sample index = 0 
        float previousSampleValue;
        {
            previousSampleValue = - gameParameters.SeaDepth;
            mSamples[0].SampleValue = previousSampleValue;
        }

        // sample index = 1...SamplesCount
        float x = Dx;
        for (int64_t i = 1; i <= SamplesCount; i++, x += Dx)
        {
            float const c1 = sinf(x * Frequency1) * 10.f;
            float const c2 = sinf(x * Frequency2) * 6.f;
            float const c3 = sinf(x * Frequency3) * 45.f;
            float const sampleValue = (c1 + c2 - c3) - gameParameters.SeaDepth;
            mSamples[i].SampleValue = sampleValue;
            mSamples[i - 1].SampleValuePlusOneMinusSampleValue = sampleValue - previousSampleValue;

            previousSampleValue = sampleValue;
        }

        // sample index = SamplesCount
        mSamples[SamplesCount].SampleValuePlusOneMinusSampleValue = mSamples[0].SampleValue - previousSampleValue;

        // Remember current sea depth
        mCurrentSeaDepth = gameParameters.SeaDepth;
    }
}

}