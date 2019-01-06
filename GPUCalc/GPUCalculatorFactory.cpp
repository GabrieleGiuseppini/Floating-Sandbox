/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GPUCalculatorFactory.h"

#include <GameCore/GameException.h>

void GPUCalculatorFactory::Initialize(
    std::function<std::unique_ptr<IOpenGLContext>()> openGLContextFactory,
    std::filesystem::path const & shadersRootDirectory)
{
    if (!!mOpenGLContextFactory)
        throw GameException("GPU Calculator Factory's OpenGL Context Factory has already been initialized");

    mShadersRootDirectory = shadersRootDirectory;
    mOpenGLContextFactory = std::move(openGLContextFactory);
}

std::unique_ptr<TestGPUCalculator> GPUCalculatorFactory::CreateTestCalculator(size_t dataPoints)
{
    CheckInitialized();

    return std::unique_ptr<TestGPUCalculator>(
        new TestGPUCalculator(
            mOpenGLContextFactory(),
            mShadersRootDirectory,
            dataPoints));
}

void GPUCalculatorFactory::CheckInitialized()
{
    if (!mOpenGLContextFactory)
        throw GameException("GPU Calculator Factory's OpenGL Context Factory has not been initialized");
}