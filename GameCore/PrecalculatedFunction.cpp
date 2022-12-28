/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-05-11
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PrecalculatedFunction.h"

#include <cmath>

PrecalculatedFunction<512> const PrecalcLoFreqSin(
    [](float x)
    {
        return std::sin(2.0f * Pi<float> * x);
    });