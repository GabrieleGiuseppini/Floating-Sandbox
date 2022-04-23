/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-02-25
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameMath.h"

inline float MeterToFoot(float metersValue) noexcept
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

inline float KilogramToPound(float kilogramValue) noexcept
{
    return kilogramValue * 2.20462f;
}

inline float KilogramToMetricTon(float kilogramValue) noexcept
{
    return kilogramValue / 1000.0f;
}

inline float KilogramToUscsTon(float kilogramValue) noexcept
{
    return kilogramValue * 2.20462f / 2000.0f;
}

inline float RadiansCWToDegrees(float angleValue) noexcept
{
    return -angleValue / Pi<float> * 180.0f;
}