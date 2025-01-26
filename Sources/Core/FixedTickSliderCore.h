/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-11-24
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ISliderCore.h"

class FixedTickSliderCore final : public ISliderCore<float>
{
public:

    FixedTickSliderCore(
        float tickSize,
        float minValue,
        float maxValue);

    virtual int GetNumberOfTicks() const override;

    virtual float TickToValue(int tick) const override;

    virtual int ValueToTick(float value) const override;

    virtual float const & GetMinValue() const override;

    virtual float const & GetMaxValue() const override;

private:

    float const mTickSize;
    float const mMinValue;
    float const mMaxValue;

    int mNumberOfTicks;
};
