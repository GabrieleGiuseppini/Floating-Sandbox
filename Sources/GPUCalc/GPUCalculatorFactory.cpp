/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GPUCalculatorFactory.h"

#include <Core/GameException.h>

#include <cassert>

GPUCalculatorFactory * GPUCalculatorFactory::instance;

void GPUCalculatorFactory::Initialize(
    std::function<std::unique_ptr<IOpenGLContext>()> openGLContextFactory,
    IAssetManager const & assetManager)
{
    assert(instance == nullptr);
    instance = new GPUCalculatorFactory(
        std::move(openGLContextFactory),
        assetManager);
}

std::unique_ptr<PixelCoordsGPUCalculator> GPUCalculatorFactory::CreatePixelCoordsCalculator(size_t dataPoints)
{
    CheckInitialized();

    return std::unique_ptr<PixelCoordsGPUCalculator>(
        new PixelCoordsGPUCalculator(
            mOpenGLContextFactory(),
            mAssetManager,
            dataPoints));
}

std::unique_ptr<AddGPUCalculator> GPUCalculatorFactory::CreateAddCalculator(size_t dataPoints)
{
    CheckInitialized();

    return std::unique_ptr<AddGPUCalculator>(
        new AddGPUCalculator(
            mOpenGLContextFactory(),
            mAssetManager,
            dataPoints));
}

void GPUCalculatorFactory::CheckInitialized()
{
    if (instance == nullptr)
        throw GameException("GPU Calculator Factory's OpenGL Context Factory has not been initialized");
}