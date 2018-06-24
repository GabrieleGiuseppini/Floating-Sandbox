/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cmath>
#include <functional>
#include <string>

/*
 * This interface is implemented by logics that drive mapping between
 * sliders and float values.
 */
class ISliderCore
{
public:

    virtual ~ISliderCore()
    {}

    virtual int GetNumberOfTicks() const = 0;

    virtual float TickToValue(int tick) const = 0;

    virtual int ValueToTick(float value) const = 0;
};
