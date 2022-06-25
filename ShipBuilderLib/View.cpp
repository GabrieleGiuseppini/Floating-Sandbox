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
    ShipSpaceSize shipSpaceSize,
    rgbColor const & canvasBackgroundColor,
    VisualizationType primaryVisualization,
    float otherVisualizationsOpacity,
    bool isGridEnabled,
    DisplayLogicalSize displaySize,
    int logicalToPhysicalPixelFactor,
    OpenGLManager & openGLManager,
    std::function<void()> swapRenderBuffersFunction,
    ResourceLocator const & resourceLocator)
    : mOpenGLContext()
    , mViewModel(
        shipSpaceSize,
        displaySize,
        logicalToPhysicalPixelFactor)
    , mShaderManager()
    , mSwapRenderBuffersFunction(swapRenderBuffersFunction)
    //////////////////////////////////
    , mBackgroundTextureSize()
    , mHasGameVisualization(false)
    , mHasStructuralLayerVisualization(false)
    , mStructuralLayerVisualizationShader(ProgramType::Texture) // Will be overwritten
    , mHasElectricalLayerVisualization(false)
    , mRopeCount(false)
    , mHasTextureLayerVisualization(false)
    , mIsGridEnabled(isGridEnabled)
    , mCircleOverlayCenter(0, 0) // Will be overwritten
    , mCircleOverlayColor(vec3f::zero()) // Will be overwritten
    , mHasCircleOverlay(false)
    , mRectOverlayRect({0, 0}, {1, 1}) // Will be overwritten
    , mRectOverlayColor(vec3f::zero()) // Will be overwritten
    , mHasRectOverlay(false)
    , mDashedLineOverlayColor(vec3f::zero()) // Will be overwritten
    , mHasCenterOfBuoyancyWaterlineMarker(false)
    , mHasCenterOfMassWaterlineMarker(false)
    , mHasWaterline(false)
    //////////////////////////////////
    , mMipMappedTextureAtlasOpenGLHandle()
    , mMipMappedTextureAtlasMetadata()
    //////////////////////////////////
    , mPrimaryVisualization(primaryVisualization)
    , mOtherVisualizationsOpacity(otherVisualizationsOpacity)
{
    //
    // Create OpenGL context and make it current
    //

    mOpenGLContext = openGLManager.MakeContextAndMakeCurrent();

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

    // Disable scissor test
    glDisable(GL_SCISSOR_TEST);

    //
    // Load shader manager
    //

    mShaderManager = ShaderManager<ShaderManagerTraits>::CreateInstance(resourceLocator.GetShipBuilderShadersRootPath());

    // Set texture samplers in programs
    mShaderManager->ActivateProgram<ProgramType::MipMappedTextureQuad>();
    mShaderManager->SetTextureParameters<ProgramType::MipMappedTextureQuad>();
    mShaderManager->ActivateProgram<ProgramType::StructureMesh>();
    mShaderManager->SetTextureParameters<ProgramType::StructureMesh>();
    mShaderManager->ActivateProgram<ProgramType::Texture>();
    mShaderManager->SetTextureParameters<ProgramType::Texture>();
    mShaderManager->ActivateProgram<ProgramType::TextureNdc>();
    mShaderManager->SetTextureParameters<ProgramType::TextureNdc>();

    //
    // Create mipmapped texture atlas
    //

    {
        // Load texture database
        auto mipmappedTextureDatabase = Render::TextureDatabase<MipMappedTextureTextureDatabaseTraits>::Load(
            resourceLocator.GetTexturesRootFolderPath());

        // Create atlas
        auto mipmappedTextureAtlas = Render::TextureAtlasBuilder<MipMappedTextureGroups>::BuildAtlas(
            mipmappedTextureDatabase,
            Render::AtlasOptions::None,
            [](float, ProgressMessageType) {});

        LogMessage("ShipBuilder mipmapped texture atlas size: ", mipmappedTextureAtlas.AtlasData.Size.ToString());

        // Activate texture
        mShaderManager->ActivateTexture<ProgramParameterType::MipMappedTexturesAtlasTexture>();

        // Create texture OpenGL handle
        GLuint tmpGLuint;
        glGenTextures(1, &tmpGLuint);
        mMipMappedTextureAtlasOpenGLHandle = tmpGLuint;

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mMipMappedTextureAtlasOpenGLHandle);
        CheckOpenGLError();

        // Upload atlas texture
        GameOpenGL::UploadMipmappedPowerOfTwoTexture(
            std::move(mipmappedTextureAtlas.AtlasData),
            mipmappedTextureAtlas.Metadata.GetMaxDimension());

        // Set repeat mode
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        CheckOpenGLError();

        // Set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CheckOpenGLError();

        // Store metadata
        mMipMappedTextureAtlasMetadata = std::make_unique<Render::TextureAtlasMetadata<MipMappedTextureGroups>>(
            mipmappedTextureAtlas.Metadata);
    }

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
        mBackgroundTexture = tmpGLuint;

        // Configure texture
        mShaderManager->ActivateTexture<ProgramParameterType::BackgroundTextureUnit>();
        glBindTexture(GL_TEXTURE_2D, *mBackgroundTexture);
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
    // Initialize game layer visualization and VAO
    //

    {
        GLuint tmpGLuint;

        //
        // Texture
        //

        // Create texture OpenGL handle
        glGenTextures(1, &tmpGLuint);
        mGameVisualizationTexture = tmpGLuint;

        // Configure texture
        mShaderManager->ActivateTexture<ProgramParameterType::TextureUnit1>();
        glBindTexture(GL_TEXTURE_2D, *mGameVisualizationTexture);
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
        mGameVisualizationVAO = tmpGLuint;
        glBindVertexArray(*mGameVisualizationVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mGameVisualizationVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mGameVisualizationVBO);
        static_assert(sizeof(TextureVertex) == (4) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Texture));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Texture), 4, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize structural layer visualization and VAO
    //

    {
        GLuint tmpGLuint;

        //
        // Texture
        //

        // Create texture OpenGL handle
        glGenTextures(1, &tmpGLuint);
        mStructuralLayerVisualizationTexture = tmpGLuint;

        // Configure texture
        mShaderManager->ActivateTexture<ProgramParameterType::TextureUnit1>();
        glBindTexture(GL_TEXTURE_2D, *mStructuralLayerVisualizationTexture);
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
        mStructuralLayerVisualizationVAO = tmpGLuint;
        glBindVertexArray(*mStructuralLayerVisualizationVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mStructuralLayerVisualizationVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mStructuralLayerVisualizationVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Texture));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Texture), 4, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize electrical layer visualization and VAO
    //

    {
        GLuint tmpGLuint;

        //
        // Texture
        //

        // Create texture OpenGL handle
        glGenTextures(1, &tmpGLuint);
        mElectricalLayerVisualizationTexture = tmpGLuint;

        // Configure texture
        mShaderManager->ActivateTexture<ProgramParameterType::TextureUnit1>();
        glBindTexture(GL_TEXTURE_2D, *mElectricalLayerVisualizationTexture);
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
        mElectricalLayerVisualizationVAO = tmpGLuint;
        glBindVertexArray(*mElectricalLayerVisualizationVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mElectricalLayerVisualizationVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mElectricalLayerVisualizationVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Texture));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Texture), 4, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize ropes layer visualization VAO
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
    // Initialize texture layer visualization and VAO
    //

    {
        GLuint tmpGLuint;

        //
        // Texture
        //

        // Create texture OpenGL handle
        glGenTextures(1, &tmpGLuint);
        mTextureLayerVisualizationTexture = tmpGLuint;

        // Configure texture
        mShaderManager->ActivateTexture<ProgramParameterType::TextureUnit1>();
        glBindTexture(GL_TEXTURE_2D, *mTextureLayerVisualizationTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CheckOpenGLError();

        //
        // VAO
        //

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mTextureLayerVisualizationVAO = tmpGLuint;
        glBindVertexArray(*mTextureLayerVisualizationVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mTextureLayerVisualizationVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mTextureLayerVisualizationVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Texture));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Texture), 4, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void *)0);
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
        static_assert(sizeof(GridVertex) == (2 + 2 + 1) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Grid1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Grid1), 4, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Grid2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Grid2), 1, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void *)(4 * sizeof(float)));
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
        static_assert(sizeof(CircleOverlayVertex) == (4 + 3) * sizeof(float));
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

    //
    // Initialize dashed line overlay VAO
    //

    {
        GLuint tmpGLuint;

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mDashedLineOverlayVAO = tmpGLuint;
        glBindVertexArray(*mDashedLineOverlayVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mDashedLineOverlayVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mDashedLineOverlayVBO);
        static_assert(sizeof(DashedLineOverlayVertex) == (3 + 3) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::DashedLineOverlay1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::DashedLineOverlay1), 3, GL_FLOAT, GL_FALSE, sizeof(DashedLineOverlayVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::DashedLineOverlay2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::DashedLineOverlay2), 3, GL_FLOAT, GL_FALSE, sizeof(DashedLineOverlayVertex), (void *)(3 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize waterline markers VAO
    //

    {
        GLuint tmpGLuint;

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mWaterlineMarkersVAO = tmpGLuint;
        glBindVertexArray(*mWaterlineMarkersVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mWaterlineMarkersVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mWaterlineMarkersVBO);
        static_assert(sizeof(TextureVertex) == (4) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Texture));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Texture), 4, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void *)0);
        CheckOpenGLError();

        // Allocate buffer for both markers
        glBufferData(GL_ARRAY_BUFFER, 2 * 6 * sizeof(TextureVertex), nullptr, GL_STATIC_DRAW);
        CheckOpenGLError();
    }

    //
    // Initialize waterline VAO
    //

    {
        GLuint tmpGLuint;

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mWaterlineVAO = tmpGLuint;
        glBindVertexArray(*mWaterlineVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mWaterlineVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mWaterlineVBO);
        static_assert(sizeof(WaterlineVertex) == (2 + 2 + 2) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Waterline1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Waterline1), 4, GL_FLOAT, GL_FALSE, sizeof(WaterlineVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Waterline2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Waterline2), 2, GL_FLOAT, GL_FALSE, sizeof(WaterlineVertex), (void *)(4 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize misc settings
    //

    SetCanvasBackgroundColor(canvasBackgroundColor);

    // Here we assume there will be an OnViewModelUpdated() call generated
}

void View::SetCanvasBackgroundColor(rgbColor const & color)
{
    mShaderManager->ActivateProgram<ProgramType::Canvas>();
    mShaderManager->SetProgramParameter<ProgramType::Canvas, ProgramParameterType::CanvasBackgroundColor>(color.toVec3f());
}

void View::EnableVisualGrid(bool doEnable)
{
    mIsGridEnabled = doEnable;
}

void View::UploadBackgroundTexture(RgbaImageData && texture)
{
    auto const textureSize = texture.Size;

    //
    // Upload texture
    //

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mBackgroundTexture);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadTexture(std::move(texture));

    //
    // Upload vertices
    //

    UpdateBackgroundTexture(textureSize);

    //
    // Remember texture size - and that we have this texture
    //

    mBackgroundTextureSize = textureSize;
}

