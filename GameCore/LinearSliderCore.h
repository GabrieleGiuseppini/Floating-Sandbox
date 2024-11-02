/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ISliderCore.h"

class LinearSliderCore final : public ISliderCore<float>
{
public:

    LinearSliderCore(
        float minValue,
        float maxValue);

    int GetNumberOfTicks() const override;

    float TickToValue(int tick) const override;

    int ValueToTick(float value) const override;

    float const & GetMinValue() const override;

    float const & GetMaxValue() const override;

private:

    float const mMinValue;
    float const mMaxValue;

    float mTickSize;
    int mNumberOfTicks;

    float mValueOffset;
    float mValueAtTickZero; // Net of offset
    float mValueAtTickMax;  // Net of offset
};
