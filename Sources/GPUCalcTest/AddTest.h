/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-01-13
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "TestCase.h"

#include <string>

class AddTest : public TestCase
{
public:

    AddTest(size_t dataPoints)
        : TestCase("Add " + std::to_string(dataPoints))
        , mDataPoints(dataPoints)
    {}

protected:

    virtual void InternalRun() override;

private:

    size_t const mDataPoints;
};
