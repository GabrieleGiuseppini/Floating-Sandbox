/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GPUCalculator.h"

#include <Core/IAssetManager.h>
#include <Core/Vectors.h>

/*
 * Simple calculator that adds two arrays of vec2's.
 *
 * For test purposes.
 */
class AddGPUCalculator : public GPUCalculator
{
public:

    void Run(
        vec2f const * a,
        vec2f const * b,
        vec2f * result);

private:

    friend class GPUCalculatorFactory;

    AddGPUCalculator(
        std::unique_ptr<IOpenGLContext> openGLContext,
        IAssetManager const & assetManager,
        size_t dataPoints);

    ImageSize CalculateRequiredFrameSize(size_t dataPoints)
    {
        //
        // Input textures and render buffer all have the same size,
        // hence we need to consider the min of all of them.
        //

        assert(GameOpenGL::MaxViewportWidth > 0 && GameOpenGL::MaxViewportHeight > 0);
        assert(GameOpenGL::MaxTextureSize > 0);
        assert(GameOpenGL::MaxRenderbufferSize > 0);

        // Each pixel has four components
        int const requiredNumberOfPixels = static_cast<int>(dataPoints) / 2;

        int const maxWidth = std::min(
            std::min(GameOpenGL::MaxViewportWidth, GameOpenGL::MaxTextureSize),
            GameOpenGL::MaxRenderbufferSize);

        int numberOfRows = requiredNumberOfPixels / maxWidth;
        if (numberOfRows == 0)
        {
            // Less than one row
            return ImageSize(requiredNumberOfPixels, 1);
        }
        else
        {
            bool hasExtraPoints = (requiredNumberOfPixels % maxWidth) != 0;
            return ImageSize(maxWidth, numberOfRows + (hasExtraPoints ? 1 : 0));
        }
    }

private:

    size_t const mDataPoints;

    ImageSize mFrameSize;
    int mWholeRows;
    int mRemainderCols;
    size_t mRemainderColsInputBufferByteOffset;

    GameOpenGLVBO mVertexVBO;
    GameOpenGLTexture mInputTextures[2];
    GameOpenGLFramebuffer mFramebuffer;
    GameOpenGLRenderbuffer mColorRenderbuffer;
};