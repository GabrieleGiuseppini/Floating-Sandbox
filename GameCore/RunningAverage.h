/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-12
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <array>
#include <cassert>

/*
 * This class maintains a running average of a scalar quantity.
 */
template<size_t NumSamples>
class RunningAverage
{

public:

    RunningAverage()
    {
        Reset();
    }

    // Make sure we don't introduce unnecessary copies inadvertently
    RunningAverage(RunningAverage const & other) = delete;
    RunningAverage & operator=(RunningAverage const & other) = delete;

    float Update(float newValue)
    {
        mCurrentAverage -= mSamples[mCurrentSampleHead];

        float newSample = newValue / static_cast<float>(NumSamples);
        mCurrentAverage += newSample;

        mSamples[mCurrentSampleHead] = newSample;

        ++mCurrentSampleHead;
        if (mCurrentSampleHead >= NumSamples)
            mCurrentSampleHead = 0;

        return mCurrentAverage;
    }

    void Reset()
    {
        mSamples.fill(0.0f);
        mCurrentSampleHead = 0;
        mCurrentAverage = 0.0f;
    }

private:

    std::array<float, NumSamples> mSamples;
    size_t mCurrentSampleHead; // Oldest
    float mCurrentAverage;
};