void View::UploadGameVisualization(RgbaImageData const & texture)
{
    //
    // Upload texture
    //

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mGameVisualizationTexture);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadTexture(texture);

    //
    // Create vertices
    //
    //
    // We assume that the *content* of this texture is already offseted (on both sides)
    // by half of a "ship pixel" (which is multiple texture pixels) in the same way as
    // we do when we build the ship at simulation time.
    // We do this so that the texture for a particle at ship coords (x, y) is sampled at the center of the
    // texture's quad for that particle.
    //
    // Here, we only shift the *quad* itself by half of a ship particle square,
    // as particles are taken to exist at the *center* of each square.
    //

    float const shipWidth = static_cast<float>(mViewModel.GetShipSize().width);
    float const shipHeight = static_cast<float>(mViewModel.GetShipSize().height);
    float constexpr QuadOffsetX = 0.5f;
    float constexpr QuadOffsetY = 0.5f;

    UploadTextureVerticesTriangleStripQuad(
        QuadOffsetX, 0.0f,
        shipWidth + QuadOffsetX, 1.0f,
        QuadOffsetY, 0.0f,
        shipHeight + QuadOffsetY, 1.0f,
        mGameVisualizationVBO);

    //
    // Remember we have this visualization
    //

    mHasGameVisualization = true;
}

