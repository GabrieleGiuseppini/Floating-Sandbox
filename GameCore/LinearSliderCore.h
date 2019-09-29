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

    virtual int GetNumberOfTicks() const override;

    virtual float TickToValue(int tick) const override;

    virtual int ValueToTick(float value) const override;

    virtual float const & GetMinValue() const override;

    virtual float const & GetMaxValue() const override;

private:

    float const mMinValue;
    float const mMaxValue;

    float mTickSize;
    int mNumberOfTicks;

    float mValueOffset;
    float mValueAtTickZero; // Net of offset
    float mValueAtTickMax;  // Net of offset
};
