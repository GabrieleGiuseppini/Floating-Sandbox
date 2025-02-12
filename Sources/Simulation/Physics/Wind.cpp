/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <Core/GameRandomEngine.h>

#include <limits>

namespace Physics {

// The number of gusts we want per second
float constexpr GustRate = 1.0f;

// The number of poisson samples we perform in a second
float constexpr PoissonSampleRate = 4.0f;
float constexpr PoissonSampleDeltaT = 1.0f / PoissonSampleRate;

Wind::Wind(SimulationEventDispatcher & simulationEventDispatcher)
    : mSimulationEventHandler(simulationEventDispatcher)
    // Pre-calculated parameters
    , mZeroSpeedMagnitude(0.0f)
    , mBaseSpeedMagnitude(0.0f)
    , mBaseAndStormSpeedMagnitude(0.0f)
    , mPreMaxSpeedMagnitude(0.0f)
    , mMaxSpeedMagnitude(0.0f)
    , mGustCdf(0.0f)
    , mCurrentDoModulateWindParameter(false)
    , mCurrentSpeedBaseParameter(std::numeric_limits<float>::lowest())
    , mCurrentSpeedMaxFactorParameter(std::numeric_limits<float>::lowest())
    , mCurrentGustFrequencyAdjustmentParameter(std::numeric_limits<float>::lowest())
    , mCurrentStormWindSpeedParameter(std::numeric_limits<float>::lowest())
    // Wind state machine
    , mCurrentState(State::Initial)
    , mNextStateTransitionTimestamp()
    , mNextPoissonSampleTimestamp()
    , mCurrentGustTransitionTimestamp()
    , mCurrentSilenceAmount(0.0f)
    , mCurrentRawWindSpeedMagnitude(0.0f)
    , mCurrentWindSpeedMagnitudeRunningAverage()
    , mCurrentWindSpeed(vec2f::zero())
    // Radial wind field
    , mCurrentRadialWindField()
{
}

void Wind::SetSilence(float silenceAmount)
{
    mCurrentSilenceAmount = silenceAmount;
}

void Wind::Update(
    Storm::Parameters const & stormParameters,
    SimulationParameters const & simulationParameters)
{
    //
    // Check whether parameters have changed
    //

    if (simulationParameters.DoModulateWind != mCurrentDoModulateWindParameter
        || simulationParameters.WindSpeedBase != mCurrentSpeedBaseParameter
        || simulationParameters.WindSpeedMaxFactor != mCurrentSpeedMaxFactorParameter
        || simulationParameters.WindGustFrequencyAdjustment != mCurrentGustFrequencyAdjustmentParameter
        || stormParameters.WindSpeed != mCurrentStormWindSpeedParameter)
    {
        RecalculateParameters(stormParameters, simulationParameters);

        mCurrentDoModulateWindParameter = simulationParameters.DoModulateWind;
        mCurrentSpeedBaseParameter = simulationParameters.WindSpeedBase;
        mCurrentSpeedMaxFactorParameter = simulationParameters.WindSpeedMaxFactor;
        mCurrentGustFrequencyAdjustmentParameter = simulationParameters.WindGustFrequencyAdjustment;
        mCurrentStormWindSpeedParameter = stormParameters.WindSpeed;
    }

    if (!mCurrentDoModulateWindParameter)
    {
        mCurrentRawWindSpeedMagnitude = mBaseAndStormSpeedMagnitude;
    }
    else
    {
        //
        // Run state machine
        //

        auto now = GameWallClock::GetInstance().Now();

        switch (mCurrentState)
        {
            case State::Initial:
            {
                mCurrentState = State::EnterBase1;
                mCurrentWindSpeedMagnitudeRunningAverage.Fill(mBaseAndStormSpeedMagnitude);

                [[fallthrough]];
            }

            case State::EnterBase1:
            {
                // Transition
                mCurrentState = State::Base1;
                mNextStateTransitionTimestamp = now + ChooseDuration(10.0f, 20.0f);

                [[fallthrough]];
            }

            case State::Base1:
            {
                mCurrentRawWindSpeedMagnitude = mBaseAndStormSpeedMagnitude;

                // Check if it's time to transition
                if (now > mNextStateTransitionTimestamp)
                {
                    // Transition
                    mCurrentState = State::EnterPreGusting;
                }

                break;
            }

            case State::EnterPreGusting:
            {
                // Transition
                mCurrentState = State::PreGusting;
                mNextStateTransitionTimestamp = now + ChooseDuration(5.0f, 10.0f);

                [[fallthrough]];
            }

            case State::PreGusting:
            {
                mCurrentRawWindSpeedMagnitude = mPreMaxSpeedMagnitude;

                // Check if it's time to transition
                if (now > mNextStateTransitionTimestamp)
                {
                    // Transition
                    mCurrentState = State::EnterGusting;
                }

                break;
            }

            case State::EnterGusting:
            {
                // Transition
                mCurrentState = State::Gusting;
                mNextStateTransitionTimestamp = now + ChooseDuration(10.0f, 20.0f);

                // Schedule next poisson sampling
                mNextPoissonSampleTimestamp =
                    now
                    + std::chrono::duration_cast<GameWallClock::duration>(std::chrono::duration<float>(PoissonSampleDeltaT));

                [[fallthrough]];
            }

            case State::Gusting:
            {
                mCurrentRawWindSpeedMagnitude = mPreMaxSpeedMagnitude;

                // Check if it's time to transition
                if (now > mNextStateTransitionTimestamp)
                {
                    // Transition
                    mCurrentState = State::EnterPostGusting;
                }
                else
                {
                    // Check if it's time to sample poisson
                    if (now >= mNextPoissonSampleTimestamp)
                    {
                        // Check if we should gust
                        if (GameRandomEngine::GetInstance().GenerateUniformBoolean(mGustCdf))
                        {
                            // Transition to EnterGust
                            mCurrentState = State::EnterGust;
                        }
                        else
                        {
                            // Schedule next poisson sampling
                            mNextPoissonSampleTimestamp =
                                now
                                + std::chrono::duration_cast<GameWallClock::duration>(std::chrono::duration<float>(PoissonSampleDeltaT));
                        }
                    }
                }

                break;
            }

            case State::EnterGust:
            {
                // Transition to Gust and choose gust duration
                mCurrentState = State::Gust;
                mCurrentGustTransitionTimestamp = now + ChooseDuration(0.5f, 1.0f);

                [[fallthrough]];
            }

            case State::Gust:
            {
                mCurrentRawWindSpeedMagnitude = mMaxSpeedMagnitude;

                // Check if it's time to transition
                if (now > mCurrentGustTransitionTimestamp)
                {
                    // Transition back
                    mCurrentState = State::Gusting;
                    mCurrentRawWindSpeedMagnitude = mPreMaxSpeedMagnitude;

                    // Schedule next poisson sampling
                    mNextPoissonSampleTimestamp =
                        now
                        + std::chrono::duration_cast<GameWallClock::duration>(std::chrono::duration<float>(PoissonSampleDeltaT));
                }

                break;
            }

            case State::EnterPostGusting:
            {
                // Transition
                mCurrentState = State::PostGusting;
                mNextStateTransitionTimestamp = now + ChooseDuration(5.0f, 10.0f);

                [[fallthrough]];
            }

            case State::PostGusting:
            {
                mCurrentRawWindSpeedMagnitude = mPreMaxSpeedMagnitude;

                // Check if it's time to transition
                if (now > mNextStateTransitionTimestamp)
                {
                    // Transition
                    mCurrentState = State::EnterBase2;
                }

                break;
            }

            case State::EnterBase2:
            {
                // Transition
                mCurrentState = State::Base2;
                mNextStateTransitionTimestamp = now + ChooseDuration(3.0f, 10.0f);

                [[fallthrough]];
            }

            case State::Base2:
            {
                mCurrentRawWindSpeedMagnitude = mBaseAndStormSpeedMagnitude;

                // Check if it's time to transition
                if (now > mNextStateTransitionTimestamp)
                {
                    // Transition
                    mCurrentState = State::EnterZero;
                }

                break;
            }

            case State::EnterZero:
            {
                // Transition
                mCurrentState = State::Zero;
                mNextStateTransitionTimestamp = now + ChooseDuration(5.0f, 15.0f);

                [[fallthrough]];
            }

            case State::Zero:
            {
                mCurrentRawWindSpeedMagnitude = mZeroSpeedMagnitude;

                // Check if it's time to transition
                if (now > mNextStateTransitionTimestamp)
                {
                    // Transition
                    mCurrentState = State::EnterBase1;
                }

                break;
            }
        }
    }

    // Update average and store current speed
    mCurrentWindSpeed =
        SimulationParameters::WindDirection
        * mCurrentWindSpeedMagnitudeRunningAverage.Update(
            mCurrentRawWindSpeedMagnitude
            * (1.0f - mCurrentSilenceAmount));

    // Publish interesting quantities for probes
    mSimulationEventHandler.OnWindSpeedUpdated(
        mZeroSpeedMagnitude,
        mBaseSpeedMagnitude,
        mBaseAndStormSpeedMagnitude,
        mPreMaxSpeedMagnitude,
        mMaxSpeedMagnitude,
        mCurrentWindSpeed);
}

void Wind::UpdateEnd()
{
    mCurrentRadialWindField.reset();
}

void Wind::Upload(RenderContext & renderContext) const
{
    renderContext.UploadWind(mCurrentWindSpeed);
}

GameWallClock::duration Wind::ChooseDuration(float minSeconds, float maxSeconds)
{
    float chosenSeconds = GameRandomEngine::GetInstance().GenerateUniformReal(minSeconds, maxSeconds);
    return std::chrono::duration_cast<GameWallClock::duration>(std::chrono::duration<float>(chosenSeconds));
}

void Wind::RecalculateParameters(
    Storm::Parameters const & stormParameters,
    SimulationParameters const & simulationParameters)
{
	mZeroSpeedMagnitude = (simulationParameters.WindSpeedBase >= 0.0f)
		? stormParameters.WindSpeed
		: -stormParameters.WindSpeed;
    mBaseSpeedMagnitude = simulationParameters.WindSpeedBase;
    mBaseAndStormSpeedMagnitude = (simulationParameters.WindSpeedBase >= 0.0f)
        ? mBaseSpeedMagnitude + stormParameters.WindSpeed
        : mBaseSpeedMagnitude - stormParameters.WindSpeed;
    mMaxSpeedMagnitude = mBaseAndStormSpeedMagnitude * simulationParameters.WindSpeedMaxFactor;
    mPreMaxSpeedMagnitude = mBaseAndStormSpeedMagnitude + (mMaxSpeedMagnitude - mBaseAndStormSpeedMagnitude) / 8.0f;

    // We want GustRate gusts every 1 seconds, and in 1 second we perform PoissonSampleRate samplings,
    // hence we want 1/PoissonSampleRate gusts per sample interval
    mGustCdf = 1.0f - exp(-GustRate / (PoissonSampleRate * simulationParameters.WindGustFrequencyAdjustment));
}

}