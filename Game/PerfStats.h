/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-12-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameChronometer.h>

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

            _Ratio()
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
    Ratio TotalOceanSurfaceUpdateDuration;
    Ratio TotalShipsUpdateDuration;
    Ratio TotalWaitForRenderUploadDuration;
    Ratio TotalNetUpdateDuration; // = TotalUpdateDuration - TotalWaitForRenderUploadDuration

    // Render-Upload
    Ratio TotalWaitForRenderDrawDuration;
    Ratio TotalNetRenderUploadDuration;

    // Render-Draw
    Ratio TotalMainThreadRenderDrawDuration;
    Ratio TotalRenderDrawDuration; // In render thread
    Ratio TotalCloudsRenderDrawDuration; // In render thread
    Ratio TotalOceanSurfaceRenderDrawDuration; // In render thread

    PerfStats()
    {
        Reset();
    }

    void Reset()
    {
        TotalUpdateDuration.Reset();
        TotalOceanSurfaceUpdateDuration.Reset();
        TotalShipsUpdateDuration.Reset();
        TotalWaitForRenderUploadDuration.Reset();
        TotalNetUpdateDuration.Reset();

        TotalWaitForRenderDrawDuration.Reset();
        TotalNetRenderUploadDuration.Reset();

        TotalMainThreadRenderDrawDuration.Reset();
        TotalRenderDrawDuration.Reset();
        TotalCloudsRenderDrawDuration.Reset();
        TotalOceanSurfaceRenderDrawDuration.Reset();
    }

    PerfStats & operator=(PerfStats const & other) = default;
};

inline PerfStats operator-(PerfStats const & lhs, PerfStats const & rhs)
{
    PerfStats perfStats;

    perfStats.TotalUpdateDuration = lhs.TotalUpdateDuration - rhs.TotalUpdateDuration;
    perfStats.TotalOceanSurfaceUpdateDuration = lhs.TotalOceanSurfaceUpdateDuration - rhs.TotalOceanSurfaceUpdateDuration;
    perfStats.TotalShipsUpdateDuration = lhs.TotalShipsUpdateDuration - rhs.TotalShipsUpdateDuration;
    perfStats.TotalWaitForRenderUploadDuration = lhs.TotalWaitForRenderUploadDuration - rhs.TotalWaitForRenderUploadDuration;
    perfStats.TotalNetUpdateDuration = lhs.TotalNetUpdateDuration - rhs.TotalNetUpdateDuration;

    perfStats.TotalWaitForRenderDrawDuration = lhs.TotalWaitForRenderDrawDuration - rhs.TotalWaitForRenderDrawDuration;
    perfStats.TotalNetRenderUploadDuration = lhs.TotalNetRenderUploadDuration - rhs.TotalNetRenderUploadDuration;

    perfStats.TotalMainThreadRenderDrawDuration = lhs.TotalMainThreadRenderDrawDuration - rhs.TotalMainThreadRenderDrawDuration;
    perfStats.TotalRenderDrawDuration = lhs.TotalRenderDrawDuration - rhs.TotalRenderDrawDuration;
    perfStats.TotalCloudsRenderDrawDuration = lhs.TotalCloudsRenderDrawDuration - rhs.TotalCloudsRenderDrawDuration;
    perfStats.TotalOceanSurfaceRenderDrawDuration = lhs.TotalOceanSurfaceRenderDrawDuration - rhs.TotalOceanSurfaceRenderDrawDuration;

    return perfStats;
}