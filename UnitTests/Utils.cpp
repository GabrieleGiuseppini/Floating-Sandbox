#include "Utils.h"

::testing::AssertionResult ApproxEquals(float a, float b, float tolerance)
{
    if (abs(a - b) < tolerance)
    {
        return ::testing::AssertionSuccess();
    }
    else
    {
        return ::testing::AssertionFailure() << "Result " << a << " too different than expected value " << b;
    }
}