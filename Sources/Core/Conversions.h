/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-02-25
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameMath.h"

class Conversions final
{
public:

    static float MeterToFoot(float metersValue) noexcept
    {
        return metersValue * 3.28084f;
    }

    static float CelsiusToFahrenheit(float celsiusValue) noexcept
    {
        return (celsiusValue - 273.15f) * 9.0f / 5.0f + 32.0f;
    }

    static float PascalToPsi(float pascalValue) noexcept
    {
        return pascalValue * 0.0001450377f;
    }

    static float KilogramToPound(float kilogramValue) noexcept
    {
        return kilogramValue * 2.20462f;
    }

    static float KilogramToMetricTon(float kilogramValue) noexcept
    {
        return kilogramValue / 1000.0f;
    }

    static float KilogramToUscsTon(float kilogramValue) noexcept
    {
        return kilogramValue * 2.20462f / 2000.0f;
    }

    static float RadiansCWToDegrees(float angleValue) noexcept
    {
        return -angleValue / Pi<float> *180.0f;
    }

    template<typename T>
    static T KmhToMs(T const & value) noexcept
    {
        return value * 1000.0f / 3600.0f;
    }
};