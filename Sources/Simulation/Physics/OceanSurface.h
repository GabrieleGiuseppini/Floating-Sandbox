/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../SimulationEventDispatcher.h"
#include "../SimulationParameters.h"

#include <Core/Buffer.h>
#include <Core/GameMath.h>
#include <Core/PrecalculatedFunction.h>
#include <Core/RunningAverage.h>
#include <Core/StrongTypeDef.h>
#include <Core/SysSpecifics.h>

#include <memory>
#include <optional>

namespace Physics
{

class OceanSurface
{
public:

    OceanSurface(
        World & parentWorld,
        SimulationEventDispatcher & simulationEventDispatcher);

    void Update(
        float currentSimulationTime,
        Wind const & wind,
        SimulationParameters const & simulationParameters);

    void Upload(RenderContext & renderContext) const;

public:

    /*
     * Assumption: x is in world boundaries.
     */
    float GetHeightAt(float x) const noexcept
    {
        assert(x >= -SimulationParameters::HalfMaxWorldWidth && x <= SimulationParameters::HalfMaxWorldWidth);

        //
        // Find sample index and interpolate in-between that sample and the next
        //

        // Fractional index in the sample array
        float const sampleIndexF = (x + SimulationParameters::HalfMaxWorldWidth) / Dx;

        // Integral part
        register_int const sampleIndexI = FastTruncateToArchInt(sampleIndexF);

        // Fractional part within sample index and the next sample index
        float const sampleIndexDx = sampleIndexF - sampleIndexI;

        assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);
        assert(sampleIndexDx >= 0.0f && sampleIndexDx < 1.0f);

        return mSamples[sampleIndexI].SampleValue
            + mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue * sampleIndexDx;
    }

    /*
     * Assumption: x is in world boundaries.
     */
    inline float GetDepth(vec2f const & position) const noexcept
    {
        return GetHeightAt(position.x) - position.y;
    }

    /*
     * Assumption: x is in world boundaries.
     */
    inline bool IsUnderwater(vec2f const & position) const noexcept
    {
        return GetDepth(position) > 0.0f;
    }

    /*
     * Assumption: x is in world boundaries.
     */
    inline vec2f GetNormalAt(float x) const noexcept
    {
        assert(x >= -SimulationParameters::HalfMaxWorldWidth && x <= SimulationParameters::HalfMaxWorldWidth);

        //
        // Find sample index and use delta from next sample
        //

        // Fractional index in the sample array
        float const sampleIndexF = (x + SimulationParameters::HalfMaxWorldWidth) / Dx;

        // Integral part
        register_int const sampleIndexI = FastTruncateToArchInt(sampleIndexF);

        assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);

        return vec2f(
            -mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue,
            Dx).normalise();
    }

    void AdjustTo(
        vec2f const & worldCoordinates,
        float worldRadius);

    inline void DisplaceAt(
        float const x,
        float const yOffset)
    {
        assert(x >= -SimulationParameters::HalfMaxWorldWidth && x <= SimulationParameters::HalfMaxWorldWidth);

        // Fractional index in the sample array - smack in the center
        float const sampleIndexF = (x + SimulationParameters::HalfMaxWorldWidth + Dx / 2.0f) / Dx;

        // Integral part
        register_int const sampleIndexI = FastTruncateToArchInt(sampleIndexF);

        assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);

        // Store - the one with the largest absolute magnitude wins
        float const yDisplacement = yOffset / SWEHeightFieldAmplification;
        if (std::abs(yDisplacement) > std::abs(mDeltaHeightBuffer[DeltaHeightBufferPrefixSize + sampleIndexI]))
        {
            mDeltaHeightBuffer[DeltaHeightBufferPrefixSize + sampleIndexI] = yDisplacement;
        }
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
    void InternalUpload(RenderContext & renderContext) const;

    static inline auto ToSampleIndex(float x) noexcept
    {
        // Calculate sample index, minimizing error
        float const sampleIndexF = (x + SimulationParameters::HalfMaxWorldWidth) / Dx;
        register_int const sampleIndexI = FastTruncateToArchInt(sampleIndexF + 0.5f);
        assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);

        return sampleIndexI;
    }

    void RecalculateWaveCoefficients(
        Wind const & wind,
        SimulationParameters const & simulationParameters);

    void RecalculateAbnormalWaveTimestamps(SimulationParameters const & simulationParameters);

    template<typename TRateDuration, typename TGraceDuration>
    static GameWallClock::time_point CalculateNextAbnormalWaveTimestamp(
        GameWallClock::time_point lastTimestamp,
        TRateDuration rate,
        TGraceDuration gracePeriod);

    void ImpartInteractiveWave(
        float x,
        float targetRelativeHeight,
        float growthRate,
        float worldRadius);

    void UpdateInteractiveWaves();

    void ResetInteractiveWaves();

    void SmoothDeltaBufferIntoHeightField();

    void ApplyDampingBoundaryConditions();

    void UpdateFields(SimulationParameters const & simulationParameters);

    void AdvectFields();

    void GenerateSamples(
        float currentSimulationTime,
        Wind const & wind,
        SimulationParameters const & simulationParameters);