void View::UpdateGameVisualization(
    RgbaImageData const & subTexture,
    ImageCoordinates const & origin)
{
    assert(mHasGameVisualization);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mGameVisualizationTexture);
    CheckOpenGLError();

    // Upload texture region
    GameOpenGL::UploadTextureRegion(
        subTexture.Data.get(),
        origin.x,
        origin.y,
        subTexture.Size.width,
        subTexture.Size.height);
}

void View::RemoveGameVisualization()
{
    mHasGameVisualization = false;
}

void View::SetStructuralLayerVisualizationDrawMode(StructuralLayerVisualizationDrawMode mode)
{
    switch (mode)
    {
        case StructuralLayerVisualizationDrawMode::MeshMode:
        {
            mStructuralLayerVisualizationShader = ProgramType::StructureMesh;
            break;
        }

        case StructuralLayerVisualizationDrawMode::PixelMode:
        {
            mStructuralLayerVisualizationShader = ProgramType::Texture;
            break;
        }
    }
}

void View::UploadStructuralLayerVisualization(RgbaImageData const & texture)
{
    //
    // Upload texture
    //

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mStructuralLayerVisualizationTexture);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadTexture(texture);

    //
    // Upload vertices
    //

    float const shipWidth = static_cast<float>(mViewModel.GetShipSize().width);
    float const shipHeight = static_cast<float>(mViewModel.GetShipSize().height);

    UploadTextureVerticesTriangleStripQuad(
        0.0f, 0.0f,
        shipWidth, 1.0f,
        0.0f, 0.0f,
        shipHeight, 1.0f,
        mStructuralLayerVisualizationVBO);

    //
    // Remember we have this visualization
    //

    mHasStructuralLayerVisualization = true;
}

void View::UpdateStructuralLayerVisualization(
    RgbaImageData const & subTexture,
    ImageCoordinates const & origin)
{
    assert(mHasStructuralLayerVisualization);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mStructuralLayerVisualizationTexture);
    CheckOpenGLError();

    // Upload texture region
    GameOpenGL::UploadTextureRegion(
        subTexture.Data.get(),
        origin.x,
        origin.y,
        subTexture.Size.width,
        subTexture.Size.height);
}

