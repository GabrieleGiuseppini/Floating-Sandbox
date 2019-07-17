/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderContext.h"

#include <Game/ImageFileTools.h>

#include <GameCore/GameException.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/ImageTools.h>
#include <GameCore/Log.h>

#include <cstring>

namespace Render {

ImageSize constexpr ThumbnailSize(32, 32);

RenderContext::RenderContext(
    ResourceLoader & resourceLoader,
    ProgressCallback const & progressCallback)
    // Buffers
    : mStarVertexBuffer()
    , mStarVBO()
    , mCloudQuadBuffer()
    , mCloudVBO()
    , mLandSegmentBuffer()
    , mLandSegmentBufferAllocatedSize(0u)
    , mLandVBO()
    , mOceanSegmentBuffer()
    , mOceanSegmentBufferAllocatedSize(0u)
    , mOceanVBO()
    , mCrossOfLightVertexBuffer()
    , mCrossOfLightVBO()
    , mHeatBlasterFlameVBO()
    , mFireExtinguisherSprayVBO()
    , mWorldBorderVertexBuffer()
    , mWorldBorderVBO()
    // VAOs
    , mStarVAO()
    , mCloudVAO()
    , mLandVAO()
    , mOceanVAO()
    , mCrossOfLightVAO()
    , mHeatBlasterFlameVAO()
    , mFireExtinguisherSprayVAO()
    , mWorldBorderVAO()
    // Textures
    , mCloudTextureAtlasOpenGLHandle()
    , mCloudTextureAtlasMetadata()
    , mOceanTextureFrameSpecifications()
    , mOceanTextureOpenGLHandle()
    , mLoadedOceanTextureIndex(std::numeric_limits<size_t>::max())
    , mLandTextureFrameSpecifications()
    , mLandTextureOpenGLHandle()
    , mLoadedLandTextureIndex(std::numeric_limits<size_t>::max())
    // Ships
    , mShips()
    , mGenericTextureAtlasOpenGLHandle()
    , mGenericTextureAtlasMetadata()
    // World border
    , mWorldBorderTextureSize(0, 0)
    , mIsWorldBorderVisible(false)
    // HeatBlaster
    , mHeatBlasterFlameShaderToRender()
    // Fire extinguisher
    , mFireExtinguisherSprayShaderToRender()
    // Managers
    , mShaderManager()
    , mUploadedTextureManager()
    , mTextRenderContext()
    // Render parameters
    , mViewModel(1.0f, vec2f::zero(), 100, 100)
    , mFlatSkyColor(0x87, 0xce, 0xfa) // (cornflower blue)
    , mAmbientLightIntensity(1.0f)
    , mOceanTransparency(0.8125f)
    , mOceanDarkeningRate(0.359375f)
    , mShowShipThroughOcean(false)
    , mWaterContrast(0.8125f)
    , mWaterLevelOfDetail(0.6875f)
    , mShipRenderMode(ShipRenderMode::Texture)
    , mDebugShipRenderMode(DebugShipRenderMode::None)
    , mOceanRenderMode(OceanRenderMode::Texture)
    , mOceanAvailableThumbnails()
    , mSelectedOceanTextureIndex(0) // Wavy Thin
    , mDepthOceanColorStart(0x8b, 0xc5, 0xfe)
    , mDepthOceanColorEnd(0x00, 0x00, 0x00)
    , mFlatOceanColor(0x00, 0x3d, 0x99)
    , mLandRenderMode(LandRenderMode::Texture)
    , mLandAvailableThumbnails()
    , mSelectedLandTextureIndex(3) // Rock Coarse 3
    , mFlatLandColor(0x72, 0x46, 0x05)
    , mVectorFieldRenderMode(VectorFieldRenderMode::None)
    , mVectorFieldLengthMultiplier(1.0f)
    , mShowStressedSprings(false)
    , mDrawHeatOverlay(false)
    , mHeatOverlayTransparency(0.1875f)
    , mShipFlameRenderMode(ShipFlameRenderMode::Mode1)
    , mShipFlameSizeAdjustment(1.0f)
    // Statistics
    , mRenderStatistics()
{
    static constexpr float TextureDatabaseProgressSteps = 20.0f;
    static constexpr float GenericTextureProgressSteps = 10.0f;
    static constexpr float CloudTextureProgressSteps = 4.0f;

    // Shaders, TextRenderContext, TextureDatabase, GenericTextureAtlas, Clouds, Noise X 2, WorldBorder
    static constexpr float TotalProgressSteps = 2.0f + TextureDatabaseProgressSteps + GenericTextureProgressSteps + CloudTextureProgressSteps + 2.0f + 1.0f;

    GLuint tmpGLuint;


    //
    // Load shader manager
    //

    progressCallback(0.0f, "Loading shaders...");

    mShaderManager = ShaderManager<ShaderManagerTraits>::CreateInstance(resourceLoader.GetRenderShadersRootPath());


    //
    // Initialize OpenGL
    //

    // Initialize the shared texture unit once and for all
    mShaderManager->ActivateTexture<ProgramParameterType::SharedTexture>();


    //
    // Initialize text render context
    //

    mTextRenderContext = std::make_unique<TextRenderContext>(
        resourceLoader,
        *(mShaderManager.get()),
        mViewModel.GetCanvasWidth(),
        mViewModel.GetCanvasHeight(),
        mAmbientLightIntensity,
        [&progressCallback](float progress, std::string const & message)
        {
            progressCallback((1.0f + progress) / TotalProgressSteps, message);
        });



    //
    // Load texture database
    //

    TextureDatabase textureDatabase = TextureDatabase::Load(
        resourceLoader,
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback((2.0f + progress * TextureDatabaseProgressSteps) / TotalProgressSteps, "Loading textures...");
        });

    // Create uploaded texture manager
    mUploadedTextureManager = std::make_unique<UploadedTextureManager>();



    //
    // Create generic texture atlas
    //
    // Atlas-ize all textures EXCEPT the following:
    // - Land, Ocean: we need these to be wrapping
    // - Clouds: we keep these in a separate atlas, we have to rebind anyway
    // - Noise, WorldBorder
    //

