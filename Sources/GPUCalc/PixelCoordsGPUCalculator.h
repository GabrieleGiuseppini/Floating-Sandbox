/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GPUCalculator.h"

#include <Core/IAssetManager.h>
#include <Core/Vectors.h>


/*
 * Simple calculator that outputs the fragment coordinates passed to the fragment shader.
 *
 * For test purposes.
 */
class PixelCoordsGPUCalculator : public GPUCalculator
{
public:

    void Run(vec4f * result);

    ImageSize const & GetFrameSize() const
    {
        return mFrameSize;
    }

private:

    friend class GPUCalculatorFactory;

    PixelCoordsGPUCalculator(
        std::unique_ptr<IOpenGLContext> openGLContext,
        IAssetManager const & assetManager,
        size_t dataPoints);

private:

    size_t const mDataPoints;
    ImageSize const mFrameSize;

    GameOpenGLVBO mVertexVBO;
    GameOpenGLFramebuffer mFramebuffer;
    GameOpenGLRenderbuffer mColorRenderbuffer;
};