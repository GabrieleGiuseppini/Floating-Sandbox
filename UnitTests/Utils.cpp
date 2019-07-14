#include "Utils.h"

void DoSomething(simdpp::float32<4> const & /*v*/)
{
}

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