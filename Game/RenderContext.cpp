/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderContext.h"

#include <GameCore/GameException.h>
#include <GameCore/Log.h>

#include <cstring>

namespace Render {

RenderContext::RenderContext(
    ResourceLoader & resourceLoader,
    ProgressCallback const & progressCallback)
    // Buffers
    : mStarVertexBuffer()
    , mStarVBO()
    , mCloudQuadBuffer()
    , mCloudVBO()
    , mLandSegmentBuffer()
    , mLandVBO()
    , mOceanSegmentBuffer()
    , mOceanVBO()
    , mCrossOfLightVertexBuffer()
    , mCrossOfLightVBO()
    // VAOs
    , mStarVAO()
    , mCloudVAO()
    , mLandVAO()
    , mOceanVAO()
    , mCrossOfLightVAO()
    // Textures
    , mCloudTextureAtlasOpenGLHandle()
    , mCloudTextureAtlasMetadata()
    // Ships
    , mShips()
    , mGenericTextureAtlasOpenGLHandle()
    , mGenericTextureAtlasMetadata()
    // Managers
    , mShaderManager()
    , mTextureRenderManager()
    , mTextRenderContext()
    // Render parameters
    , mViewModel(1.0f, vec2f::zero(), 100, 100)
    , mFlatSkyColor(0x87, 0xce, 0xfa) // (cornflower blue)
    , mAmbientLightIntensity(1.0f)
    , mOceanTransparency(0.8125f)
    , mShowShipThroughOcean(false)
    , mWaterContrast(0.6875f)
    , mWaterLevelOfDetail(0.6875f)
    , mShipRenderMode(ShipRenderMode::Texture)
    , mDebugShipRenderMode(DebugShipRenderMode::None)
    , mOceanRenderMode(OceanRenderMode::Texture)
    , mDepthOceanColorStart(0xda, 0xf1, 0xfe)
    , mDepthOceanColorEnd(0x00, 0x00, 0x00)
    , mFlatOceanColor(0x00, 0x3d, 0x99)
    , mLandRenderMode(LandRenderMode::Texture)
    , mFlatLandColor(0x72, 0x46, 0x05)
    , mVectorFieldRenderMode(VectorFieldRenderMode::None)
    , mVectorFieldLengthMultiplier(1.0f)
    , mShowStressedSprings(false)
    // Statistics
    , mRenderStatistics()
{
    static constexpr float GenericTextureProgressSteps = 10.0f;
    static constexpr float CloudTextureProgressSteps = 4.0f;

    // Shaders, TextRenderContext, TextureDatabase, GenericTextureAtlas, Clouds, Land, Ocean
    static constexpr float TotalProgressSteps = 3.0f + GenericTextureProgressSteps + CloudTextureProgressSteps + 2.0f;

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

    progressCallback(2.0f / TotalProgressSteps, "Loading textures...");

    TextureDatabase textureDatabase = TextureDatabase::Load(
        resourceLoader,
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback((2.0f + progress) / TotalProgressSteps, "Loading textures...");
        });

    // Create texture render manager
    mTextureRenderManager = std::make_unique<TextureRenderManager>();



    //
    // Create generic texture atlas
    //
    // Atlas-ize all textures EXCEPT the following:
    // - Land, Ocean: we need these to be wrapping
    // - Clouds: we keep these in a separate atlas, we have to rebind anyway
    //

    mShaderManager->ActivateTexture<ProgramParameterType::GenericTexturesAtlasTexture>();

    TextureAtlasBuilder genericTextureAtlasBuilder;
    for (auto const & group : textureDatabase.GetGroups())
    {
        if (TextureGroupType::Land != group.Group
            && TextureGroupType::Ocean != group.Group
            && TextureGroupType::Cloud != group.Group)
        {
            genericTextureAtlasBuilder.Add(group);
        }
    }