    mShaderManager->ActivateTexture<ProgramParameterType::GenericTexturesAtlasTexture>();

    TextureAtlasBuilder genericTextureAtlasBuilder;
    for (auto const & group : textureDatabase.GetGroups())
    {
        if (TextureGroupType::Land != group.Group
            && TextureGroupType::Ocean != group.Group
            && TextureGroupType::Cloud != group.Group
            && TextureGroupType::Noise != group.Group
            && TextureGroupType::WorldBorder != group.Group)
        {
            genericTextureAtlasBuilder.Add(group);
        }
    }

    TextureAtlas genericTextureAtlas = genericTextureAtlasBuilder.BuildAtlas(
        [&progressCallback](float progress, std::string const & message)
        {
            progressCallback((2.0f + TextureDatabaseProgressSteps + progress * GenericTextureProgressSteps) / TotalProgressSteps, message);
        });

    LogMessage("Generic texture atlas size: ", genericTextureAtlas.AtlasData.Size.Width, "x", genericTextureAtlas.AtlasData.Size.Height);

    // Create texture OpenGL handle
    glGenTextures(1, &tmpGLuint);
    mGenericTextureAtlasOpenGLHandle = tmpGLuint;

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mGenericTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    GameOpenGL::UploadMipmappedPowerOfTwoTexture(
        std::move(genericTextureAtlas.AtlasData),
        genericTextureAtlas.Metadata.GetMaxDimension());

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckOpenGLError();

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Store metadata
    mGenericTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata>(genericTextureAtlas.Metadata);

    // Set texture parameter
    mShaderManager->ActivateProgram<ProgramType::ShipGenericTextures>();
    mShaderManager->SetTextureParameters<ProgramType::ShipGenericTextures>();


    //
    // Initialize buffers
    //

    GLuint vbos[8];
    glGenBuffers(7, vbos);
    mStarVBO = vbos[0];
    mCloudVBO = vbos[1];
    mLandVBO = vbos[2];
    mOceanVBO = vbos[3];
    mCrossOfLightVBO = vbos[4];
    mHeatBlasterFlameVBO = vbos[5];
    mFireExtinguisherSprayVBO = vbos[6];
    mWorldBorderVBO = vbos[7];


    //
    // Initialize Star VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mStarVAO = tmpGLuint;

