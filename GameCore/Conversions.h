/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-02-25
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

inline float MetersToFeet(float metersValue) noexcept
{
    return metersValue * 3.28084f;
}

inline float CelsiusToFahrenheit(float celsiusValue) noexcept
{
    return (celsiusValue - 273.15f) * 9.0f / 5.0f + 32.0f;
}

inline float PascalToPsi(float pascalValue) noexcept
{
    return pascalValue * 0.0001450377f;
}