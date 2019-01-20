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
    , mZeroMagnitude(0.0f)
    , mBaseMagnitude(0.0f)
    , mPreMaxMagnitude(0.0f)
    , mMaxMagnitude(0.0f)
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
    , mCurrentRawWindForceMagnitude(0.0f)
    , mCurrentWindForceMagnitudeRunningAverage()
    , mCurrentWindForce(vec2f::zero())
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
        mCurrentRawWindForceMagnitude = mBaseMagnitude;
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
                mCurrentWindForceMagnitudeRunningAverage.Fill(mBaseMagnitude);

                [[fallthrough]];
            }

            case State::EnterBase1:
            {
                // Transition
                mCurrentState = State::Base1;
                mCurrentRawWindForceMagnitude = mBaseMagnitude;
                mNextStateTransitionTimestamp = now + ChooseDuration(10.0, 20.0f);

                [[fallthrough]];
            }

            case State::Base1:
            {
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
                mCurrentRawWindForceMagnitude = mPreMaxMagnitude;
                mNextStateTransitionTimestamp = now + ChooseDuration(5.0, 10.0f);

                [[fallthrough]];
            }

            case State::PreGusting:
            {
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
                mCurrentRawWindForceMagnitude = mPreMaxMagnitude;
                mNextStateTransitionTimestamp = now + ChooseDuration(10.0, 20.0f);

                // Schedule next poisson sampling
                mNextPoissonSampleTimestamp =
                    now
                    + std::chrono::duration_cast<GameWallClock::duration>(std::chrono::duration<float>(PoissonSampleDeltaT));

                [[fallthrough]];
            }

            case State::Gusting:
            {
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
                        // Draw random number
                        float const sample = GameRandomEngine::GetInstance().GenerateRandomNormalizedReal();

                        // Check if we should gust
                        if (sample < mGustCdf)
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
                mCurrentRawWindForceMagnitude = mMaxMagnitude;
                mCurrentGustTransitionTimestamp = now + ChooseDuration(0.5f, 1.0f);

                [[fallthrough]];
            }

            case State::Gust:
            {
                // Check if it's time to transition
                if (now > mCurrentGustTransitionTimestamp)
                {
                    // Transition back
                    mCurrentState = State::Gusting;
                    mCurrentRawWindForceMagnitude = mPreMaxMagnitude;

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
                mCurrentRawWindForceMagnitude = mPreMaxMagnitude;
                mNextStateTransitionTimestamp = now + ChooseDuration(5.0, 10.0f);

                [[fallthrough]];
            }

            case State::PostGusting:
            {
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
                mCurrentRawWindForceMagnitude = mBaseMagnitude;
                mNextStateTransitionTimestamp = now + ChooseDuration(3.0, 10.0f);

                [[fallthrough]];
            }

            case State::Base2:
            {
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
                mCurrentRawWindForceMagnitude = mZeroMagnitude;
                mNextStateTransitionTimestamp = now + ChooseDuration(5.0, 15.0f);

                [[fallthrough]];
            }

            case State::Zero:
            {
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

    // Update average and store current force
    mCurrentWindForce =
        GameParameters::WindDirection
        * mCurrentWindForceMagnitudeRunningAverage.Update(mCurrentRawWindForceMagnitude);

    // Publish interesting quantities for probes
    mGameEventHandler->OnWindForceUpdated(
        mZeroMagnitude,
        mBaseMagnitude,
        mPreMaxMagnitude,
        mMaxMagnitude,
        mCurrentWindForce);
}

GameWallClock::duration Wind::ChooseDuration(float minSeconds, float maxSeconds)
{
    float chosenSeconds = GameRandomEngine::GetInstance().GenerateRandomReal(minSeconds, maxSeconds);
    return std::chrono::duration_cast<GameWallClock::duration>(std::chrono::duration<float>(chosenSeconds));
}

void Wind::RecalculateParameters(GameParameters const & gameParameters)
{
    mZeroMagnitude = 0.0f;
    mBaseMagnitude = gameParameters.WindSpeedBase;
    mMaxMagnitude = gameParameters.WindSpeedBase * gameParameters.WindSpeedMaxFactor;
    mPreMaxMagnitude = mBaseMagnitude + (mMaxMagnitude - mBaseMagnitude) / 8.0f;

    mGustCdf = 1.0f - exp(-GustLambda / (PoissonSampleRate * gameParameters.WindGustFrequencyAdjustment));
}

}