    glBindVertexArray(*mStarVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mStarVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Star));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Star), 3, GL_FLOAT, GL_FALSE, sizeof(StarVertex), (void*)0);
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Initialize Cloud VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mCloudVAO = tmpGLuint;

    glBindVertexArray(*mCloudVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Cloud));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Cloud), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Initialize Land VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mLandVAO = tmpGLuint;

    glBindVertexArray(*mLandVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Land));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Land), 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Initialize Ocean VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mOceanVAO = tmpGLuint;

    glBindVertexArray(*mOceanVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mOceanVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Ocean));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Ocean), (2 + 1), GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void*)0);
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Initialize CrossOfLight VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mCrossOfLightVAO = tmpGLuint;

    glBindVertexArray(*mCrossOfLightVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mCrossOfLightVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::CrossOfLight1));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::CrossOfLight1), 4, GL_FLOAT, GL_FALSE, sizeof(CrossOfLightVertex), (void*)0);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::CrossOfLight2));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::CrossOfLight2), 1, GL_FLOAT, GL_FALSE, sizeof(CrossOfLightVertex), (void*)(4 * sizeof(float)));
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Initialize HeatBlaster flame VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mHeatBlasterFlameVAO = tmpGLuint;

    glBindVertexArray(*mHeatBlasterFlameVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mHeatBlasterFlameVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::HeatBlasterFlame));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::HeatBlasterFlame), 4, GL_FLOAT, GL_FALSE, sizeof(HeatBlasterFlameVertex), (void*)0);
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Initialize fire extinguisher spray VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mFireExtinguisherSprayVAO = tmpGLuint;

    glBindVertexArray(*mFireExtinguisherSprayVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mFireExtinguisherSprayVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::FireExtinguisherSpray));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::FireExtinguisherSpray), 4, GL_FLOAT, GL_FALSE, sizeof(FireExtinguisherSprayVertex), (void*)0);
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Initialize WorldBorder VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mWorldBorderVAO = tmpGLuint;

    glBindVertexArray(*mWorldBorderVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mWorldBorderVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::WorldBorder));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::WorldBorder), 4, GL_FLOAT, GL_FALSE, sizeof(WorldBorderVertex), (void*)0);
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Initialize cloud texture atlas
    //

    mShaderManager->ActivateTexture<ProgramParameterType::CloudTexture>();

    TextureAtlasBuilder cloudAtlasBuilder;
    cloudAtlasBuilder.Add(textureDatabase.GetGroup(TextureGroupType::Cloud));

    TextureAtlas cloudTextureAtlas = cloudAtlasBuilder.BuildAtlas(
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback((2.0f + TextureDatabaseProgressSteps + GenericTextureProgressSteps + progress * CloudTextureProgressSteps) / TotalProgressSteps, "Loading cloud textures...");
        });

    // Create OpenGL handle
    glGenTextures(1, &tmpGLuint);
    mCloudTextureAtlasOpenGLHandle = tmpGLuint;

    // Bind texture atlas
    glBindTexture(GL_TEXTURE_2D, *mCloudTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    GameOpenGL::UploadTexture(std::move(cloudTextureAtlas.AtlasData));

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckOpenGLError();

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Store metadata
    mCloudTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata>(cloudTextureAtlas.Metadata);

    // Set texture in shader
    mShaderManager->ActivateProgram<ProgramType::Clouds>();
    mShaderManager->SetTextureParameters<ProgramType::Clouds>();


    //
    // Initialize ocean textures
    //

    mOceanTextureFrameSpecifications = textureDatabase.GetGroup(TextureGroupType::Ocean).GetFrameSpecifications();

    // Create list of available textures for user
    for (auto const & tfs : mOceanTextureFrameSpecifications)
    {
        auto textureThumbnail = ImageFileTools::LoadImageRgbaLowerLeftAndResize(
            tfs.FilePath,
            ThumbnailSize);

        assert(static_cast<size_t>(tfs.Metadata.FrameId.FrameIndex) == mOceanAvailableThumbnails.size());

        mOceanAvailableThumbnails.emplace_back(
            tfs.Metadata.FrameName,
            std::move(textureThumbnail));
    }


    //
    // Initialize land textures
    //

    mLandTextureFrameSpecifications = textureDatabase.GetGroup(TextureGroupType::Land).GetFrameSpecifications();

    // Create list of available textures for user
    for (auto const & tfs : mLandTextureFrameSpecifications)
    {
        auto textureThumbnail = ImageFileTools::LoadImageRgbaLowerLeftAndResize(
            tfs.FilePath,
            ThumbnailSize);

        assert(static_cast<size_t>(tfs.Metadata.FrameId.FrameIndex) == mLandAvailableThumbnails.size());

        mLandAvailableThumbnails.emplace_back(
            tfs.Metadata.FrameName,
            std::move(textureThumbnail));
    }


    //
    // Initialize noise textures
    //

    // Noise 1

    mShaderManager->ActivateTexture<ProgramParameterType::NoiseTexture1>();

    mUploadedTextureManager->UploadNextFrame(
        textureDatabase.GetGroup(TextureGroupType::Noise),
        0,
        GL_LINEAR);

    progressCallback((2.0f + TextureDatabaseProgressSteps + GenericTextureProgressSteps + CloudTextureProgressSteps + 1.0f) / TotalProgressSteps, "Loading noise textures...");

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, mUploadedTextureManager->GetOpenGLHandle(TextureGroupType::Noise, 0));
    CheckOpenGLError();

    // Set noise texture in shaders
    mShaderManager->ActivateProgram<ProgramType::ShipFlamesBackground1>();
    mShaderManager->SetTextureParameters<ProgramType::ShipFlamesBackground1>();
    mShaderManager->ActivateProgram<ProgramType::ShipFlamesBackground2>();
    mShaderManager->SetTextureParameters<ProgramType::ShipFlamesBackground2>();
    mShaderManager->ActivateProgram<ProgramType::ShipFlamesForeground1>();
    mShaderManager->SetTextureParameters<ProgramType::ShipFlamesForeground1>();
    mShaderManager->ActivateProgram<ProgramType::ShipFlamesForeground2>();
    mShaderManager->SetTextureParameters<ProgramType::ShipFlamesForeground2>();

    // Noise 2

    mShaderManager->ActivateTexture<ProgramParameterType::NoiseTexture2>();

    mUploadedTextureManager->UploadNextFrame(
        textureDatabase.GetGroup(TextureGroupType::Noise),
        1,
        GL_LINEAR);

    progressCallback((2.0f + TextureDatabaseProgressSteps + GenericTextureProgressSteps + CloudTextureProgressSteps + 2.0f) / TotalProgressSteps, "Loading noise textures...");

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, mUploadedTextureManager->GetOpenGLHandle(TextureGroupType::Noise, 1));
    CheckOpenGLError();

    // Set noise texture in shaders
    mShaderManager->ActivateProgram<ProgramType::HeatBlasterFlameCool>();
    mShaderManager->SetTextureParameters<ProgramType::HeatBlasterFlameCool>();
    mShaderManager->ActivateProgram<ProgramType::HeatBlasterFlameHeat>();
    mShaderManager->SetTextureParameters<ProgramType::HeatBlasterFlameHeat>();
    mShaderManager->ActivateProgram<ProgramType::FireExtinguisherSpray>();
    mShaderManager->SetTextureParameters<ProgramType::FireExtinguisherSpray>();


    //
    // Initialize world end texture
    //

    mShaderManager->ActivateTexture<ProgramParameterType::WorldBorderTexture>();

    mUploadedTextureManager->UploadMipmappedGroup(
        textureDatabase.GetGroup(TextureGroupType::WorldBorder),
        GL_LINEAR_MIPMAP_NEAREST,
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback((2.0f + TextureDatabaseProgressSteps + GenericTextureProgressSteps + CloudTextureProgressSteps + 2.0f + progress) / TotalProgressSteps, "Loading world end textures...");
        });

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, mUploadedTextureManager->GetOpenGLHandle(TextureGroupType::WorldBorder, 0));
    CheckOpenGLError();

    // Store metadata
    auto const & worldBoderTextureMetadata = textureDatabase.GetFrameMetadata(TextureGroupType::WorldBorder, 0);
    mWorldBorderTextureSize = worldBoderTextureMetadata.Size;

    // Set texture in shader
    mShaderManager->ActivateProgram<ProgramType::WorldBorder>();
    mShaderManager->SetTextureParameters<ProgramType::WorldBorder>();


    //
    // Initialize global settings
    //

    // Set anti-aliasing for lines
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // Enable blend for alpha transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth test
    glDisable(GL_DEPTH_TEST);

    // Set depth test parameters for when we'll need them
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);


    //
    // Update parameters
    //

    OnViewModelUpdated();

    OnAmbientLightIntensityUpdated();
    OnOceanTransparencyUpdated();
    OnOceanDarkeningRateUpdated();
    OnOceanRenderParametersUpdated();
    OnOceanTextureIndexUpdated();
    OnLandRenderParametersUpdated();
    OnLandTextureIndexUpdated();
    OnWaterContrastUpdated();
    OnWaterLevelOfDetailUpdated();
    OnShipRenderModeUpdated();
    OnDebugShipRenderModeUpdated();
    OnVectorFieldRenderModeUpdated();
    OnShowStressedSpringsUpdated();
    OnDrawHeatOverlayUpdated();
    OnHeatOverlayTransparencyUpdated();
    OnShipFlameRenderModeUpdated();
    OnShipFlameSizeAdjustmentUpdated();


    //
    // Flush all pending operations
    //

    glFinish();


    //
    // Notify progress
    //

    progressCallback(1.0f, "Loading textures...");
}

