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

    // Render-Upload
    GameChronometer::duration TotalRenderUploadDuration;

    // Render-Draw
    GameChronometer::duration TotalRenderDrawDuration;

    // TODOOLD
    // Render
    GameChronometer::duration TotalRenderDuration;
    GameChronometer::duration TotalSwapRenderBuffersDuration;
    GameChronometer::duration TotalCloudRenderDuration;
    GameChronometer::duration TotalOceanFloorRenderDuration;
    GameChronometer::duration TotalShipsRenderDuration;

    PerfStats()
        //
        : TotalUpdateDuration(0)
        , TotalOceanSurfaceUpdateDuration(0)
        , TotalShipsUpdateDuration(0)
        //
        , TotalRenderUploadDuration(0)
        //
        , TotalRenderDrawDuration(0)
        // TODOOLD
        , TotalRenderDuration(0)
        , TotalSwapRenderBuffersDuration(0)
        , TotalCloudRenderDuration(0)
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

    perfStats.TotalRenderUploadDuration = lhs.TotalRenderUploadDuration - rhs.TotalRenderUploadDuration;

    perfStats.TotalRenderDrawDuration = lhs.TotalRenderDrawDuration - rhs.TotalRenderDrawDuration;

    // TODOOLD
    perfStats.TotalRenderDuration = lhs.TotalRenderDuration - rhs.TotalRenderDuration;
    perfStats.TotalSwapRenderBuffersDuration = lhs.TotalSwapRenderBuffersDuration - rhs.TotalSwapRenderBuffersDuration;
    perfStats.TotalCloudRenderDuration = lhs.TotalCloudRenderDuration - rhs.TotalCloudRenderDuration;
    perfStats.TotalOceanFloorRenderDuration = lhs.TotalOceanFloorRenderDuration - rhs.TotalOceanFloorRenderDuration;
    perfStats.TotalShipsRenderDuration = lhs.TotalShipsRenderDuration - rhs.TotalShipsRenderDuration;

    return perfStats;
}