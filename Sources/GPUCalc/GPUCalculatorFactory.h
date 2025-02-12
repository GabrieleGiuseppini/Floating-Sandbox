/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "AddGPUCalculator.h"
#include "GPUCalcShaderSets.h"
#include "GPUCalculator.h"
#include "IOpenGLContext.h"
#include "PixelCoordsGPUCalculator.h"

#include <Core/IAssetManager.h>

#include <cassert>
#include <functional>
#include <memory>

class GPUCalculatorFactory
{
public:

    static GPUCalculatorFactory & GetInstance()
    {
        CheckInitialized();

        return *instance;
    }

    static void Initialize(
        std::function<std::unique_ptr<IOpenGLContext>()> openGLContextFactory,
        IAssetManager const & assetManager);

    std::unique_ptr<PixelCoordsGPUCalculator> CreatePixelCoordsCalculator(size_t dataPoints);

    std::unique_ptr<AddGPUCalculator> CreateAddCalculator(size_t dataPoints);

private:

    explicit GPUCalculatorFactory(
        std::function<std::unique_ptr<IOpenGLContext>()> openGLContextFactory,
        IAssetManager const & assetManager)
        : mOpenGLContextFactory(std::move(openGLContextFactory))
        , mAssetManager(assetManager)
    {}

    static void CheckInitialized();

    std::function<std::unique_ptr<IOpenGLContext>()> mOpenGLContextFactory;
    IAssetManager const & mAssetManager;

    static GPUCalculatorFactory * instance;
};