RenderContext::~RenderContext()
{
    glUseProgram(0u);
}

//////////////////////////////////////////////////////////////////////////////////

void RenderContext::Reset()
{
    // Clear ships
    mShips.clear();
}

void RenderContext::ValidateShip(
    ShipDefinition const & shipDefinition) const
{
    // Check texture against max texture size
    if (shipDefinition.TextureLayerImage.Size.Width > GameOpenGL::MaxTextureSize
        || shipDefinition.TextureLayerImage.Size.Height > GameOpenGL::MaxTextureSize)
    {
        throw GameException("We are sorry, but this ship's texture image is too large for your graphics driver");
    }
}

void RenderContext::AddShip(
    ShipId shipId,
    size_t pointCount,
    RgbaImageData texture,
    ShipDefinition::TextureOriginType textureOrigin)
{
    assert(shipId == mShips.size());

    size_t const newShipCount = mShips.size() + 1;

    // Tell all ships that there's a new ship
    for (auto & ship : mShips)
    {
        ship->SetShipCount(newShipCount);
    }

    // Add the ship
    mShips.emplace_back(
        new ShipRenderContext(
            shipId,
            newShipCount,
            pointCount,
            std::move(texture),
            textureOrigin,
            *mShaderManager,
            mGenericTextureAtlasOpenGLHandle,
            *mGenericTextureAtlasMetadata,
            mRenderStatistics,
            mViewModel,
            mAmbientLightIntensity,
            CalculateWaterColor(),
            mWaterContrast,
            mWaterLevelOfDetail,
            mShipRenderMode,
            mDebugShipRenderMode,
            mVectorFieldRenderMode,
            mShowStressedSprings,
            mDrawHeatOverlay,
            mHeatOverlayTransparency,
            mShipFlameRenderMode,
            mShipFlameSizeAdjustment));
}

RgbImageData RenderContext::TakeScreenshot()
{
    //
    // Flush draw calls
    //

    glFinish();

    //
    // Allocate buffer
    //

    int const canvasWidth = mViewModel.GetCanvasWidth();
    int const canvasHeight = mViewModel.GetCanvasHeight();

    auto pixelBuffer = std::make_unique<rgbColor[]>(canvasWidth * canvasHeight);

    //
    // Read pixels
    //

    // Alignment is byte
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    CheckOpenGLError();

    // Read the front buffer
    glReadBuffer(GL_FRONT);
    CheckOpenGLError();

    // Read
    glReadPixels(0, 0, canvasWidth, canvasHeight, GL_RGB, GL_UNSIGNED_BYTE, pixelBuffer.get());
    CheckOpenGLError();

    return RgbImageData(
        ImageSize(canvasWidth, canvasHeight),
        std::move(pixelBuffer));
}

//////////////////////////////////////////////////////////////////////////////////

void RenderContext::RenderStart()
{
    // Set polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Clear canvas - and depth buffer
    vec3f const clearColor = mFlatSkyColor.toVec3f() * mAmbientLightIntensity;
    glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Reset crosses of light, they are uploaded as needed
    mCrossOfLightVertexBuffer.clear();

    // Reset HeatBlaster flame, it's uploaded as needed
    mHeatBlasterFlameShaderToRender.reset();

    // Reset fire extinguisher spray, it's uploaded as needed
    mFireExtinguisherSprayShaderToRender.reset();

    // Communicate start to child contextes
    mTextRenderContext->RenderStart();

    // Reset stats
    mRenderStatistics.Reset();
}

void RenderContext::RenderSkyStart()
{
}

void RenderContext::UploadStarsStart(size_t starCount)
{
    //
    // Prepare star vertex buffer
    //

    if (starCount != mStarVertexBuffer.max_size())
    {
        // Reallocate GPU buffer
        glBindBuffer(GL_ARRAY_BUFFER, *mStarVBO);
        glBufferData(GL_ARRAY_BUFFER, starCount * sizeof(StarVertex), nullptr, GL_STATIC_DRAW);
        CheckOpenGLError();
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Reallocate CPU buffer
        mStarVertexBuffer.reset(starCount);
    }
    else
    {
        mStarVertexBuffer.clear();
    }
}

void RenderContext::UploadStarsEnd()
{
    //
    // Upload star vertex buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mStarVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mStarVertexBuffer.size() * sizeof(StarVertex), mStarVertexBuffer.data());
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderContext::UploadCloudsStart(size_t cloudCount)
{
    if (cloudCount > 0)
    {
        //
        // Prepare cloud quad buffer
        //

        glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);

        glBufferData(GL_ARRAY_BUFFER, cloudCount * sizeof(CloudQuad), nullptr, GL_STREAM_DRAW);
        CheckOpenGLError();

        mCloudQuadBuffer.map(cloudCount);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    else
    {
        mCloudQuadBuffer.reset();
    }
}

void RenderContext::UploadCloudsEnd()
{
    if (mCloudQuadBuffer.size() > 0)
    {
        //
        // Upload cloud quad buffer
        //

        glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);

        mCloudQuadBuffer.unmap();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void RenderContext::RenderSkyEnd()
{
    ////////////////////////////////////////////////////
    // Draw stars
    ////////////////////////////////////////////////////

    glBindVertexArray(*mStarVAO);

    mShaderManager->ActivateProgram<ProgramType::Stars>();

    glPointSize(0.5f);

    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(mStarVertexBuffer.size()));
    CheckOpenGLError();

    ////////////////////////////////////////////////////
    // Draw clouds
    ////////////////////////////////////////////////////

    if (mCloudQuadBuffer.size() > 0)
    {
        glBindVertexArray(*mCloudVAO);

        mShaderManager->ActivateProgram<ProgramType::Clouds>();

        if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
            glLineWidth(0.1f);

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(6 * mCloudQuadBuffer.size()));
        CheckOpenGLError();
    }

    ////////////////////////////////////////////////////

    glBindVertexArray(0);
}

