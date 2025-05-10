/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-12-20
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <chrono>

/*
 * A wall clock used to gather difference between two time points.
 * It's independent of game state (paused vs. running).
 */
class GameChronometer final
{
public:

    using time_point = std::chrono::steady_clock::time_point;
    using duration = std::chrono::steady_clock::duration;

    static time_point Now()
    {
        return std::chrono::steady_clock::now();
    }

    static float ElapsedSeconds(
        time_point const & now,
        time_point const & prev)
    {
        return std::chrono::duration_cast<std::chrono::duration<float>>(now - prev).count();
    }

    template<typename TDuration>
    static float Progress(
        time_point const & now,
        time_point const & prev,
        TDuration const & interval)
    {
        return
            std::chrono::duration_cast<std::chrono::duration<float>>(now - prev).count()
            / std::chrono::duration_cast<std::chrono::duration<float>>(interval).count();
    }
};