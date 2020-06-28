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
    GameChronometer::duration TotalWaitForRenderUploadDuration;

    // Render-Draw
    GameChronometer::duration TotalRenderDrawDuration;
    GameChronometer::duration TotalWaitForRenderDrawDuration;

    // TODOOLD
    // Render
    GameChronometer::duration TotalRenderDuration;
    GameChronometer::duration TotalSwapRenderBuffersDuration;
    GameChronometer::duration TotalCloudRenderDuration;
    GameChronometer::duration TotalOceanFloorRenderDuration;
    GameChronometer::duration TotalShipsRenderDuration;

    PerfStats()
    {
        Reset();
    }

    void Reset()
    {
        TotalUpdateDuration = GameChronometer::duration::zero();
        TotalOceanSurfaceUpdateDuration = GameChronometer::duration::zero();
        TotalShipsUpdateDuration = GameChronometer::duration::zero();

        TotalRenderUploadDuration = GameChronometer::duration::zero();
        TotalWaitForRenderUploadDuration = GameChronometer::duration::zero();

        TotalRenderDrawDuration = GameChronometer::duration::zero();
        TotalWaitForRenderDrawDuration = GameChronometer::duration::zero();

        // TODOOLD
        TotalRenderDuration = GameChronometer::duration::zero();
        TotalSwapRenderBuffersDuration = GameChronometer::duration::zero();
        TotalCloudRenderDuration = GameChronometer::duration::zero();
        TotalOceanFloorRenderDuration = GameChronometer::duration::zero();
        TotalShipsRenderDuration = GameChronometer::duration::zero();
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
    perfStats.TotalWaitForRenderUploadDuration = lhs.TotalWaitForRenderUploadDuration - rhs.TotalWaitForRenderUploadDuration;

    perfStats.TotalRenderDrawDuration = lhs.TotalRenderDrawDuration - rhs.TotalRenderDrawDuration;
    perfStats.TotalWaitForRenderDrawDuration = lhs.TotalWaitForRenderDrawDuration - rhs.TotalWaitForRenderDrawDuration;

    // TODOOLD
    perfStats.TotalRenderDuration = lhs.TotalRenderDuration - rhs.TotalRenderDuration;
    perfStats.TotalSwapRenderBuffersDuration = lhs.TotalSwapRenderBuffersDuration - rhs.TotalSwapRenderBuffersDuration;
    perfStats.TotalCloudRenderDuration = lhs.TotalCloudRenderDuration - rhs.TotalCloudRenderDuration;
    perfStats.TotalOceanFloorRenderDuration = lhs.TotalOceanFloorRenderDuration - rhs.TotalOceanFloorRenderDuration;
    perfStats.TotalShipsRenderDuration = lhs.TotalShipsRenderDuration - rhs.TotalShipsRenderDuration;

    return perfStats;
}