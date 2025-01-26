/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-11-24
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FixedTickSliderCore.h"

#include <cassert>

FixedTickSliderCore::FixedTickSliderCore(
    float tickSize,
    float minValue,
    float maxValue)
    : mTickSize(tickSize)
    , mMinValue(minValue)
    , mMaxValue(maxValue)
{
    assert(tickSize > 0.0f);
    assert(maxValue >= minValue);

    // Calculate number of ticks
    mNumberOfTicks = static_cast<int>((maxValue - minValue) / tickSize) + 1;
}

int FixedTickSliderCore::GetNumberOfTicks() const
{
    return mNumberOfTicks;
}

float FixedTickSliderCore::TickToValue(int tick) const
{
    return std::min(mMinValue + mTickSize * tick, mMaxValue);
}

int FixedTickSliderCore::ValueToTick(float value) const
{
    return static_cast<int>((value - mMinValue) / mTickSize);
}

float const & FixedTickSliderCore::GetMinValue() const
{
    return mMinValue;
}

float const & FixedTickSliderCore::GetMaxValue() const
{
    return mMaxValue;
}
