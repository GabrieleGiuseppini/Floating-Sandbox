/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>
#include <GameCore/GameWallClock.h>

#include <algorithm>
#include <chrono>

namespace Physics {

// The number of slices we want to render the water surface as;
// this is the graphical resolution
template<typename T>
T constexpr RenderSlices = 500;

//
// SWE Layer
//

// The rest height of the height field - indirectly determines velocity
// of waves (via dv/dt <= dh/dx, with dh/dt <= h*dv/dx).
// Sensitive to Dx - With Dx=1.22, a good offset is 100; with dx=0.61, a good offset is 50
float constexpr SWEHeightFieldOffset = 50.0f;

// The factor by which we amplify the height field perturbations;
// higher values allow for smaller height field variations with the same visual height,
// and smaller height field variations allow for greater stability
float constexpr SWEHeightFieldAmplification = 50.0f;

// The number of samples we raise with a state machine
int32_t constexpr SWEWaveStateMachinePerturbedSamplesCount = 3;

// The number of samples we set apart in the SWE buffers for wave generation at each end of a buffer
int32_t constexpr SWEWaveGenerationSamples = 1;

// The number of samples we set apart in the SWE buffers for boundary conditions at each end of a buffer
int32_t constexpr SWEBoundaryConditionsSamples = 3;

int32_t constexpr SWEOuterLayerSamples =
    SWEWaveGenerationSamples
    + SWEBoundaryConditionsSamples;

// The total number of samples in the SWE buffers
int32_t constexpr SWETotalSamples =
    SWEOuterLayerSamples
    + OceanSurface::SamplesCount
    + SWEOuterLayerSamples;

OceanSurface::OceanSurface(std::shared_ptr<GameEventDispatcher> gameEventDispatcher)
    : mGameEventHandler(std::move(gameEventDispatcher))
    , mSamples(new Sample[SamplesCount + 1])
    ////////
    , mBasalWaveAmplitude1(0.0f)
    , mBasalWaveAmplitude2(0.0f)
    , mBasalWaveNumber1(0.0f)
    , mBasalWaveNumber2(0.0f)
    , mBasalWaveAngularVelocity1(0.0f)
    , mBasalWaveAngularVelocity2(0.0f)
    , mBasalWaveSin1()
    , mNextTsunamiTimestamp(GameWallClock::duration::max())
    , mNextRogueWaveTimestamp(GameWallClock::duration::max())
    ////////
    , mWindBaseSpeedMagnitude(std::numeric_limits<float>::max())
    , mBasalWaveHeightAdjustment(std::numeric_limits<float>::max())
    , mBasalWaveLengthAdjustment(std::numeric_limits<float>::max())
    , mBasalWaveSpeedAdjustment(std::numeric_limits<float>::max())
    , mTsunamiRate(std::numeric_limits<float>::max())
    , mRogueWaveRate(std::numeric_limits<float>::max())
    ////////
    , mHeightField(new float[SWETotalSamples + 1]) // One extra cell just to ease interpolations
    , mVelocityField(new float[SWETotalSamples + 1]) // One extra cell just to ease interpolations
    ////////
    , mSWEInteractiveWaveStateMachine()
    , mSWETsunamiWaveStateMachine()
    , mSWERogueWaveWaveStateMachine()
    , mLastTsunamiTimestamp(GameWallClock::GetInstance().Now())
    , mLastRogueWaveTimestamp(GameWallClock::GetInstance().Now())
{
    //
    // Initialize SWE layer
    // - Initialize *all* values - including extra unused sample
    //

    for (int32_t i = 0; i <= SWETotalSamples; ++i)
    {
        mHeightField[i] = SWEHeightFieldOffset;
        mVelocityField[i] = 0.0f;
    }

    //
    // Initialize constant sample values
    //

    mSamples[SamplesCount].SampleValuePlusOneMinusSampleValue = 0.0f;
}

void OceanSurface::Update(
    float currentSimulationTime,
    Wind const & wind,
    GameParameters const & gameParameters)
{
    auto const now = GameWallClock::GetInstance().Now();

    //
    // Check whether parameters have changed
    //

    if (mWindBaseSpeedMagnitude != wind.GetBaseSpeedMagnitude()
        || mBasalWaveHeightAdjustment != gameParameters.BasalWaveHeightAdjustment
        || mBasalWaveLengthAdjustment != gameParameters.BasalWaveLengthAdjustment
        || mBasalWaveSpeedAdjustment != gameParameters.BasalWaveSpeedAdjustment
        || mTsunamiRate != gameParameters.TsunamiRate
        || mRogueWaveRate != gameParameters.RogueWaveRate)
    {
        RecalculateCoefficients(wind, gameParameters);
    }


    //
    // 1. Advance SWE Wave State Machines
    //

    // Interactive
    if (!!mSWEInteractiveWaveStateMachine)
    {
        auto heightValue = mSWEInteractiveWaveStateMachine->Update(currentSimulationTime);
        if (!heightValue)
        {
            // Done
            mSWEInteractiveWaveStateMachine.reset();
        }
        else
        {
            // Apply
            SetSWEWaveHeight(
                mSWEInteractiveWaveStateMachine->GetCenterIndex(),
                *heightValue);
        }
    }

    // Tsunami
    if (!!mSWETsunamiWaveStateMachine)
    {
        auto heightValue = mSWETsunamiWaveStateMachine->Update(currentSimulationTime);
        if (!heightValue)
        {
            // Done
            mSWETsunamiWaveStateMachine.reset();
        }
        else
        {
            // Apply
            SetSWEWaveHeight(
                mSWETsunamiWaveStateMachine->GetCenterIndex(),
                *heightValue);
        }
    }
    else
    {
        //
        // See if it's time to generate a tsunami
        //

        if (now > mNextTsunamiTimestamp)
        {
            // Tsunami!
            TriggerTsunami(currentSimulationTime);

            mLastTsunamiTimestamp = now;

            // Reset automatically-generated tsunamis
            mNextTsunamiTimestamp = CalculateNextAbnormalWaveTimestamp(
                now,
                gameParameters.TsunamiRate * 60.0f);
        }
    }

    // Rogue Wave
    if (!!mSWERogueWaveWaveStateMachine)
    {
        auto heightValue = mSWERogueWaveWaveStateMachine->Update(currentSimulationTime);
        if (!heightValue)
        {
            // Done
            mSWERogueWaveWaveStateMachine.reset();
        }
        else
        {
            // Apply
            SetSWEWaveHeight(
                mSWERogueWaveWaveStateMachine->GetCenterIndex(),
                *heightValue);
        }
    }
    else
    {
        //
        // See if it's time to generate a rogue wave
        //

        if (now > mNextRogueWaveTimestamp)
        {
            // RogueWave!
            TriggerRogueWave(currentSimulationTime, wind);

            mLastRogueWaveTimestamp = now;

            // Reset automatically-generated rogue waves
            mNextRogueWaveTimestamp = CalculateNextAbnormalWaveTimestamp(
                now,
                gameParameters.RogueWaveRate * 60.0f);
        }
    }


    //
    // 2. SWE Update
    //

    ApplyDampingBoundaryConditions();

    UpdateFields();

    ////// Calc avg height among all samples
    ////float avgHeight = 0.0f;
    ////for (int32_t i = SWEOuterLayerSamples; i < SWEOuterLayerSamples + SamplesCount; ++i)
    ////{
    ////    avgHeight += mHeightField[i];
    ////}
    ////avgHeight /= static_cast<float>(SamplesCount);
    ////LogMessage("AVG:", avgHeight);


    //
    // 3. Generate samples
    //

    GenerateSamples(
        currentSimulationTime,
        wind,
        gameParameters);
}

void OceanSurface::Upload(
    GameParameters const & gameParameters,
    Render::RenderContext & renderContext) const
{
    //
    // We want to upload at most RenderSlices slices
    //

    // Find index of leftmost sample, and its corresponding world X
    auto sampleIndex = FastTruncateInt64((renderContext.GetVisibleWorldLeft() + GameParameters::HalfMaxWorldWidth) / Dx);
    float sampleIndexX = -GameParameters::HalfMaxWorldWidth + (Dx * sampleIndex);

    // Calculate number of samples required to cover screen from leftmost sample
    // up to the visible world right (included)
    float const coverageWidth = renderContext.GetVisibleWorldRight() - sampleIndexX;
    auto const numberOfSamplesToRender = static_cast<int64_t>(ceil(coverageWidth / Dx));

    if (numberOfSamplesToRender >= RenderSlices<int64_t>)
    {
        //
        // Have to take more than 1 sample per slice
        //

        renderContext.UploadOceanStart(RenderSlices<int>);

        // Calculate dx between each pair of slices with want to upload
        float const sliceDx = coverageWidth / RenderSlices<float>;

        // We do one extra iteration as the number of slices is the number of quads, and the last vertical
        // quad side must be at the end of the width
        for (int64_t s = 0; s <= RenderSlices<int64_t>; ++s, sampleIndexX += sliceDx)
        {
            renderContext.UploadOcean(
                sampleIndexX,
                GetHeightAt(sampleIndexX),
                gameParameters.SeaDepth);
        }
    }
    else
    {
        //
        // We just upload the required number of samples, which is less than
        // the max number of slices we're prepared to upload, and we let OpenGL
        // interpolate on our behalf
        //

        renderContext.UploadOceanStart(numberOfSamplesToRender);

        // We do one extra iteration as the number of slices is the number of quads, and the last vertical
        // quad side must be at the end of the width
        for (std::int64_t s = 0; s <= numberOfSamplesToRender; ++s, sampleIndexX += Dx)
        {
            renderContext.UploadOcean(
                sampleIndexX,
                mSamples[s + sampleIndex].SampleValue,
                gameParameters.SeaDepth);
        }
    }

    renderContext.UploadOceanEnd();
}

void OceanSurface::AdjustTo(
    std::optional<vec2f> const & worldCoordinates,
    float currentSimulationTime)
{
    if (!!worldCoordinates)
    {
        // Calculate target height
        float constexpr MaxRelativeHeight = 4.0f; // Carefully selected; 4.5 makes waves unstable (velocities oscillating around 0.5 and diverging) after a while
        float constexpr MinRelativeHeight = -2.0f;
        float targetHeight =
            std::max(MinRelativeHeight, std::min(MaxRelativeHeight, (worldCoordinates->y / SWEHeightFieldAmplification)))
            + SWEHeightFieldOffset;

        // Check whether we are already advancing an interactive wave
        if (!mSWEInteractiveWaveStateMachine)
        {
            //
            // Start advancing a new interactive wave
            //

            auto const sampleIndex = ToSampleIndex(worldCoordinates->x);

            // Start wave
            mSWEInteractiveWaveStateMachine.emplace(
                SWEOuterLayerSamples + sampleIndex,
                mHeightField[SWEOuterLayerSamples + sampleIndex],   // LowHeight == current height
                targetHeight,                                       // HighHeight == target
                currentSimulationTime);
        }
        else
        {
            //
            // Restart currently-advancing interactive wave
            //

            mSWEInteractiveWaveStateMachine->Restart(
                targetHeight,
                currentSimulationTime);
        }
    }
    else
    {
        //
        // Start release of currently-advancing interactive wave
        //

        assert(!!mSWEInteractiveWaveStateMachine);
        mSWEInteractiveWaveStateMachine->Release(currentSimulationTime);
    }
}

void OceanSurface::TriggerTsunami(float currentSimulationTime)
{
    // Choose X
    float const tsunamiWorldX = GameRandomEngine::GetInstance().GenerateRandomReal(
        -GameParameters::HalfMaxWorldWidth,
        GameParameters::HalfMaxWorldWidth);

    // Choose height (good: 5 at 50-50)
    float constexpr AverageTsunamiHeight = 250.0f / SWEHeightFieldAmplification;
    float const tsunamiHeight = GameRandomEngine::GetInstance().GenerateRandomReal(
        AverageTsunamiHeight * 0.96f,
        AverageTsunamiHeight * 1.04f)
        + SWEHeightFieldOffset;

    // Make it a sample index
    auto const sampleIndex = ToSampleIndex(tsunamiWorldX);

    // (Re-)start state machine
    mSWETsunamiWaveStateMachine.emplace(
        SWEOuterLayerSamples + sampleIndex,
        mHeightField[SWEOuterLayerSamples + sampleIndex],   // LowHeight == current height
        tsunamiHeight,                                      // HighHeight == tsunami height
        7.0f,
        5.0f,
        currentSimulationTime);

    // Fire tsunami event
    assert(!!mGameEventHandler);
    mGameEventHandler->OnTsunami(tsunamiWorldX);
}

void OceanSurface::TriggerRogueWave(
    float currentSimulationTime,
    Wind const & wind)
{
    // Choose locus
    int32_t centerIndex;
    if (wind.GetBaseSpeedMagnitude() >= 0.0f)
    {
        // Left locus
        centerIndex = SWEBoundaryConditionsSamples;
    }
    else
    {
        // Right locus
        centerIndex = SWEOuterLayerSamples + OceanSurface::SamplesCount;
    }

    // Choose height
    float constexpr MaxRogueWaveHeight = 50.0f / SWEHeightFieldAmplification;
    float const rogueWaveHeight = GameRandomEngine::GetInstance().GenerateRandomReal(
        MaxRogueWaveHeight * 0.35f,
        MaxRogueWaveHeight)
        + SWEHeightFieldOffset;

    // Choose rate
    float const rogueWaveDelay = GameRandomEngine::GetInstance().GenerateRandomReal(
        0.7f,
        2.0f);

    // (Re-)start state machine
    mSWERogueWaveWaveStateMachine.emplace(
        centerIndex,
        mHeightField[centerIndex],  // LowHeight == current height
        rogueWaveHeight,            // HighHeight == rogue wave height
        rogueWaveDelay, // Rise delay
        rogueWaveDelay, // Fall delay
        currentSimulationTime);
}

///////////////////////////////////////////////////////////////////////////////////////////////

void OceanSurface::SetSWEWaveHeight(
    int32_t centerIndex,
    float height)
{
    int32_t const firstSampleIndex = centerIndex - SWEWaveStateMachinePerturbedSamplesCount / 2;

    for (int32_t i = 0; i < SWEWaveStateMachinePerturbedSamplesCount; ++i)
    {
        int32_t idx = firstSampleIndex + i;
        if (idx >= SWEBoundaryConditionsSamples
            && idx < SWEOuterLayerSamples + SamplesCount + SWEWaveGenerationSamples)
        {
            mHeightField[idx] = height;
        }
    }
}

void OceanSurface::RecalculateCoefficients(
    Wind const & wind,
    GameParameters const & gameParameters)
{
    //
    // Basal waves
    //

    float baseWindSpeedMagnitude = abs(wind.GetBaseSpeedMagnitude()); // km/h
    if (baseWindSpeedMagnitude < 60)
        // y = 63.09401 - 63.09401*e^(-0.05025263*x)
        baseWindSpeedMagnitude = 63.09401f - 63.09401f * exp(-0.05025263f * baseWindSpeedMagnitude); // Dramatize

    float const baseWindSpeedSign = wind.GetBaseSpeedMagnitude() >= 0.0f ? 1.0f : -1.0f;

    // Amplitude
    // - Amplitude = f(WindSpeed, km/h), with f fitted over points from Full Developed Waves
    //   (H. V. Thurman, Introductory Oceanography, 1988)
    // y = 1.039702 - 0.08155357*x + 0.002481548*x^2

    float const basalWaveHeightBase = (baseWindSpeedMagnitude != 0.0f)
        ? 0.002481548f * (baseWindSpeedMagnitude * baseWindSpeedMagnitude)
          - 0.08155357f * baseWindSpeedMagnitude
          + 1.039702f
        : 0.0f;

    mBasalWaveAmplitude1 = basalWaveHeightBase / 2.0f * gameParameters.BasalWaveHeightAdjustment;
    mBasalWaveAmplitude2 = 0.75f * mBasalWaveAmplitude1;

    // Wavelength
    // - Wavelength = f(WaveHeight (adjusted), m), with f fitted over points from same table
    // y = -738512.1 + 738525.2*e^(+0.00001895026*x)

    float const basalWaveLengthBase =
        -738512.1f
        + 738525.2f * exp(0.00001895026f * (2.0f * mBasalWaveAmplitude1));

    float const basalWaveLength = basalWaveLengthBase * gameParameters.BasalWaveLengthAdjustment;

    assert(basalWaveLength != 0.0f);
    mBasalWaveNumber1 = baseWindSpeedSign * 2.0f * Pi<float> / basalWaveLength;
    mBasalWaveNumber2 = 0.66f * mBasalWaveNumber1;

    // Period
    // - Technically, period = sqrt(2 * Pi * L / g), however this doesn't fit the table, so:
    // - Period = f(WaveLength (adjusted), m), with f fitted over points from same table
    // y = 17.91851 - 15.52928*e^(-0.006572834*x)

    float const basalWavePeriodBase =
        17.91851f
        - 15.52928f * exp(-0.006572834f * basalWaveLength);

    assert(gameParameters.BasalWaveSpeedAdjustment != 0.0f);
    float const basalWavePeriod = basalWavePeriodBase / gameParameters.BasalWaveSpeedAdjustment;

    assert(basalWavePeriod != 0.0f);
    mBasalWaveAngularVelocity1 = 2.0f * Pi<float> / basalWavePeriod;
    mBasalWaveAngularVelocity2 = 0.75f * mBasalWaveAngularVelocity1;

    //
    // Pre-calculate basal wave sinusoid
    //
    // By pre-multiplying with the first basal wave's amplitude we may save
    // one multiplication
    //

    mBasalWaveSin1.Recalculate(
        [a = mBasalWaveAmplitude1](float x)
        {
            return a * sin(2.0f * Pi<float> * x);
        });


    //
    // Abnormal wave timestamps
    //

    if (gameParameters.TsunamiRate > 0.0f)
    {
        mNextTsunamiTimestamp = CalculateNextAbnormalWaveTimestamp(
            mLastTsunamiTimestamp,
            gameParameters.TsunamiRate * 60.0f);
    }
    else
    {
        mNextTsunamiTimestamp = GameWallClock::time_point::max();
    }

    if (gameParameters.RogueWaveRate > 0.0f)
    {
        mNextRogueWaveTimestamp = CalculateNextAbnormalWaveTimestamp(
            mLastRogueWaveTimestamp,
            gameParameters.RogueWaveRate * 60.0f);
    }
    else
    {
        mNextRogueWaveTimestamp = GameWallClock::time_point::max();
    }


    //
    // Store new parameter values that we are now current with
    //

    mWindBaseSpeedMagnitude = wind.GetBaseSpeedMagnitude();
    mBasalWaveHeightAdjustment = gameParameters.BasalWaveHeightAdjustment;
    mBasalWaveLengthAdjustment = gameParameters.BasalWaveLengthAdjustment;
    mBasalWaveSpeedAdjustment = gameParameters.BasalWaveSpeedAdjustment;
    mTsunamiRate = gameParameters.TsunamiRate;
    mRogueWaveRate = gameParameters.RogueWaveRate;
}

GameWallClock::time_point OceanSurface::CalculateNextAbnormalWaveTimestamp(
    GameWallClock::time_point lastTimestamp,
    float rateSeconds)
{
    return lastTimestamp
        + std::chrono::duration_cast<GameWallClock::duration>(
            std::chrono::duration<float>(
                60.0f // Grace period between tsunami waves
                + GameRandomEngine::GetInstance().GenerateExponentialReal(1.0f / rateSeconds)));
}

/* Note: in this implementation we let go of the field advections,
   as they dont's seem to improve the simulation in a visible way.

void OceanSurface::AdvectHeightField()
{
    //
    // Semi-Lagrangian method
    //

    // Process all height samples, except for boundary condition samples
    for (int32_t i = SWEBoundaryConditionsSamples; i < SWETotalSamples - SWEBoundaryConditionsSamples; ++i)
    {
        // The height field values are at the center of the cell,
        // while velocities are at the edges - hence we need to take
        // the two neighboring velocities
        float const v = (mCurrentVelocityField[i] + mCurrentVelocityField[i + 1]) / 2.0f;

        // Calculate the (fractional) index that this height sample had one time step ago
        float const prevCellIndex =
            static_cast<float>(i)
            - v * GameParameters::SimulationStepTimeDuration<float> / Dx;

        // Transform index to ease interpolations, constraining the cell
        // to our grid at the same time
        float const prevCellIndex2 = std::min(
            std::max(0.0f, prevCellIndex),
            static_cast<float>(SWETotalSamples - 1));

        // Calculate integral and fractional parts of the index
        int32_t const prevCellIndexI = FastTruncateInt32(prevCellIndex2);
        float const prevCellIndexF = prevCellIndex2 - prevCellIndexI;
        assert(prevCellIndexF >= 0.0f && prevCellIndexF < 1.0f);

        // Set this height field sample as the previous (in time) sample,
        // interpolated between its two neighbors
        mNextHeightField[i] =
            (1.0f - prevCellIndexF) * mCurrentHeightField[prevCellIndexI]
            + prevCellIndexF * mCurrentHeightField[prevCellIndexI + 1];
    }
}

void OceanSurface::AdvectVelocityField()
{
    //
    // Semi-Lagrangian method
    //

    // Process all velocity samples, except for boundary condition samples
    //
    // Note: the last velocity sample is the one after the last height field sample
    for (int32_t i = SWEBoundaryConditionsSamples; i <= SWETotalSamples - SWEBoundaryConditionsSamples; ++i)
    {
        // Velocity values are at the edges of the cell
        float const v = mCurrentVelocityField[i];

        // Calculate the (fractional) index that this velocity sample had one time step ago
        float const prevCellIndex =
            static_cast<float>(i)
            - v * GameParameters::SimulationStepTimeDuration<float> / Dx;

        // Transform index to ease interpolations, constraining the cell
        // to our grid at the same time
        float const prevCellIndex2 = std::min(
            std::max(0.0f, prevCellIndex),
            static_cast<float>(SWETotalSamples - 1));

        // Calculate integral and fractional parts of the index
        int32_t const prevCellIndexI = FastTruncateInt32(prevCellIndex2);
        float const prevCellIndexF = prevCellIndex2 - prevCellIndexI;
        assert(prevCellIndexF >= 0.0f && prevCellIndexF < 1.0f);

        // Set this velocity field sample as the previous (in time) sample,
        // interpolated between its two neighbors
        mNextVelocityField[i] =
            (1.0f - prevCellIndexF) * mCurrentVelocityField[prevCellIndexI]
            + prevCellIndexF * mCurrentVelocityField[prevCellIndexI + 1];
    }
}
*/

void OceanSurface::ApplyDampingBoundaryConditions()
{
    for (size_t i = 0; i < SWEBoundaryConditionsSamples; ++i)
    {
        float const damping = static_cast<float>(i) / static_cast<float>(SWEBoundaryConditionsSamples);

        mHeightField[i] =
            (mHeightField[i] - SWEHeightFieldOffset) * damping
            + SWEHeightFieldOffset;

        mVelocityField[i] *= damping;

        mHeightField[SWEOuterLayerSamples + SamplesCount + SWEOuterLayerSamples - 1 - i] =
            (mHeightField[SWEOuterLayerSamples + SamplesCount + SWEOuterLayerSamples - 1 - i] - SWEHeightFieldOffset) * damping
            + SWEHeightFieldOffset;

        // For symmetry we actually damp the v-sample after this height field sample
        mVelocityField[SWEOuterLayerSamples + SamplesCount + SWEOuterLayerSamples - 1 - i + 1] *= damping;
    }

}

void OceanSurface::UpdateFields()
{
    // Height field  : from 0 to SWETotalSamples
    // Velocity field: from 1 to SWETotalSamples

    // We will divide deltaField by Dx (spatial derivatives) and
    // then multiply by dt (because we are integrating over time)
    float constexpr FactorH = GameParameters::SimulationStepTimeDuration<float> / Dx;
    float constexpr FactorV = FactorH * GameParameters::GravityMagnitude;

    mHeightField[0] -=
        mHeightField[0]
        * (mVelocityField[0 + 1] - mVelocityField[0])
        * FactorH;

    for (int32_t i = 1; i < SWETotalSamples; ++i)
    {
        mHeightField[i] -=
            mHeightField[i]
            * (mVelocityField[i + 1] - mVelocityField[i])
            * FactorH;

        mVelocityField[i] +=
            (mHeightField[i - 1] - mHeightField[i])
            * FactorV;
    }
}

void OceanSurface::GenerateSamples(
    float currentSimulationTime,
    Wind const & wind,
    GameParameters const & /*gameParameters*/)
{
    //
    // Sample values are a combination of:
    //  - SWE's height field
    //  - Basal waves
    //  - Wind gust ripples
    //

    // Secondary basal component
    float const secondaryBasalComponentPhase = Pi<float> * sin(currentSimulationTime);

    //
    // Wind gust ripples
    //

    static float constexpr WindRippleWaveNumber = 0.5f;

    float const windSpeedAbsoluteMagnitude = wind.GetCurrentWindSpeed().length();
    float const windSpeedGustRelativeAmplitude = wind.GetMaxSpeedMagnitude() - wind.GetBaseSpeedMagnitude();
    float const rawWindNormalizedIncisiveness = (windSpeedGustRelativeAmplitude == 0.0f)
        ? 0.0f
        : std::max(0.0f, windSpeedAbsoluteMagnitude - abs(wind.GetBaseSpeedMagnitude()))
        / abs(windSpeedGustRelativeAmplitude);

    float const windRipplesAngularVelocity = (wind.GetBaseSpeedMagnitude() >= 0)
        ? 128.0f
        : -128.0f;

    float const smoothedWindNormalizedIncisiveness = mWindIncisivenessRunningAverage.Update(rawWindNormalizedIncisiveness);
    float const windRipplesWaveHeight = 0.7f * smoothedWindNormalizedIncisiveness;


    //
    // Generate samples
    //

    float const x = -GameParameters::HalfMaxWorldWidth;

    float const basalWave2AmplitudeCoeff =
        (mBasalWaveAmplitude1 != 0.0f)
        ? mBasalWaveAmplitude2 / mBasalWaveAmplitude1
        : 0.0f;

    float const rippleWaveAmplitudeCoeff =
        (mBasalWaveAmplitude1 != 0.0f)
        ? windRipplesWaveHeight / mBasalWaveAmplitude1
        : 0.0f;

    float sinArg1 = (mBasalWaveNumber1 * x - mBasalWaveAngularVelocity1 * currentSimulationTime) / (2 * Pi<float>);
    float sinArg2 = (mBasalWaveNumber2 * x - mBasalWaveAngularVelocity2 * currentSimulationTime + secondaryBasalComponentPhase) / (2 * Pi<float>);
    float sinArgRipple = (WindRippleWaveNumber * x - windRipplesAngularVelocity * currentSimulationTime) / (2 * Pi<float>);

    // sample index = 0
    float previousSampleValue;
    {
        float const sweValue =
            (mHeightField[SWEOuterLayerSamples + 0] - SWEHeightFieldOffset)
            * SWEHeightFieldAmplification;

        float const basalValue1 =
            mBasalWaveSin1.GetLinearlyInterpolatedPeriodic(sinArg1);

        float const basalValue2 =
            basalWave2AmplitudeCoeff
            * mBasalWaveSin1.GetLinearlyInterpolatedPeriodic(sinArg2);

        float const rippleValue =
            rippleWaveAmplitudeCoeff
            * mBasalWaveSin1.GetLinearlyInterpolatedPeriodic(sinArgRipple);

        previousSampleValue =
            sweValue
            + basalValue1
            + basalValue2
            + rippleValue;

        mSamples[0].SampleValue = previousSampleValue;
    }

    float const sinArg1Dx = mBasalWaveNumber1 * Dx / (2 * Pi<float>);
    float const sinArg2Dx = mBasalWaveNumber2 * Dx / (2 * Pi<float>);
    float const sinArgRippleDx = WindRippleWaveNumber * Dx / (2 * Pi<float>);

    // sample index = 1...SamplesCount - 1
    for (int64_t i = 1; i < SamplesCount; ++i)
    {
        float const sweValue =
            (mHeightField[SWEOuterLayerSamples + i] - SWEHeightFieldOffset)
            * SWEHeightFieldAmplification;

        sinArg1 += sinArg1Dx;
        float const basalValue1 =
            mBasalWaveSin1.GetLinearlyInterpolatedPeriodic(sinArg1);

        sinArg2 += sinArg2Dx;
        float const basalValue2 =
            basalWave2AmplitudeCoeff
            * mBasalWaveSin1.GetLinearlyInterpolatedPeriodic(sinArg2);

        sinArgRipple += sinArgRippleDx;
        float const rippleValue =
            rippleWaveAmplitudeCoeff
            * mBasalWaveSin1.GetLinearlyInterpolatedPeriodic(sinArgRipple);

        float const sampleValue =
            sweValue
            + basalValue1
            + basalValue2
            + rippleValue;

        mSamples[i].SampleValue = sampleValue;
        mSamples[i - 1].SampleValuePlusOneMinusSampleValue = sampleValue - previousSampleValue;

        previousSampleValue = sampleValue;
    }

    // Populate last delta (extra sample will have same value as this sample)
    mSamples[SamplesCount - 1].SampleValuePlusOneMinusSampleValue = 0.0f;

    // Populate extra sample - same value as last sample
    assert(previousSampleValue == mSamples[SamplesCount - 1].SampleValue);
    mSamples[SamplesCount].SampleValue = previousSampleValue;

    assert(mSamples[SamplesCount].SampleValuePlusOneMinusSampleValue == 0.0f);
}

///////////////////////////////////////////////////////////////////////////////////////////

OceanSurface::SWEInteractiveWaveStateMachine::SWEInteractiveWaveStateMachine(
    int32_t centerIndex,
    float startHeight,
    float targetHeight,
    float currentSimulationTime)
    : mCenterIndex(centerIndex)
    , mLowHeight(startHeight)
    , mCurrentPhaseStartHeight(startHeight)
    , mCurrentPhaseTargetHeight(targetHeight)
    , mCurrentHeight(startHeight)
    , mCurrentProgress(0.0f)
    , mStartSimulationTime(currentSimulationTime)
    , mCurrentWavePhase(WavePhaseType::Rise)
    , mCurrentSmoothingDelay(CalculateSmoothingDelay())
{}

void OceanSurface::SWEInteractiveWaveStateMachine::Restart(
    float restartHeight,
    float currentSimulationTime)
{
    // Rise in any case, and our new target is the restart height
    mCurrentPhaseStartHeight = mCurrentHeight;
    mCurrentPhaseTargetHeight = restartHeight;
    mCurrentProgress = 0.0f;
    mStartSimulationTime = currentSimulationTime;
    mCurrentWavePhase = WavePhaseType::Rise;

    // Recalculate delay
    mCurrentSmoothingDelay = CalculateSmoothingDelay();
}

void OceanSurface::SWEInteractiveWaveStateMachine::Release(float currentSimulationTime)
{
    assert(mCurrentWavePhase == WavePhaseType::Rise);

    // Start falling
    mCurrentPhaseStartHeight = mCurrentHeight;
    mCurrentPhaseTargetHeight = mLowHeight;
    mCurrentProgress = 0.0f;
    mStartSimulationTime = currentSimulationTime;
    mCurrentWavePhase = WavePhaseType::Fall;
    mCurrentSmoothingDelay = CalculateSmoothingDelay();
}

std::optional<float> OceanSurface::SWEInteractiveWaveStateMachine::Update(
    float currentSimulationTime)
{
    // Advance iff we are not done yet
    if (mCurrentProgress < 1.0f)
    {
        mCurrentProgress =
            (currentSimulationTime - mStartSimulationTime)
            / mCurrentSmoothingDelay;
    }

    // Calculate sinusoidal progress
    float const sinProgress = sin(Pi<float> / 2.0f * std::min(mCurrentProgress, 1.0f));

    // Calculate new height value
    mCurrentHeight =
        mCurrentPhaseStartHeight + (mCurrentPhaseTargetHeight - mCurrentPhaseStartHeight) * sinProgress;

    // Check whether it's time to shut down
    if (mCurrentProgress >= 1.0f
        && WavePhaseType::Fall == mCurrentWavePhase)
    {
        // We're done
        return std::nullopt;
    }

    return mCurrentHeight;
}

float OceanSurface::SWEInteractiveWaveStateMachine::CalculateSmoothingDelay()
{
    float const deltaH = std::min(
        abs(mCurrentPhaseTargetHeight - mCurrentHeight),
        SWEHeightFieldOffset / 5.0f);

    float delayTicks;
    if (mCurrentWavePhase == WavePhaseType::Rise
        || mCurrentPhaseStartHeight < mCurrentPhaseTargetHeight) // If falling up, we want a slower fall
    {
        //
        // Number of ticks must fit:
        //  DeltaH=0.0  => Ticks=0.0
        //  DeltaH=0.2  => Ticks=8.0
        //  DeltaH=2.0  => Ticks=150.0
        //  DeltaH=4.0  => Ticks=200.0
        //  DeltaH>4.0  => Ticks~=200.0
        // y = -19.88881 - (-147.403/0.6126081)*(1 - e^(-0.6126081*x))
        delayTicks =
            -19.88881f
            + (147.403f / 0.6126081f) * (1.0f - exp(-0.6126081f * deltaH));
    }
    else
    {
        //
        // Number of ticks must fit:
        //  DeltaH=0.1  => Ticks=2.0
        //  DeltaH=0.25 => Ticks=3.0
        //  DeltaH=1.0  => Ticks=7.0
        //  DeltaH=2.0  => Ticks=10.0
        // y = 1.220013 - (-7.8394/0.6485749)*(1 - e^(-0.6485749*x))
        delayTicks =
            1.220013f
            + (7.8394f / 0.6485749f) * (1.0f - exp(-0.6485749f * deltaH));
    }

    float const delay =
        std::max(delayTicks, 1.0f)
        * GameParameters::SimulationStepTimeDuration<float>;

    return delay;
}

///////////////////////////////////////////////////////////////////////////////////////////

OceanSurface::SWEAbnormalWaveStateMachine::SWEAbnormalWaveStateMachine(
    int32_t centerIndex,
    float lowHeight,
    float highHeight,
    float riseDelay, // sec
    float fallDelay, // sec
    float currentSimulationTime)
    : mCenterIndex(centerIndex)
    , mLowHeight(lowHeight)
    , mHighHeight(highHeight)
    , mRiseDelay(riseDelay)
    , mFallDelay(fallDelay)
    , mCurrentProgress(0.0f)
    , mCurrentPhaseStartSimulationTime(currentSimulationTime)
    , mCurrentPhaseDelay(riseDelay)
    , mCurrentWavePhase(WavePhaseType::Rise)
{}

std::optional<float> OceanSurface::SWEAbnormalWaveStateMachine::Update(
    float currentSimulationTime)
{
    // Advance
    mCurrentProgress =
        (currentSimulationTime - mCurrentPhaseStartSimulationTime)
        / mCurrentPhaseDelay;

    // Calculate sinusoidal progress
    float const sinProgress = sin(Pi<float> / 2.0f * std::min(mCurrentProgress, 1.0f));

    // Calculate new height value
    float currentHeight =
        (WavePhaseType::Rise == mCurrentWavePhase)
        ? mLowHeight + (mHighHeight - mLowHeight) * sinProgress
        : mHighHeight - (mHighHeight - mLowHeight) * sinProgress;

    // Check whether it's time to switch phase
    if (mCurrentProgress >= 1.0f)
    {
        if (mCurrentWavePhase == WavePhaseType::Rise)
        {
            // Start falling
            mCurrentProgress = 0.0f;
            mCurrentPhaseStartSimulationTime = currentSimulationTime;
            mCurrentPhaseDelay = mFallDelay;
            mCurrentWavePhase = WavePhaseType::Fall;
        }
        else
        {
            // We're done
            return std::nullopt;
        }
    }

    return currentHeight;
}

}