void RenderContext::UploadLandStart(size_t slices)
{
    //
    // Prepare land segment buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);

    if (slices + 1 != mLandSegmentBufferAllocatedSize)
    {
        glBufferData(GL_ARRAY_BUFFER, (slices + 1) * sizeof(LandSegment), nullptr, GL_STREAM_DRAW);
        CheckOpenGLError();

        mLandSegmentBufferAllocatedSize = slices + 1;
    }

    mLandSegmentBuffer.map(slices + 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderContext::UploadLandEnd()
{
    //
    // Upload land segment buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);

    mLandSegmentBuffer.unmap();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderContext::RenderLand()
{
    glBindVertexArray(*mLandVAO);

    switch (mLandRenderMode)
    {
        case LandRenderMode::Flat:
        {
            mShaderManager->ActivateProgram<ProgramType::LandFlat>();
            break;
        }

        case LandRenderMode::Texture:
        {
            mShaderManager->ActivateProgram<ProgramType::LandTexture>();
            break;
        }
    }

    if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
        glLineWidth(0.1f);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mLandSegmentBuffer.size()));

    glBindVertexArray(0);
}

void RenderContext::UploadOceanStart(size_t slices)
{
    //
    // Prepare ocean segment buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mOceanVBO);

    if (slices + 1 != mOceanSegmentBufferAllocatedSize)
    {
        glBufferData(GL_ARRAY_BUFFER, (slices + 1) * sizeof(OceanSegment), nullptr, GL_STREAM_DRAW);
        CheckOpenGLError();

        mOceanSegmentBufferAllocatedSize = slices + 1;
    }

    mOceanSegmentBuffer.map(slices + 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderContext::UploadOceanEnd()
{
    //
    // Upload ocean segment buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mOceanVBO);

    mOceanSegmentBuffer.unmap();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderContext::RenderOcean(bool opaquely)
{
    float const transparency = opaquely ? 0.0f : mOceanTransparency;

    glBindVertexArray(*mOceanVAO);

    switch (mOceanRenderMode)
    {
        case OceanRenderMode::Depth:
        {
            mShaderManager->ActivateProgram<ProgramType::OceanDepth>();
            mShaderManager->SetProgramParameter<ProgramType::OceanDepth, ProgramParameterType::OceanTransparency>(
                transparency);

            break;
        }

        case OceanRenderMode::Flat:
        {
            mShaderManager->ActivateProgram<ProgramType::OceanFlat>();
            mShaderManager->SetProgramParameter<ProgramType::OceanFlat, ProgramParameterType::OceanTransparency>(
                transparency);

            break;
        }

        case OceanRenderMode::Texture:
        {
            mShaderManager->ActivateProgram<ProgramType::OceanTexture>();
            mShaderManager->SetProgramParameter<ProgramType::OceanTexture, ProgramParameterType::OceanTransparency>(
                transparency);

            break;
        }
    }

    if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
        glLineWidth(0.1f);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mOceanSegmentBuffer.size()));

    glBindVertexArray(0);
}

void RenderContext::RenderShipsStart()
{
    // Enable depth test, required by ships
    glEnable(GL_DEPTH_TEST);
}

void RenderContext::RenderShipsEnd()
{
    // Disable depth test
    glDisable(GL_DEPTH_TEST);
}

void RenderContext::RenderEnd()
{
    // Render crosses of light
    if (!mCrossOfLightVertexBuffer.empty())
    {
        RenderCrossesOfLight();
    }

    // Render HeatBlaster flames
    if (!!mHeatBlasterFlameShaderToRender)
    {
        RenderHeatBlasterFlame();
    }

    // Render fire extinguisher spray
    if (!!mFireExtinguisherSprayShaderToRender)
    {
        RenderFireExtinguisherSpray();
    }

    // Render world end
    RenderWorldBorder();

    // Communicate end to child contextes
    mTextRenderContext->RenderEnd();

    // Flush all pending commands (but not the GPU buffer)
    GameOpenGL::Flush();
}

////////////////////////////////////////////////////////////////////////////////////

void RenderContext::RenderCrossesOfLight()
{
    //
    // Upload buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mCrossOfLightVBO);
    glBufferData(GL_ARRAY_BUFFER,
        sizeof(CrossOfLightVertex) * mCrossOfLightVertexBuffer.size(),
        mCrossOfLightVertexBuffer.data(),
        GL_DYNAMIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    //
    // Render
    //

    glBindVertexArray(*mCrossOfLightVAO);

    mShaderManager->ActivateProgram<ProgramType::CrossOfLight>();

    assert((mCrossOfLightVertexBuffer.size() % 6) == 0);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mCrossOfLightVertexBuffer.size()));

    glBindVertexArray(0);
}

void RenderContext::RenderHeatBlasterFlame()
{
    //
    // Upload buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mHeatBlasterFlameVBO);
    glBufferData(GL_ARRAY_BUFFER,
        sizeof(HeatBlasterFlameVertex) * mHeatBlasterFlameVertexBuffer.size(),
        mHeatBlasterFlameVertexBuffer.data(),
        GL_DYNAMIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    //
    // Render
    //

    glBindVertexArray(*mHeatBlasterFlameVAO);

    assert(!!mHeatBlasterFlameShaderToRender);

    mShaderManager->ActivateProgram(*mHeatBlasterFlameShaderToRender);

    // Set time parameter
    mShaderManager->SetProgramParameter<ProgramParameterType::Time>(
        *mHeatBlasterFlameShaderToRender,
        GameWallClock::GetInstance().NowAsFloat());

    assert((mHeatBlasterFlameVertexBuffer.size() % 6) == 0);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mHeatBlasterFlameVertexBuffer.size()));

    glBindVertexArray(0);
}

