/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "View.h"

#include <GameCore/GameException.h>
#include <GameCore/Log.h>
#include <GameCore/SysSpecifics.h>

namespace ShipBuilder {

View::View(
    DisplayLogicalSize initialDisplaySize,
    int logicalToPhysicalPixelFactor,
    std::function<void()> swapRenderBuffersFunction,
    ResourceLocator const & resourceLocator)
    : mViewModel(
        initialDisplaySize,
        logicalToPhysicalPixelFactor)
    , mShaderManager()
    , mSwapRenderBuffersFunction(swapRenderBuffersFunction)
    //////////////////////////////////
    , mHasBackgroundTexture(false)
    , mHasStructuralRenderColorTexture(false)
{
    //
    // Initialize global OpenGL settings
    //

    // Set anti-aliasing for lines
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // Enable blending for alpha transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    // Disable depth test
    glDisable(GL_DEPTH_TEST);

    //
    // Load shader manager
    //

    mShaderManager = ShaderManager<ShaderManagerTraits>::CreateInstance(resourceLocator.GetShipBuilderShadersRootPath());

    //
    // Initialize Background texture
    //

    {
        GLuint tmpGLuint;

        //
        // Texture
        //

        mShaderManager->ActivateTexture<ProgramParameterType::BackgroundTexture>();

        // Create texture OpenGL handle
        glGenTextures(1, &tmpGLuint);
        mBackgroundTextureOpenGLHandle = tmpGLuint;

        // Configure texture
        glBindTexture(GL_TEXTURE_2D, *mBackgroundTextureOpenGLHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CheckOpenGLError();

        // Set texture in program
        mShaderManager->ActivateProgram<ProgramType::BackgroundTexture>();
        mShaderManager->SetTextureParameters<ProgramType::BackgroundTexture>();

        //
        // VAO
        //

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mBackgroundTextureVAO = tmpGLuint;
        glBindVertexArray(*mBackgroundTextureVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mBackgroundTextureVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mBackgroundTextureVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::TextureNdc));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::TextureNdc), 4, GL_FLOAT, GL_FALSE, sizeof(TextureNdcVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize Structural Render Color texture
    //

    {
        GLuint tmpGLuint;

        //
        // Texture
        //

        mShaderManager->ActivateTexture<ProgramParameterType::Texture1>();

        // Create texture OpenGL handle
        glGenTextures(1, &tmpGLuint);
        mStructuralRenderColorTextureOpenGLHandle = tmpGLuint;

        // Configure texture
        glBindTexture(GL_TEXTURE_2D, *mStructuralRenderColorTextureOpenGLHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        CheckOpenGLError();

        // Set texture in programs
        mShaderManager->ActivateProgram<ProgramType::Texture>();
        mShaderManager->SetTextureParameters<ProgramType::Texture>();

        //
        // VAO
        //

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mStructuralRenderColorTextureVAO = tmpGLuint;
        glBindVertexArray(*mStructuralRenderColorTextureVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mStructuralRenderColorTextureVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mStructuralRenderColorTextureVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Texture));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Texture), 4, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }
}

void View::UploadBackgroundTexture(RgbaImageData && texture)
{
    //
    // Upload texture
    //

    // Bind texture
    mShaderManager->ActivateTexture<ProgramParameterType::BackgroundTexture>();
    glBindTexture(GL_TEXTURE_2D, *mBackgroundTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadTexture(std::move(texture));

    //
    // Create vertices (in NDC)
    //

    // The texture coordinate at the bottom of the quad obeys the texture's aspect ratio,
    // rather than the screen's

    float const textureBottom =
        -(static_cast<float>(texture.Size.Height) - static_cast<float>(mViewModel.GetDisplayPhysicalSize().height))
            / static_cast<float>(mViewModel.GetDisplayPhysicalSize().height);

    std::array<TextureNdcVertex, 4> vertexBuffer;

    // Top-left
    vertexBuffer[0] = TextureNdcVertex(
        vec2f(-1.0f, 1.0f),
        vec2f(0.0f, 1.0f));

    // Bottom-left
    vertexBuffer[1] = TextureNdcVertex(
        vec2f(-1.0f, -1.0f),
        vec2f(0.0f, textureBottom));

    // Top-right
    vertexBuffer[2] = TextureNdcVertex(
        vec2f(1.0f, 1.0f),
        vec2f(1.0f, 1.0f));

    // Bottom-right
    vertexBuffer[3] = TextureNdcVertex(
        vec2f(1.0f, -1.0f),
        vec2f(1.0f, textureBottom));

    //
    // Upload vertices
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mBackgroundTextureVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(TextureNdcVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Remember we have this texture
    //

    mHasBackgroundTexture = true;
}


void View::UploadStructuralRenderColorTexture(RgbaImageData const & texture)
{
    //
    // Upload texture
    //

    // Bind texture
    mShaderManager->ActivateTexture<ProgramParameterType::Texture1>();
    glBindTexture(GL_TEXTURE_2D, *mStructuralRenderColorTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadTexture(texture);

    //
    // Create vertices
    //

    float const fWidth = static_cast<float>(texture.Size.Width);
    float const fHeight = static_cast<float>(texture.Size.Height);

    std::array<TextureVertex, 4> vertexBuffer;

    // Top-left
    vertexBuffer[0] = TextureVertex(
        vec2f(0.0f, 0.0f),
        vec2f(0.0f, 1.0f));

    // Bottom-left
    vertexBuffer[1] = TextureVertex(
        vec2f(0.0f, fHeight),
        vec2f(0.0f, 0.0f));

    // Top-right
    vertexBuffer[2] = TextureVertex(
        vec2f(fWidth, 0.0f),
        vec2f(1.0f, 1.0f));

    // Bottom-right
    vertexBuffer[3] = TextureVertex(
        vec2f(fWidth, fHeight),
        vec2f(1.0f, 0.0f));

    //
    // Upload vertices
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mStructuralRenderColorTextureVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(TextureVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Remember we have this texture
    //

    mHasStructuralRenderColorTexture = true;
}

void View::Render()
{
    //
    // Initialize
    //

    // Set viewport and scissor
    glViewport(0, 0, mViewModel.GetDisplayPhysicalSize().width, mViewModel.GetDisplayPhysicalSize().height);

    // Background texture
    if (mHasBackgroundTexture)
    {
        // Bind VAO
        glBindVertexArray(*mBackgroundTextureVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::BackgroundTexture>();

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CheckOpenGLError();

        glBindVertexArray(0);
    }
    else
    {
        // Clear canvas
        glClearColor(0.985f, 0.985f, 0.985f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // Structural Render Color texture
    if (mHasStructuralRenderColorTexture)
    {
        // Bind this texture
        //glBindTexture(GL_TEXTURE_2D, *mStructuralRenderColorTextureOpenGLHandle);

        // Set texture in shader
        // TODOHERE
        //mShaderManager->ActivateTexture<ProgramParameterType::Texture1>();
        //mShaderManager->SetTextureParameters<ProgramType::Texture>();

        // Bind VAO
        glBindVertexArray(*mStructuralRenderColorTextureVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::Texture>();

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    // Flip the back buffer onto the screen
    mSwapRenderBuffersFunction();
}

///////////////////////////////////////////////////////////////////////////////

void View::RefreshOrthoMatrix()
{
    auto const orthoMatrix = mViewModel.GetOrthoMatrix();

    mShaderManager->ActivateProgram<ProgramType::Texture>();
    mShaderManager->SetProgramParameter<ProgramType::Texture, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);
}

}