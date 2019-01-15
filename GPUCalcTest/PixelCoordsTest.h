/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-01-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "TestCase.h"

#include <string>

class PixelCoordsTest : public TestCase
{
public:

    PixelCoordsTest(size_t dataPoints)
        : TestCase("PixelCoords " + std::to_string(dataPoints))
        , mDataPoints(dataPoints)
    {}

protected:

    virtual void InternalRun() override;

private:

    size_t const mDataPoints;
};
