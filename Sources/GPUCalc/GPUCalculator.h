/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GPUCalcShaderSets.h"
#include "IOpenGLContext.h"

#include <OpenGLCore/ShaderManager.h>

#include <Core/IAssetManager.h>

#include <cassert>
#include <memory>

/*
 * Base class of task-specific calculators that perform calculations on the GPU.
 */
class GPUCalculator
{
protected:

    GPUCalculator(
        std::unique_ptr<IOpenGLContext> openGLContext,
        IAssetManager const & assetManager);

    static ImageSize CalculateRequiredRenderBufferSize(size_t pixels);
    static ImageSize CalculateRequiredTextureSize(size_t pixels);

    void ActivateOpenGLContext()
    {
        assert(!!mOpenGLContext);

        mOpenGLContext->Activate();
    }

    ShaderManager<GPUCalcShaderSets::ShaderSet> & GetShaderManager()
    {
        return *mShaderManager;
    }

private:

    std::unique_ptr<IOpenGLContext> const mOpenGLContext;

    std::unique_ptr<ShaderManager<GPUCalcShaderSets::ShaderSet>> mShaderManager;
};