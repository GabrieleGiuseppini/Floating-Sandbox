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
#include <GameCore/GameWallClock.h>
#include <GameCore/RunningAverage.h>

namespace Physics
{

class Wind
{
public:

    Wind(std::shared_ptr<IGameEventHandler> gameEventHandler);

    void Update(GameParameters const & gameParameters);

    float GetBaseMagnitude() const
    {
        return mBaseMagnitude;
    }

    float GetMaxMagnitude() const
    {
        return mMaxMagnitude;
    }

    vec2f const & GetCurrentWindForce() const
    {
        return mCurrentWindForce;
    }

private:

    static GameWallClock::duration ChooseDuration(float minSeconds, float maxSeconds);

    void RecalculateParameters(GameParameters const & gameParameters);

private:

    std::shared_ptr<IGameEventHandler> mGameEventHandler;

    //
    // Pre-calculated parameters
    //

    float mZeroMagnitude;
    float mBaseMagnitude;
    float mPreMaxMagnitude;
    float mMaxMagnitude;

    float mGustCdf;

    // The last parameter values our pre-calculated values are current with
    bool mCurrentDoModulateWindParameter;
    float mCurrentSpeedBaseParameter;
    float mCurrentSpeedMaxFactorParameter;
    float mCurrentGustFrequencyAdjustmentParameter;

    //
    // Wind state machine
    //

    enum class State
    {
        Initial,

        EnterBase1,
        Base1,

        EnterPreGusting,
        PreGusting,

        EnterGusting,
        Gusting,

        EnterGust,
        Gust,

        EnterPostGusting,
        PostGusting,

        EnterBase2,
        Base2,

        EnterZero,
        Zero
    };

    State mCurrentState;

    // The timestamp of the next state transition
    GameWallClock::time_point mNextStateTransitionTimestamp;

    // The next time at which we should sample the poisson distribution
    GameWallClock::time_point mNextPoissonSampleTimestamp;

    // The next time at which the current gust should end
    GameWallClock::time_point mCurrentGustTransitionTimestamp;

    // The current wind force magnitude, before averaging
    float mCurrentRawWindForceMagnitude;

    // The (short) running average of the wind force magnitude
    //
    // We average it just to prevent big impulses
    RunningAverage<4> mCurrentWindForceMagnitudeRunningAverage;

    // The current wind force
    vec2f mCurrentWindForce;
};

}
