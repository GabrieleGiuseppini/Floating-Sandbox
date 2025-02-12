/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GPUCalculator.h"
#include "IOpenGLContext.h"
#include "ShaderTraits.h"

#include "AddGPUCalculator.h"
#include "PixelCoordsGPUCalculator.h"

#include <cassert>
#include <filesystem>
#include <functional>
#include <memory>

class GPUCalculatorFactory
{
public:

    static GPUCalculatorFactory & GetInstance()
    {
        static GPUCalculatorFactory * instance = new GPUCalculatorFactory();

        return *instance;
    }

    void Initialize(
        std::function<std::unique_ptr<IOpenGLContext>()> openGLContextFactory,
        std::filesystem::path const & shadersRootDirectory);

    std::unique_ptr<PixelCoordsGPUCalculator> CreatePixelCoordsCalculator(size_t dataPoints);

    std::unique_ptr<AddGPUCalculator> CreateAddCalculator(size_t dataPoints);

private:

    GPUCalculatorFactory()
    {}

    void CheckInitialized();

    std::function<std::unique_ptr<IOpenGLContext>()> mOpenGLContextFactory;
    std::filesystem::path mShadersRootDirectory;
};