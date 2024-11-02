/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-07-12
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ISliderCore.h"

#include <cassert>
#include <cmath>

template <typename TValue>
class IntegralLinearSliderCore final : public ISliderCore<TValue>
{
public:

    IntegralLinearSliderCore(
        TValue minValue,
        TValue maxValue)
        : mMinValue(minValue)
        , mMaxValue(maxValue)
    {
        //
        // Calculate number of ticks and tick size, i.e. value delta for each tick, as follows:
        //
        // NumberOfTicks * TickSize = Max - Min
        //
        // With: TickSize = 1/2**n
        //

        // Start with an approximate number of ticks
        float n = std::floor(std::log(100.0f / (maxValue - minValue)) / std::log(2.0f));
        mTickSize = 1.0f / std::pow(2.0f, n);

        // Now calculate the real number of ticks
        float numberOfTicks = std::ceil((maxValue - minValue) / mTickSize);
        mNumberOfTicks = static_cast<int>(numberOfTicks);

        // Re-adjust min: calc min at tick 0 (exclusive of offset), and offset to add to slider's value
        mValueOffset = static_cast<TValue>(std::floor(minValue / mTickSize) * mTickSize);
        assert(minValue >= mValueOffset);
        mValueAtTickZero = minValue - mValueOffset;
        assert(static_cast<float>(mValueAtTickZero) < mTickSize);

        // Store maximum tick value and maximum value (exclusive of offset) at that tick
        float theoreticalMaxValue = static_cast<float>(mValueOffset) + numberOfTicks * mTickSize;
        assert(theoreticalMaxValue - maxValue < mTickSize);
        (void)theoreticalMaxValue;
        mValueAtTickMax = maxValue - mValueOffset;
    }

    int GetNumberOfTicks() const override
    {
        return mNumberOfTicks;
    }

    TValue TickToValue(int tick) const override
    {
        TValue sliderValue;

        if (tick == 0)
        {
            sliderValue = mValueAtTickZero;
        }
        else if (tick == mNumberOfTicks)
        {
            sliderValue = mValueAtTickMax;
        }
        else
        {
            sliderValue = static_cast<TValue>(std::round(mTickSize * static_cast<float>(tick)));
        }

        return mValueOffset + sliderValue;
    }


    int ValueToTick(TValue value) const override
    {
        value -= mValueOffset;

        if (value <= mValueAtTickZero)
        {
            return 0;
        }
        else if (value >= mValueAtTickMax)
        {
            return mNumberOfTicks;
        }
        else
        {
            return static_cast<int>(floorf(value / mTickSize));
        }
    }

    TValue const & GetMinValue() const override
    {
        return mMinValue;
    }

    TValue const & GetMaxValue() const override
    {
        return mMaxValue;
    }

private:

    TValue const mMinValue;
    TValue const mMaxValue;

    float mTickSize;
    int mNumberOfTicks;

    TValue mValueOffset;
    TValue mValueAtTickZero; // Net of offset
    TValue mValueAtTickMax;  // Net of offset
};