private:

    World & mParentWorld;
    SimulationEventDispatcher & mSimulationEventHandler;

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
    PrecalculatedFunction<8192> mBasalWaveSin1;
    GameWallClock::time_point mNextTsunamiTimestamp;
    GameWallClock::time_point mNextRogueWaveTimestamp;

    // Parameters that the calculated values are current with
    float mWindBaseAndStormSpeedMagnitude;
    float mBasalWaveHeightAdjustment;
    float mBasalWaveLengthAdjustment;
    float mBasalWaveSpeedAdjustment;
    std::chrono::minutes mTsunamiRate;
    std::chrono::seconds mRogueWaveRate;

    //
    // SWE Constants
    //

    // The rest height of the height field - indirectly determines speed
    // of waves (via dv/dt <= dh/dx, with dh/dt <= h*dv/dx).
    // Sensitive to Dx - With Dx=1.22, a good offset is 100; with dx=0.61, a good offset is 50
    static float constexpr SWEHeightFieldOffset = 50.0f;

    // The factor by which we amplify the height field perturbations;
    // higher values allow for smaller height field variations with the same visual height,
    // and smaller height field variations allow for greater stability. However, higher
    // values also cause more steepness in waves, with ugly vertical walls.
    // World offset = SWE offset * SWEHeightFieldAmplification
    static float constexpr SWEHeightFieldAmplification = 50.0f;

    //
    // Samples buffer
    //
    // - Contains actual ocean surface heightfield, result of all other buffers
    // - Geometry:
    //      - Buffer "body" (size == SamplesCount + 1, one extra sample to allow for numeric imprecisions falling over boundary)
    //

    // The number of samples for the entire world width;
    // a higher value means more resolution at the expense of Update() and cache misses
    static size_t constexpr SamplesCount = 16384;

    // The x step of the samples
    static float constexpr Dx = SimulationParameters::MaxWorldWidth / static_cast<float>(SamplesCount - 1);

    // What we store for each sample
    struct Sample
    {
        float SampleValue; // Value of this sample
        float SampleValuePlusOneMinusSampleValue; // Delta between next sample and this sample
    };

    // The samples
    Buffer<Sample> mSamples;

    //
    // SWE Buffers
    //
    // - Geometry:
    //      - Padding for making buffer "body" below aligned (size == SWEBufferAlignmentPrefixSize)
    //      - Floats set aside for SWE's boundary conditions (size == SWEBoundaryConditionsSamples)
    //      - Buffer "body" (size == SamplesCount)
    //      - Floats set aside for SWE's boundary conditions (size == SWEBoundaryConditionsSamples)
    //      - Velocity buffer only: one extra sample, as this buffer surrounds the height buffer
    //

    // The number of samples we set apart in the SWE buffers for boundary conditions at each end of a buffer
    static size_t constexpr SWEBoundaryConditionsSamples = 3;

    // The extra float's at the beginning of the SWE buffers necessary
    // to make each buffer "body" (i.e. the non-outer section) aligned
    static size_t constexpr SWEBufferAlignmentPrefixSize = make_aligned_float_element_count(SWEBoundaryConditionsSamples) - SWEBoundaryConditionsSamples;

    // For convenience: offset of "body"
    static size_t constexpr SWEBufferPrefixSize = SWEBufferAlignmentPrefixSize + SWEBoundaryConditionsSamples;
    static_assert(is_aligned_to_float_element_count(SWEBufferPrefixSize));

    // SWE height field
    // - Height values are at the center of the staggered grid cells
    Buffer<float> mSWEHeightField;

    // SWE velocity field
    // - Velocity values are at the edges of the staggered grid cells
    //      - H[i] has V[i] at its left and V[i+1] at its right
    Buffer<float> mSWEVelocityField;

    //
    // Interactive waves
    //

    // Absolute desired height of SWE field; continuously updated during interacting
    Buffer<float> mInteractiveWaveTargetHeight;

    // We reach target height by this "growth coefficient" (fraction) of the remaining height;
    // this is basically the strength with which we pull the SWE height field.
    // The coefficient itself varies over time
    Buffer<float> mInteractiveWaveCurrentHeightGrowthCoefficient;
    Buffer<float> mInteractiveWaveTargetHeightGrowthCoefficient;

    // The rate at which the growth coefficient grows itself.
    // During interaction (rising), this is the speed at which the height growth coefficient raises;
    // during release (falling), this is basically the rate at which we let go of pulling the SWE height field
    Buffer<float> mInteractiveWaveHeightGrowthCoefficientGrowthRate;

    //
    // Delta height buffer
    //
    // - Contains surface height delta's that are taken into account during update step, being smoothed back
    //   into height field.
    //
    // - Geometry:
    //      - Padding for making buffer "body" below aligned (size == DeltaHeightBufferAlignmentPrefixSize)
    //      - Half smoothing window (which will be filled with zeroes, size == DeltaHeightSmoothing / 2)
    //      - Buffer "body" (size == SamplesCount)
    //      - Half smoothing window (which will be filled with zeroes, size == DeltaHeightSmoothing / 2)
    //

    // The width of the delta-height smoothing
    static size_t constexpr DeltaHeightSmoothing = 5;
    static_assert((DeltaHeightSmoothing % 2) == 1);

    // The extra float's at the beginning of the delta-height buffer necessary
    // to make the delta-height buffer *body* (i.e. the section after the zero's prefix)
    // aligned
    static size_t constexpr DeltaHeightBufferAlignmentPrefixSize = make_aligned_float_element_count(DeltaHeightSmoothing / 2) - (DeltaHeightSmoothing / 2);

    // For convenience: offset of "body"
    static size_t constexpr DeltaHeightBufferPrefixSize = DeltaHeightBufferAlignmentPrefixSize + (DeltaHeightSmoothing / 2);
    static_assert(is_aligned_to_float_element_count(DeltaHeightBufferPrefixSize));

    static size_t constexpr DeltaHeightBufferSize = DeltaHeightBufferAlignmentPrefixSize + (DeltaHeightSmoothing / 2) + SamplesCount + (DeltaHeightSmoothing / 2);

    Buffer<float> mDeltaHeightBuffer;

private:

    //
    // Abnormal waves
    //

    class SWEAbnormalWaveStateMachine
    {
    public:

        SWEAbnormalWaveStateMachine(
            float centerX,
            float targetRelativeHeight,
            float rate,
            float currentSimulationTime)
            : mCenterX(centerX)
            , mTargetRelativeHeight(targetRelativeHeight)
            , mRate(rate)
            , mStartSimulationTime(currentSimulationTime)
        {}

        float GetCenterX() const
        {
            return mCenterX;
        }

        float GetTargetRelativeHeight() const
        {
            return mTargetRelativeHeight;
        }

        float GetRate() const
        {
            return mRate;
        }

        float GetStartSimulationTime() const
        {
            return mStartSimulationTime;
        }

    private:

        float const mCenterX;
        float const mTargetRelativeHeight;
        float const mRate;
        float mStartSimulationTime;
    };

    std::optional<SWEAbnormalWaveStateMachine> mSWETsunamiWaveStateMachine;
    std::optional<SWEAbnormalWaveStateMachine> mSWERogueWaveWaveStateMachine;

    GameWallClock::time_point mLastTsunamiTimestamp;
    GameWallClock::time_point mLastRogueWaveTimestamp;
};

}
