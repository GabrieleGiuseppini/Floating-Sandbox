/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ExponentialSliderCore.h"

#include <cassert>

/*
 * This slider is two exponentials, meeting at the center (number of ticks=beta).
 * The upper exponential starts slowly from beta and reaches the maximum with an increasing slope.
 * Its definition is:
 *     value = a*exp(b * (tick - beta))
 * The lower exponential goes down from beta slowly and reaches the minimum with an increasing slope.
 * Its definition is:
 *     value = a - b*exp(gamma * (beta - tick))
 */


template<typename T>
constexpr T NumberOfTicks = 100;

template<typename T>
constexpr T Beta = NumberOfTicks<T> / 2;

constexpr float Gamma = 0.01f;

ExponentialSliderCore::ExponentialSliderCore(
    float minValue,
    float zeroValue,
    float maxValue)
    : mMinValue(minValue)
    , mZeroValue(zeroValue)
    , mMaxValue(maxValue)
    , mLowerA((zeroValue * exp(Gamma * Beta<float>) - minValue) / (exp(Gamma * Beta<float>) - 1.0f))
    , mLowerB((zeroValue - minValue) / (exp(Gamma * Beta<float>) - 1.0f))
    , mUpperA(zeroValue)
    , mUpperB(logf(maxValue / zeroValue) / Beta<float>)
{
}

int ExponentialSliderCore::GetNumberOfTicks() const
{
    return NumberOfTicks<int>;
}

float ExponentialSliderCore::TickToValue(int tick) const
{
    if (tick < NumberOfTicks<int> / 2.0f)
    {
        // Lower part
        return mLowerA - mLowerB * exp(Gamma * (NumberOfTicks<float> / 2.0f - static_cast<float>(tick)));
    }
    else
    {
        // Upper part
        if (tick == NumberOfTicks<int>)
            return mMaxValue;
        else
            return mUpperA * exp(mUpperB * (static_cast<float>(tick) - NumberOfTicks<float> / 2.0f));
    }
}

int ExponentialSliderCore::ValueToTick(float value) const
{
    if (value < mZeroValue)
    {
        // Lower part
        float x = Beta<float> - log((mLowerA - value) / mLowerB) / Gamma;
        return static_cast<int>(roundf(x));
    }
    else
    {
        // Upper part
        float x = (log(value) - log(mUpperA)) / mUpperB;
        return static_cast<int>(roundf(Beta<float> + x));
    }
}