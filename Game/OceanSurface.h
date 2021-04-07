/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventDispatcher.h"
#include "GameParameters.h"

#include <GameCore/FixedSizeVector.h>
#include <GameCore/GameMath.h>
#include <GameCore/PrecalculatedFunction.h>
#include <GameCore/RunningAverage.h>
#include <GameCore/StrongTypeDef.h>

#include <optional>

namespace Physics
{

class OceanSurface
{
public:

    OceanSurface(
        World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher);

    void Update(
        float currentSimulationTime,
        Wind const & wind,
        GameParameters const & gameParameters);

    void Upload(Render::RenderContext & renderContext) const;

public:

    /*
     * Assumption: x is in world boundaries.
     */
    float GetHeightAt(float x) const noexcept
    {
        assert(x >= -GameParameters::HalfMaxWorldWidth && x <= GameParameters::HalfMaxWorldWidth);

        //
        // Find sample index and interpolate in-between that sample and the next
        //

        // Fractional index in the sample array
        float const sampleIndexF = (x + GameParameters::HalfMaxWorldWidth) / Dx;

        // Integral part
        auto const sampleIndexI = FastTruncateToArchInt(sampleIndexF);

        // Fractional part within sample index and the next sample index
        float const sampleIndexDx = sampleIndexF - sampleIndexI;

        assert(sampleIndexI >= 0 && sampleIndexI <= SamplesCount);
        assert(sampleIndexDx >= 0.0f && sampleIndexDx <= 1.0f);

        return mSamples[sampleIndexI].SampleValue
            + mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue * sampleIndexDx;
    }

    /*
     * Assumption: x is in world boundaries.
     */
    inline vec2f GetNormalAt(float x) const noexcept
    {
        assert(x >= -GameParameters::HalfMaxWorldWidth && x <= GameParameters::HalfMaxWorldWidth);

        //
        // Find sample index and use delta from next sample
        //

        // Fractional index in the sample array
        float const sampleIndexF = (x + GameParameters::HalfMaxWorldWidth) / Dx;

        // Integral part
        auto const sampleIndexI = FastTruncateToArchInt(sampleIndexF);

        assert(sampleIndexI >= 0 && sampleIndexI <= SamplesCount);

        return vec2f(
            -mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue,
            Dx).normalise();
    }

    void AdjustTo(
        std::optional<vec2f> const & worldCoordinates,
        float currentSimulationTime);

    inline void DisplaceAt(
        float const x,
        float const yOffset)
    {
        assert(x >= -GameParameters::HalfMaxWorldWidth
            && x <= GameParameters::HalfMaxWorldWidth);

        // Fractional index in the sample array - smack in the center
        float const sampleIndexF = (x + GameParameters::HalfMaxWorldWidth + Dx / 2.0f) / Dx;

        // Integral part
        auto const sampleIndexI = FastTruncateToArchInt(sampleIndexF);

        assert(sampleIndexI >= 0 && sampleIndexI <= SamplesCount);

        // Store
        mDeltaHeightBuffer[(DeltaHeightSmoothing / 2) + sampleIndexI] += yOffset / SWEHeightFieldAmplification;
    }

    void ApplyThanosSnap(
        float leftFrontX,
        float rightFrontX);

    void TriggerTsunami(float currentSimulationTime);

    void TriggerRogueWave(
        float currentSimulationTime,
        Wind const & wind);

private:

    template<OceanRenderDetailType DetailType>
    void InternalUpload(Render::RenderContext & renderContext) const;

    static inline auto ToSampleIndex(float x) noexcept
    {
        // Calculate sample index, minimizing error
        float const sampleIndexF = (x + GameParameters::HalfMaxWorldWidth) / Dx;
        auto const sampleIndexI = FastTruncateToArchInt(sampleIndexF + 0.5f);
        assert(sampleIndexI >= 0 && sampleIndexI <= SamplesCount);

        return sampleIndexI;
    }

    void SetSWEWaveHeight(
        size_t centerIndex,
        float height);

    void RecalculateWaveCoefficients(
        Wind const & wind,
        GameParameters const & gameParameters);

    void RecalculateAbnormalWaveTimestamps(GameParameters const & gameParameters);

    template<typename TDuration>
    static GameWallClock::time_point CalculateNextAbnormalWaveTimestamp(
        GameWallClock::time_point lastTimestamp,
        TDuration rate);

    void ApplyDampingBoundaryConditions();

    //void AdvectFieldsTest();

    void UpdateFields();

    void GenerateSamples(
        float currentSimulationTime,
        Wind const & wind,
        GameParameters const & gameParameters);

private:

    World & mParentWorld;
    std::shared_ptr<GameEventDispatcher> mGameEventHandler;

    // Smoothing of wind incisiveness
    RunningAverage<15> mWindIncisivenessRunningAverage;

    //
    // Constants
    //

    // The number of samples for the entire world width;
    // a higher value means more resolution at the expense of Update() and of cache misses
    static size_t constexpr SamplesCount = 16384;

    // The x step of the samples
    static float constexpr Dx = GameParameters::MaxWorldWidth / static_cast<float>(SamplesCount);

    //
    // SWE Layer constants
    //

    // The rest height of the height field - indirectly determines velocity
    // of waves (via dv/dt <= dh/dx, with dh/dt <= h*dv/dx).
    // Sensitive to Dx - With Dx=1.22, a good offset is 100; with dx=0.61, a good offset is 50
    static float constexpr SWEHeightFieldOffset = 50.0f;

    // The factor by which we amplify the height field perturbations;
    // higher values allow for smaller height field variations with the same visual height,
    // and smaller height field variations allow for greater stability
    // World offset = SWE offset * SWEHeightFieldAmplification
    static float constexpr SWEHeightFieldAmplification = 50.0f;

    // The number of samples we raise with a state machine
    static size_t constexpr SWEWaveStateMachinePerturbedSamplesCount = 3;

    // The number of samples we set apart in the SWE buffers for wave generation at each end of a buffer
    static size_t constexpr SWEWaveGenerationSamples = 1;

    // The number of samples we set apart in the SWE buffers for boundary conditions at each end of a buffer
    static size_t constexpr SWEBoundaryConditionsSamples = 3;

    static size_t constexpr SWEOuterLayerSamples =
        SWEWaveGenerationSamples
        + SWEBoundaryConditionsSamples;

    // The total number of samples in the SWE buffers
    static size_t constexpr SWETotalSamples =
        SWEOuterLayerSamples
        + SamplesCount
        + SWEOuterLayerSamples;

    // The width of the delta-height smoothing
    static size_t constexpr DeltaHeightSmoothing = 5;
    static_assert((DeltaHeightSmoothing % 2) == 1);

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
    // Buffers
    //

    // What we store for each sample
    struct Sample
    {
        float SampleValue; // Value of this sample
        float SampleValuePlusOneMinusSampleValue; // Delta between next sample and this sample
    };

    // The samples (plus 1 to account for x==MaxWorldWidth)
    FixedSizeVector<Sample, SamplesCount + 1> mSamples; // One extra sample for the rightmost X

    //
    // SWE buffers
    //

    // Height field
    // - Height values are at the center of the staggered grid cells
    FixedSizeVector<float, SWETotalSamples + 1> mHeightField; // One extra cell just to ease interpolations

    // Velocity field
    // - Velocity values are at the edges of the staggered grid cells
    //      - H[i] has V[i] at its left and V[i+1] at its right
    FixedSizeVector<float, SWETotalSamples + 1> mVelocityField; // One extra cell just to ease interpolations

    // Delta height buffer
    // - Contains interactive surface height delta's that are taken into account during update step
    FixedSizeVector<float, (DeltaHeightSmoothing / 2) + (SamplesCount + 1) + (DeltaHeightSmoothing / 2)> mDeltaHeightBuffer; // One extra sample for the rightmost X

private:

    //
    // Interactive waves
    //

    class SWEInteractiveWaveStateMachine
    {
    public:

        SWEInteractiveWaveStateMachine(
            size_t centerIndex,
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

        bool MayBeOverridden() const;

    private:

        enum class WavePhaseType
        {
            Rise,
            Fall
        };

        static float CalculateRisingPhaseDuration(float deltaHeight);

        static float CalculateFallingPhaseDecayCoefficient(float deltaHeight);

        size_t const mCenterIndex;
        float const mOriginalHeight;
        float mCurrentPhaseStartHeight;
        float mCurrentPhaseTargetHeight;
        float mCurrentHeight;
        float mStartSimulationTime;
        WavePhaseType mCurrentWavePhase;

        float mRisingPhaseDuration;
        float mFallingPhaseDecayCoefficient;
    };

    std::optional<SWEInteractiveWaveStateMachine> mSWEInteractiveWaveStateMachine;


    //
    // Abnormal waves
    //

    class SWEAbnormalWaveStateMachine
    {
    public:

        SWEAbnormalWaveStateMachine(
            size_t centerIndex,
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

        size_t const mCenterIndex;
        float const mLowHeight;
        float const mHighHeight;
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
