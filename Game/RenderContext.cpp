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
    : mShaderManager()
    , mTextureRenderManager()
    , mTextRenderContext()
    // Stars
    , mStarElementBuffer()
    , mStarVBO()
    // Clouds
    , mCloudElementBuffer()
    , mCurrentCloudElementCount(0u)
    , mCloudElementCount(0u)
    , mCloudVBO()
    , mCloudTextureAtlasOpenGLHandle()
    , mCloudTextureAtlasMetadata()
    // Land
    , mLandElementBuffer()
    , mCurrentLandElementCount(0u)
    , mLandElementCount(0u)
    , mLandVBO()
    // Ocean
    , mOceanElementBuffer()
    , mCurrentOceanElementCount(0u)
    , mOceanElementCount(0u)
    , mOceanVBO()
    // Ships
    , mShips()
    , mGenericTextureAtlasOpenGLHandle()
    , mGenericTextureAtlasMetadata()
    // Cross of light
    , mCrossOfLightBuffer()
    , mCrossOfLightVBO()
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
    , mDepthOceanColorStart(0xe6, 0xf0, 0xff)
    , mDepthOceanColorEnd(0x00, 0x0a, 0x1a)
    , mFlatOceanColor(0x00, 0x3d, 0x99)
    , mLandRenderMode(LandRenderMode::Texture)
    , mFlatLandColor(0x9b, 0x60, 0x07)
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
    // Setup OpenGL context
    //

    // Activate shared texture unit
    mShaderManager->ActivateTexture<ProgramParameterType::SharedTexture>();


    //
    // Load shader manager
    //

    progressCallback(0.0f, "Loading shaders...");

    mShaderManager = ShaderManager<ShaderManagerTraits>::CreateInstance(resourceLoader.GetRenderShadersRootPath());


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
    // - Clouds: we keep these separate, we have to rebind anyway
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

    // Set hardcoded parameters
    mShaderManager->ActivateProgram<ProgramType::ShipGenericTextures>();
    mShaderManager->SetTextureParameters<ProgramType::ShipGenericTextures>();


    //
    // Initialize stars
    //

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mStarVBO = tmpGLuint;


    //
    // Initialize clouds
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

    // Bind texture
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

    // Set hardcoded parameters
    mShaderManager->ActivateProgram<ProgramType::Clouds>();
    mShaderManager->SetTextureParameters<ProgramType::Clouds>();

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mCloudVBO = tmpGLuint;


    //
    // Initialize land
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

    // Set hardcoded parameters
    auto const & landTextureMetadata = textureDatabase.GetFrameMetadata(TextureGroupType::Land, 0);
    mShaderManager->ActivateProgram<ProgramType::LandTexture>();
    mShaderManager->SetProgramParameter<ProgramType::LandTexture, ProgramParameterType::TextureScaling>(
            1.0f / landTextureMetadata.WorldWidth,
            1.0f / landTextureMetadata.WorldHeight);
    mShaderManager->SetTextureParameters<ProgramType::LandTexture>();

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mLandVBO = tmpGLuint;



    //
    // Initialize ocean
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

    // Set hardcoded parameters
    auto const & oceanTextureMetadata = textureDatabase.GetFrameMetadata(TextureGroupType::Ocean, 0);
    mShaderManager->ActivateProgram<ProgramType::OceanTexture>();
    mShaderManager->SetProgramParameter<ProgramType::OceanTexture, ProgramParameterType::TextureScaling>(
            1.0f / oceanTextureMetadata.WorldWidth,
            1.0f / oceanTextureMetadata.WorldHeight);
    mShaderManager->SetTextureParameters<ProgramType::OceanTexture>();

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mOceanVBO = tmpGLuint;

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mOceanVBO);

    // Associate ocean vertex attribute with this VBO and describe it
    // (it's fully dedicated)
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::OceanAttribute), (2 + 1), GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void*)0);


    //
    // Initialize cross of light
    //

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mCrossOfLightVBO = tmpGLuint;


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

    // Reset crosses of light
    mCrossOfLightBuffer.clear();

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
    // Prepare stars buffer
    //

    mStarElementBuffer.clear();
    mStarElementBuffer.reserve(starCount);
}

void RenderContext::UploadStarsEnd()
{
}

void RenderContext::UploadCloudsStart(size_t cloudCount)
{
    //
    // Prepare clouds buffers
    //

    if (cloudCount != mCloudElementCount)
    {
        // Bind VBO
        glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);
        CheckOpenGLError();

        // Realloc GPU buffer
        mCloudElementCount = cloudCount;
        glBufferData(GL_ARRAY_BUFFER, mCloudElementCount * sizeof(CloudElement), nullptr, GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        // Realloc buffer
        mCloudElementBuffer.reset(new CloudElement[mCloudElementCount]);
    }

    // Reset current count of clouds
    mCurrentCloudElementCount = 0u;
}

void RenderContext::UploadCloudsEnd()
{
}

