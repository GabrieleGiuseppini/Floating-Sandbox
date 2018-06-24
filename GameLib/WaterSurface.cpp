/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

WaterSurface::WaterSurface()
    : mSamples(new float[SamplesCount + 1])
{
}

void WaterSurface::Update(
    float currentTime,
    GameParameters const & gameParameters)
{
    // Fill-in an extra sample, so we can avoid having to wrap around
    float x = 0;
    for (int64_t i = 0; i < SamplesCount + 1; i++, x += Dx)
    {
        float const c1 = sinf(x * Frequency1 + currentTime) * 0.5f;
        float const c2 = sinf(x * Frequency2 - currentTime * 1.1f) * 0.3f;
        mSamples[i] = (c1 + c2) * gameParameters.WaveHeight;
    }
}

}