void View::RemoveStructuralLayerVisualization()
{
    mHasStructuralLayerVisualization = false;
}

void View::UploadElectricalLayerVisualization(RgbaImageData const & texture)
{
    //
    // Upload texture
    //

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mElectricalLayerVisualizationTexture);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadTexture(texture);

    //
    // Upload vertices
    //

    float const shipWidth = static_cast<float>(mViewModel.GetShipSize().width);
    float const shipHeight = static_cast<float>(mViewModel.GetShipSize().height);

    UploadTextureVerticesTriangleStripQuad(
        0.0f, 0.0f,
        shipWidth, 1.0f,
        0.0f, 0.0f,
        shipHeight, 1.0f,
        mElectricalLayerVisualizationVBO);

    //
    // Remember we have this visualization
    //

    mHasElectricalLayerVisualization = true;
}

void View::RemoveElectricalLayerVisualization()
{
    mHasElectricalLayerVisualization = false;
}

void View::UploadRopesLayerVisualization(RopeBuffer const & ropeBuffer)
{
    //
    // Create vertices
    //

    std::vector<RopeVertex> vertexBuffer;
    vertexBuffer.reserve(ropeBuffer.GetSize());

    for (auto const & e : ropeBuffer)
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

    mRopeCount = ropeBuffer.GetSize();
}

void View::RemoveRopesLayerVisualization()
{
    mRopeCount = 0;
}

void View::UploadTextureLayerVisualization(RgbaImageData const & texture)
{
    //
    // Upload texture
    //

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mTextureLayerVisualizationTexture);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadTexture(texture);

    //
    // Create vertices
    //
    // We map the texture with the same "0.5 ship offset" that we use at ShipFactory,
    // which is for sampling the texture at the center of each ship particle square
    // (thus the texture exterior is slightly out, but we take this hit).
    //
    // Moreover, we draw the *quad* itself shifted by half of a ship particle square,
    // as particles are taken to exist at the *center* of each square.
    //

    float const shipWidth = static_cast<float>(mViewModel.GetShipSize().width);
    float const shipHeight = static_cast<float>(mViewModel.GetShipSize().height);
    float constexpr QuadOffsetX = 0.5f;
    float constexpr QuadOffsetY = 0.5f;
    float const texOffsetX = 0.5f / shipWidth;
    float const texOffsetY = 0.5f / shipHeight;

    UploadTextureVerticesTriangleStripQuad(
        QuadOffsetX, texOffsetX,
        shipWidth - QuadOffsetX, 1.0f - texOffsetX,
        QuadOffsetY, texOffsetY,
        shipHeight - QuadOffsetY, 1.0f - texOffsetY,
        mTextureLayerVisualizationVBO);

    //
    // Remember we have this visualization
    //

    mHasTextureLayerVisualization = true;

    //
    // Tell view model
    //

    mViewModel.SetTextureLayerVisualizationTextureSize(texture.Size);
}

void View::RemoveTextureLayerVisualization()
{
    mHasTextureLayerVisualization = false;

    mViewModel.RemoveTextureLayerVisualizationTextureSize();
}

void View::UploadCircleOverlay(
    ShipSpaceCoordinates const & center,
    OverlayMode mode)
{
    // Store center
    mCircleOverlayCenter = center;

    // Store color
    mCircleOverlayColor = GetOverlayColor(mode);

    mHasCircleOverlay = true;

    // Update overlay
    UpdateCircleOverlay();
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

    mHasRectOverlay = true;

    // Update overlay
    UpdateRectOverlay();
}

void View::RemoveRectOverlay()
{
    assert(mHasRectOverlay);

    mHasRectOverlay = false;
}

void View::UploadDashedLineOverlay(
    ShipSpaceCoordinates const & start,
    ShipSpaceCoordinates const & end,
    OverlayMode mode)
{
    // Store line
    mDashedLineOverlaySet.clear();
    mDashedLineOverlaySet.emplace_back(start, end);

    // Store color
    mDashedLineOverlayColor = GetOverlayColor(mode);

    // Update overlay
    UpdateDashedLineOverlay();
}

void View::RemoveDashedLineOverlay()
{
    assert(!mDashedLineOverlaySet.empty());

    mDashedLineOverlaySet.clear();
}

