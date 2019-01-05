/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TestGPUCalcContext.h"

#include <GameOpenGL/GameOpenGL.h>

TestGPUCalcContext::TestGPUCalcContext(
    std::unique_ptr<IOpenGLContext> openGLContext,
    size_t dataPoints)
    : GPUCalcContext(std::move(openGLContext))
    , mDataPoints(dataPoints)
{
    //
    // Initialize this context
    //

    this->ActivateOpenGLContext();

    // Set polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Clear canvas - and stencil buffer
    // TODOTEST
    glClearColor(0.8f, 0.0f, 0.0f, 1.0f);
    glClearStencil(0x00);
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void TestGPUCalcContext::Add(
    vec2f const * a,
    vec2f const * b,
    vec2f * result)
{
    assert(nullptr != a);
    assert(nullptr != b);
    assert(nullptr != result);

    this->ActivateOpenGLContext();

    // TODO
}