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
constexpr float DownFromBaseLambda = 1.0f / 6.0f;
constexpr float UpFromBaseLambda = 1.0f / 1.0f;
constexpr float UpFromPreMaxLambda = 1.0f / 1.0f;
constexpr float DownFromPreMaxLambda = 1.0f / 5.0f;
constexpr float UpFromZeroLambda = 1.0f / 1.7f;
constexpr float DownFromMaxLambda = 1.0f / 0.5f;

Wind::Wind(std::shared_ptr<IGameEventHandler> gameEventHandler)
    : mGameEventHandler(std::move(gameEventHandler))
    // Pre-calculated parameters
    , mZeroMagnitude(0.0f)
    , mBaseMagnitude(0.0f)
    , mPreMaxMagnitude(0.0f)
    , mMaxMagnitude(0.0f)
    , mDownFromBaseCdf(0.0f)
    , mUpFromBaseCdf(0.0f)
    , mUpFromPreMaxCdf(0.0f)
    , mDownFromPreMaxCdf(0.0f)
    , mUpFromZeroCdf(0.0f)
    , mDownFromMaxCdf(0.0f)
    , mCurrentSpeedBaseParameter(std::numeric_limits<float>::lowest())
    , mCurrentSpeedMaxFactorParameter(std::numeric_limits<float>::lowest())
    , mCurrentGustFrequencyAdjustmentParameter(std::numeric_limits<float>::lowest())
    // State
    , mCurrentState(State::Initial)
    , mNextPoissonSampleSimulationTime()
    , mCurrentRawWindForceMagnitude(0.0f)
    , mCurrentWindForceMagnitudeRunningAverage()
    , mCurrentWindForce(vec2f::zero())
{
}

void Wind::Update(
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    // Check whether parameters have changed
    if (gameParameters.WindSpeedBase != mCurrentSpeedBaseParameter
        || gameParameters.WindSpeedMaxFactor != mCurrentSpeedMaxFactorParameter
        || gameParameters.WindGustFrequencyAdjustment != mCurrentGustFrequencyAdjustmentParameter)
    {
        RecalculateParameters(gameParameters);

        mCurrentSpeedBaseParameter = gameParameters.WindSpeedBase;
        mCurrentSpeedMaxFactorParameter = gameParameters.WindSpeedMaxFactor;
        mCurrentGustFrequencyAdjustmentParameter = gameParameters.WindGustFrequencyAdjustment;
    }

    switch (mCurrentState)
    {
        case State::Initial:
        {
            // Transition to base
            mCurrentState = State::Base;
            mCurrentRawWindForceMagnitude = mBaseMagnitude;

            // Schedule next poisson sampling
            mNextPoissonSampleSimulationTime = currentSimulationTime + PoissonSampleDeltaT;

            break;
        }

        case State::Max:
        {
            // Check if it's time to sample poisson
            if (currentSimulationTime >= mNextPoissonSampleSimulationTime)
            {
                // Draw random number
                float const sample = GameRandomEngine::GetInstance().GenerateRandomNormalizedReal();

                // Check if we can transition
                if (sample < mDownFromMaxCdf)
                {
                    // Transition to PreMax
                    mCurrentState = State::PreMax;
                    mCurrentRawWindForceMagnitude = mPreMaxMagnitude;
                }

                // Schedule next poisson sampling
                mNextPoissonSampleSimulationTime = currentSimulationTime + PoissonSampleDeltaT;
            }

            break;
        }

        case State::PreMax:
        {
            // Check if it's time to sample poisson
            if (currentSimulationTime >= mNextPoissonSampleSimulationTime)
            {
                // Draw random number
                float const sample = GameRandomEngine::GetInstance().GenerateRandomNormalizedReal();

                // Check if we can transition
                assert(mDownFromPreMaxCdf < mUpFromPreMaxCdf);
                if (sample < mDownFromPreMaxCdf)
                {
                    // Transition to Base
                    mCurrentState = State::Base;
                    mCurrentRawWindForceMagnitude = mBaseMagnitude;
                }
                else if (sample < mUpFromPreMaxCdf)
                {
                    // Transition to Max
                    mCurrentState = State::Max;
                    mCurrentRawWindForceMagnitude = mMaxMagnitude;
                }

                // Schedule next poisson sampling
                mNextPoissonSampleSimulationTime = currentSimulationTime + PoissonSampleDeltaT;
            }

            break;
        }

        case State::Base:
        {
            // Check if it's time to sample poisson
            if (currentSimulationTime >= mNextPoissonSampleSimulationTime)
            {
                // Draw random number
                float const sample = GameRandomEngine::GetInstance().GenerateRandomNormalizedReal();

                // Check if we can transition
                assert(mDownFromBaseCdf < mUpFromBaseCdf);
                if (sample < mDownFromBaseCdf)
                {
                    // Transition to Zero
                    mCurrentState = State::Zero;
                    mCurrentRawWindForceMagnitude = mZeroMagnitude;
                }
                else if (sample < mUpFromBaseCdf)
                {
                    // Transition to PreMax
                    mCurrentState = State::PreMax;
                    mCurrentRawWindForceMagnitude = mPreMaxMagnitude;
                }

                // Schedule next poisson sampling
                mNextPoissonSampleSimulationTime = currentSimulationTime + PoissonSampleDeltaT;
            }

            break;
        }

        case State::Zero:
        {
            // Check if it's time to sample poisson
            if (currentSimulationTime >= mNextPoissonSampleSimulationTime)
            {
                // Draw random number
                float const sample = GameRandomEngine::GetInstance().GenerateRandomNormalizedReal();

                // Check if we can transition
                if (sample < mUpFromZeroCdf)
                {
                    // Transition to Base
                    mCurrentState = State::Base;
                    mCurrentRawWindForceMagnitude = mBaseMagnitude;
                }

                // Schedule next poisson sampling
                mNextPoissonSampleSimulationTime = currentSimulationTime + PoissonSampleDeltaT;
            }

            break;
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

void Wind::RecalculateParameters(GameParameters const & gameParameters)
{
    mZeroMagnitude = 0.0f;
    mBaseMagnitude = gameParameters.WindSpeedBase;
    mMaxMagnitude = gameParameters.WindSpeedBase * gameParameters.WindSpeedMaxFactor;
    mPreMaxMagnitude = mBaseMagnitude + (mMaxMagnitude - mBaseMagnitude) / 8.0f;

    mDownFromBaseCdf = 1.0f - exp(-DownFromBaseLambda / (PoissonSampleRate * gameParameters.WindGustFrequencyAdjustment));
    mUpFromBaseCdf = 1.0f - exp(-UpFromBaseLambda / (PoissonSampleRate * gameParameters.WindGustFrequencyAdjustment));
    mUpFromPreMaxCdf = 1.0f - exp(-UpFromPreMaxLambda / (PoissonSampleRate * gameParameters.WindGustFrequencyAdjustment));
    mDownFromPreMaxCdf = 1.0f - exp(-DownFromPreMaxLambda / (PoissonSampleRate * gameParameters.WindGustFrequencyAdjustment));
    mUpFromZeroCdf = 1.0f - exp(-UpFromZeroLambda / (PoissonSampleRate * gameParameters.WindGustFrequencyAdjustment));
    mDownFromMaxCdf = 1.0f - exp(-DownFromMaxLambda / (PoissonSampleRate * gameParameters.WindGustFrequencyAdjustment));
}

}