void View::UploadWaterlineMarker(
    vec2f const & center,
    WaterlineMarkerType type)
{
    TextureFrameIndex textureFrameIndex = 0;
    GLintptr bufferOffset = 0;
    GLsizeiptr constexpr bufferSize = 6 * sizeof(TextureVertex);

    switch (type)
    {
        case WaterlineMarkerType::CenterOfBuoyancy:
        {
            textureFrameIndex = 0;
            bufferOffset = 0;
            mHasCenterOfBuoyancyWaterlineMarker = true;

            break;
        }

        case WaterlineMarkerType::CenterOfMass:
        {
            textureFrameIndex = 1;
            bufferOffset = bufferSize;
            mHasCenterOfMassWaterlineMarker = true;

            break;
        }
    }

    //
    // Upload quad
    //

    auto const & atlasFrameMetadata = mMipMappedTextureAtlasMetadata->GetFrameMetadata(MipMappedTextureGroups::WaterlineMarker, textureFrameIndex);

    float const leftX = center.x - atlasFrameMetadata.FrameMetadata.AnchorCenterWorld.x + 0.5f; // At center of ship coord's square
    float const leftXTexture = atlasFrameMetadata.TextureCoordinatesBottomLeft.x;

    float const rightX = leftX + atlasFrameMetadata.FrameMetadata.WorldWidth;
    float const rightXTexture = atlasFrameMetadata.TextureCoordinatesTopRight.x;

    float const bottomY = center.y - atlasFrameMetadata.FrameMetadata.AnchorCenterWorld.y + 0.5f; // At center of ship coord's square
    float const bottomYTexture = atlasFrameMetadata.TextureCoordinatesBottomLeft.y;

    float const topY = bottomY + atlasFrameMetadata.FrameMetadata.WorldHeight;
    float const topYTexture = atlasFrameMetadata.TextureCoordinatesTopRight.y;

    std::array<TextureVertex, 6> vertexBuffer;

    // Bottom-left
    vertexBuffer[0] = TextureVertex(
        vec2f(leftX, bottomY),
        vec2f(leftXTexture, bottomYTexture));

    // Top-left
    vertexBuffer[1] = TextureVertex(
        vec2f(leftX, topY),
        vec2f(leftXTexture, topYTexture));

    // Bottom-right
    vertexBuffer[2] = TextureVertex(
        vec2f(rightX, bottomY),
        vec2f(rightXTexture, bottomYTexture));

    // Top-left
    vertexBuffer[3] = TextureVertex(
        vec2f(leftX, topY),
        vec2f(leftXTexture, topYTexture));

    // Bottom-right
    vertexBuffer[4] = TextureVertex(
        vec2f(rightX, bottomY),
        vec2f(rightXTexture, bottomYTexture));

    // Top-right
    vertexBuffer[5] = TextureVertex(
        vec2f(rightX, topY),
        vec2f(rightXTexture, topYTexture));

    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, *mWaterlineMarkersVBO);
    glBufferSubData(GL_ARRAY_BUFFER, bufferOffset, bufferSize, vertexBuffer.data());
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void View::RemoveWaterlineMarker(WaterlineMarkerType type)
{
    switch (type)
    {
        case WaterlineMarkerType::CenterOfBuoyancy:
        {
            mHasCenterOfBuoyancyWaterlineMarker = false;
            break;
        }

        case WaterlineMarkerType::CenterOfMass:
        {
            mHasCenterOfMassWaterlineMarker = false;
            break;
        }
    }
}

