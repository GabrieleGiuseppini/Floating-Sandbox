/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "AddGPUCalculator.h"

#include <OpenGLCore/GameOpenGL.h>

#include <Core/Log.h>

AddGPUCalculator::AddGPUCalculator(
    std::unique_ptr<IOpenGLContext> openGLContext,
    IAssetManager const & assetManager,
    size_t dataPoints)
    : GPUCalculator(
        std::move(openGLContext),
        assetManager)
    , mDataPoints(dataPoints)
    , mFrameSize(0, 0) // Temporary
{
    assert(dataPoints > 0);

    GLuint tmpGLuint;

    //
    // Calculate geometry of buffers
    //

    assert(GameOpenGL::MaxViewportWidth > 0 && GameOpenGL::MaxViewportHeight > 0);
    assert(GameOpenGL::MaxTextureSize > 0);
    assert(GameOpenGL::MaxRenderbufferSize > 0);

    // Input textures and render buffer all are going to be the same size,
    // hence the max possible width we consider is the min of the three max widths
    int const maxWidth = std::min(
        std::min(GameOpenGL::MaxViewportWidth, GameOpenGL::MaxTextureSize),
        GameOpenGL::MaxRenderbufferSize);

    int const requiredNumberOfFloats = static_cast<int>(dataPoints) * 2;
    mWholeRows = requiredNumberOfFloats / (maxWidth * 4);
    int const remainderFloats = requiredNumberOfFloats % (maxWidth * 4);
    mRemainderCols = (remainderFloats / 4) + ((remainderFloats % 4) ? 1 : 0);

    if (mWholeRows == 0)
    {
        // Less than a width
        mFrameSize = ImageSize(mRemainderCols, 1);
    }
    else
    {
        // More than one full row
        mFrameSize = ImageSize(maxWidth, mWholeRows + (mRemainderCols > 0 ? 1 : 0));
    }

    mRemainderColsInputBufferByteOffset = mFrameSize.width * mWholeRows * 4 * sizeof(float);

    LogMessage(
        "AddGPUCalculator: FrameSize=", mFrameSize.width, "x", mFrameSize.height,
        ", WholeRows=", mWholeRows, ", RemainderCols=", mRemainderCols);


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
    // Initialize program
    //

    GetShaderManager().ActivateProgram<GPUCalcShaderSets::ProgramKind::Add>();

    GetShaderManager().SetTextureParameters<GPUCalcShaderSets::ProgramKind::Add>();


    //
    // Prepare input texture 0
    //

    glActiveTexture(GL_TEXTURE0);
    CheckOpenGLError();

    GLuint tmpTexture;
    glGenTextures(1, &tmpTexture);
    mInputTextures[0] = tmpTexture;

    glBindTexture(GL_TEXTURE_2D, *mInputTextures[0]);
    CheckOpenGLError();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mFrameSize.width, mFrameSize.height, 0, GL_RGBA, GL_FLOAT, nullptr);
    CheckOpenGLError();

    // Make sure we don't do any fancy filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    CheckOpenGLError();


    //
    // Prepare input texture 1
    //

    glActiveTexture(GL_TEXTURE1);
    CheckOpenGLError();

    glGenTextures(1, &tmpTexture);
    mInputTextures[1] = tmpTexture;

    glBindTexture(GL_TEXTURE_2D, *mInputTextures[1]);
    CheckOpenGLError();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mFrameSize.width, mFrameSize.height, 0, GL_RGBA, GL_FLOAT, nullptr);
    CheckOpenGLError();

    // Make sure we don't do any fancy filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    CheckOpenGLError();



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

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mVertexVBO);
    CheckOpenGLError();

    // Initialize buffer
    // - Quad NDC coords
    // - Input texture coords
    static vec4f quadVertices[6] = {
        {-1.0f, -1.0f, 0.0f, 0.0f}, // LB
        {-1.0f, 1.0f, 0.0f, 1.0f},  // LT
        {1.0f, -1.0f, 1.0f, 0.0f},  // RB
        {-1.0f, 1.0f, 0.0f, 1.0f},  // LT
        {1.0f, -1.0f, 1.0f, 0.0f},  // RB
        {1.0f, 1.0f, 1.0f, 1.0f}    // RT
    };

    // Upload buffer
    glBufferData(
        GL_ARRAY_BUFFER,
        4 * sizeof(float) * 6,
        quadVertices,
        GL_STATIC_DRAW);

    // Describe vertex attribute
    glVertexAttribPointer(
        static_cast<GLuint>(GPUCalcShaderSets::VertexAttributeKind::VertexShaderInput0),
        4,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(float),
        (void*)0);

    // Enable vertex attribute
    glEnableVertexAttribArray(static_cast<GLuint>(GPUCalcShaderSets::VertexAttributeKind::VertexShaderInput0));
}

void AddGPUCalculator::Run(
    vec2f const * a,
    vec2f const * b,
    vec2f * result)
{
    assert(nullptr != a);
    assert(nullptr != b);
    assert(nullptr != result);

    this->ActivateOpenGLContext();

    //
    // Upload input 0
    //

    glBindTexture(GL_TEXTURE_2D, *mInputTextures[0]);

   if (mWholeRows > 0)
    {
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,                              // Level
            0, 0,                           // X offset, Y offset
            mFrameSize.width, mWholeRows,   // Width, Height
            GL_RGBA, GL_FLOAT,
            a);

        CheckOpenGLError();
    }

    if (mRemainderCols > 0)
    {
        glTexSubImage2D(
           GL_TEXTURE_2D,
           0,                               // Level
           0, mWholeRows,                   // X offset, Y offset
           mRemainderCols, 1,               // Width, Height
           GL_RGBA, GL_FLOAT,
           &(a[mRemainderColsInputBufferByteOffset / sizeof(vec2f)]));

        CheckOpenGLError();
    }

    //
    // Upload input 1
    //

    glBindTexture(GL_TEXTURE_2D, *mInputTextures[1]);

    if (mWholeRows > 0)
    {
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,                              // Level
            0, 0,                           // X offset, Y offset
            mFrameSize.width, mWholeRows,   // Width, Height
            GL_RGBA, GL_FLOAT,
            b);

        CheckOpenGLError();
    }

    if (mRemainderCols > 0)
    {
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,                              // Level
            0, mWholeRows,                  // X offset, Y offset
            mRemainderCols, 1,              // Width, Height
            GL_RGBA, GL_FLOAT,
            &(b[mRemainderColsInputBufferByteOffset / sizeof(vec2f)]));

        CheckOpenGLError();
    }


    //
    // Draw
    //

    glDrawArrays(GL_TRIANGLES, 0, 6);
    CheckOpenGLError();


    //
    // Read
    //

    if (mWholeRows > 0)
    {
        glReadPixels(
            0, 0,
            mFrameSize.width, mWholeRows,
            GL_RGBA, GL_FLOAT,
            result);

        CheckOpenGLError();
    }

    if (mRemainderCols > 0)
    {
        glReadPixels(
            0, mWholeRows,
            mRemainderCols, 1,
            GL_RGBA, GL_FLOAT,
            &(result[mRemainderColsInputBufferByteOffset / sizeof(vec2f)]));

        CheckOpenGLError();
    }

    glFlush();
}