/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IOpenGLContext.h"
#include "ShaderTraits.h"

#include <GameOpenGL/ShaderManager.h>

#include <cassert>
#include <filesystem>
#include <memory>

/*
 * Base class of task-specific calculators that perform calculations on the GPU.
 */
class GPUCalculator
{
protected:

    GPUCalculator(
        std::unique_ptr<IOpenGLContext> openGLContext,
        std::filesystem::path const & shadersRootDirectory);

    void ActivateOpenGLContext()
    {
        assert(!!mOpenGLContext);

        mOpenGLContext->Activate();
    }

    ShaderManager<GPUCalcShaderManagerTraits> & GetShaderManager()
    {
        return *mShaderManager;
    }

private:

    std::unique_ptr<IOpenGLContext> const mOpenGLContext;

    std::unique_ptr<ShaderManager<GPUCalcShaderManagerTraits>> mShaderManager;
};