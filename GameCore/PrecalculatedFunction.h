/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-05-11
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameMath.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <functional>

/*
 * This class implements a buffer containing values of a pre-calculated function between
 * two user-specified extremes.
 */
template<size_t SamplesCount>
class PrecalculatedFunction
{
    static_assert(SamplesCount > 1);

public:

    PrecalculatedFunction()
        : mSamples()
    {
    }

    PrecalculatedFunction(std::function<float(float)> calculator)
        : mSamples()
    {
        PopulateSamples(calculator);
    }

    void Recalculate(std::function<float(float)> calculator)
    {
        PopulateSamples(calculator);
    }

    /*
     * Gets the sample nearest to the specified value, which is expected to be between
     * zero (first sample) and one-Dx (last sample). One is also fine, but that would
     * repeat the last sample.
     */
    inline float GetNearest(float x) const
    {
        // Calculate sample index, minimizing error
        float const sampleIndexF = x / Dx;
        auto const sampleIndexI = FastTruncateToArchInt(sampleIndexF + 0.5f);
        assert(sampleIndexI >= 0 && sampleIndexI <= SamplesCount);

        return mSamples[sampleIndexI].SampleValue;
    }

    /*
     * Gets the sample nearest to the specified value,
     * assumed to be periodic around one.
     */
    inline float GetNearestPeriodic(float x) const
    {
        // Fractional absolute index in the (infinite) sample array
        float const absoluteSampleIndexF = x / Dx;

        // Integral part - absolute, minimizing error
        // Note: -7.6 => -7
        auto const absoluteSampleIndexI = FastTruncateToArchInt(absoluteSampleIndexF + 0.5f);

        // Integral part - sample
        // Note: -7 % 3 == -1
        auto sampleIndexI = absoluteSampleIndexI % static_cast<decltype(absoluteSampleIndexI)>(SamplesCount);

        if (sampleIndexI < 0)
        {
            // Wrap around and anchor to the left sample
            sampleIndexI += SamplesCount - 1; // Includes shift to left
        }

        assert(sampleIndexI >= 0 && sampleIndexI <= SamplesCount);

        return mSamples[sampleIndexI].SampleValue;
    }

    /*
     * Gets the value linearly-interpolated between the two samples at the specified value,
     * which is assumed to be between zero (first sample) and one-Dx (last sample).
     * One is also fine, but that would repeat the last sample.
     */
    inline float GetLinearlyInterpolated(float x) const
    {
        // Fractional index in the sample array
        float const sampleIndexF = x / Dx;

        // Integral part
        auto const sampleIndexI = FastTruncateToArchInt(sampleIndexF);

        // Fractional part within sample index and the next sample index
        float sampleIndexDx = sampleIndexF - sampleIndexI;

        assert(sampleIndexI >= 0 && sampleIndexI <= SamplesCount);
        assert(sampleIndexDx >= 0.0f && sampleIndexDx <= 1.0f);

        return mSamples[sampleIndexI].SampleValue
            + mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue * sampleIndexDx;
    }

    /*
     * Gets the value linearly-interpolated between the two samples at the specified value,
     * assumed to be periodic around one.
     */
    inline float GetLinearlyInterpolatedPeriodic(float x) const
    {
        // Fractional absolute index in the (infinite) sample array
        float const absoluteSampleIndexF = x / Dx;

        // Integral part
        // Note: -7.6 => -7
        auto const absoluteSampleIndexI = FastTruncateToArchInt(absoluteSampleIndexF);

        // Integral part - sample
        // Note: -7 % 3 == -1
        auto sampleIndexI = absoluteSampleIndexI % static_cast<decltype(absoluteSampleIndexI)>(SamplesCount);

        // Fractional part within sample index and the next sample index
        float sampleIndexDx = absoluteSampleIndexF - absoluteSampleIndexI;

        if (x < 0.0f)
        {
            // Wrap around and anchor to the left sample
            sampleIndexI += SamplesCount - 1; // Includes shift to left
            sampleIndexDx += 1.0f;
        }

        assert(sampleIndexI >= 0 && sampleIndexI < SamplesCount);
        assert(sampleIndexDx >= 0.0f && sampleIndexDx <= 1.0f);

        return mSamples[sampleIndexI].SampleValue
            + mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue * sampleIndexDx;
    }

private:

    void PopulateSamples(std::function<float(float)> calculator)
    {
        // First sample
        float sampleValue = calculator(0.0f);
        mSamples[0].SampleValue = sampleValue;

        // Samples 1...SamplesCount-1
        float x = Dx;
        float previousValue = sampleValue;
        for (size_t i = 1; i < SamplesCount; ++i, x += Dx)
        {
            sampleValue = calculator(x);

            mSamples[i].SampleValue = sampleValue;
            mSamples[i - 1].SampleValuePlusOneMinusSampleValue = sampleValue - previousValue;

            previousValue = sampleValue;
        }

        // Assume periodic
        mSamples[SamplesCount - 1].SampleValuePlusOneMinusSampleValue = mSamples[0].SampleValue - previousValue;

        // Last (extra) sample
        mSamples[SamplesCount].SampleValue = previousValue;
        mSamples[SamplesCount].SampleValuePlusOneMinusSampleValue = 0.0f; // Never used
    }

    struct Sample
    {
        float SampleValue; // Value of this sample
        float SampleValuePlusOneMinusSampleValue; // Delta between next sample and this sample
    };

    // One extra sample to ease interpolation
    std::array<Sample, SamplesCount + 1> mSamples;

    static float constexpr Dx = 1.0f / SamplesCount;
};

extern PrecalculatedFunction<512> const PrecalcLoFreqSin;