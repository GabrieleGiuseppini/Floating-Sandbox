/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-24
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <chrono>
#include <optional>

/*
 * A monotonic clock that can be paused. Wish it were for real.
 *
 * Note: it's not really a wall clock - its values do not measure time.
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

    /*
     * Returns the current time as a fractional number of seconds since an arbitrary
     * reference moment.
     *
     * Useful as a "t" variable when the trend is important - not its absolute value.
     */
    inline float NowAsFloat() const
    {
        return ElapsedAsFloat(mClockStartTime);
    }

    inline duration Elapsed(time_point previousTimePoint) const
    {
        return Now() - previousTimePoint;
    }

    inline float ElapsedAsFloat(time_point previousTimePoint) const
    {
        return std::chrono::duration_cast<std::chrono::duration<float>>(Now() - previousTimePoint).count();
    }

    /*
     * Returns the time elapsed since the specified moment as a fraction of the
     * specified interval.
     */
    template<typename TDuration>
    inline float ProgressSince(
        time_point previousTimePoint,
        TDuration interval) const
    {
        assert(interval.count() != 0);

        return ElapsedAsFloat(previousTimePoint)
            / std::chrono::duration_cast<std::chrono::duration<float>>(interval).count();
    }

    /*
     * Returns the time elapsed since the specified moment as a fraction of the
     * specified interval.
     */
    template<typename TDuration>
    inline float ProgressSince(
        float previousTime,
        TDuration interval) const
    {
        assert(interval.count() != 0);

        return ProgressSince(
            NowAsFloat(),
            previousTime,
            interval);
    }

    /*
     * Returns the time elapsed since the specified moment as a fraction of the
     * specified interval.
     */
    template<typename TDuration>
    inline static float Progress(
        float time,
        float previousTime,
        TDuration interval)
    {
        assert(interval.count() != 0);

        return (time - previousTime)
            / std::chrono::duration_cast<std::chrono::duration<float>>(interval).count();
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
        : mClockStartTime(std::chrono::steady_clock::now())
        , mLastPauseTime(std::chrono::steady_clock::now())
        , mLastResumeTime(mLastPauseTime)
    {

    }

    time_point const mClockStartTime;
    time_point mLastPauseTime;
    std::optional<time_point> mLastResumeTime;
};