void RenderContext::RenderFireExtinguisherSpray()
{
    //
    // Upload buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mFireExtinguisherSprayVBO);
    glBufferData(GL_ARRAY_BUFFER,
        sizeof(FireExtinguisherSprayVertex) * mFireExtinguisherSprayVertexBuffer.size(),
        mFireExtinguisherSprayVertexBuffer.data(),
        GL_DYNAMIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    //
    // Render
    //

    glBindVertexArray(*mFireExtinguisherSprayVAO);

    assert(!!mFireExtinguisherSprayShaderToRender);

    mShaderManager->ActivateProgram(*mFireExtinguisherSprayShaderToRender);

    // Set time parameter
    mShaderManager->SetProgramParameter<ProgramParameterType::Time>(
        *mFireExtinguisherSprayShaderToRender,
        GameWallClock::GetInstance().NowAsFloat());

    assert((mFireExtinguisherSprayVertexBuffer.size() % 6) == 0);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mFireExtinguisherSprayVertexBuffer.size()));

    glBindVertexArray(0);
}

void RenderContext::RenderWorldBorder()
{
    if (mIsWorldBorderVisible)
    {
        //
        // Render
        //

        glBindVertexArray(*mWorldBorderVAO);

        mShaderManager->ActivateProgram<ProgramType::WorldBorder>();

        assert((mWorldBorderVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mWorldBorderVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

////////////////////////////////////////////////////////////////////////////////////

void RenderContext::OnViewModelUpdated()
{
    //
    // Update ortho matrix
    //

    constexpr float ZFar = 1000.0f;
    constexpr float ZNear = 1.0f;

    ViewModel::ProjectionMatrix globalOrthoMatrix;
    mViewModel.CalculateGlobalOrthoMatrix(ZFar, ZNear, globalOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::LandFlat>();
    mShaderManager->SetProgramParameter<ProgramType::LandFlat, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::LandTexture>();
    mShaderManager->SetProgramParameter<ProgramType::LandTexture, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::OceanDepth>();
    mShaderManager->SetProgramParameter<ProgramType::OceanDepth, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::OceanFlat>();
    mShaderManager->SetProgramParameter<ProgramType::OceanFlat, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::OceanTexture>();
    mShaderManager->SetProgramParameter<ProgramType::OceanTexture, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::CrossOfLight>();
    mShaderManager->SetProgramParameter<ProgramType::CrossOfLight, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::HeatBlasterFlameCool>();
    mShaderManager->SetProgramParameter<ProgramType::HeatBlasterFlameCool, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::HeatBlasterFlameHeat>();
    mShaderManager->SetProgramParameter<ProgramType::HeatBlasterFlameHeat, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::FireExtinguisherSpray>();
    mShaderManager->SetProgramParameter<ProgramType::FireExtinguisherSpray, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::WorldBorder>();
    mShaderManager->SetProgramParameter<ProgramType::WorldBorder, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    //
    // Update canvas size
    //

    mShaderManager->ActivateProgram<ProgramType::CrossOfLight>();
    mShaderManager->SetProgramParameter<ProgramType::CrossOfLight, ProgramParameterType::ViewportSize>(
        static_cast<float>(mViewModel.GetCanvasWidth()),
        static_cast<float>(mViewModel.GetCanvasHeight()));

    //
    // Update all ships
    //

    for (auto & ship : mShips)
    {
        ship->OnViewModelUpdated();
    }

    //
    // Update world border
    //

    UpdateWorldBorder();
}

void RenderContext::OnAmbientLightIntensityUpdated()
{
    // Set parameters in all programs

    mShaderManager->ActivateProgram<ProgramType::Stars>();
    mShaderManager->SetProgramParameter<ProgramType::Stars, ProgramParameterType::StarTransparency>(
        pow(std::max(0.0f, 1.0f - mAmbientLightIntensity), 3.0f));

    mShaderManager->ActivateProgram<ProgramType::Clouds>();
    mShaderManager->SetProgramParameter<ProgramType::Clouds, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::LandFlat>();
    mShaderManager->SetProgramParameter<ProgramType::LandFlat, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::LandTexture>();
    mShaderManager->SetProgramParameter<ProgramType::LandTexture, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::OceanDepth>();
    mShaderManager->SetProgramParameter<ProgramType::OceanDepth, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::OceanFlat>();
    mShaderManager->SetProgramParameter<ProgramType::OceanFlat, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::OceanTexture>();
    mShaderManager->SetProgramParameter<ProgramType::OceanTexture, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::WorldBorder>();
    mShaderManager->SetProgramParameter<ProgramType::WorldBorder, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    // Update all ships
    for (auto & ship : mShips)
    {
        ship->SetAmbientLightIntensity(mAmbientLightIntensity);
    }

    // Update text context
    mTextRenderContext->UpdateAmbientLightIntensity(mAmbientLightIntensity);
}

void RenderContext::OnOceanTransparencyUpdated()
{
    // Set parameter in all programs

    mShaderManager->ActivateProgram<ProgramType::OceanDepth>();
    mShaderManager->SetProgramParameter<ProgramType::OceanDepth, ProgramParameterType::OceanTransparency>(
        mOceanTransparency);

    mShaderManager->ActivateProgram<ProgramType::OceanFlat>();
    mShaderManager->SetProgramParameter<ProgramType::OceanFlat, ProgramParameterType::OceanTransparency>(
        mOceanTransparency);

    mShaderManager->ActivateProgram<ProgramType::OceanTexture>();
    mShaderManager->SetProgramParameter<ProgramType::OceanTexture, ProgramParameterType::OceanTransparency>(
        mOceanTransparency);
}

void RenderContext::OnOceanDarkeningRateUpdated()
{
    // Set parameter in all programs

    mShaderManager->ActivateProgram<ProgramType::LandTexture>();
    mShaderManager->SetProgramParameter<ProgramType::LandTexture, ProgramParameterType::OceanDarkeningRate>(
        mOceanDarkeningRate / 50.0f);

    mShaderManager->ActivateProgram<ProgramType::OceanDepth>();
    mShaderManager->SetProgramParameter<ProgramType::OceanDepth, ProgramParameterType::OceanDarkeningRate>(
        mOceanDarkeningRate / 50.0f);

    mShaderManager->ActivateProgram<ProgramType::OceanTexture>();
    mShaderManager->SetProgramParameter<ProgramType::OceanTexture, ProgramParameterType::OceanDarkeningRate>(
        mOceanDarkeningRate / 50.0f);
}

void RenderContext::OnOceanRenderParametersUpdated()
{
    // Set ocean parameters in all water programs

    auto depthColorStart = mDepthOceanColorStart.toVec3f();
    mShaderManager->ActivateProgram<ProgramType::OceanDepth>();
    mShaderManager->SetProgramParameter<ProgramType::OceanDepth, ProgramParameterType::OceanDepthColorStart>(
        depthColorStart.x,
        depthColorStart.y,
        depthColorStart.z);

    auto depthColorEnd = mDepthOceanColorEnd.toVec3f();
    mShaderManager->ActivateProgram<ProgramType::OceanDepth>();
    mShaderManager->SetProgramParameter<ProgramType::OceanDepth, ProgramParameterType::OceanDepthColorEnd>(
        depthColorEnd.x,
        depthColorEnd.y,
        depthColorEnd.z);

    auto flatColor = mFlatOceanColor.toVec3f();
    mShaderManager->ActivateProgram<ProgramType::OceanFlat>();
    mShaderManager->SetProgramParameter<ProgramType::OceanFlat, ProgramParameterType::OceanFlatColor>(
        flatColor.x,
        flatColor.y,
        flatColor.z);

    //
    // Tell ships about the water color
    //

    auto const waterColor = CalculateWaterColor();
    for (auto & s : mShips)
    {
        s->SetWaterColor(waterColor);
    }
}

void RenderContext::OnOceanTextureIndexUpdated()
{
    if (mSelectedOceanTextureIndex != mLoadedOceanTextureIndex)
    {
        //
        // Reload the ocean texture
        //

        // Clamp the texture index
        mLoadedOceanTextureIndex = std::min(mSelectedOceanTextureIndex, mOceanTextureFrameSpecifications.size() - 1);

        // Load texture image
        auto oceanTextureFrame = mOceanTextureFrameSpecifications[mLoadedOceanTextureIndex].LoadFrame();

        // Soften texture image
        ImageTools::BlendWithColor(
            oceanTextureFrame.TextureData,
            rgbColor(0x87, 0xce, 0xfa), // cornflower blue
            mOceanTransparency);

        // Activate texture
        mShaderManager->ActivateTexture<ProgramParameterType::OceanTexture>();

        // Create texture
        GLuint tmpGLuint;
        glGenTextures(1, &tmpGLuint);
        mOceanTextureOpenGLHandle = tmpGLuint; // Eventually destroy previous one

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mOceanTextureOpenGLHandle);
        CheckOpenGLError();

        // Upload texture
        GameOpenGL::UploadMipmappedTexture(std::move(oceanTextureFrame.TextureData));

        // Set repeat mode
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        CheckOpenGLError();

        // Set filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CheckOpenGLError();

        // Set texture and texture parameters in shader
        mShaderManager->ActivateProgram<ProgramType::OceanTexture>();
        mShaderManager->SetProgramParameter<ProgramType::OceanTexture, ProgramParameterType::TextureScaling>(
            1.0f / oceanTextureFrame.Metadata.WorldWidth,
            1.0f / oceanTextureFrame.Metadata.WorldHeight);
        mShaderManager->SetTextureParameters<ProgramType::OceanTexture>();
    }
}

void RenderContext::OnLandRenderParametersUpdated()
{
    // Set land parameters in all water programs

    auto flatColor = mFlatLandColor.toVec3f();
    mShaderManager->ActivateProgram<ProgramType::LandFlat>();
    mShaderManager->SetProgramParameter<ProgramType::LandFlat, ProgramParameterType::LandFlatColor>(
        flatColor.x,
        flatColor.y,
        flatColor.z);
}

void RenderContext::OnLandTextureIndexUpdated()
{
    if (mSelectedLandTextureIndex != mLoadedLandTextureIndex)
    {
        //
        // Reload the land texture
        //

        // Clamp the texture index
        mLoadedLandTextureIndex = std::min(mSelectedLandTextureIndex, mLandTextureFrameSpecifications.size() - 1);

        // Load texture image
        auto landTextureFrame = mLandTextureFrameSpecifications[mLoadedLandTextureIndex].LoadFrame();

        // Activate texture
        mShaderManager->ActivateTexture<ProgramParameterType::LandTexture>();

        // Create texture
        GLuint tmpGLuint;
        glGenTextures(1, &tmpGLuint);
        mLandTextureOpenGLHandle = tmpGLuint; // Eventually destroy previous one

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mLandTextureOpenGLHandle);
        CheckOpenGLError();

        // Upload texture
        GameOpenGL::UploadMipmappedTexture(std::move(landTextureFrame.TextureData));

        // Set repeat mode
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        CheckOpenGLError();

        // Set filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CheckOpenGLError();

        // Set texture and texture parameters in shader
        mShaderManager->ActivateProgram<ProgramType::LandTexture>();
        mShaderManager->SetProgramParameter<ProgramType::LandTexture, ProgramParameterType::TextureScaling>(
            1.0f / landTextureFrame.Metadata.WorldWidth,
            1.0f / landTextureFrame.Metadata.WorldHeight);
        mShaderManager->SetTextureParameters<ProgramType::LandTexture>();
    }
}

void RenderContext::OnWaterContrastUpdated()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->SetWaterContrast(mWaterContrast);
    }
}

void RenderContext::OnWaterLevelOfDetailUpdated()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->SetWaterLevelThreshold(mWaterLevelOfDetail);
    }
}