void RenderContext::RenderSkyEnd()
{
    ////////////////////////////////////////////////////
    // Draw ocean stencil
    ////////////////////////////////////////////////////

    // Enable stencil test
    glEnable(GL_STENCIL_TEST);

    // Use matte ocean program
    mShaderManager->ActivateProgram<ProgramType::MatteOcean>();

    // Disable writing to the color buffer
    glColorMask(false, false, false, false);

    // Write all one's to stencil buffer
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    // Disable vertex attribute 0, as we don't use it
    glDisableVertexAttribArray(0);

    // Make sure polygons are filled in any case
    if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mOceanElementCount));

    // Don't write anything to stencil buffer now
    glStencilMask(0x00);

    // Re-enable writing to the color buffer
    glColorMask(true, true, true, true);

    // Reset wireframe mode, if enabled
    if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Enable stenciling - only draw where there are no 1's
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

    ////////////////////////////////////////////////////
    // Draw stars with stencil test
    ////////////////////////////////////////////////////

    // Use program
    mShaderManager->ActivateProgram<ProgramType::Stars>();

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mStarVBO);
    CheckOpenGLError();

    // Upload buffer
    glBufferData(GL_ARRAY_BUFFER, mStarElementBuffer.size() * sizeof(StarElement), mStarElementBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();

    // Describe vertex attribute 0
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::SharedAttribute0), (2 + 1), GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void*)0);
    CheckOpenGLError();

    // Enable vertex attribute 0
    glEnableVertexAttribArray(0);

    // Set point size
    glPointSize(0.5f);

    // Draw
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(mStarElementBuffer.size()));

    ////////////////////////////////////////////////////
    // Draw clouds with stencil test
    ////////////////////////////////////////////////////

    assert(mCurrentCloudElementCount == mCloudElementCount);

    // Use program
    mShaderManager->ActivateProgram<ProgramType::Clouds>();

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);
    CheckOpenGLError();

    // Upload buffer
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(CloudElement) * mCloudElementCount, mCloudElementBuffer.get());
    CheckOpenGLError();

    // Describe vertex attribute 0
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::SharedAttribute0), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    CheckOpenGLError();

    if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
        glLineWidth(0.1f);

    // Draw
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLint>(6 * mCloudElementCount));

    ////////////////////////////////////////////////////

    // Disable stencil test
    glDisable(GL_STENCIL_TEST);
}

void RenderContext::UploadLandAndOceanStart(size_t slices)
{
    //
    // Prepare land buffer
    //

    if (slices + 1 != mLandElementCount)
    {
        // Bind VBO
        glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);
        CheckOpenGLError();

        // Realloc GPU buffer
        mLandElementCount = slices + 1;
        glBufferData(GL_ARRAY_BUFFER, mLandElementCount * sizeof(LandElement), nullptr, GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        // Realloc buffer
        mLandElementBuffer.reset(new LandElement[mLandElementCount]);
    }

    // Reset current count of land elements
    mCurrentLandElementCount = 0u;


    //
    // Prepare ocean buffer
    //

    if (slices + 1 != mOceanElementCount)
    {
        // Bind VBO
        glBindBuffer(GL_ARRAY_BUFFER, *mOceanVBO);
        CheckOpenGLError();

        // Realloc GPU buffer
        mOceanElementCount = slices + 1;
        glBufferData(GL_ARRAY_BUFFER, mOceanElementCount * sizeof(OceanElement), nullptr, GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        // Realloc buffer
        mOceanElementBuffer.reset(new OceanElement[mOceanElementCount]);
    }

    // Reset count of ocean elements
    mCurrentOceanElementCount = 0u;
}

void RenderContext::UploadLandAndOceanEnd()
{
    // Bind land VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);
    CheckOpenGLError();

    // Upload buffer
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(LandElement) * mLandElementCount, mLandElementBuffer.get());

    // Describe vertex attribute 1
    // (we know we'll be using it before CrossOfLight - which is the only subsequent user of this attribute,
    //  so we can describe it now and avoid a bind later)
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::SharedAttribute1), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    CheckOpenGLError();



    // Bind ocean VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mOceanVBO);
    CheckOpenGLError();

    // Upload ocean buffer
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(OceanElement) * mOceanElementCount, mOceanElementBuffer.get());

    // No need to describe ocean's vertex attribute as it is dedicated and we have described it already once and for all
}

void RenderContext::RenderLand()
{
    assert(mCurrentLandElementCount == mLandElementCount);

    // Use program
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

    // No need to bind VBO - we've done that at UploadLandAndOceanEnd(),
    // and we know nothing's been intervening

    // Disable vertex attribute 0
    glDisableVertexAttribArray(0);

    if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
        glLineWidth(0.1f);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mLandElementCount));
}

void RenderContext::RenderOcean()
{
    assert(mCurrentOceanElementCount == mOceanElementCount);

    // Use program
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

    // Disable vertex attribute 0
    glDisableVertexAttribArray(0);

    if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
        glLineWidth(0.1f);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mOceanElementCount));
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
    if (!mCrossOfLightBuffer.empty())
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
    // Use program
    mShaderManager->ActivateProgram<ProgramType::CrossOfLight>();

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mCrossOfLightVBO);
    CheckOpenGLError();

    // Upload buffer
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(CrossOfLightElement) * mCrossOfLightBuffer.size(),
        mCrossOfLightBuffer.data(),
        GL_DYNAMIC_DRAW);

    // Describe vertex attributes 0 and 1
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::SharedAttribute0), 4, GL_FLOAT, GL_FALSE, sizeof(CrossOfLightElement), (void*)0);
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::SharedAttribute1), 1, GL_FLOAT, GL_FALSE, sizeof(CrossOfLightElement), (void*)((2 + 2) * sizeof(float)));

    // Enable vertex attribute 0
    glEnableVertexAttribArray(0);

    // Draw
    assert(0 == mCrossOfLightBuffer.size() % 6);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mCrossOfLightBuffer.size()));
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

    mShaderManager->ActivateProgram<ProgramType::Matte>();
    mShaderManager->SetProgramParameter<ProgramType::Matte, ProgramParameterType::OrthoMatrix>(
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