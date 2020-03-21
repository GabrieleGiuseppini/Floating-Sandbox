/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventDispatcher.h"
#include "GameParameters.h"

#include <GameCore/GameMath.h>
#include <GameCore/PrecalculatedFunction.h>
#include <GameCore/RunningAverage.h>

#include <memory>
#include <optional>

namespace Physics
{

class OceanSurface
{
public:

    OceanSurface(std::shared_ptr<GameEventDispatcher> gameEventDispatcher);

    void Update(
        float currentSimulationTime,
        Wind const & wind,
        GameParameters const & gameParameters);

    void Upload(
        GameParameters const & gameParameters,
        Render::RenderContext & renderContext) const;

public:

    /*
     * Assumption: x is in world boundaries.
     */
    float GetHeightAt(float x) const noexcept
    {
        assert(x >= -GameParameters::HalfMaxWorldWidth
            && x <= GameParameters::HalfMaxWorldWidth + 0.01f); // Allow for derivative taking

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

    void AdjustTo(
        std::optional<vec2f> const & worldCoordinates,
        float currentSimulationTime);

    void ApplyThanosSnap(
        float leftFrontX,
        float rightFrontX);

    void TriggerTsunami(float currentSimulationTime);

    void TriggerRogueWave(
        float currentSimulationTime,
        Wind const & wind);

public:

    // The number of samples for the entire world width;
    // a higher value means more resolution at the expense of Update() and of cache misses
    static int64_t constexpr SamplesCount = 8192;

    // The x step of the samples
    static float constexpr Dx = GameParameters::MaxWorldWidth / static_cast<float>(SamplesCount);

private:

    static int32_t ToSampleIndex(float x)
    {
        // Calculate sample index, minimizing error
        float const sampleIndexF = (x + GameParameters::HalfMaxWorldWidth) / Dx;
        int32_t sampleIndexI = FastTruncateInt32(sampleIndexF + 0.5f);
        assert(sampleIndexI >= 0 && sampleIndexI <= SamplesCount);

        return sampleIndexI;
    }

    void SetSWEWaveHeight(
        int32_t centerIndex,
        float height);

    void RecalculateCoefficients(
        Wind const & wind,
        GameParameters const & gameParameters);

    template<typename TDuration>
    static GameWallClock::time_point CalculateNextAbnormalWaveTimestamp(
        GameWallClock::time_point lastTimestamp,
        TDuration rate);

    void ApplyDampingBoundaryConditions();
    void UpdateFields();

    void GenerateSamples(
        float currentSimulationTime,
        Wind const & wind,
        GameParameters const & gameParameters);

private:

    std::shared_ptr<GameEventDispatcher> mGameEventHandler;

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
    // Calculated coefficients
    //

    // Calculated values
    float mBasalWaveAmplitude1;
    float mBasalWaveAmplitude2;
    float mBasalWaveNumber1;
    float mBasalWaveNumber2;
    float mBasalWaveAngularVelocity1;
    float mBasalWaveAngularVelocity2;
    PrecalculatedFunction<SamplesCount> mBasalWaveSin1;
    GameWallClock::time_point mNextTsunamiTimestamp;
    GameWallClock::time_point mNextRogueWaveTimestamp;

    // Parameters that the calculated values are current with
    float mWindBaseAndStormSpeedMagnitude;
    float mBasalWaveHeightAdjustment;
    float mBasalWaveLengthAdjustment;
    float mBasalWaveSpeedAdjustment;
    std::chrono::minutes mTsunamiRate;
    std::chrono::minutes mRogueWaveRate;


    //
    // Shallow water equations
    //

    // Height field
    // - Height values are at the center of the staggered grid cells
    std::unique_ptr<float[]> mHeightField;

    // Velocity field
    // - Velocity values are at the edges of the staggered grid cells
    std::unique_ptr<float[]> mVelocityField;

private:

    //
    // Interactive waves
    //

    class SWEInteractiveWaveStateMachine
    {
    public:

        SWEInteractiveWaveStateMachine(
            int32_t centerIndex,
            float lowHeight,
            float highHeight,
            float currentSimulationTime);

        // Absolute coordinate, not sample coordinate
        auto GetCenterIndex() const
        {
            return mCenterIndex;
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

        enum class WavePhaseType
        {
            Rise,
            Fall
        };

        int32_t const mCenterIndex;
        float const mLowHeight;
        float mCurrentPhaseStartHeight;
        float mCurrentPhaseTargetHeight;
        float mCurrentHeight;
        float mCurrentProgress; // Between 0 and 1, regardless of direction
        float mStartSimulationTime;
        WavePhaseType mCurrentWavePhase;
        float mCurrentSmoothingDelay;
    };

    std::optional<SWEInteractiveWaveStateMachine> mSWEInteractiveWaveStateMachine;


    //
    // Abnormal waves
    //

    class SWEAbnormalWaveStateMachine
    {
    public:

        SWEAbnormalWaveStateMachine(
            int32_t centerIndex,
            float lowHeight,
            float highHeight,
            float riseDelay, // sec
            float fallDelay, // sec
            float currentSimulationTime);

        // Absolute coordinate, not sample coordinate
        auto GetCenterIndex() const
        {
            return mCenterIndex;
        }

        /*
         * Returns none when it may be retired.
         */
        std::optional<float> Update(
            float currentSimulationTime);

    private:

        enum class WavePhaseType
        {
            Rise,
            Fall
        };

        int32_t const mCenterIndex;
        float const mLowHeight;
        float const mHighHeight;
        float const mRiseDelay; // sec
        float const mFallDelay; // sec
        float mCurrentProgress; // Between 0 and 1, regardless of direction
        float mCurrentPhaseStartSimulationTime;
        float mCurrentPhaseDelay;
        WavePhaseType mCurrentWavePhase;
    };

    std::optional<SWEAbnormalWaveStateMachine> mSWETsunamiWaveStateMachine;
    std::optional<SWEAbnormalWaveStateMachine> mSWERogueWaveWaveStateMachine;

    GameWallClock::time_point mLastTsunamiTimestamp;
    GameWallClock::time_point mLastRogueWaveTimestamp;
};

}
