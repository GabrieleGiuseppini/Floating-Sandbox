/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "View.h"

#include <GameCore/GameException.h>
#include <GameCore/Log.h>
#include <GameCore/SysSpecifics.h>

#include <vector>

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
    // Activate texture units
    //

    mShaderManager->ActivateTexture<ProgramParameterType::Texture1>();

    //
    // Initialize test VAO
    //

    {
        GLuint tmpGLuint;

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mTestVAO = tmpGLuint;
        glBindVertexArray(*mTestVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mTestVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mTestVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Test));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Test), 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize Structural Render Color Texture VAO
    //

    {
        GLuint tmpGLuint;

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mStructuralRenderTextureColorVAO = tmpGLuint;
        glBindVertexArray(*mStructuralRenderTextureColorVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mStructuralRenderTextureColorVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mStructuralRenderTextureColorVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Texture));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Texture), 4, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);

        // Create texture OpenGL handle
        glGenTextures(1, &tmpGLuint);
        mStructuralRenderTextureOpenGLHandle = tmpGLuint;

        // Configure texture
        glBindTexture(GL_TEXTURE_2D, *mStructuralRenderTextureOpenGLHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        CheckOpenGLError();
    }
}

void View::UploadStructuralRenderColorTexture(RgbaImageData const & texture)
{
    //
    // Upload texture
    //

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mStructuralRenderTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadTexture(texture);

    //
    // Create vertices
    //

    mStructuralRenderColorTextureVertexBuffer.clear();

    // TODOHERE
}

void View::Render()
{
    //
    // Initialize
    //

    // Set viewport and scissor
    glViewport(0, 0, mViewModel.GetDisplayPhysicalSize().width, mViewModel.GetDisplayPhysicalSize().height);

    // Clear canvas
    // TODO: this becomes drawing the background texture
    glClearColor(0.3f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // TODOTEST
    // Draw test quad
    {
        std::vector<vec2f> buffer;

        // TODO: for this test we're using display physical size in lieu of WorkSpace size
        float const h = static_cast<float>(mViewModel.GetDisplayPhysicalSize().height);
        float const w = static_cast<float>(mViewModel.GetDisplayPhysicalSize().width);
        buffer.emplace_back(0.0f, h);
        buffer.emplace_back(0.0f, 0.0f);
        buffer.emplace_back(w, h);
        buffer.emplace_back(0.0f, 0.0f);
        buffer.emplace_back(w, h);
        buffer.emplace_back(w, 0.0f);

        glBindBuffer(GL_ARRAY_BUFFER, *mTestVBO);
        glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(vec2f), buffer.data(), GL_DYNAMIC_DRAW);
        CheckOpenGLError();
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(*mTestVAO);
        mShaderManager->ActivateProgram<ProgramType::Test>();
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(buffer.size()));
        CheckOpenGLError();
        glBindVertexArray(0);
    }

    // Structural Render Color Texture
    if (!mStructuralRenderColorTextureVertexBuffer.empty())
    {
        // Activate generic Texture program
        mShaderManager->ActivateProgram<ProgramType::Texture>();

        // Bind this texture
        glBindTexture(GL_TEXTURE_2D, *mStructuralRenderTextureOpenGLHandle);

        // Set texture in shader
        mShaderManager->SetTextureParameters<ProgramType::Texture>();

        // Bind VAO
        glBindVertexArray(*mStructuralRenderTextureColorVAO);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(mStructuralRenderColorTextureVertexBuffer.size()));
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

    mShaderManager->ActivateProgram<ProgramType::Test>();
    mShaderManager->SetProgramParameter<ProgramType::Test, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::Texture>();
    mShaderManager->SetProgramParameter<ProgramType::Texture, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);
}

}