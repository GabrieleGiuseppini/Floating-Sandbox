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

class ParameterSmoother
{
public:

    ParameterSmoother(
        std::function<float()> getter,
        std::function<void(float)> setter,
        std::chrono::milliseconds trajectoryTime)
        : ParameterSmoother(
            getter,
            [setter](float value) -> float
            {
                setter(value);
                return value;
            },
            [](float value) -> float
            {
                return value;
            },
            trajectoryTime)
    {}

    ParameterSmoother(
        std::function<float()> getter,
        std::function<float(float)> setter,
        std::function<float(float)> clamper,
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
    float GetValue() const
    {
        return mTargetValue;
    }

    void SetValue(float value)
    {
        SetValue(value, GameWallClock::GetInstance().Now());
    }

    void SetValue(
        float value,
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
                    + (mTargetValue - mStartValue) * SmoothStep(0.0f, 1.0f, progress);
            }

            // Adjust for overshooting
            if (mStartValue < mTargetValue)
                mCurrentValue = std::min(mCurrentValue, mTargetValue);
            else
                mCurrentValue = std::max(mCurrentValue, mTargetValue);

            mSetter(mCurrentValue);
        }
    }

private:

    std::function<float()> const mGetter;
    std::function<float(float)> const mSetter;
    std::function<float(float)> const mClamper;
    std::chrono::milliseconds const mTrajectoryTime;

    float mStartValue;
    float mTargetValue;
    float mCurrentValue;
    GameWallClock::time_point mCurrentTimestamp;
    GameWallClock::time_point mEndTimestamp;
};
