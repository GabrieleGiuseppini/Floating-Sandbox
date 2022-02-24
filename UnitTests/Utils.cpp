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

StructuralMaterial MakeTestStructuralMaterial(std::string name)
{
    return StructuralMaterial(
        rgbColor(1, 2, 3),
        name,
        rgbColor::zero());
}

ElectricalMaterial MakeTestElectricalMaterial(std::string name)
{
    return ElectricalMaterial(
        rgbColor(1, 2, 3), // ColorKey
        name,
        rgbColor::zero(),
        false);
}
