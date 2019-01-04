/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-24
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <chrono>
#include <optional>

/*
 * A wall clock that can be paused. Wish it were for real.
 *
 * Singleton.
 */
class GameWallClock
{
public:

    using time_point = std::chrono::steady_clock::time_point;
    using duration = std::chrono::steady_clock::duration;

public:

    static GameWallClock & GetInstance()
    {
        static GameWallClock * instance = new GameWallClock();

        return *instance;
    }

    inline time_point Now() const
    {
        if (!!mLastResumeTime)
        {
            // We're running
            return mLastPauseTime + (std::chrono::steady_clock::now() - *mLastResumeTime);
        }
        else
        {
            // We're paused
            return mLastPauseTime;
        }
    }

    inline duration Elapsed(time_point previousTimePoint) const
    {
        return Now() - previousTimePoint;
    }

    void SetPaused(bool isPaused)
    {
        if (isPaused)
        {
            if (!!mLastResumeTime)
            {
                mLastPauseTime = Now();
                mLastResumeTime.reset();
            }
        }
        else
        {
            if (!mLastResumeTime)
            {
                mLastResumeTime = std::chrono::steady_clock::now();
            }
        }
    }

private:

    GameWallClock()
        : mLastPauseTime(std::chrono::steady_clock::now())
        , mLastResumeTime(mLastPauseTime)
    {

    }

    time_point mLastPauseTime;
    std::optional<time_point> mLastResumeTime;
};
