/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-11-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Simulation/SimulationParameters.h>

#include <Render/RenderContext.h>

struct ComputerCalibrationScore
{
    float NormalizedCPUScore; // 0.0 -> 1.0
    float NormalizedGfxScore; // 0.0 -> 1.0

    ComputerCalibrationScore(
        float normalizedCPUScore,
        float normalizedGfxScore)
        : NormalizedCPUScore(normalizedCPUScore)
        , NormalizedGfxScore(normalizedGfxScore)
    {}
};

class ComputerCalibrator
{
public:

    static ComputerCalibrationScore Calibrate();

    static void TuneGame(
        ComputerCalibrationScore const & score,
        SimulationParameters & simulationParameters,
        RenderContext & renderContext);

private:

    static float RunComputation();
};