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
    ShipSpaceSize initialShipSpaceSize,
    DisplayLogicalSize initialDisplaySize,
    int logicalToPhysicalPixelFactor,
    std::function<void()> swapRenderBuffersFunction,
    ResourceLocator const & resourceLocator)
    : mViewModel(
        initialShipSpaceSize,
        initialDisplaySize,
        logicalToPhysicalPixelFactor)
    , mShaderManager()
    , mSwapRenderBuffersFunction(swapRenderBuffersFunction)
    //////////////////////////////////
    , mHasBackgroundTexture(false)
    , mHasStructuralTexture(false)
    , mHasElectricalTexture(false)
    , mRopeCount(false)
    , mOtherLayersOpacity(0.75f)
    , mIsGridEnabled(false)
    , mCircleOverlayCenter(0, 0) // Will be overwritten
    , mCircleOverlayColor(vec3f::zero()) // Will be overwritten
    , mHasCircleOverlay(false)
    , mRectOverlayRect({0, 0}, {1, 1}) // Will be overwritten
    , mRectOverlayColor(vec3f::zero()) // Will be overwritten
    , mHasRectOverlay(false)
    //////////////////////////////////
    , mPrimaryLayer(LayerType::Structural)
{
    //
    // Initialize global OpenGL settings
    //

    // Set anti-aliasing for lines
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // Set line width
    glLineWidth(2.0f);

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

    // Set texture samplers in programs
    mShaderManager->ActivateProgram<ProgramType::Texture>();
    mShaderManager->SetTextureParameters<ProgramType::Texture>();
    mShaderManager->ActivateProgram<ProgramType::TextureNdc>();
    mShaderManager->SetTextureParameters<ProgramType::TextureNdc>();

    //
    // Initialize Background texture and VAO
    //

    {
        GLuint tmpGLuint;

        //
        // Texture
        //

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
    // Initialize canvas VAO
    //

    {
        GLuint tmpGLuint;

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mCanvasVAO = tmpGLuint;
        glBindVertexArray(*mCanvasVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mCanvasVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mCanvasVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Canvas));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Canvas), 4, GL_FLOAT, GL_FALSE, sizeof(CanvasVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize Structural texture and VAO
    //

    {
        GLuint tmpGLuint;

        //
        // Texture
        //

        // Create texture OpenGL handle
        glGenTextures(1, &tmpGLuint);
        mStructuralTextureOpenGLHandle = tmpGLuint;

        // Configure texture
        glBindTexture(GL_TEXTURE_2D, *mStructuralTextureOpenGLHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        CheckOpenGLError();

        //
        // VAO
        //

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mStructuralTextureVAO = tmpGLuint;
        glBindVertexArray(*mStructuralTextureVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mStructuralTextureVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mStructuralTextureVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Texture));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Texture), 4, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize Electrical texture and VAO
    //

    {
        GLuint tmpGLuint;

        //
        // Texture
        //

        // Create texture OpenGL handle
        glGenTextures(1, &tmpGLuint);
        mElectricalTextureOpenGLHandle = tmpGLuint;

        // Configure texture
        glBindTexture(GL_TEXTURE_2D, *mElectricalTextureOpenGLHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        CheckOpenGLError();

        //
        // VAO
        //

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mElectricalTextureVAO = tmpGLuint;
        glBindVertexArray(*mElectricalTextureVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mElectricalTextureVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mElectricalTextureVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Texture));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Texture), 4, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize Ropes VAO
    //

    {
        GLuint tmpGLuint;

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mRopesVAO = tmpGLuint;
        glBindVertexArray(*mRopesVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mRopesVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mRopesVBO);
        static_assert(sizeof(RopeVertex) == (2 + 4) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Rope1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Rope1), 2, GL_FLOAT, GL_FALSE, sizeof(RopeVertex), (void *)(0));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Rope2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Rope2), 4, GL_FLOAT, GL_FALSE, sizeof(RopeVertex), (void *)(2 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize Grid
    //

    {
        GLuint tmpGLuint;

        //
        // VAO
        //

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mGridVAO = tmpGLuint;
        glBindVertexArray(*mGridVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mGridVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mGridVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Grid));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Grid), 4, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize circle overlay VAO
    //

    {
        GLuint tmpGLuint;

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mCircleOverlayVAO = tmpGLuint;
        glBindVertexArray(*mCircleOverlayVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mCircleOverlayVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mCircleOverlayVBO);
        static_assert(sizeof(RectOverlayVertex) == (4 + 3) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::CircleOverlay1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::CircleOverlay1), 4, GL_FLOAT, GL_FALSE, sizeof(CircleOverlayVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::CircleOverlay2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::CircleOverlay2), 3, GL_FLOAT, GL_FALSE, sizeof(CircleOverlayVertex), (void *)(4 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize rect overlay VAO
    //

    {
        GLuint tmpGLuint;

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mRectOverlayVAO = tmpGLuint;
        glBindVertexArray(*mRectOverlayVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mRectOverlayVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mRectOverlayVBO);
        static_assert(sizeof(RectOverlayVertex) == (4 + 3) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::RectOverlay1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::RectOverlay1), 4, GL_FLOAT, GL_FALSE, sizeof(RectOverlayVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::RectOverlay2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::RectOverlay2), 3, GL_FLOAT, GL_FALSE, sizeof(RectOverlayVertex), (void *)(4 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }
}

void View::EnableVisualGrid(bool doEnable)
{
    mIsGridEnabled = doEnable;
}

void View::UploadBackgroundTexture(RgbaImageData && texture)
{
    //
    // Upload texture
    //

    // Bind texture
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
        -(static_cast<float>(texture.Size.height) - static_cast<float>(mViewModel.GetDisplayPhysicalSize().height))
            / static_cast<float>(mViewModel.GetDisplayPhysicalSize().height);

    std::array<TextureNdcVertex, 4> vertexBuffer;

    // Bottom-left
    vertexBuffer[0] = TextureNdcVertex(
        vec2f(-1.0f, -1.0f),
        vec2f(0.0f, textureBottom));

    // Top-left
    vertexBuffer[1] = TextureNdcVertex(
        vec2f(-1.0f, 1.0f),
        vec2f(0.0f, 1.0f));

    // Bottom-right
    vertexBuffer[2] = TextureNdcVertex(
        vec2f(1.0f, -1.0f),
        vec2f(1.0f, textureBottom));

    // Top-right
    vertexBuffer[3] = TextureNdcVertex(
        vec2f(1.0f, 1.0f),
        vec2f(1.0f, 1.0f));

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

void View::UploadStructuralLayerVisualizationTexture(RgbaImageData const & texture)
{
    //
    // Upload texture
    //

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mStructuralTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadTexture(texture);

    //
    // Create vertices
    //

    float const shipWidth = static_cast<float>(mViewModel.GetShipSize().width);
    float const shipHeight = static_cast<float>(mViewModel.GetShipSize().height);

    std::array<TextureVertex, 4> vertexBuffer;

    // Bottom-left
    vertexBuffer[0] = TextureVertex(
        vec2f(0.0f, 0.0f),
        vec2f(0.0f, 0.0f));

    // Top-left
    vertexBuffer[1] = TextureVertex(
        vec2f(0.0f, shipHeight),
        vec2f(0.0f, 1.0f));

    // Bottom-right
    vertexBuffer[2] = TextureVertex(
        vec2f(shipWidth, 0.0f),
        vec2f(1.0f, 0.0f));

    // Top-right
    vertexBuffer[3] = TextureVertex(
        vec2f(shipWidth, shipHeight),
        vec2f(1.0f, 1.0f));

    //
    // Upload vertices
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mStructuralTextureVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(TextureVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Remember we have this texture
    //

    mHasStructuralTexture = true;
}

void View::UpdateStructuralLayerVisualizationTexture(
    rgbaColor const * regionPixels,
    int xOffset,
    int yOffset,
    int width,
    int height)
{
    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mStructuralTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture region
    GameOpenGL::UploadTextureRegion(
        regionPixels,
        xOffset,
        yOffset,
        width,
        height);
}

void View::UploadElectricalLayerVisualizationTexture(RgbaImageData const & texture)
{
    //
    // Upload texture
    //

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mElectricalTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadTexture(texture);

    //
    // Create vertices
    //

    float const fWidth = static_cast<float>(texture.Size.width);
    float const fHeight = static_cast<float>(texture.Size.height);

    std::array<TextureVertex, 4> vertexBuffer;

    // Bottom-left
    vertexBuffer[0] = TextureVertex(
        vec2f(0.0f, 0.0f),
        vec2f(0.0f, 0.0f));

    // Top-left
    vertexBuffer[1] = TextureVertex(
        vec2f(0.0f, fHeight),
        vec2f(0.0f, 1.0f));

    // Bottom-right
    vertexBuffer[2] = TextureVertex(
        vec2f(fWidth, 0.0f),
        vec2f(1.0f, 0.0f));

    // Top-right
    vertexBuffer[3] = TextureVertex(
        vec2f(fWidth, fHeight),
        vec2f(1.0f, 1.0f));

    //
    // Upload vertices
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mElectricalTextureVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(TextureVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Remember we have this texture
    //

    mHasElectricalTexture = true;
}

void View::UpdateElectricalLayerVisualizationTexture(
    rgbaColor const * regionPixels,
    int xOffset,
    int yOffset,
    int width,
    int height)
{
    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mElectricalTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture region
    GameOpenGL::UploadTextureRegion(
        regionPixels,
        xOffset,
        yOffset,
        width,
        height);
}

void View::RemoveElectricalLayerVisualizationTexture()
{
    mHasElectricalTexture = false;
}

void View::UploadRopesLayerVisualization(std::vector<RopeElement> const & ropeElements)
{
    //
    // Create vertices
    //

    std::vector<RopeVertex> vertexBuffer;
    vertexBuffer.reserve(ropeElements.size());

    for (auto const & e : ropeElements)
    {
        vertexBuffer.emplace_back(
            vec2f(
                static_cast<float>(e.StartCoords.x) + 0.5f,
                static_cast<float>(e.StartCoords.y) + 0.5f),
            e.RenderColor.toVec4f());

        vertexBuffer.emplace_back(
            vec2f(
                static_cast<float>(e.EndCoords.x) + 0.5f,
                static_cast<float>(e.EndCoords.y) + 0.5f),
            e.RenderColor.toVec4f());
    }

    //
    // Upload vertices
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mRopesVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(RopeVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Remember we have ropes
    //

    mRopeCount = ropeElements.size();
}

void View::RemoveRopesLayerVisualization()
{
    mRopeCount = 0;
}

void View::UploadCircleOverlay(
    ShipSpaceCoordinates const & center,
    OverlayMode mode)
{
    // Store center
    mCircleOverlayCenter = center;

    // Store color
    mCircleOverlayColor = GetOverlayColor(mode);

    // Update overlay
    UpdateCircleOverlay();

    mHasCircleOverlay = true;
}

void View::RemoveCircleOverlay()
{
    assert(mHasCircleOverlay);

    mHasCircleOverlay = false;
}

void View::UploadRectOverlay(
    ShipSpaceRect const & rect,
    OverlayMode mode)
{
    // Store rect
    mRectOverlayRect = rect;

    // Store color
    mRectOverlayColor = GetOverlayColor(mode);

    // Update overlay
    UpdateRectOverlay();

    mHasRectOverlay = true;
}

void View::RemoveRectOverlay()
{
    assert(mHasRectOverlay);

    mHasRectOverlay = false;
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
        // Set this texture in the shader's sampler
        mShaderManager->ActivateTexture<ProgramParameterType::BackgroundTextureUnit>();
        glBindTexture(GL_TEXTURE_2D, *mBackgroundTextureOpenGLHandle);

        // Bind VAO
        glBindVertexArray(*mBackgroundTextureVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::TextureNdc>();

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CheckOpenGLError();
    }
    else
    {
        // Just clear canvas
        glClearColor(0.985f, 0.985f, 0.985f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // Canvas
    {
        // Bind VAO
        glBindVertexArray(*mCanvasVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::Canvas>();

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CheckOpenGLError();
    }

    // Ropes, unless they're primary layer
    if (mRopeCount > 0 && mPrimaryLayer != LayerType::Ropes)
    {
        RenderRopes();
    }

    // Structural texture
    if (mHasStructuralTexture)
    {
        // Set this texture in the shader's sampler
        mShaderManager->ActivateTexture<ProgramParameterType::TextureUnit1>();
        glBindTexture(GL_TEXTURE_2D, *mStructuralTextureOpenGLHandle);

        // Bind VAO
        glBindVertexArray(*mStructuralTextureVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::Texture>();

        // Set opacity
        mShaderManager->SetProgramParameter<ProgramType::Texture, ProgramParameterType::Opacity>(
            mPrimaryLayer == LayerType::Structural ? 1.0f : mOtherLayersOpacity);

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CheckOpenGLError();
    }

    // Electrical texture
    if (mHasElectricalTexture)
    {
        // Set this texture in the shader's sampler
        mShaderManager->ActivateTexture<ProgramParameterType::TextureUnit1>();
        glBindTexture(GL_TEXTURE_2D, *mElectricalTextureOpenGLHandle);

        // Bind VAO
        glBindVertexArray(*mElectricalTextureVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::Texture>();

        // Set opacity
        mShaderManager->SetProgramParameter<ProgramType::Texture, ProgramParameterType::Opacity>(
            mPrimaryLayer == LayerType::Electrical ? 1.0f : mOtherLayersOpacity);

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CheckOpenGLError();
    }

    // Ropes, but only when they're primary layer
    if (mRopeCount > 0 && mPrimaryLayer == LayerType::Ropes)
    {
        RenderRopes();
    }

    // Grid
    if (mIsGridEnabled)
    {
        // Bind VAO
        glBindVertexArray(*mGridVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::Grid>();

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CheckOpenGLError();
    }

    // Circle overlay
    if (mHasCircleOverlay)
    {
        // Bind VAO
        glBindVertexArray(*mCircleOverlayVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::CircleOverlay>();

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CheckOpenGLError();
    }

    // Rect overlay
    if (mHasRectOverlay)
    {
        // Bind VAO
        glBindVertexArray(*mRectOverlayVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::RectOverlay>();

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CheckOpenGLError();
    }

    // Unbind VAOs
    glBindVertexArray(0);

    // Flip the back buffer onto the screen
    mSwapRenderBuffersFunction();
}

///////////////////////////////////////////////////////////////////////////////

void View::OnViewModelUpdated()
{
    UpdateCanvas();

    UpdateGrid();

    UpdateCircleOverlay();

    UpdateRectOverlay();

    //
    // Ortho matrix
    //

    auto const orthoMatrix = mViewModel.GetOrthoMatrix();

    mShaderManager->ActivateProgram<ProgramType::Canvas>();
    mShaderManager->SetProgramParameter<ProgramType::Canvas, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::CircleOverlay>();
    mShaderManager->SetProgramParameter<ProgramType::CircleOverlay, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::Grid>();
    mShaderManager->SetProgramParameter<ProgramType::Grid, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::RectOverlay>();
    mShaderManager->SetProgramParameter<ProgramType::RectOverlay, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::Rope>();
    mShaderManager->SetProgramParameter<ProgramType::Rope, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::Texture>();
    mShaderManager->SetProgramParameter<ProgramType::Texture, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);
}

void View::UpdateCanvas()
{
    //
    // Upload vertices
    //

    // Calculate space size of 1 pixel
    float const borderSize = mViewModel.GetShipSpaceForOnePhysicalDisplayPixel();

    // Ship space size
    float const shipWidth = static_cast<float>(mViewModel.GetShipSize().width);
    float const shipHeight = static_cast<float>(mViewModel.GetShipSize().height);

    std::array<CanvasVertex, 4> vertexBuffer;

    // Left, Top
    vertexBuffer[0] = CanvasVertex(
        vec2f(-borderSize, shipHeight + borderSize),
        vec2f(0.0f, 0.0f));

    // Left, Bottom
    vertexBuffer[1] = CanvasVertex(
        vec2f(-borderSize, -borderSize),
        vec2f(0.0f, 1.0f));

    // Right, Top
    vertexBuffer[2] = CanvasVertex(
        vec2f(shipWidth + borderSize, shipHeight + borderSize),
        vec2f(1.0f, 0.0f));

    // Right, Bottom
    vertexBuffer[3] = CanvasVertex(
        vec2f(shipWidth + borderSize, -borderSize),
        vec2f(1.0f, 1.0f));

    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, *mCanvasVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(CanvasVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Set pixel size parameter - normalized size (i.e. in the 0->1 space) of 1 pixel (w, h separately)
    //

    DisplayPhysicalSize const canvasPhysSize = mViewModel.ShipSpaceSizeToPhysicalDisplaySize({
        static_cast<int>(shipWidth + 2.0f * borderSize),
        static_cast<int>(shipHeight + 2.0f * borderSize) });

    vec2f const pixelSize = vec2f(
        1.0f / canvasPhysSize.width,
        1.0f / canvasPhysSize.height);

    mShaderManager->ActivateProgram<ProgramType::Canvas>();
    mShaderManager->SetProgramParameter<ProgramType::Canvas, ProgramParameterType::PixelSize>(pixelSize.x, pixelSize.y);
}

void View::UpdateGrid()
{
    //
    // Calculate vertex attributes
    //

    // Ship space
    float const shipWidth = static_cast<float>(mViewModel.GetShipSize().width);
    float const shipHeight = static_cast<float>(mViewModel.GetShipSize().height);
    DisplayPhysicalSize const shipPixelSize = mViewModel.ShipSpaceSizeToPhysicalDisplaySize(mViewModel.GetShipSize());
    float const pixelWidth = static_cast<float>(shipPixelSize.width);
    float const pixelHeight = static_cast<float>(shipPixelSize.height);

    std::array<GridVertex, 4> vertexBuffer;

    // Notes:
    //  - Grid origin is in upper-left corner

    // Bottom-left
    vertexBuffer[0] = GridVertex(
        vec2f(0.0f, 0.0f),
        vec2f(0.0f, pixelHeight));

    // Top-left
    vertexBuffer[1] = GridVertex(
        vec2f(0.0f, shipHeight),
        vec2f(0.0f, 0.0f));

    // Bottom-right
    vertexBuffer[2] = GridVertex(
        vec2f(shipWidth, 0.0f),
        vec2f(pixelWidth, pixelHeight));

    // Top-right
    vertexBuffer[3] = GridVertex(
        vec2f(shipWidth, shipHeight),
        vec2f(pixelWidth, 0.0f));

    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, *mGridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(GridVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Calculate step size
    //

    float const pixelStepSize = mViewModel.CalculateGridPhysicalPixelStepSize();

    mShaderManager->ActivateProgram<ProgramType::Grid>();
    mShaderManager->SetProgramParameter<ProgramType::Grid, ProgramParameterType::PixelStep>(
        pixelStepSize);
}

void View::UpdateCircleOverlay()
{
    //
    // Upload vertices
    //

    std::array<CircleOverlayVertex, 4> vertexBuffer;

    // Left, Top
    vertexBuffer[0] = CircleOverlayVertex(
        vec2f(
            static_cast<float>(mCircleOverlayCenter.x),
            static_cast<float>(mCircleOverlayCenter.y + 1.0f)),
        vec2f(0.0f, 0.0f),
        mCircleOverlayColor);

    // Left, Bottom
    vertexBuffer[1] = CircleOverlayVertex(
        vec2f(
            static_cast<float>(mCircleOverlayCenter.x),
            static_cast<float>(mCircleOverlayCenter.y)),
        vec2f(0.0f, 1.0f),
        mCircleOverlayColor);

    // Right, Top
    vertexBuffer[2] = CircleOverlayVertex(
        vec2f(
            static_cast<float>(mCircleOverlayCenter.x + 1.0f),
            static_cast<float>(mCircleOverlayCenter.y + 1.0f)),
        vec2f(1.0f, 0.0f),
        mCircleOverlayColor);

    // Right, Bottom
    vertexBuffer[3] = CircleOverlayVertex(
        vec2f(
            static_cast<float>(mCircleOverlayCenter.x + 1.0f),
            static_cast<float>(mCircleOverlayCenter.y)),
        vec2f(1.0f, 1.0f),
        mCircleOverlayColor);

    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, *mCircleOverlayVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(CircleOverlayVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Set pixel size parameter - normalized size (i.e. in the 0->1 space) of 1 pixel (w, h separately)
    //

    DisplayPhysicalSize const squarePhysSize = mViewModel.ShipSpaceSizeToPhysicalDisplaySize({ 1, 1 });

    vec2f const pixelSize = vec2f(
        1.0f / squarePhysSize.width,
        1.0f / squarePhysSize.height);

    mShaderManager->ActivateProgram<ProgramType::CircleOverlay>();
    mShaderManager->SetProgramParameter<ProgramType::CircleOverlay, ProgramParameterType::PixelSize>(pixelSize.x, pixelSize.y);
}

void View::UpdateRectOverlay()
{
    //
    // Upload vertices
    //

    std::array<RectOverlayVertex, 4> vertexBuffer;

    // Left, Top
    vertexBuffer[0] = RectOverlayVertex(
        vec2f(
            static_cast<float>(mRectOverlayRect.origin.x),
            static_cast<float>(mRectOverlayRect.origin.y + mRectOverlayRect.size.height)),
        vec2f(0.0f, 0.0f),
        mRectOverlayColor);

    // Left, Bottom
    vertexBuffer[1] = RectOverlayVertex(
        vec2f(
            static_cast<float>(mRectOverlayRect.origin.x),
            static_cast<float>(mRectOverlayRect.origin.y)),
        vec2f(0.0f, 1.0f),
        mRectOverlayColor);

    // Right, Top
    vertexBuffer[2] = RectOverlayVertex(
        vec2f(
            static_cast<float>(mRectOverlayRect.origin.x + mRectOverlayRect.size.width),
            static_cast<float>(mRectOverlayRect.origin.y + mRectOverlayRect.size.height)),
        vec2f(1.0f, 0.0f),
        mRectOverlayColor);

    // Right, Bottom
    vertexBuffer[3] = RectOverlayVertex(
        vec2f(
            static_cast<float>(mRectOverlayRect.origin.x + mRectOverlayRect.size.width),
            static_cast<float>(mRectOverlayRect.origin.y)),
        vec2f(1.0f, 1.0f),
        mRectOverlayColor);

    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, *mRectOverlayVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(RectOverlayVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Set pixel size parameter - normalized size (i.e. in the 0->1 space) of 1 pixel (w, h separately)
    //

    DisplayPhysicalSize const squarePhysSize = mViewModel.ShipSpaceSizeToPhysicalDisplaySize(mRectOverlayRect.size);

    vec2f const pixelSize = vec2f(
        1.0f / squarePhysSize.width,
        1.0f / squarePhysSize.height);

    mShaderManager->ActivateProgram<ProgramType::RectOverlay>();
    mShaderManager->SetProgramParameter<ProgramType::RectOverlay, ProgramParameterType::PixelSize>(pixelSize.x, pixelSize.y);
}

void View::RenderRopes()
{
    // Bind VAO
    glBindVertexArray(*mRopesVAO);

    // Activate program
    mShaderManager->ActivateProgram<ProgramType::Rope>();

    // Draw
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mRopeCount * 2));
    CheckOpenGLError();
}

vec3f View::GetOverlayColor(OverlayMode mode) const
{
    switch (mode)
    {
        case OverlayMode::Default:
        {
            return vec3f(0.05f, 0.05f, 0.05f);
        }

        case OverlayMode::Error:
        {
            return vec3f(1.0f, 0.0f, 0.0f);
        }
    }

    assert(false);
    return vec3f::zero();
}

}