    TextureAtlas genericTextureAtlas = genericTextureAtlasBuilder.BuildAtlas(
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback((3.0f + progress * GenericTextureProgressSteps) / TotalProgressSteps, "Loading textures...");
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

    GLuint vbos[5];
    glGenBuffers(5, vbos);
    mStarVBO = vbos[0];
    mCloudVBO = vbos[1];
    mLandVBO = vbos[2];
    mOceanVBO = vbos[3];
    mCrossOfLightVBO = vbos[4];


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
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Land), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
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
    // Initialize cloud texture atlas
    //

    mShaderManager->ActivateTexture<ProgramParameterType::CloudTexture>();

    TextureAtlasBuilder cloudAtlasBuilder;
    cloudAtlasBuilder.Add(textureDatabase.GetGroup(TextureGroupType::Cloud));

    TextureAtlas cloudTextureAtlas = cloudAtlasBuilder.BuildAtlas(
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback((3.0f + GenericTextureProgressSteps + progress * CloudTextureProgressSteps) / TotalProgressSteps, "Loading textures...");
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
    // Initialize land texture
    //

    mShaderManager->ActivateTexture<ProgramParameterType::LandTexture>();

    mTextureRenderManager->UploadMipmappedGroup(
        textureDatabase.GetGroup(TextureGroupType::Land),
        GL_LINEAR_MIPMAP_NEAREST,
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback((3.0f + GenericTextureProgressSteps + CloudTextureProgressSteps + progress) / TotalProgressSteps, "Loading textures...");
        });

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, mTextureRenderManager->GetOpenGLHandle(TextureGroupType::Land, 0));
    CheckOpenGLError();

    // Set texture and texture parameters in shader
    auto const & landTextureMetadata = textureDatabase.GetFrameMetadata(TextureGroupType::Land, 0);
    mShaderManager->ActivateProgram<ProgramType::LandTexture>();
    mShaderManager->SetProgramParameter<ProgramType::LandTexture, ProgramParameterType::TextureScaling>(
            1.0f / landTextureMetadata.WorldWidth,
            1.0f / landTextureMetadata.WorldHeight);
    mShaderManager->SetTextureParameters<ProgramType::LandTexture>();


    //
    // Initialize ocean texture
    //

    // Activate texture
    mShaderManager->ActivateTexture<ProgramParameterType::OceanTexture>();

    // Upload texture
    mTextureRenderManager->UploadMipmappedGroup(
        textureDatabase.GetGroup(TextureGroupType::Ocean),
        GL_LINEAR_MIPMAP_NEAREST,
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback((3.0f + GenericTextureProgressSteps + CloudTextureProgressSteps + 1.0f + progress) / TotalProgressSteps, "Loading textures...");
        });

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, mTextureRenderManager->GetOpenGLHandle(TextureGroupType::Ocean, 0));
    CheckOpenGLError();

    // Set texture and texture parameters in shader
    auto const & oceanTextureMetadata = textureDatabase.GetFrameMetadata(TextureGroupType::Ocean, 0);
    mShaderManager->ActivateProgram<ProgramType::OceanTexture>();
    mShaderManager->SetProgramParameter<ProgramType::OceanTexture, ProgramParameterType::TextureScaling>(
            1.0f / oceanTextureMetadata.WorldWidth,
            1.0f / oceanTextureMetadata.WorldHeight);
    mShaderManager->SetTextureParameters<ProgramType::OceanTexture>();


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
    OnWaterContrastUpdated();
    OnWaterLevelOfDetailUpdated();
    OnShipRenderModeUpdated();
    OnDebugShipRenderModeUpdated();
    OnOceanRenderParametersUpdated();
    OnLandRenderParametersUpdated();
    OnVectorFieldRenderModeUpdated();
    OnShowStressedSpringsUpdated();

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
            mWaterContrast,
            mWaterLevelOfDetail,
            mShipRenderMode,
            mDebugShipRenderMode,
            mVectorFieldRenderMode,
            mShowStressedSprings));
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

    // Clear canvas - and stencil buffer
    vec3f const clearColor = mFlatSkyColor.toVec3f() * mAmbientLightIntensity;
    glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);
    glClearStencil(0x00);
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Reset crosses of light, they are uploaded as needed
    mCrossOfLightVertexBuffer.clear();

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
    //
    // Prepare cloud quad buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);

    glBufferData(GL_ARRAY_BUFFER, cloudCount * sizeof(CloudQuad), nullptr, GL_STREAM_DRAW);
    CheckOpenGLError();

    mCloudQuadBuffer.map(cloudCount);
}

