/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

#include "GameParameters.h"
#include "IGameEventHandler.h"

#include <GameCore/GameMath.h>
#include <GameCore/RunningAverage.h>

namespace Physics
{

class Wind
{
public:

    Wind(std::shared_ptr<IGameEventHandler> gameEventHandler);

    void Update(
        float currentSimulationTime,
        GameParameters const & gameParameters);

    inline vec2f const & GetCurrentWindForce() const
    {
        return mCurrentWindForce;
    }

private:

    void RecalculateParameters(GameParameters const & gameParameters);

private:

    std::shared_ptr<IGameEventHandler> mGameEventHandler;

    // Pre-calculated parameters
    float mZeroMagnitude;
    float mBaseMagnitude;
    float mPreMaxMagnitude;
    float mMaxMagnitude;
    float mDownFromBaseCdf;
    float mUpFromBaseCdf;
    float mUpFromPreMaxCdf;
    float mDownFromPreMaxCdf;
    float mUpFromZeroCdf;
    float mDownFromMaxCdf;

    // The last parameter values our pre-calculated values are current with
    float mCurrentSpeedBaseParameter;
    float mCurrentSpeedMaxFactorParameter;
    float mCurrentGustFrequencyAdjustmentParameter;

    //
    // Wind state machine
    //

    enum class State
    {
        Initial,

        Max,
        PreMax,
        Base,
        Zero
    };

    State mCurrentState;

    // The next (simulation) time at which we should sample the poisson distribution
    float mNextPoissonSampleSimulationTime;

    // The current wind force magnitude, before averaging
    float mCurrentRawWindForceMagnitude;

    // The (short) running average of the wind force magnitude
    RunningAverage<4> mCurrentWindForceMagnitudeRunningAverage;

    // The current wind force
    vec2f mCurrentWindForce;
};

}
