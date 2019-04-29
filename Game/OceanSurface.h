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
#include <optional>

namespace Physics
{

class OceanSurface
{
public:

    OceanSurface();

    void Update(
        float currentSimulationTime,
        Wind const & wind,
        GameParameters const & gameParameters);

    void Upload(
        GameParameters const & gameParameters,
        Render::RenderContext & renderContext) const;

public:

    void AdjustTo(
        std::optional<vec2f> const & worldCoordinates,
        float currentSimulationTime);

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
    static int64_t constexpr SamplesCount = 8192;

    // The x step of the samples
    static float constexpr Dx = GameParameters::MaxWorldWidth / static_cast<float>(SamplesCount);

private:

    void AdvectHeightField();
    void AdvectVelocityField();
    void UpdateHeightField();
    void UpdateVelocityField();

    void GenerateSamples(
        float currentSimulationTime,
        Wind const & wind,
        GameParameters const & gameParameters);

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

private:

    class SWEWaveStateMachine
    {
    public:

        enum class ReleaseModeType
        {
            // Begins the wave downfall only when the rise is completed
            Automatic,

            // Begins the wave downfall only when Release is invoked
            OnCue
        };

    public:

        SWEWaveStateMachine(
            int32_t sampleIndex,
            float lowHeight,
            float highHeight,
            ReleaseModeType releaseMode,
            float currentSimulationTime);

        auto GetSampleIndex() const
        {
            return mSampleIndex;
        }

        void Restart(
            float newTargetHeight,
            float currentSimulationTime);

        void Release(float currentSimulationTime);

        /*
         * Returns none when it may be retired.
         */
        std::optional<float> Update(
            float currentSimulationTime);

    private:

        float CalculateSmoothingDelay();

        void StartFallPhase(float currentSimulationTime);

        enum class WavePhaseType
        {
            Rise,
            Fall
        };

        int32_t const mSampleIndex;
        float const mLowHeight;
        float mCurrentPhaseStartHeight;
        float mCurrentPhaseTargetHeight;
        float mCurrentHeight;
        float mCurrentProgress; // Between 0 and 1, regardless of direction
        float mStartSimulationTime;
        WavePhaseType mCurrentWavePhase;
        ReleaseModeType const mReleaseMode;
        float mSmoothingDelay;
    };

    // Interactive wave
    std::optional<SWEWaveStateMachine> mSWEExternalWaveStateMachine;
};

}
