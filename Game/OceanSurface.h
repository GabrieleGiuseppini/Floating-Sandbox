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

class OceanSurface
{
public:

    OceanSurface(
        float currentSimulationTime,
        Wind const & wind,
        GameParameters const & gameParameters);

    void Update(
        float currentSimulationTime,
        Wind const & wind,
        GameParameters const & gameParameters);

    void Upload(
        GameParameters const & gameParameters,
        Render::RenderContext & renderContext) const;

public:

    void AdjustTo(
        float x,
        float y);

    float GetHeightAt(float x) const
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

public:

    // The number of samples for the entire world width;
    // a higher value means more resolution at the expense of Update() and of cache misses
    static constexpr int64_t SamplesCount = 8192;

    // The x step of the samples
    static constexpr float Dx = GameParameters::MaxWorldWidth / static_cast<float>(SamplesCount);

private:

    void AdvectHeightField();
    void AdvectVelocityField();
    void UpdateHeightField();
    void UpdateVelocityField();

private:

    // What we store for each sample
    struct Sample
    {
        float SampleValue; // Value of this sample
        float SampleValuePlusOneMinusSampleValue; // Delta between next sample and this sample
    };

    // The samples (plus 1 to account for x==MaxWorldWidth)
    std::unique_ptr<Sample[]> mSamples;

    // Smoothing of wind incisiveness
    RunningAverage<15> mWindIncisivenessRunningAverage;

    //
    // Shallow water equations
    //

    // Height field (dual-buffered)
    // - Height values are at the center of the staggered grid cells
    std::unique_ptr<float> mHeightFieldBuffer1;
    std::unique_ptr<float> mHeightFieldBuffer2;
    float * mCurrentHeightField;
    float * mNextHeightField;

    // Velocity field (dual-buffered)
    // - Velocity values are at the edges of the staggered grid cells
    std::unique_ptr<float> mVelocityFieldBuffer1;
    std::unique_ptr<float> mVelocityFieldBuffer2;
    float * mCurrentVelocityField;
    float * mNextVelocityField;
};

}
