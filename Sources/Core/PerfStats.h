/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-12-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameChronometer.h"

#include <atomic>

struct PerfStats
{
    struct Ratio
    {
    private:

        struct _Ratio
        {
            GameChronometer::duration Duration;
            size_t Denominator;

            _Ratio() noexcept
                : Duration(GameChronometer::duration::zero())
                , Denominator(0)
            {}

            _Ratio(
                GameChronometer::duration duration,
                size_t denominator)
                : Duration(duration)
                , Denominator(denominator)
            {}
        };

        std::atomic<_Ratio> mRatio;

    public:

        Ratio()
            : mRatio()
        {}

        Ratio(Ratio const & other)
        {
            mRatio.store(other.mRatio.load());
        }

        Ratio const & operator=(Ratio const & other)
        {
            mRatio.store(other.mRatio.load());
            return *this;
        }

        inline void Update(GameChronometer::duration duration)
        {
            auto ratio = mRatio.load();
            ratio.Duration += duration;
            ratio.Denominator += 1;
            mRatio.store(ratio);
        }

        template<typename TDuration>
        inline float ToRatio() const
        {
            _Ratio const ratio = mRatio.load();

            if (ratio.Denominator == 0)
                return 0.0f;

            auto fs = std::chrono::duration_cast<std::chrono::duration<float>>(ratio.Duration);
            return fs.count() * static_cast<float>(TDuration::period::den) / static_cast<float>(TDuration::period::num)
                / static_cast<float>(ratio.Denominator);
        }

        inline void Reset()
        {
            mRatio.store(_Ratio());
        }

        friend Ratio operator-(Ratio const & lhs, Ratio const & rhs)
        {
            auto const lRatio = lhs.mRatio.load();
            auto const rRatio = rhs.mRatio.load();
            _Ratio result(
                lRatio.Duration - rRatio.Duration,
                lRatio.Denominator - rRatio.Denominator);

            Ratio res;
            res.mRatio.store(result);
            return res;
        }
    };

    // Update
    Ratio TotalUpdateDuration;
    Ratio TotalNpcUpdateDuration;
    Ratio TotalFishUpdateDuration;
    Ratio TotalOceanSurfaceUpdateDuration;
    Ratio TotalShipsUpdateDuration;
    Ratio TotalShipsSpringsUpdateDuration;
    Ratio TotalWaitForRenderUploadDuration;
    Ratio TotalNetUpdateDuration; // = TotalUpdateDuration - TotalWaitForRenderUploadDuration

    // Render-Upload
    Ratio TotalWaitForRenderDrawDuration;
    Ratio TotalNetRenderUploadDuration;

    // Render-Draw
    Ratio TotalMainThreadRenderDrawDuration;
    Ratio TotalRenderDrawDuration; // In render thread
    Ratio TotalUploadRenderDrawDuration;

    PerfStats()
    {
        Reset();
    }

    void Reset()
    {
        TotalUpdateDuration.Reset();
        TotalNpcUpdateDuration.Reset();
        TotalFishUpdateDuration.Reset();
        TotalOceanSurfaceUpdateDuration.Reset();
        TotalShipsUpdateDuration.Reset();
        TotalShipsSpringsUpdateDuration.Reset();
        TotalWaitForRenderUploadDuration.Reset();
        TotalNetUpdateDuration.Reset();

        TotalWaitForRenderDrawDuration.Reset();
        TotalNetRenderUploadDuration.Reset();

        TotalMainThreadRenderDrawDuration.Reset();
        TotalRenderDrawDuration.Reset();
        TotalUploadRenderDrawDuration.Reset();
    }

    PerfStats & operator=(PerfStats const & other) = default;
};

inline PerfStats operator-(PerfStats const & lhs, PerfStats const & rhs)
{
    PerfStats perfStats;

    perfStats.TotalUpdateDuration = lhs.TotalUpdateDuration - rhs.TotalUpdateDuration;
    perfStats.TotalNpcUpdateDuration = lhs.TotalNpcUpdateDuration - rhs.TotalNpcUpdateDuration;
    perfStats.TotalFishUpdateDuration = lhs.TotalFishUpdateDuration - rhs.TotalFishUpdateDuration;
    perfStats.TotalOceanSurfaceUpdateDuration = lhs.TotalOceanSurfaceUpdateDuration - rhs.TotalOceanSurfaceUpdateDuration;
    perfStats.TotalShipsUpdateDuration = lhs.TotalShipsUpdateDuration - rhs.TotalShipsUpdateDuration;
    perfStats.TotalShipsSpringsUpdateDuration = lhs.TotalShipsSpringsUpdateDuration - rhs.TotalShipsSpringsUpdateDuration;
    perfStats.TotalWaitForRenderUploadDuration = lhs.TotalWaitForRenderUploadDuration - rhs.TotalWaitForRenderUploadDuration;
    perfStats.TotalNetUpdateDuration = lhs.TotalNetUpdateDuration - rhs.TotalNetUpdateDuration;

    perfStats.TotalWaitForRenderDrawDuration = lhs.TotalWaitForRenderDrawDuration - rhs.TotalWaitForRenderDrawDuration;
    perfStats.TotalNetRenderUploadDuration = lhs.TotalNetRenderUploadDuration - rhs.TotalNetRenderUploadDuration;

    perfStats.TotalMainThreadRenderDrawDuration = lhs.TotalMainThreadRenderDrawDuration - rhs.TotalMainThreadRenderDrawDuration;
    perfStats.TotalRenderDrawDuration = lhs.TotalRenderDrawDuration - rhs.TotalRenderDrawDuration;
    perfStats.TotalUploadRenderDrawDuration = lhs.TotalUploadRenderDrawDuration - rhs.TotalUploadRenderDrawDuration;

    return perfStats;
}