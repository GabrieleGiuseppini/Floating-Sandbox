/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-01-13
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "AddTest.h"

#include <GPUCalc/GPUCalculatorFactory.h>

#include <cmath>
#include <vector>

void AddTest::InternalRun()
{
    auto calculator = GPUCalculatorFactory::GetInstance().CreateAddCalculator(mDataPoints);

    //
    // Create inputs and outputs
    //

    size_t roundedUpDataPoints = mDataPoints + ((mDataPoints % 2) ? 1 : 0);

    std::vector<vec2f> a(roundedUpDataPoints, vec2f::zero());
    std::vector<vec2f> b(roundedUpDataPoints, vec2f::zero());

    for (size_t i = 0; i < mDataPoints; ++i)
    {
        a[i] = vec2f(
            static_cast<float>(i),
            static_cast<float>(i) / 100.0f);

        b[i] = vec2f(
            static_cast<float>(i) + 10000.0f,
            (static_cast<float>(i) / 100.0f) + 10000.0f);
    }

    std::vector<vec2f> results(roundedUpDataPoints, vec2f::zero());

    calculator->Run(
        a.data(),
        b.data(),
        results.data());

    //
    // Verify
    //

    LogBuffer("results", results.data(), mDataPoints);

    float maxDelta = 0.0f;
    for (size_t i = 0; i < mDataPoints; ++i)
    {
        vec2f expectedResult = a[i] + b[i];

        TEST_VERIFY_FLOAT_EQ(results[i].x, expectedResult.x);
        TEST_VERIFY_FLOAT_EQ(results[i].y, expectedResult.y);

        if (std::abs(results[i].x - expectedResult.x) > maxDelta)
            maxDelta = std::abs(results[i].x - expectedResult.x);
        if (std::abs(results[i].y - expectedResult.y) > maxDelta)
            maxDelta = std::abs(results[i].y - expectedResult.y);
    }

    LogMessage("MaxDelta=", maxDelta);
}