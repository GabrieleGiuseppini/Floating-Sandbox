/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-09-29
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "GameMath.h"
#include "GameWallClock.h"

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
        mCurrentTimestamp = mEndTimestamp = GameWallClock::time_point::min();
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
        // Skip straight to current target, in case we're already smoothing
        mCurrentValue = mSetter(mTargetValue);

        // Start from scratch from here
        mStartValue = mCurrentValue;
        mCurrentTimestamp = now;
        mEndTimestamp =
            mCurrentTimestamp
            + mTrajectoryTime
            + std::chrono::milliseconds(1); // Just to make sure we do at least an update

        // Our new target is the clamped target
        mTargetValue = mClamper(value);
    }

    void SetValueImmediate(TValue value)
    {
        mCurrentValue = mTargetValue = mSetter(value);
        mCurrentTimestamp = mEndTimestamp; // Prevent Update from advancing
    }

    void Update(GameWallClock::time_point now)
    {
        if (mCurrentTimestamp < mEndTimestamp)
        {
            // Advance

            mCurrentTimestamp = std::min(now, mEndTimestamp);

            if (mTrajectoryTime == std::chrono::milliseconds::zero())
            { 
                // Degenerate case - we've reached our goal
                mCurrentValue = mTargetValue;
            }
            else
            {
                float const progress = 
                    1.0f 
                    - (static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(mEndTimestamp - mCurrentTimestamp).count())
                       / static_cast<float>(mTrajectoryTime.count()));

                mCurrentValue =
                    mStartValue
                    + (mTargetValue - mStartValue) * SmoothStep(0.0f, 1.0f, Clamp(progress, 0.0f, 1.0f));
            }

            mSetter(mCurrentValue);
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
    GameWallClock::time_point mCurrentTimestamp;
    GameWallClock::time_point mEndTimestamp;
};
