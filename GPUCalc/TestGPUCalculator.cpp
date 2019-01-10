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
        Width,
        Height);
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