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
        std::function<TValue const &()> getter,
        std::function<void(TValue const &)> setter,
        std::chrono::milliseconds trajectoryTime)
        : ParameterSmoother(
            getter,
            [setter](TValue const & value) -> TValue const &
            {
                setter(value);
                return value;
            },
            [](TValue const & value) -> TValue
            {
                return value;
            },
            trajectoryTime)
    {}

    ParameterSmoother(
        std::function<TValue const &()> getter,
        std::function<TValue const &(TValue const &)> setter,
        std::function<TValue(TValue const &)> clamper,
        std::chrono::milliseconds trajectoryTime)
        : mGetter(std::move(getter))
        , mSetter(std::move(setter))
        , mClamper(std::move(clamper))
        , mTrajectoryTime(std::chrono::duration_cast<std::chrono::duration<float>>(trajectoryTime).count())
    {
        mStartValue = mTargetValue = mGetter();
        mStartTimestamp = mCurrentTimestamp = mEndTimestamp = 0.0f;
    }

    /*
     * Returns the current value, which is the target value as smoothing is assumed to happen "offline".
     */
    TValue const & GetValue() const
    {
        return mTargetValue;
    }

    void SetValue(TValue const & value)
    {
        SetValue(value, GameWallClock::GetInstance().NowAsFloat());
    }

    void SetValue(
        TValue const & value,
        float now)
    {
        if (mCurrentTimestamp < mEndTimestamp
            && now < mEndTimestamp)
        {
            // We are already smoothing and we're caught during smoothing...
            // ...extend the current smoothing, keeping the following invariants:
            // - The current value
            // - The current time
            // - The "slope" at the current time 
            // - The new end timestamp == now + delta T

            // Advance time
            mCurrentTimestamp = now;
            TValue currentValue = GetValueAt(mCurrentTimestamp);

            // Calculate current timestamp as fraction of current timespan
            //
            // We need to make sure we're not too close to 1.0f, or else
            // values start diverging too much.
            // We may safely clamp down to 0.9 as the value will stay and the slope
            // will only change marginally.
            assert(mEndTimestamp > mStartTimestamp);
            float const progressFraction = std::min(
                (mCurrentTimestamp - mStartTimestamp) / (mEndTimestamp - mStartTimestamp),
                0.9f);
                
            // Calculate new end timestamp
            mEndTimestamp = now + mTrajectoryTime;

            // Calculate fictitious start timestamp so that current timestamp is 
            // to new timespan like current timestamp was to old timespan
            mStartTimestamp = 
                mCurrentTimestamp
                - (mEndTimestamp - mCurrentTimestamp) * progressFraction / (1.0f - progressFraction);

            // Our new target is the clamped target
            mTargetValue = mClamper(value);

            // Calculate fictitious start value so that calculated current value 
            // at current timestamp matches current value:
            //  newStartValue = currentValue - f(newEndValue - newStartValue)
            float const valueFraction = SmoothStep(0.0f, 1.0f, progressFraction);
            mStartValue =
                (currentValue - mTargetValue * valueFraction)
                / (1.0f - valueFraction);
        }
        else
        {
            // We are not smoothing...
            // ...start a new smoothing

            mStartValue = mTargetValue;

            mStartTimestamp = now;
            mCurrentTimestamp = now;
            mEndTimestamp = now + mTrajectoryTime;

            // Our new target is the clamped target
            mTargetValue = mClamper(value);
        }
    }

    void SetValueImmediate(TValue const & value)
    {
        mTargetValue = mSetter(value);
        mCurrentTimestamp = mEndTimestamp; // Prevent Update from advancing
    }

    void Update(float now)
    {
        if (mCurrentTimestamp < mEndTimestamp)
        {
            // Advance            

            mCurrentTimestamp = std::min(now, mEndTimestamp);

            auto currentValue = GetValueAt(mCurrentTimestamp);

            mSetter(currentValue);

            // In case conditions have changed, we pickup the new target value
            // and we will return the correct value
            mTargetValue = mClamper(mTargetValue);            
        }
    }

private:

    TValue GetValueAt(float now) const
    {
        if (mTrajectoryTime == 0.0f)
        {
            // Degenerate case - we've reached our goal
            return mTargetValue;
        }
        else
        {
            float const progress =
                1.0f
                - ((mEndTimestamp - now) / (mEndTimestamp - mStartTimestamp));

            return
                mStartValue
                + (mTargetValue - mStartValue) * SmoothStep(0.0f, 1.0f, Clamp(progress, 0.0f, 1.0f));
        }
    }

private:

    std::function<TValue const & ()> const mGetter;
    std::function<TValue const &(TValue const &)> const mSetter;
    std::function<TValue(TValue const &)> const mClamper;
    float const mTrajectoryTime;

    TValue mStartValue;
    TValue mTargetValue;
    float mStartTimestamp;
    float mCurrentTimestamp;
    float mEndTimestamp;
};
