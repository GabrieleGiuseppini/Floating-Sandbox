/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

#include "../SimulationEventDispatcher.h"
#include "../SimulationParameters.h"

#include <Core/GameMath.h>
#include <Core/GameWallClock.h>
#include <Core/RunningAverage.h>

#include <optional>

namespace Physics
{

/*
 * Wind consists of two components:
 * - A linear (horizontal) wind, whose intensity is modulated by various actors (storm, gusting state machine, etc.)
 * - A radial wind, when triggered interactively
 */
class Wind
{
public:

    struct RadialWindField
    {
        vec2f SourcePos;
        float PreFrontRadius;
        float PreFrontWindForceMagnitude;
        float MainFrontRadius;
        float MainFrontWindForceMagnitude;

        RadialWindField(
            vec2f sourcePos,
            float preFrontRadius,
            float preFrontWindForceMagnitude,
            float mainFrontRadius,
            float mainFrontWindForceMagnitude)
            : SourcePos(sourcePos)
            , PreFrontRadius(preFrontRadius)
            , PreFrontWindForceMagnitude(preFrontWindForceMagnitude)
            , MainFrontRadius(mainFrontRadius)
            , MainFrontWindForceMagnitude(mainFrontWindForceMagnitude)
        {}
    };

public:

    Wind(SimulationEventDispatcher & simulationEventDispatcher);

    void SetSilence(float silenceAmount);

    void Update(
        Storm::Parameters const & stormParameters,
        SimulationParameters const & simulationParameters);

    void UpdateEnd();

    void Upload(RenderContext & renderContext) const;

    /*
     * Returns the (signed) base speed magnitude - i.e. the magnitude of the unmodulated
     * wind speed - with the storm speed magnitude on top of it.
     *
     * Km/h.
     */
    float GetBaseAndStormSpeedMagnitude() const
    {
        return mBaseAndStormSpeedMagnitude;
    }

    /*
     * Returns the (signed) maximum magnitude, i.e. the full magnitude of the speed of a gust.
     *
     * Km/h.
     */
    float GetMaxSpeedMagnitude() const
    {
        return mMaxSpeedMagnitude;
    }

    /*
     * Returns the current modulated wind speed.
     *
     * Km/h.
     */
    vec2f const & GetCurrentWindSpeed() const
    {
        return mCurrentWindSpeed;
    }

    /*
     * Returns the current radial wind field, if any.
     */
    std::optional<RadialWindField> const & GetCurrentRadialWindField() const
    {
        return mCurrentRadialWindField;
    }

    /*
     * Sets the current radial wind field.
     * Will be wiped at the end of the update cycle.
     */
    void SetRadialWindField(RadialWindField const & radialWindField)
    {
        mCurrentRadialWindField = radialWindField;
    }

private:

    static GameWallClock::duration ChooseDuration(float minSeconds, float maxSeconds);

    void RecalculateParameters(
        Storm::Parameters const & stormParameters,
        SimulationParameters const & simulationParameters);

private:

    SimulationEventDispatcher & mSimulationEventHandler;

    //
    // Pre-calculated parameters
    //

    float mZeroSpeedMagnitude;
    float mBaseSpeedMagnitude;
    float mBaseAndStormSpeedMagnitude;
    float mPreMaxSpeedMagnitude;
    float mMaxSpeedMagnitude;

    float mGustCdf; // Poisson CDF for gust emission

    // The last parameter values our pre-calculated values are current with
    bool mCurrentDoModulateWindParameter;
    float mCurrentSpeedBaseParameter;
    float mCurrentSpeedMaxFactorParameter;
    float mCurrentGustFrequencyAdjustmentParameter;
    float mCurrentStormWindSpeedParameter;

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

    // The current silence amount
    float mCurrentSilenceAmount;

    // The current wind speed magnitude, before averaging
    float mCurrentRawWindSpeedMagnitude;

    // The (short) running average of the wind speed magnitude
    //
    // We average it just to prevent big impulses
    RunningAverage<4> mCurrentWindSpeedMagnitudeRunningAverage;

    // The current wind speed
    vec2f mCurrentWindSpeed;

    //
    // Radial wind field
    //
    // Set interactively before an Update cycle, and reset at the
    // end of the update cycle.
    //

    std::optional<RadialWindField> mCurrentRadialWindField;
};

}
