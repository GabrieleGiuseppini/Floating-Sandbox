/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TestGPUCalculator.h"

#include <GameOpenGL/GameOpenGL.h>

// TODOTEST
constexpr int Width = 40;
constexpr int Height = 40;

TestGPUCalculator::TestGPUCalculator(
    std::unique_ptr<IOpenGLContext> openGLContext,
    std::filesystem::path const & shadersRootDirectory,
    size_t dataPoints)
    : GPUCalculator(
        std::move(openGLContext),
        shadersRootDirectory)
    , mDataPoints(dataPoints)
{
    GLuint tmpGLuint;

    //
    // Initialize this context
    //

    this->ActivateOpenGLContext();

    // Set viewport size
    glViewport(0, 0, Width, Height);

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

    if (!!gglFramebuffer_Core)
    {
        // Create frame buffer
        gglFramebuffer_Core->GenFramebuffers(1, &tmpGLuint);
        mFramebuffer = tmpGLuint;

        // Bind frame buffer
        gglFramebuffer_Core->BindFramebuffer(gglFramebuffer_Core->FRAMEBUFFER, *mFramebuffer);
        CheckOpenGLError();

        //
        // Create color render buffer
        //

        gglFramebuffer_Core->GenRenderbuffers(1, &tmpGLuint);
        mColorRenderbuffer = tmpGLuint;

        gglFramebuffer_Core->BindRenderbuffer(gglFramebuffer_Core->RENDERBUFFER, *mColorRenderbuffer);
        CheckOpenGLError();

        // Allocate render buffer with 32-bit float RGBA format
        gglFramebuffer_Core->RenderbufferStorage(
            gglFramebuffer_Core->RENDERBUFFER,
            !!gglTextureFloat_Core ? gglTextureFloat_Core->RGBA32F : gglTextureFloat_ARB->RGBA32F_ARB,
            Width,
            Height);
        CheckOpenGLError();

        // Attach color buffer to FBO
        gglFramebuffer_Core->FramebufferRenderbuffer(gglFramebuffer_Core->FRAMEBUFFER, gglFramebuffer_Core->COLOR_ATTACHMENT0, gglFramebuffer_Core->RENDERBUFFER, *mColorRenderbuffer);
        CheckOpenGLError();

        // Verify framebuffer is complete
        if (gglFramebuffer_Core->CheckFramebufferStatus(gglFramebuffer_Core->FRAMEBUFFER) != gglFramebuffer_Core->FRAMEBUFFER_COMPLETE)
        {
            throw GameException("Framebuffer is not complete");
        }
    }
    else
    {
        assert(!!gglFramebuffer_EXT);

        // Create frame buffer
        gglFramebuffer_EXT->GenFramebuffersEXT(1, &tmpGLuint);
        mFramebuffer = tmpGLuint;

        // Bind frame buffer
        gglFramebuffer_EXT->BindFramebufferEXT(gglFramebuffer_EXT->FRAMEBUFFER_EXT, *mFramebuffer);
        CheckOpenGLError();

        //
        // Create color render buffer
        //

        gglFramebuffer_EXT->GenRenderbuffersEXT(1, &tmpGLuint);
        mColorRenderbuffer = tmpGLuint;

        gglFramebuffer_EXT->BindRenderbufferEXT(gglFramebuffer_EXT->RENDERBUFFER_EXT, *mColorRenderbuffer);
        CheckOpenGLError();

        // Allocate render buffer with 32-bit float RGBA format
        gglFramebuffer_EXT->RenderbufferStorageEXT(
            gglFramebuffer_EXT->RENDERBUFFER_EXT,
            !!gglTextureFloat_Core ? gglTextureFloat_Core->RGBA32F : gglTextureFloat_ARB->RGBA32F_ARB,
            Width,
            Height);
        CheckOpenGLError();

        // Attach color buffer to FBO
        gglFramebuffer_EXT->FramebufferRenderbufferEXT(gglFramebuffer_EXT->FRAMEBUFFER_EXT, gglFramebuffer_EXT->COLOR_ATTACHMENT0_EXT, gglFramebuffer_EXT->RENDERBUFFER_EXT, *mColorRenderbuffer);
        CheckOpenGLError();

        // Verify framebuffer is complete
        if (gglFramebuffer_EXT->CheckFramebufferStatusEXT(gglFramebuffer_EXT->FRAMEBUFFER_EXT) != gglFramebuffer_EXT->FRAMEBUFFER_COMPLETE_EXT)
        {
            throw GameException("Framebuffer is not complete");
        }
    }

    // Clear canvas
    // TODOTEST
    glClearColor(0.123f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);



    //
    // Create VBO and populate it with whole NDC world
    //

    glGenBuffers(1, &tmpGLuint);
    mVertexVBO = tmpGLuint;

    // Use program
    GetShaderManager().ActivateProgram<GPUCalcProgramType::Test>();

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
        static_cast<GLuint>(GPUCalcVertexAttributeType::VertexCoords),
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        (void*)0);

    // Enable vertex attribute
    glEnableVertexAttribArray(static_cast<GLuint>(GPUCalcVertexAttributeType::VertexCoords));

}

void TestGPUCalculator::Add(
    vec2f const * a,
    vec2f const * b,
    vec2f * result)
{
    assert(nullptr != a);
    assert(nullptr != b);
    assert(nullptr != result);

    this->ActivateOpenGLContext();

    // Draw
    glDrawArrays(GL_TRIANGLES, 0, 6);
    CheckOpenGLError();

    // Read
    // TODOTEST
    //GLubyte pixels[4 * 256 * 256];
    //glReadPixels(0, 0, 256, 256, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
    std::unique_ptr<float[]> pixels = std::make_unique<float[]>(4 * Width * Height);
    glReadPixels(0, 0, Width, Height, GL_BGRA, GL_FLOAT, pixels.get());
    CheckOpenGLError();

    // Flush all pending commands
    glFlush();
}