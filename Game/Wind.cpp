/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

#include <limits.h>

namespace Physics {

// The number of poisson samples we perform in a second
constexpr float PoissonSampleRate = 4.0f;
constexpr float PoissonSampleDeltaT = 1.0f / PoissonSampleRate;

// The event rates for transitions, in 1/second
constexpr float GustLambda = 1.0f / 1.0f;

Wind::Wind(std::shared_ptr<IGameEventHandler> gameEventHandler)
    : mGameEventHandler(std::move(gameEventHandler))
    // Pre-calculated parameters
    , mZeroSpeedMagnitude(0.0f)
    , mBaseSpeedMagnitude(0.0f)
    , mPreMaxSpeedMagnitude(0.0f)
    , mMaxSpeedMagnitude(0.0f)
    , mGustCdf(0.0f)
    , mCurrentDoModulateWindParameter(false)
    , mCurrentSpeedBaseParameter(std::numeric_limits<float>::lowest())
    , mCurrentSpeedMaxFactorParameter(std::numeric_limits<float>::lowest())
    , mCurrentGustFrequencyAdjustmentParameter(std::numeric_limits<float>::lowest())
    // State
    , mCurrentState(State::Initial)
    , mNextStateTransitionTimestamp()
    , mNextPoissonSampleTimestamp()
    , mCurrentGustTransitionTimestamp()
    , mCurrentRawWindSpeedMagnitude(0.0f)
    , mCurrentWindSpeedMagnitudeRunningAverage()
    , mCurrentWindSpeed(vec2f::zero())
{
}

void Wind::Update(GameParameters const & gameParameters)
{
    //
    // Check whether parameters have changed
    //

    if (gameParameters.DoModulateWind != mCurrentDoModulateWindParameter
        || gameParameters.WindSpeedBase != mCurrentSpeedBaseParameter
        || gameParameters.WindSpeedMaxFactor != mCurrentSpeedMaxFactorParameter
        || gameParameters.WindGustFrequencyAdjustment != mCurrentGustFrequencyAdjustmentParameter)
    {
        RecalculateParameters(gameParameters);

        mCurrentDoModulateWindParameter = gameParameters.DoModulateWind;
        mCurrentSpeedBaseParameter = gameParameters.WindSpeedBase;
        mCurrentSpeedMaxFactorParameter = gameParameters.WindSpeedMaxFactor;
        mCurrentGustFrequencyAdjustmentParameter = gameParameters.WindGustFrequencyAdjustment;
    }

    if (!mCurrentDoModulateWindParameter)
    {
        mCurrentRawWindSpeedMagnitude = mBaseSpeedMagnitude;
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
                mCurrentWindSpeedMagnitudeRunningAverage.Fill(mBaseSpeedMagnitude);

                [[fallthrough]];
            }

            case State::EnterBase1:
            {
                // Transition
                mCurrentState = State::Base1;
                mNextStateTransitionTimestamp = now + ChooseDuration(10.0, 20.0f);

                [[fallthrough]];
            }

            case State::Base1:
            {
                mCurrentRawWindSpeedMagnitude = mBaseSpeedMagnitude;

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
                mNextStateTransitionTimestamp = now + ChooseDuration(5.0, 10.0f);

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
                mNextStateTransitionTimestamp = now + ChooseDuration(10.0, 20.0f);

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
                        if (GameRandomEngine::GetInstance().GenerateRandomBoolean(mGustCdf))
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
                mNextStateTransitionTimestamp = now + ChooseDuration(5.0, 10.0f);

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
                mNextStateTransitionTimestamp = now + ChooseDuration(3.0, 10.0f);

                [[fallthrough]];
            }

            case State::Base2:
            {
                mCurrentRawWindSpeedMagnitude = mBaseSpeedMagnitude;

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
                mNextStateTransitionTimestamp = now + ChooseDuration(5.0, 15.0f);

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
        GameParameters::WindDirection
        * mCurrentWindSpeedMagnitudeRunningAverage.Update(mCurrentRawWindSpeedMagnitude);

    // Publish interesting quantities for probes
    mGameEventHandler->OnWindSpeedUpdated(
        mZeroSpeedMagnitude,
        mBaseSpeedMagnitude,
        mPreMaxSpeedMagnitude,
        mMaxSpeedMagnitude,
        mCurrentWindSpeed);
}

GameWallClock::duration Wind::ChooseDuration(float minSeconds, float maxSeconds)
{
    float chosenSeconds = GameRandomEngine::GetInstance().GenerateRandomReal(minSeconds, maxSeconds);
    return std::chrono::duration_cast<GameWallClock::duration>(std::chrono::duration<float>(chosenSeconds));
}

void Wind::RecalculateParameters(GameParameters const & gameParameters)
{
    mZeroSpeedMagnitude = 0.0f;
    mBaseSpeedMagnitude = gameParameters.WindSpeedBase;
    mMaxSpeedMagnitude = mBaseSpeedMagnitude * gameParameters.WindSpeedMaxFactor;
    mPreMaxSpeedMagnitude = mBaseSpeedMagnitude + (mMaxSpeedMagnitude - mBaseSpeedMagnitude) / 8.0f;

    mGustCdf = 1.0f - exp(-GustLambda / (PoissonSampleRate * gameParameters.WindGustFrequencyAdjustment));
}

}