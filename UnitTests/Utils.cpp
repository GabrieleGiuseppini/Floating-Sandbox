#include "Utils.h"

#include <cmath>

::testing::AssertionResult ApproxEquals(float a, float b, float tolerance)
{
    if (std::abs(a - b) < tolerance)
    {
        return ::testing::AssertionSuccess();
    }
    else
    {
        return ::testing::AssertionFailure() << "Result " << a << " too different than expected value " << b;
    }
}

float DivideByTwo(float value)
{
    return value / 2.0f;
}

StructuralMaterial MakeTestStructuralMaterial(std::string name, rgbColor colorKey)
{
    return StructuralMaterial(
        colorKey,
        name,
        rgbaColor::zero());
}

ElectricalMaterial MakeTestElectricalMaterial(std::string name, rgbColor colorKey, bool isInstanced)
{
    return ElectricalMaterial(
        colorKey,
        name,
        rgbColor::zero(),
        isInstanced);
}