void View::UploadWaterline(
    vec2f const & center, // Ship space coords
    vec2f const & waterDirection)
{
    //
    // Upload vertices
    //

    float const shipWidth = static_cast<float>(mViewModel.GetShipSize().width);
    float const shipHeight = static_cast<float>(mViewModel.GetShipSize().height);

    std::array<WaterlineVertex, 4> vertexBuffer;

    // Bottom-left
    vertexBuffer[0] = WaterlineVertex(
        vec2f(0.0f, 0.0f),
        center,
        waterDirection);

    // Top-left
    vertexBuffer[1] = WaterlineVertex(
        vec2f(0.0f, shipHeight),
        center,
        waterDirection);

    // Bottom-right
    vertexBuffer[2] = WaterlineVertex(
        vec2f(shipWidth, 0.0f),
        center,
        waterDirection);

    // Top-right
    vertexBuffer[3] = WaterlineVertex(
        vec2f(shipWidth, shipHeight),
        center,
        waterDirection);

    glBindBuffer(GL_ARRAY_BUFFER, *mWaterlineVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(WaterlineVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Remember we have a waterline
    //

    mHasWaterline = true;
}

void View::RemoveWaterline()
{
    mHasWaterline = false;
}

void View::Render()
{
    //
    // Initialize
    //

    // Set viewport
    glViewport(0, 0, mViewModel.GetDisplayPhysicalSize().width, mViewModel.GetDisplayPhysicalSize().height);

    //
    // Following is with scissor test disabled
    //

    glDisable(GL_SCISSOR_TEST);

    // Background texture
    if (mBackgroundTextureSize.has_value())
    {
        // Set this texture in the shader's sampler
        mShaderManager->ActivateTexture<ProgramParameterType::BackgroundTextureUnit>();
        glBindTexture(GL_TEXTURE_2D, *mBackgroundTexture);

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

    //
    // Visualizations
    //

    // Ropes - when they're not the primary viz (in which case we render them on top of structural)
    if (mRopeCount > 0 && mPrimaryVisualization != VisualizationType::RopesLayer)
    {
        RenderRopesLayerVisualization();
    }

    // Game, structural, and texture visualizations - whichever is primary goes first
    if (mPrimaryVisualization == VisualizationType::Game)
    {
        if (mHasGameVisualization)
        {
            RenderGameVisualization();
        }
    }
    else if (mPrimaryVisualization == VisualizationType::StructuralLayer)
    {
        if (mHasStructuralLayerVisualization)
        {
            RenderStructuralLayerVisualization();
        }
    }
    else if (mPrimaryVisualization == VisualizationType::TextureLayer)
    {
        if (mHasTextureLayerVisualization)
        {
            RenderTextureLayerVisualization();
        }
    }

    // Game, structural, and texture visualizations - when they're not primary

    if (mPrimaryVisualization != VisualizationType::Game)
    {
        if (mHasGameVisualization)
        {
            RenderGameVisualization();
        }
    }

    if (mPrimaryVisualization != VisualizationType::StructuralLayer)
    {
        if (mHasStructuralLayerVisualization)
        {
            RenderStructuralLayerVisualization();
        }
    }

    if (mPrimaryVisualization != VisualizationType::TextureLayer)
    {
        if (mHasTextureLayerVisualization)
        {
            RenderTextureLayerVisualization();
        }
    }

    // Electrical layer visualization
    if (mHasElectricalLayerVisualization)
    {
        RenderElectricalLayerVisualization();
    }

    // Ropes layer, but only when it's primary viz
    if (mRopeCount > 0 && mPrimaryVisualization == VisualizationType::RopesLayer)
    {
        RenderRopesLayerVisualization();
    }

    //
    // Misc stuff on top of visualizations
    //

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

    // Waterline
    if (mHasWaterline)
    {
        // Bind VAO
        glBindVertexArray(*mWaterlineVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::Waterline>();

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CheckOpenGLError();
    }

    // Waterline marker
    if (mHasCenterOfBuoyancyWaterlineMarker || mHasCenterOfMassWaterlineMarker)
    {
        // Bind VAO
        glBindVertexArray(*mWaterlineMarkersVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::MipMappedTextureQuad>();

        int const first = mHasCenterOfBuoyancyWaterlineMarker ? 0 : 6;
        int count = 0;
        if (mHasCenterOfBuoyancyWaterlineMarker)
        {
            count += 6;
        }
        if (mHasCenterOfMassWaterlineMarker)
        {
            count += 6;
        }

        // Draw
        glDrawArrays(GL_TRIANGLES, first, count);
        CheckOpenGLError();
    }

    //
    // Following is with scissor test enabled
    //

    glEnable(GL_SCISSOR_TEST);

    // Dashed line overlay
    if (!mDashedLineOverlaySet.empty())
    {
        // Bind VAO
        glBindVertexArray(*mDashedLineOverlayVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::DashedLineOverlay>();

        // Set line width
        glLineWidth(1.5f);

        // Draw
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mDashedLineOverlaySet.size() * 2));
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
    UpdateStructuralLayerVisualizationParameters();

    if (mBackgroundTextureSize.has_value())
    {
        UpdateBackgroundTexture(*mBackgroundTextureSize);
    }

    UpdateCanvas();

    UpdateGrid();

    if (mHasCircleOverlay)
    {
        UpdateCircleOverlay();
    }

    if (mHasRectOverlay)
    {
        UpdateRectOverlay();
    }

    if (!mDashedLineOverlaySet.empty())
    {
        UpdateDashedLineOverlay();
    }

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

    mShaderManager->ActivateProgram<ProgramType::DashedLineOverlay>();
    mShaderManager->SetProgramParameter<ProgramType::DashedLineOverlay, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::Grid>();
    mShaderManager->SetProgramParameter<ProgramType::Grid, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::MipMappedTextureQuad>();
    mShaderManager->SetProgramParameter<ProgramType::MipMappedTextureQuad, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::RectOverlay>();
    mShaderManager->SetProgramParameter<ProgramType::RectOverlay, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::Rope>();
    mShaderManager->SetProgramParameter<ProgramType::Rope, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::StructureMesh>();
    mShaderManager->SetProgramParameter<ProgramType::StructureMesh, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::Texture>();
    mShaderManager->SetProgramParameter<ProgramType::Texture, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::Waterline>();
    mShaderManager->SetProgramParameter<ProgramType::Waterline, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    //
    // Scissor test
    //

    auto const physicalCanvasRect = mViewModel.GetPhysicalVisibleShipRegion();
    glScissor(
        physicalCanvasRect.origin.x,
        mViewModel.GetDisplayPhysicalSize().height - 1 - (physicalCanvasRect.origin.y + physicalCanvasRect.size.height), // Origin is bottom
        physicalCanvasRect.size.width,
        physicalCanvasRect.size.height);
    CheckOpenGLError();
}

void View::UpdateStructuralLayerVisualizationParameters()
{
    //
    // Set ship particle texture size - normalized size (i.e. in the 0->1 texture space) of 1 ship particle pixel (w, h separately)
    //

    vec2f const pixelsPerShipParticle = mViewModel.ShipSpaceSizeToPhysicalDisplaySize({ 1, 1 }).ToFloat();

    float const shipWidth = static_cast<float>(mViewModel.GetShipSize().width);
    float const shipHeight = static_cast<float>(mViewModel.GetShipSize().height);

    vec2f const shipParticleTextureSize = vec2f(
        1.0f / shipWidth,
        1.0f / shipHeight);

    mShaderManager->ActivateProgram<ProgramType::StructureMesh>();
    mShaderManager->SetProgramParameter<ProgramType::StructureMesh, ProgramParameterType::PixelsPerShipParticle>(pixelsPerShipParticle.x, pixelsPerShipParticle.y);
    mShaderManager->SetProgramParameter<ProgramType::StructureMesh, ProgramParameterType::ShipParticleTextureSize>(shipParticleTextureSize.x, shipParticleTextureSize.y);
}

void View::UpdateBackgroundTexture(ImageSize const & textureSize)
{
    //
    // Create vertices (in NDC)
    //

    // The texture coordinate at the bottom of the quad obeys the texture's aspect ratio,
    // rather than the screen's

    float const textureBottom =
        -(static_cast<float>(textureSize.height) - static_cast<float>(mViewModel.GetDisplayPhysicalSize().height))
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
    float const pixelMidX = pixelWidth / 2.0f;

    std::array<GridVertex, 4> vertexBuffer;

    // Notes:
    //  - Grid origin is in upper-left corner

    // Bottom-left
    vertexBuffer[0] = GridVertex(
        vec2f(0.0f, 0.0f),
        vec2f(0.0f, pixelHeight),
        pixelMidX);

    // Top-left
    vertexBuffer[1] = GridVertex(
        vec2f(0.0f, shipHeight),
        vec2f(0.0f, 0.0f),
        pixelMidX);

    // Bottom-right
    vertexBuffer[2] = GridVertex(
        vec2f(shipWidth, 0.0f),
        vec2f(pixelWidth, pixelHeight),
        pixelMidX);

    // Top-right
    vertexBuffer[3] = GridVertex(
        vec2f(shipWidth, shipHeight),
        vec2f(pixelWidth, 0.0f),
        pixelMidX);

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
    assert(mHasCircleOverlay);

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

    DisplayPhysicalSize const shipParticlePhysSize = mViewModel.ShipSpaceSizeToPhysicalDisplaySize({ 1, 1 });

    vec2f const pixelSize = vec2f(
        1.0f / std::max(shipParticlePhysSize.width, 1),
        1.0f / std::max(shipParticlePhysSize.height, 1));

    mShaderManager->ActivateProgram<ProgramType::CircleOverlay>();
    mShaderManager->SetProgramParameter<ProgramType::CircleOverlay, ProgramParameterType::PixelSize>(pixelSize.x, pixelSize.y);
}

void View::UpdateRectOverlay()
{
    assert(mHasRectOverlay);

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
        1.0f / std::max(squarePhysSize.width, 1),
        1.0f / std::max(squarePhysSize.height, 1));

    mShaderManager->ActivateProgram<ProgramType::RectOverlay>();
    mShaderManager->SetProgramParameter<ProgramType::RectOverlay, ProgramParameterType::PixelSize>(pixelSize.x, pixelSize.y);
}

void View::UpdateDashedLineOverlay()
{
    assert(!mDashedLineOverlaySet.empty());

    //
    // Upload vertices
    //

    std::vector<DashedLineOverlayVertex> vertexBuffer;

    for (auto const & p : mDashedLineOverlaySet)
    {
        //
        // Calculate length, in pixels
        //

        ShipSpaceSize const shipRect(std::abs(p.first.x - p.second.x), std::abs(p.first.y - p.second.y));
        DisplayPhysicalSize physRec = mViewModel.ShipSpaceSizeToPhysicalDisplaySize(shipRect);
        float pixelLength = physRec.ToFloat().length();

        // Normalize length so it's a multiple of the period + 1/2 period
        float constexpr DashPeriod = 8.0f; // 4 + 4
        float const leftover = std::fmod(pixelLength + DashPeriod / 2.0f, DashPeriod);
        pixelLength += (DashPeriod - leftover);

        //
        // Populate vertices
        //

        vertexBuffer.emplace_back(
            p.first.ToFloat() + vec2f(0.5f, 0.5f),
            0.0f,
            mDashedLineOverlayColor);

        vertexBuffer.emplace_back(
            p.second.ToFloat() + vec2f(0.5f, 0.5f),
            pixelLength,
            mDashedLineOverlayColor);
    }

    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, *mDashedLineOverlayVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(DashedLineOverlayVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void View::UploadTextureVerticesTriangleStripQuad(
    float leftXShip, float leftXTex,
    float rightXShip, float rightTex,
    float bottomYShip, float bottomYTex,
    float topYShip, float topYTex,
    GameOpenGLVBO const & vbo)
{
    std::array<TextureVertex, 4> vertexBuffer;

    // Bottom-left
    vertexBuffer[0] = TextureVertex(
        vec2f(leftXShip, bottomYShip),
        vec2f(leftXTex, bottomYTex));

    // Top-left
    vertexBuffer[1] = TextureVertex(
        vec2f(leftXShip, topYShip),
        vec2f(leftXTex, topYTex));

    // Bottom-right
    vertexBuffer[2] = TextureVertex(
        vec2f(rightXShip, bottomYShip),
        vec2f(rightTex, bottomYTex));

    // Top-right
    vertexBuffer[3] = TextureVertex(
        vec2f(rightXShip, topYShip),
        vec2f(rightTex, topYTex));

    //
    // Upload vertices
    //

    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(TextureVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void View::RenderGameVisualization()
{
    // Set this texture in the shader's sampler
    mShaderManager->ActivateTexture<ProgramParameterType::TextureUnit1>();
    glBindTexture(GL_TEXTURE_2D, *mGameVisualizationTexture);

    // Bind VAO
    glBindVertexArray(*mGameVisualizationVAO);

    // Activate program
    mShaderManager->ActivateProgram<ProgramType::Texture>();

    // Set opacity
    mShaderManager->SetProgramParameter<ProgramType::Texture, ProgramParameterType::Opacity>(
        mPrimaryVisualization == VisualizationType::Game ? 1.0f : mOtherVisualizationsOpacity);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CheckOpenGLError();
}

void View::RenderStructuralLayerVisualization()
{
    // Set this texture in the shader's sampler
    mShaderManager->ActivateTexture<ProgramParameterType::TextureUnit1>();
    glBindTexture(GL_TEXTURE_2D, *mStructuralLayerVisualizationTexture);

    // Bind VAO
    glBindVertexArray(*mStructuralLayerVisualizationVAO);

    // Activate program
    mShaderManager->ActivateProgram(mStructuralLayerVisualizationShader);

    // Set opacity
    mShaderManager->SetProgramParameter<ProgramParameterType::Opacity>(
        mStructuralLayerVisualizationShader,
        mPrimaryVisualization == VisualizationType::StructuralLayer ? 1.0f : mOtherVisualizationsOpacity);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CheckOpenGLError();
}

void View::RenderElectricalLayerVisualization()
{
    // Set this texture in the shader's sampler
    mShaderManager->ActivateTexture<ProgramParameterType::TextureUnit1>();
    glBindTexture(GL_TEXTURE_2D, *mElectricalLayerVisualizationTexture);

    // Bind VAO
    glBindVertexArray(*mElectricalLayerVisualizationVAO);

    // Activate program
    mShaderManager->ActivateProgram<ProgramType::Texture>();

    // Set opacity
    mShaderManager->SetProgramParameter<ProgramType::Texture, ProgramParameterType::Opacity>(
        mPrimaryVisualization == VisualizationType::ElectricalLayer ? 1.0f : mOtherVisualizationsOpacity);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CheckOpenGLError();
}

void View::RenderRopesLayerVisualization()
{
    // Bind VAO
    glBindVertexArray(*mRopesVAO);

    // Activate program
    mShaderManager->ActivateProgram<ProgramType::Rope>();

    // Set opacity
    mShaderManager->SetProgramParameter<ProgramType::Rope, ProgramParameterType::Opacity>(
        mPrimaryVisualization == VisualizationType::RopesLayer ? 1.0f : mOtherVisualizationsOpacity);

    // Set line width
    glLineWidth(2.5f);

    // Draw
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mRopeCount * 2));
    CheckOpenGLError();
}

void View::RenderTextureLayerVisualization()
{
    // Set this texture in the shader's sampler
    mShaderManager->ActivateTexture<ProgramParameterType::TextureUnit1>();
    glBindTexture(GL_TEXTURE_2D, *mTextureLayerVisualizationTexture);

    // Bind VAO
    glBindVertexArray(*mTextureLayerVisualizationVAO);

    // Activate program
    mShaderManager->ActivateProgram<ProgramType::Texture>();

    // Set opacity
    mShaderManager->SetProgramParameter<ProgramType::Texture, ProgramParameterType::Opacity>(
        mPrimaryVisualization == VisualizationType::TextureLayer ? 1.0f : mOtherVisualizationsOpacity);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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