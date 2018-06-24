/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ExponentialSliderCore.h"

#include <cassert>

ExponentialSliderCore::ExponentialSliderCore(
    float minValue,
    float maxValue)
    : mMinValue(minValue)
    , mMaxValue(maxValue)
{
}

int ExponentialSliderCore::GetNumberOfTicks() const
{
    return 100;
}

float ExponentialSliderCore::TickToValue(int tick) const
{
    // TODO: formula is based off max~=3.0 (2.98..)
    // min + ((exp(tick/70)-1)/2.3)^3.4

    float realValue = (exp(static_cast<float>(tick) / 70.0f) - 1.0f) / 2.3f;
    realValue = mMinValue + powf(realValue, 3.4f);
    return realValue;
}

int ExponentialSliderCore::ValueToTick(float value) const
{
    value = powf(value - mMinValue, 1.0f / 3.4f);
    float tick = 70.0f * log((2.3f * value) + 1.0f);
    return static_cast<int>(roundf(tick));
}
