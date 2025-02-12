/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GPUCalculator.h"

#include <algorithm>

GPUCalculator::GPUCalculator(
    std::unique_ptr<IOpenGLContext> openGLContext,
    std::filesystem::path const & shadersRootDirectory)
    : mOpenGLContext(std::move(openGLContext))
{
    ActivateOpenGLContext();

    //
    // Initialize shader manager
    //

    mShaderManager = ShaderManager<GPUCalcShaderManagerTraits>::CreateInstance(shadersRootDirectory);
}

ImageSize GPUCalculator::CalculateRequiredRenderBufferSize(size_t pixels)
{
    assert(GameOpenGL::MaxViewportWidth > 0 && GameOpenGL::MaxViewportHeight > 0);
    assert(GameOpenGL::MaxRenderbufferSize > 0);

    int const maxWidth = std::min(GameOpenGL::MaxViewportWidth, GameOpenGL::MaxRenderbufferSize);

    int numberOfRows = static_cast<int>(pixels) / maxWidth;
    if (numberOfRows == 0)
    {
        // Less than one row
        return ImageSize(static_cast<int>(pixels), 1);
    }
    else
    {
        bool hasExtraPixels = (static_cast<int>(pixels) % maxWidth) != 0;
        return ImageSize(maxWidth, numberOfRows + (hasExtraPixels ? 1 : 0));
    }
}

ImageSize GPUCalculator::CalculateRequiredTextureSize(size_t pixels)
{
    assert(GameOpenGL::MaxTextureSize > 0);

    int numberOfRows = static_cast<int>(pixels) / GameOpenGL::MaxTextureSize;
    if (numberOfRows == 0)
    {
        // Less than one row
        return ImageSize(static_cast<int>(pixels), 1);
    }
    else
    {
        bool hasExtraPixels = static_cast<int>(pixels) % GameOpenGL::MaxTextureSize != 0;
        return ImageSize(GameOpenGL::MaxTextureSize, numberOfRows + (hasExtraPixels ? 1 : 0));
    }
}