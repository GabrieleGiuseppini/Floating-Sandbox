/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GPUCalcContext.h"

#include <GameOpenGL/GameOpenGL.h>

GPUCalcContext::GPUCalcContext(std::unique_ptr<IOpenGLContext> openGLContext)
    : mOpenGLContext(std::move(openGLContext))
{
    //
    // Initialize OpenGL for this context
    //

    ActivateOpenGLContext();

    GameOpenGL::InitOpenGL();
}