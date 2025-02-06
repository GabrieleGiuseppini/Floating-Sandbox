/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-12-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameChronometer.h"

#include <algorithm>
#include <atomic>
#include <vector>

enum class PerfMeasurement : size_t
{
    // Update
    TotalUpdate = 0,
    TotalNpcUpdate,
    TotalFishUpdate,
    TotalOceanSurfaceUpdate,
    TotalShipsUpdate,
    TotalShipsSpringsUpdate,
    TotalWaitForRenderUpload,
    TotalNetUpdate, // = TotalUpdate - TotalWaitForRenderUpload

    // Render-Upload
    TotalWaitForRenderDraw,
    TotalNetRenderUpload,

    // Render-Draw
    TotalMainThreadRenderDraw,
    TotalRenderDraw, // In render thread
    TotalUploadRenderDraw,

    _Last = TotalUploadRenderDraw
};

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

            auto fDuration = std::chrono::duration_cast<std::chrono::duration<float>>(ratio.Duration);
            return fDuration.count() * static_cast<float>(TDuration::period::den) / static_cast<float>(TDuration::period::num)
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

    PerfStats()
    {
        mMeasurements.resize(static_cast<size_t>(PerfMeasurement::_Last));
        Reset();
    }

    template<PerfMeasurement PM>
    Ratio const & GetMeasurement() const
    {
        return mMeasurements[static_cast<std::size_t>(PM)];
    }

    template<PerfMeasurement PM>
    void Update(GameChronometer::duration duration)
    {
        mMeasurements[static_cast<std::size_t>(PM)].Update(duration);
    }

    void Reset()
    {
        std::for_each(
            mMeasurements.begin(),
            mMeasurements.end(),
            [](auto & m) { m.Reset(); });
    }

    PerfStats & operator=(PerfStats const & other) = default;

    friend PerfStats operator-(PerfStats const & lhs, PerfStats const & rhs);

private:

    // Indexed by PerfMeasurement integral
    std::vector<Ratio> mMeasurements;
};

inline PerfStats operator-(PerfStats const & lhs, PerfStats const & rhs)
{
    PerfStats perfStats;

    for (size_t i = 0; i < static_cast<size_t>(PerfMeasurement::_Last); ++i)
    {
        perfStats.mMeasurements[i] = lhs.mMeasurements[i] - rhs.mMeasurements[i];
    }

    return perfStats;
}