void RenderContext::UploadCloudsEnd()
{
    //
    // Upload cloud quad buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);
    mCloudQuadBuffer.unmap();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderContext::RenderSkyEnd()
{
    ////////////////////////////////////////////////////
    // Draw ocean stencil
    ////////////////////////////////////////////////////

    // Enable stencil test
    glEnable(GL_STENCIL_TEST);

    // Disable writing to the color buffer
    glColorMask(false, false, false, false);

    // Write all one's to stencil buffer
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    //
    // Draw ocean
    //

    glBindVertexArray(*mOceanVAO);

    // Use matte ocean program
    mShaderManager->ActivateProgram<ProgramType::MatteOcean>();

    // Make sure polygons are filled in any case
    if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mOceanSegmentBuffer.size()));

    // Don't write anything to stencil buffer now
    glStencilMask(0x00);

    // Re-enable writing to the color buffer
    glColorMask(true, true, true, true);

    // Reset wireframe mode, if enabled
    if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Enable stenciling - now only draw where there are no 1's
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

    ////////////////////////////////////////////////////
    // Draw stars with stencil test
    ////////////////////////////////////////////////////

    glBindVertexArray(*mStarVAO);

    mShaderManager->ActivateProgram<ProgramType::Stars>();

    glPointSize(0.5f);

    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(mStarVertexBuffer.size()));
    CheckOpenGLError();

    ////////////////////////////////////////////////////
    // Draw clouds with stencil test
    ////////////////////////////////////////////////////

    glBindVertexArray(*mCloudVAO);

    mShaderManager->ActivateProgram<ProgramType::Clouds>();

    if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
        glLineWidth(0.1f);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(6 * mCloudQuadBuffer.size()));
    CheckOpenGLError();

    ////////////////////////////////////////////////////

    glBindVertexArray(0);

    // Disable stencil test
    glDisable(GL_STENCIL_TEST);
}

void RenderContext::UploadLandAndOceanStart(size_t slices)
{
    //
    // Prepare land segment buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);

    glBufferData(GL_ARRAY_BUFFER, (slices + 1) * sizeof(LandSegment), nullptr, GL_STREAM_DRAW);
    CheckOpenGLError();

    mLandSegmentBuffer.map(slices + 1);


    //
    // Prepare ocean segment buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mOceanVBO);

    glBufferData(GL_ARRAY_BUFFER, (slices + 1) * sizeof(OceanSegment), nullptr, GL_STREAM_DRAW);
    CheckOpenGLError();

    mOceanSegmentBuffer.map(slices + 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderContext::UploadLandAndOceanEnd()
{
    //
    // Upload land segment buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);
    mLandSegmentBuffer.unmap();


    //
    // Upload ocean segment buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mOceanVBO);
    mOceanSegmentBuffer.unmap();

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

void RenderContext::RenderOcean()
{
    glBindVertexArray(*mOceanVAO);

    switch (mOceanRenderMode)
    {
        case OceanRenderMode::Depth:
        {
            mShaderManager->ActivateProgram<ProgramType::OceanDepth>();
            break;
        }

        case OceanRenderMode::Flat:
        {
            mShaderManager->ActivateProgram<ProgramType::OceanFlat>();
            break;
        }

        case OceanRenderMode::Texture:
        {
            mShaderManager->ActivateProgram<ProgramType::OceanTexture>();
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

    mShaderManager->ActivateProgram<ProgramType::MatteOcean>();
    mShaderManager->SetProgramParameter<ProgramType::MatteOcean, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::CrossOfLight>();
    mShaderManager->SetProgramParameter<ProgramType::CrossOfLight, ProgramParameterType::OrthoMatrix>(
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

}