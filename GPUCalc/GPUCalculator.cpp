/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GPUCalculator.h"

#include <GameOpenGL/GameOpenGL.h>

GPUCalculator::GPUCalculator(
    std::unique_ptr<IOpenGLContext> openGLContext,
    std::filesystem::path const & shadersRootDirectory)
    : mOpenGLContext(std::move(openGLContext))
{
    //
    // Initialize OpenGL for this context
    //

    ActivateOpenGLContext();

    GameOpenGL::InitOpenGL();

    //
    // Initialize shader manager
    //

    mShaderManager = ShaderManager<GPUCalcShaderManagerTraits>::CreateInstance(shadersRootDirectory);
}