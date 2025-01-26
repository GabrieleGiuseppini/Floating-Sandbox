/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ExponentialSliderCore.h"

#include <cassert>

/*
 * This slider is two exponentials, meeting at the center. The number of ticks on each side is
 * beta, but they share a tick in the middle, hence the number of ticks is 2*beta - 1, and tick
 * values are [0 -> beta-1] and [beta-1 -> 2*beta - 2].
 *
 * The upper exponential starts slowly from zeroValue @ tick=beta-1 and reaches maxValue @ tick=2*beta-2 with an increasing slope.
 * Its definition is:
 *     value = a + exp(b * (tick - (beta-1)))
 *     With:
 *          zeroValue (@ beta-1) = a + 1
 *          maxValue (@ 2 * beta - 2) = a + exp(b * (beta - 1))
 *
 * The lower exponential goes down slowly from zeroValue @ tick=beta-1 and reaches minValue @ tick=0 with an increasing slope.
 * Its definition is:
 *     value = a - exp(b * ((beta-1) - tick))
 *     With:
 *          minValue (@ 0) = a - exp(b * (beta - 1))
 *          zeroValue (@ beta-1) = a - 1
 */


template<typename T>
constexpr T Beta = 50; // Number of ticks on either side, one overlapping in center

ExponentialSliderCore::ExponentialSliderCore(
    float minValue,
    float zeroValue,
    float maxValue)
    : mMinValue(minValue)
    , mZeroValue(zeroValue)
    , mMaxValue(maxValue)
    , mLowerA(zeroValue + 1.0f)
    , mLowerB(logf(zeroValue + 1.0f - minValue) / (Beta<float> - 1.0f))
    , mUpperA(zeroValue - 1.0f)
    , mUpperB(logf(maxValue - zeroValue + 1.0f) / (Beta<float> - 1.0f))
{
    assert(maxValue > zeroValue && zeroValue > minValue);
}

int ExponentialSliderCore::GetNumberOfTicks() const
{
    return 2 * Beta<int> - 1;
}

float ExponentialSliderCore::TickToValue(int tick) const
{
    if (tick < Beta<int>)
    {
        // Lower part
        if (tick == 0)
            return mMinValue;
        else
            return mLowerA - exp(mLowerB * static_cast<float>(Beta<float> - 1.0f - tick));
    }
    else
    {
        // Upper part
        if (tick >= GetNumberOfTicks() - 1)
            return mMaxValue;
        else
            return mUpperA + exp(mUpperB * (static_cast<float>(tick) - (Beta<float> - 1.0f)));
    }
}

int ExponentialSliderCore::ValueToTick(float value) const
{
    if (value < mZeroValue)
    {
        // Lower part
        float x = Beta<float> - 1.0f - log(mLowerA - value) / mLowerB;
        return static_cast<int>(roundf(x));
    }
    else
    {
        // Upper part
        float x = log(value - mUpperA) / mUpperB + Beta<float> - 1.0f;
        return static_cast<int>(roundf(x));
    }
}