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

#include <memory>
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

    inline void DisplaceSmallScaleAt(
        float const x,
        float const yOffset)
    {
        assert(x >= -GameParameters::HalfMaxWorldWidth
            && x <= GameParameters::HalfMaxWorldWidth);

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

        // Distribute the displacement offset among the two samples

        ChangeHeightSmooth(
            SWEOuterLayerSamples + sampleIndexI,
            (1.0f - sampleIndexDx) * yOffset / SWEHeightFieldAmplification);

        ChangeHeightSmooth(
            SWEOuterLayerSamples + sampleIndexI + 1,
            sampleIndexDx * yOffset / SWEHeightFieldAmplification);
    }

    inline void DisplaceTODOTESTAt(
        float const x,
        float const yOffset)
    {
        assert(x >= -GameParameters::HalfMaxWorldWidth
            && x <= GameParameters::HalfMaxWorldWidth);

        /* TODOTEST: now trying with delta's
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

        // Distribute the displacement offset among the two samples
        // TODOHERE: there's no guarantee we may access SWEOuterLayerSamples + sampleIndexI + 1
        mHeightField[SWEOuterLayerSamples + sampleIndexI] += (1.0f - sampleIndexDx) * yOffset / SWEHeightFieldAmplification;
        mHeightField[SWEOuterLayerSamples + sampleIndexI + 1] += sampleIndexDx * yOffset / SWEHeightFieldAmplification;
        */

        // Fractional index in the sample array - smack in the center
        float const sampleIndexF = (x + GameParameters::HalfMaxWorldWidth + Dx / 2.0f) / Dx;

        // Integral part
        auto const sampleIndexI = FastTruncateToArchInt(sampleIndexF);

        assert(sampleIndexI >= 0 && sampleIndexI <= SamplesCount);

        // TODOTEST

        // This makes tiny spikes
        // Store
        mDeltaHeightBuffer[(DeltaHeightSmoothing / 2) + sampleIndexI] += yOffset / SWEHeightFieldAmplification;

        /* This smooths
        float const sampleIndexDx = sampleIndexF - sampleIndexI;

        assert(sampleIndexDx >= 0.0f && sampleIndexDx <= 1.0f);

        // Distribute the displacement offset among the two samples
        mDeltaHeightBuffer[sampleIndexI] += (1.0f - sampleIndexDx) * yOffset / SWEHeightFieldAmplification;
        mDeltaHeightBuffer[sampleIndexI + 1] += sampleIndexDx * yOffset / SWEHeightFieldAmplification;
        */
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

    // Adds to a height field sample, ensuring we don't excdeed
    // a maximum slope to keep surface perturbations "smooth"
    inline void ChangeHeightSmooth(
        size_t heightFieldIndex,
        float delta)
    {
        // The index is within the "workable" mid-section of the height field
        assert(heightFieldIndex >= SWEOuterLayerSamples && heightFieldIndex <= SWEOuterLayerSamples + SamplesCount + 1);

        // This is the maximum derivative we allow,
        // based on empirical observations of (hf[i] - hf[i+/-1]) / Dx
        float constexpr MaxDerivative = 0.01f;

        if (delta >= 0.0f)
        {
            float const leftMin = std::min(
                mHeightField[heightFieldIndex] + delta,
                mHeightField[heightFieldIndex - 1] + MaxDerivative * Dx);

            float const rightMin = std::min(
                mHeightField[heightFieldIndex] + delta,
                mHeightField[heightFieldIndex + 1] + MaxDerivative * Dx);

            mHeightField[heightFieldIndex] = std::max( // We don't want to lower the current height
                mHeightField[heightFieldIndex],
                std::min(leftMin, rightMin));
        }
        else
        {
            float const leftMax = std::max(
                mHeightField[heightFieldIndex] + delta,
                mHeightField[heightFieldIndex - 1] - MaxDerivative * Dx);

            float const rightMax = std::max(
                mHeightField[heightFieldIndex] + delta,
                mHeightField[heightFieldIndex + 1] - MaxDerivative * Dx);

            mHeightField[heightFieldIndex] = std::min( // We don't want to raise the current height
                mHeightField[heightFieldIndex],
                std::max(leftMax, rightMax));
        }
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

    void UpdateFields();

    void GenerateSamples(
        float currentSimulationTime,
        Wind const & wind,
        GameParameters const & gameParameters);

private:

    World & mParentWorld;
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
    // Constants
    //

    // The number of samples for the entire world width;
    // a higher value means more resolution at the expense of Update() and of cache misses
    static size_t constexpr SamplesCount = 8192;

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
    static size_t constexpr DeltaHeightSmoothing = 3;
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
    // SWE buffers
    //

    // Height field
    // - Height values are at the center of the staggered grid cells
    std::unique_ptr<float[]> mHeightField;

    // Velocity field
    // - Velocity values are at the edges of the staggered grid cells
    std::unique_ptr<float[]> mVelocityField;

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