void RenderContext::OnShipRenderModeUpdated()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->SetShipRenderMode(mShipRenderMode);
    }
}

void RenderContext::OnDebugShipRenderModeUpdated()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->SetDebugShipRenderMode(mDebugShipRenderMode);
    }
}

void RenderContext::OnVectorFieldRenderModeUpdated()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->SetVectorFieldRenderMode(mVectorFieldRenderMode);
    }
}

void RenderContext::OnShowStressedSpringsUpdated()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->SetShowStressedSprings(mShowStressedSprings);
    }
}

void RenderContext::OnDrawHeatOverlayUpdated()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->SetDrawHeatOverlay(mDrawHeatOverlay);
    }
}

void RenderContext::OnHeatOverlayTransparencyUpdated()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->SetHeatOverlayTransparency(mHeatOverlayTransparency);
    }
}

void RenderContext::OnShipFlameRenderModeUpdated()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->SetShipFlameRenderMode(mShipFlameRenderMode);
    }
}

void RenderContext::OnShipFlameSizeAdjustmentUpdated()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->SetShipFlameSizeAdjustment(mShipFlameSizeAdjustment);
    }
}

template <typename TVertexBuffer>
static void MakeQuad(
    float x1,
    float y1,
    float tx1,
    float ty1,
    float x2,
    float y2,
    float tx2,
    float ty2,
    TVertexBuffer & buffer)
{
    buffer.emplace_back(x1, y1, tx1, ty1);
    buffer.emplace_back(x1, y2, tx1, ty2);
    buffer.emplace_back(x2, y1, tx2, ty1);
    buffer.emplace_back(x1, y2, tx1, ty2);
    buffer.emplace_back(x2, y1, tx2, ty1);
    buffer.emplace_back(x2, y2, tx2, ty2);
}

