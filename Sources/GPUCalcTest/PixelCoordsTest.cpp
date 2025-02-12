/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-01-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "PixelCoordsTest.h"

#include <GPUCalc/GPUCalculatorFactory.h>

#include <vector>

void PixelCoordsTest::InternalRun()
{
    auto calculator = GPUCalculatorFactory::GetInstance().CreatePixelCoordsCalculator(mDataPoints);

    std::vector<vec4f> results;
    results.resize(mDataPoints);

    calculator->Run(results.data());

    //
    // Verify
    //

    LogBuffer("results", results.data(), results.size());

    for (size_t i = 0; i < mDataPoints; ++i)
    {
        int iRow = static_cast<int>(i) / calculator->GetFrameSize().width;
        int iCol = static_cast<int>(i) - iRow * calculator->GetFrameSize().width;

        TEST_VERIFY(results[i].x == static_cast<float>(iCol) + 0.5f);
        TEST_VERIFY(results[i].y == static_cast<float>(iRow) + 0.5f);
    }
}