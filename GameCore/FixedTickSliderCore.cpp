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

    // Calculate number of ticks
    mNumberOfTicks = static_cast<int>((maxValue - minValue) / tickSize);
}

int FixedTickSliderCore::GetNumberOfTicks() const
{
    return mNumberOfTicks;
}

float FixedTickSliderCore::TickToValue(int tick) const
{
    return mMinValue + mTickSize * tick;
}

int FixedTickSliderCore::ValueToTick(float value) const
{
    return static_cast<int>((value - mMinValue) / mTickSize);
}