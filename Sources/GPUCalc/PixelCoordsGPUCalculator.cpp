/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PixelCoordsGPUCalculator.h"

#include <OpenGLCore/GameOpenGL.h>

#include <Core/Log.h>

PixelCoordsGPUCalculator::PixelCoordsGPUCalculator(
    std::unique_ptr<IOpenGLContext> openGLContext,
    IAssetManager const & assetManager,
    size_t dataPoints)
    : GPUCalculator(
        std::move(openGLContext),
        assetManager)
    , mDataPoints(dataPoints)
    , mFrameSize(CalculateRequiredRenderBufferSize(dataPoints))
{
    LogMessage("PixelCoordsGPUCalculator: FrameSize=", mFrameSize.width, "x", mFrameSize.height);

    GLuint tmpGLuint;


    //
    // Initialize this context
    //

    this->ActivateOpenGLContext();

    // Set viewport size
    glViewport(0, 0, mFrameSize.width, mFrameSize.height);
    CheckOpenGLError();

    // Set polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Disable stenciling, blend, and depth test
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_STENCIL_TEST);



    //
    // Create framebuffer and bind it
    //

    // Create frame buffer
    glGenFramebuffers(1, &tmpGLuint);
    mFramebuffer = tmpGLuint;

    // Bind frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, *mFramebuffer);
    CheckOpenGLError();

    //
    // Create color render buffer
    //

    glGenRenderbuffers(1, &tmpGLuint);
    mColorRenderbuffer = tmpGLuint;

    glBindRenderbuffer(GL_RENDERBUFFER, *mColorRenderbuffer);
    CheckOpenGLError();

    // Allocate render buffer with 32-bit float RGBA format
    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_RGBA32F,
        mFrameSize.width,
        mFrameSize.height);
    CheckOpenGLError();

    // Attach color buffer to FBO
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, *mColorRenderbuffer);
    CheckOpenGLError();

    // Verify framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw GameException("Framebuffer is not complete");
    }

    // Clear canvas
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);



    //
    // Create VBO and populate it with whole NDC world
    //

    glGenBuffers(1, &tmpGLuint);
    mVertexVBO = tmpGLuint;

    // Use program
    GetShaderManager().ActivateProgram<GPUCalcShaderSets::ProgramKind::PixelCoords>();

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mVertexVBO);
    CheckOpenGLError();

    // Initialize buffer
    static vec2f quadVertices[6] = {
        {-1.0f, -1.0f},
        {-1.0f, 1.0f},
        {1.0f, -1.0f},
        {-1.0f, 1.0f},
        {1.0f, -1.0f},
        {1.0f, 1.0f}
    };

    // Upload buffer
    glBufferData(
        GL_ARRAY_BUFFER,
        2 * sizeof(float) * 6,
        quadVertices,
        GL_STATIC_DRAW);

    // Describe vertex attribute
    glVertexAttribPointer(
        static_cast<GLuint>(GPUCalcShaderSets::VertexAttributeKind::VertexShaderInput0),
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        (void*)0);

    // Enable vertex attribute
    glEnableVertexAttribArray(static_cast<GLuint>(GPUCalcShaderSets::VertexAttributeKind::VertexShaderInput0));
}

void PixelCoordsGPUCalculator::Run(vec4f * result)
{
    assert(nullptr != result);

    this->ActivateOpenGLContext();

    //
    // Draw
    //

    glDrawArrays(GL_TRIANGLES, 0, 6);
    CheckOpenGLError();


    //
    // Read
    //

    int wholeRows = static_cast<int>(mDataPoints) / mFrameSize.width;
    if (wholeRows > 0)
    {
        glReadPixels(0, 0, mFrameSize.width, wholeRows, GL_RGBA, GL_FLOAT, result);
        CheckOpenGLError();
    }

    int reminderCols = static_cast<int>(mDataPoints) % mFrameSize.width;
    if (reminderCols > 0)
    {
        glReadPixels(0, wholeRows, reminderCols, 1, GL_RGBA, GL_FLOAT, result + sizeof(vec4f) * wholeRows * mFrameSize.width);
        CheckOpenGLError();
    }

    glFlush();
}