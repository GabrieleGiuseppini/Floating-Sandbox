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

    void Upload(
        GameParameters const & gameParameters,
        Render::RenderContext & renderContext) const;

public:

    float GetWaterHeightAt(float x) const
    {
        //
        // Find sample index and interpolate in-between that sample and the next
        //

        // Fractional index in the sample array
        float const sampleIndexF = (x + GameParameters::HalfMaxWorldWidth) / Dx;

        // Integral part
        int64_t sampleIndexI = FastTruncateInt64(sampleIndexF);

        // Fractional part within sample index and the next sample index
        float sampleIndexDx = sampleIndexF - sampleIndexI;

        assert(sampleIndexI >= 0 && sampleIndexI <= SamplesCount);
        assert(sampleIndexDx >= 0.0f && sampleIndexDx <= 1.0f);

        return mSamples[sampleIndexI].SampleValue
            + mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue * sampleIndexDx;
    }

private:

    // Smoothing of wind incisiveness
    RunningAverage<15> mWindIncisivenessRunningAverage;

    // The number of samples for the entire world width;
    // a higher value means more resolution at the expense of Update() and of cache misses
    static constexpr int64_t SamplesCount = 8192;

    // The x step of the samples
    static constexpr float Dx = GameParameters::MaxWorldWidth / static_cast<float>(SamplesCount);

    // What we store for each sample
    struct Sample
    {
        float SampleValue;
        float SampleValuePlusOneMinusSampleValue;
    };

    // The samples (plus 1 to account for x==MaxWorldWidth)
    std::unique_ptr<Sample[]> mSamples;
};

}
