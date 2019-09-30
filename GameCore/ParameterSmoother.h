/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-09-29
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "GameMath.h"
#include "GameWallClock.h"
#include "Log.h"

#include <chrono>
#include <functional>

template<typename TValue>
class ParameterSmoother
{
public:

    ParameterSmoother(
        std::function<TValue()> getter,
        std::function<void(TValue)> setter,
        std::chrono::milliseconds trajectoryTime)
        : ParameterSmoother(
            getter,
            [setter](TValue value) -> TValue
            {
                setter(value);
                return value;
            },
            [](TValue value) -> TValue
            {
                return value;
            },
            trajectoryTime)
    {}

    ParameterSmoother(
        std::function<TValue()> getter,
        std::function<TValue(TValue)> setter,
        std::function<TValue(TValue)> clamper,
        std::chrono::milliseconds trajectoryTime)
        : mGetter(std::move(getter))
        , mSetter(std::move(setter))
        , mClamper(std::move(clamper))
        , mTrajectoryTime(trajectoryTime)
    {
        mStartValue = mTargetValue = mCurrentValue = mGetter();
        mStartTimestamp = mCurrentTimestamp = mEndTimestamp = GameWallClock::time_point::min();
    }

    /*
     * Returns the current value, which is the target value as smoothing is assumed to happen "offline".
     */
    TValue GetValue() const
    {
        return mTargetValue;
    }

    void SetValue(TValue value)
    {
        SetValue(value, GameWallClock::GetInstance().Now());
    }

    void SetValue(
        TValue value,
        GameWallClock::time_point now)
    {
        /* TODOOLD
        auto elapsed = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(mEndTimestamp - mCurrentTimestamp).count())
            / static_cast<float>(mTrajectoryTime.count());
        LogMessage("TODO: start=", mStartValue, " cur=", mCurrentValue, " (@", elapsed, ") old_tar=", mTargetValue, " new_tar=", value);

        // Skip straight to current target, in case we're already smoothing
        mCurrentValue = mSetter(mTargetValue);

        // Start from where we are at now
        mStartValue = mCurrentValue;
        mCurrentTimestamp = now;
        mEndTimestamp =
            mCurrentTimestamp
            + mTrajectoryTime
            + std::chrono::milliseconds(1); // Just to make sure we do at least an update

        // Our new target is the clamped target
        mTargetValue = mClamper(value);
        */

        if (mCurrentTimestamp < mEndTimestamp
            && now < mEndTimestamp)
        {
            // We are already smoothing...
            // ...extend the current smoothing, keeping the following invariants:
            // - The current value
            // - The current time
            // - The "slope" at the current time 
            // - The new end timestamp == now + delta T

            mCurrentTimestamp = now;
            auto newEndTimestamp = 
                now
                + mTrajectoryTime
                + std::chrono::milliseconds(1); // Just to make sure we do at least an update

            mCurrentValue = GetValueAt(mCurrentTimestamp);            

            float const oldCurOverWhole =
                static_cast<float>((mCurrentTimestamp - mStartTimestamp).count())
                / static_cast<float>((mEndTimestamp - mStartTimestamp).count());

            LogMessage("RESMOOTHING: Begin: cur/whole=",
                oldCurOverWhole,
                " startT=", mStartTimestamp.time_since_epoch().count(), " curT=", mCurrentTimestamp.time_since_epoch().count(),
                " endT=", mEndTimestamp.time_since_epoch().count(),
                " startVal=", mStartValue, " curVal=", mCurrentValue, " targetVal=", mTargetValue);
                
            // Now calculate fictitious StartTimestamp so that 
            // currentTimestamp is to old endTimestamp like 
            // new currentTimestamp it to new endTimestamp
            float const fraction =
                static_cast<float>((mCurrentTimestamp - mStartTimestamp).count())
                / static_cast<float>((mEndTimestamp - mCurrentTimestamp).count());
            mStartTimestamp = mCurrentTimestamp - std::chrono::milliseconds(
                static_cast<std::chrono::milliseconds::rep>(fraction * mTrajectoryTime.count()));

            // Update new target value
            mTargetValue = mClamper(value);

            // Now calculate fictitious StartValue so that
            // calculated current value at current time matches current value:
            //  newStartValue = currentValue - f(newEndValue - newStartValue)
            float const f = SmoothStep(0.0f, 1.0f, oldCurOverWhole);
            mStartValue =
                (mCurrentValue - mTargetValue * f)
                / (1.0f - f);

            mEndTimestamp = newEndTimestamp;


            float const newCurOverWhole =
                static_cast<float>((mCurrentTimestamp - mStartTimestamp).count())
                / static_cast<float>((mEndTimestamp - mStartTimestamp).count());

            LogMessage("RESMOOTHING: End: cur/whole=",
                newCurOverWhole,
                " startT=", mStartTimestamp.time_since_epoch().count(), " curT=", mCurrentTimestamp.time_since_epoch().count(),
                " endT=", mEndTimestamp.time_since_epoch().count(),
                " startVal=", mStartValue, " curVal=", mCurrentValue, " targetVal=", mTargetValue,
                " checkVal=", GetValueAt(mCurrentTimestamp));
        }
        else
        {
            // We are not smoothing...
            // ...start a new smoothing

            mCurrentValue = mTargetValue; // Just in case
            mStartValue = mCurrentValue;

            mStartTimestamp = now;
            mCurrentTimestamp = now;
            mEndTimestamp =
                mStartTimestamp
                + mTrajectoryTime
                + std::chrono::milliseconds(1); // Just to make sure we do at least an update

            // Our new target is the clamped target
            mTargetValue = mClamper(value);
        }
    }

    void SetValueImmediate(TValue value)
    {
        mCurrentValue = mTargetValue = mSetter(value);
        mCurrentTimestamp = mEndTimestamp; // Prevent Update from advancing
    }

    void Update(GameWallClock::time_point now)
    {
        mCurrentTimestamp = std::min(now, mEndTimestamp);

        if (mCurrentTimestamp < mEndTimestamp)
        {
            // Advance            

            mCurrentValue = GetValueAt(mCurrentTimestamp);

            mSetter(mCurrentValue);
        }
    }

private:

    TValue GetValueAt(GameWallClock::time_point now) const
    {
        if (mTrajectoryTime == std::chrono::milliseconds::zero())
        {
            // Degenerate case - we've reached our goal
            return mTargetValue;
        }
        else
        {
            float const progress =
                1.0f
                - (static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(mEndTimestamp - now).count())
                    // tODOTEST
                    /// static_cast<float>(mTrajectoryTime.count()));
                    / static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(mEndTimestamp - mStartTimestamp).count()));

            return
                mStartValue
                + (mTargetValue - mStartValue) * SmoothStep(0.0f, 1.0f, Clamp(progress, 0.0f, 1.0f));
        }
    }

private:

    std::function<TValue()> const mGetter;
    std::function<TValue(TValue)> const mSetter;
    std::function<TValue(TValue)> const mClamper;
    std::chrono::milliseconds const mTrajectoryTime;

    TValue mStartValue;
    TValue mTargetValue;
    TValue mCurrentValue;
    GameWallClock::time_point mStartTimestamp;
    GameWallClock::time_point mCurrentTimestamp;
    GameWallClock::time_point mEndTimestamp;
};
