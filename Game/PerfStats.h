/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-12-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameChronometer.h>

struct PerfStats
{
    // Update
    GameChronometer::duration TotalUpdateDuration;
    GameChronometer::duration TotalOceanSurfaceUpdateDuration;
    GameChronometer::duration TotalShipsUpdateDuration;

    // Render
    GameChronometer::duration TotalRenderDuration;
    GameChronometer::duration TotalSwapRenderBuffersDuration;
    GameChronometer::duration TotalSkyRenderDuration;
    GameChronometer::duration TotalOceanFloorRenderDuration;
    GameChronometer::duration TotalShipsRenderDuration;

    PerfStats()
        //
        : TotalUpdateDuration(0)
        , TotalOceanSurfaceUpdateDuration(0)
        , TotalShipsUpdateDuration(0)
        //
        , TotalRenderDuration(0)
        , TotalSwapRenderBuffersDuration(0)
        , TotalSkyRenderDuration(0)
        , TotalOceanFloorRenderDuration(0)
        , TotalShipsRenderDuration(0)
    {
    }

    PerfStats & operator=(PerfStats const & other) = default;
};

inline PerfStats operator-(PerfStats const & lhs, PerfStats const & rhs)
{
    PerfStats perfStats;

    perfStats.TotalUpdateDuration = lhs.TotalUpdateDuration - rhs.TotalUpdateDuration;
    perfStats.TotalOceanSurfaceUpdateDuration = lhs.TotalOceanSurfaceUpdateDuration - rhs.TotalOceanSurfaceUpdateDuration;
    perfStats.TotalShipsUpdateDuration = lhs.TotalShipsUpdateDuration - rhs.TotalShipsUpdateDuration;

    perfStats.TotalRenderDuration = lhs.TotalRenderDuration - rhs.TotalRenderDuration;
    perfStats.TotalSwapRenderBuffersDuration = lhs.TotalSwapRenderBuffersDuration - rhs.TotalSwapRenderBuffersDuration;
    perfStats.TotalSkyRenderDuration = lhs.TotalSkyRenderDuration - rhs.TotalSkyRenderDuration;
    perfStats.TotalOceanFloorRenderDuration = lhs.TotalOceanFloorRenderDuration - rhs.TotalOceanFloorRenderDuration;
    perfStats.TotalShipsRenderDuration = lhs.TotalShipsRenderDuration - rhs.TotalShipsRenderDuration;

    return perfStats;
}