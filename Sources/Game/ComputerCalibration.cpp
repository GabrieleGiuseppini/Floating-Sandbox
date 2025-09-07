/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-11-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ComputerCalibration.h"

#include <OpenGLCore/GameOpenGL.h>

#include <Core/GameMath.h>
#include <Core/Log.h>
#include <Core/ThreadManager.h>
#include <Core/Vectors.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <random>
#include <thread>

ComputerCalibrationScore ComputerCalibrator::Calibrate()
{
    //
    // CPU calibration
    //

    auto const startTime = std::chrono::steady_clock::now();

    std::uint64_t iterationCount;
    for (iterationCount = 0;
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() < 1000;
        ++iterationCount)
    {
        float val = RunComputation();
        if (val > 2.0f) // Impossible
            break;
    }

    float const normalizedCpuScore = SmoothStep(
        0.0f,
        100.0f,
        static_cast<float>(iterationCount));

    LogMessage("CPU Calibration: iterationCount=", iterationCount, " score=", normalizedCpuScore);

    //
    // Graphics calibration
    //

    float const normalizedGraphicsScore =
        SmoothStep(0.0f, 16384.0f, static_cast<float>(GameOpenGL::MaxRenderbufferSize))
        * SmoothStep(0.0f, 4.0f, static_cast<float>(GameOpenGL::MaxSupportedOpenGLVersionMajor));

    LogMessage("Graphics Calibration: score=", normalizedGraphicsScore);

    return ComputerCalibrationScore(normalizedCpuScore, normalizedGraphicsScore);
}

void ComputerCalibrator::TuneGame(
    ComputerCalibrationScore const & score,
    SimulationParameters & simulationParameters,
    RenderContext & renderContext)
{
    //
    // This is the algorithm that decides settings based on the computer
    // performance
    //

    if (score.NormalizedCPUScore < 0.65f
        || score.NormalizedGfxScore < 0.1f)
    {
        // Ocean detail requires CPU as well as GPU
        renderContext.SetOceanRenderDetail(OceanRenderDetailType::Basic);

        // Flames require CPU as well as GPU
        simulationParameters.MaxBurningParticlesPerShip = 112;
    }

    if (score.NormalizedGfxScore < 0.1f)
    {
        // These all require GPU

        simulationParameters.UnderwaterPlantsDensity = 64.0f;

        renderContext.SetCloudRenderDetail(CloudRenderDetailType::Basic);
        renderContext.SetLandRenderDetail(LandRenderDetailType::Basic);
        renderContext.SetDoCrepuscularGradient(false);
        renderContext.SetShipDepthDarkeningSensitivity(0.0f);
    }

    if (score.NormalizedGfxScore < 0.1f
        || ThreadManager::GetNumberOfProcessors() == 1)
    {
        // These require huge vertex attribute uploads, hence best when rendering is multi-threaded
        renderContext.SetHeatRenderMode(HeatRenderModeType::None);
    }
}

float ComputerCalibrator::RunComputation()
{
    size_t constexpr SampleSize = 100000;

    //
    // Prepare input
    //

    std::seed_seq seed_seq({ 1, 242, 19730528 });
    std::ranlux48_base randomEngine(seed_seq);
    std::uniform_real_distribution<float> randomUniformDistribution(0.0f, 1.0f);

    std::vector<vec2f> inputData;
    inputData.reserve(SampleSize);
    for (size_t i = 0; i < SampleSize; ++i)
    {
        inputData.emplace_back(
            randomUniformDistribution(randomEngine),
            randomUniformDistribution(randomEngine));
    }

    //
    // Calculate
    //

    float accum = 0.0f;
    for (size_t i = 0; i < SampleSize; ++i)
    {
        auto const n1 = inputData[i % SampleSize].normalise().length();
        auto const n2 = SmoothStep(0.0f, 1.0f, n1);
        if (n2 < 0.5f)
            accum += 1.0;
        else
            accum += -1.0f;
    }

    return std::abs(accum / static_cast<float>(SampleSize));
}