void RenderContext::UpdateWorldBorder()
{
    // Calculate width, in world coordinates, of the world border, under the constraint
    // that we want to ensure that the texture is rendered with its original size
    float const worldBorderWorldWidth = mViewModel.PixelWidthToWorldWidth(static_cast<float>(mWorldBorderTextureSize.Width / 2));
    float const worldBorderWorldHeight = mViewModel.PixelHeightToWorldHeight(static_cast<float>(mWorldBorderTextureSize.Height / 2));

    // Max texture coordinates - chosen so that texture dimensions do not depend on zoom
    float const textureWidth = GameParameters::MaxWorldWidth / worldBorderWorldWidth;
    float const textureHeight = GameParameters::MaxWorldHeight / worldBorderWorldHeight;

    // Dx for drawing texture at dead-center pixel
    float const dx = 0.5f / static_cast<float>(mWorldBorderTextureSize.Width);

    //
    // Check which sides of the border we need to draw
    //

    mWorldBorderVertexBuffer.clear();

    // Left
    if (-GameParameters::HalfMaxWorldWidth + worldBorderWorldWidth >= mViewModel.GetVisibleWorldTopLeft().x)
    {
        MakeQuad(
            // Top-left
            -GameParameters::HalfMaxWorldWidth,
            GameParameters::HalfMaxWorldHeight,
            0.0f + dx,
            0.0f + dx,
            // Bottom-right
            -GameParameters::HalfMaxWorldWidth + worldBorderWorldWidth,
            -GameParameters::HalfMaxWorldHeight,
            1.0f - dx,
            textureHeight - dx,
            mWorldBorderVertexBuffer);
    }

    // Right
    if (GameParameters::HalfMaxWorldWidth - worldBorderWorldWidth <= mViewModel.GetVisibleWorldBottomRight().x)
    {
        MakeQuad(
            // Top-left
            GameParameters::HalfMaxWorldWidth - worldBorderWorldWidth,
            GameParameters::HalfMaxWorldHeight,
            0.0f + dx,
            0.0f + dx,
            // Bottom-right
            GameParameters::HalfMaxWorldWidth,
            -GameParameters::HalfMaxWorldHeight,
            1.0f - dx,
            textureHeight - dx,
            mWorldBorderVertexBuffer);
    }

    // Top
    if (GameParameters::HalfMaxWorldHeight - worldBorderWorldHeight <= mViewModel.GetVisibleWorldTopLeft().y)
    {
        MakeQuad(
            // Top-left
            -GameParameters::HalfMaxWorldWidth,
            GameParameters::HalfMaxWorldHeight,
            0.0f + dx,
            0.0f + dx,
            // Bottom-right
            GameParameters::HalfMaxWorldWidth,
            GameParameters::HalfMaxWorldHeight - worldBorderWorldHeight,
            textureWidth - dx,
            1.0f - dx,
            mWorldBorderVertexBuffer);
    }

    // Bottom
    if (-GameParameters::HalfMaxWorldHeight + worldBorderWorldHeight >= mViewModel.GetVisibleWorldBottomRight().y)
    {
        MakeQuad(
            // Top-left
            -GameParameters::HalfMaxWorldWidth,
            -GameParameters::HalfMaxWorldHeight + worldBorderWorldHeight,
            0.0f + dx,
            0.0f + dx,
            // Bottom-right
            GameParameters::HalfMaxWorldWidth,
            -GameParameters::HalfMaxWorldHeight,
            textureWidth - dx,
            1.0f - dx,
            mWorldBorderVertexBuffer);
    }

    if (!mWorldBorderVertexBuffer.empty())
    {
        //
        // Upload buffer
        //

        glBindBuffer(GL_ARRAY_BUFFER, *mWorldBorderVBO);

        glBufferData(GL_ARRAY_BUFFER,
            sizeof(WorldBorderVertex) * mWorldBorderVertexBuffer.size(),
            mWorldBorderVertexBuffer.data(),
            GL_STATIC_DRAW);
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Remember we have to draw the world border
        mIsWorldBorderVisible = true;
    }
    else
    {
        // No need to draw the world border
        mIsWorldBorderVisible = false;
    }
}

vec4f RenderContext::CalculateWaterColor() const
{
    switch (mOceanRenderMode)
    {
        case OceanRenderMode::Depth:
        {
            return
                (mDepthOceanColorStart.toVec4f(1.0f) + mDepthOceanColorEnd.toVec4f(1.0f))
                / 2.0f;
        }

        case OceanRenderMode::Flat:
        {
            return mFlatOceanColor.toVec4f(1.0f);
        }

        default:
        {
            assert(mOceanRenderMode == OceanRenderMode::Texture); // Darn VS - warns

            return vec4f(0.0f, 0.0f, 0.8f, 1.0f);
        }
    }
}

}