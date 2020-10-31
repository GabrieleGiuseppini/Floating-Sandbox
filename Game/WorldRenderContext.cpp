/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-07-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "WorldRenderContext.h"

#include <Game/ImageFileTools.h>

#include <GameCore/GameChronometer.h>
#include <GameCore/GameException.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/ImageTools.h>
#include <GameCore/Log.h>

#include <cstring>

namespace Render {

ImageSize constexpr ThumbnailSize(32, 32);

WorldRenderContext::WorldRenderContext(
    ShaderManager<ShaderManagerTraits> & shaderManager,
    GlobalRenderContext const & globalRenderContext)
    // Buffers
    : mStarVertexBuffer()
    , mIsStarVertexBufferDirty(true)
    , mStarVBO()
    , mStarVBOAllocatedVertexSize(0u)
    , mLightningVertexBuffer()
    , mBackgroundLightningVertexCount(0)
    , mForegroundLightningVertexCount(0)
    , mLightningVBO()
    , mLightningVBOAllocatedVertexSize(0u)
    , mCloudVertexBuffer()
    , mCloudVBO()
    , mCloudVBOAllocatedVertexSize(0u)
    , mLandSegmentBuffer()
    , mLandSegmentVBO()
    , mLandSegmentVBOAllocatedVertexSize(0u)
    , mOceanSegmentBuffer()
    , mOceanSegmentVBO()
    , mOceanSegmentVBOAllocatedVertexSize(0u)
    , mFishVertexBuffer()
    , mFishVBO()
    , mFishVBOAllocatedVertexSize(0u)
    , mAMBombPreImplosionVertexBuffer()
    , mAMBombPreImplosionVBO()
    , mAMBombPreImplosionVBOAllocatedVertexSize(0u)
    , mCrossOfLightVertexBuffer()
    , mCrossOfLightVBO()
    , mCrossOfLightVBOAllocatedVertexSize(0u)
    , mStormAmbientDarkening(0.0f)
    , mRainVBO()
    , mRainDensity(0.0)
    , mIsRainDensityDirty(true)
    , mWorldBorderVertexBuffer()
    , mWorldBorderVBO()
    // VAOs
    , mStarVAO()
    , mLightningVAO()
    , mCloudVAO()
    , mLandVAO()
    , mOceanVAO()
    , mFishVAO()
    , mAMBombPreImplosionVAO()
    , mCrossOfLightVAO()
    , mRainVAO()
    , mWorldBorderVAO()
    // Textures
    , mCloudTextureAtlasMetadata()
    , mCloudTextureAtlasOpenGLHandle()
    , mUploadedWorldTextureManager()
    , mOceanTextureFrameSpecifications()
    , mOceanTextureOpenGLHandle()
    , mLandTextureFrameSpecifications()
    , mLandTextureOpenGLHandle()
    , mFishTextureAtlasMetadata()
    , mFishTextureAtlasOpenGLHandle()
    , mGenericLinearTextureAtlasMetadata(globalRenderContext.GetGenericLinearTextureAtlasMetadata())
    // ShaderManager
    , mShaderManager(shaderManager)
    // Thumbnails
    , mOceanAvailableThumbnails()
    , mLandAvailableThumbnails()
    // Calculated params
{
    GLuint tmpGLuint;

    //
    // Initialize buffers
    //

    GLuint vbos[10];
    glGenBuffers(10, vbos);
    mStarVBO = vbos[0];
    mLightningVBO = vbos[1];
    mCloudVBO = vbos[2];
    mLandSegmentVBO = vbos[3];
    mOceanSegmentVBO = vbos[4];
    mFishVBO = vbos[5];
    mAMBombPreImplosionVBO = vbos[6];
    mCrossOfLightVBO = vbos[7];
    mRainVBO = vbos[8];
    mWorldBorderVBO = vbos[9];


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
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Star), 3, GL_FLOAT, GL_FALSE, sizeof(StarVertex), (void *)0);
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Initialize Lightning VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mLightningVAO = tmpGLuint;

    glBindVertexArray(*mLightningVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mLightningVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Lightning1));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Lightning1), 4, GL_FLOAT, GL_FALSE, sizeof(LightningVertex), (void *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Lightning2));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Lightning2), 3, GL_FLOAT, GL_FALSE, sizeof(LightningVertex), (void *)(4 * sizeof(float)));
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
    static_assert(sizeof(CloudVertex) == 8 * sizeof(float));
    glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Cloud1));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Cloud1), 4, GL_FLOAT, GL_FALSE, sizeof(CloudVertex), (void *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Cloud2));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Cloud2), 4, GL_FLOAT, GL_FALSE, sizeof(CloudVertex), (void *)(4 * sizeof(float)));
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
    glBindBuffer(GL_ARRAY_BUFFER, *mLandSegmentVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Land));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Land), 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
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
    glBindBuffer(GL_ARRAY_BUFFER, *mOceanSegmentVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Ocean));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Ocean), (2 + 1), GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void *)0);
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Initialize Fish VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mFishVAO = tmpGLuint;

    glBindVertexArray(*mFishVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    static_assert(sizeof(FishVertex) == 12 * sizeof(float));
    glBindBuffer(GL_ARRAY_BUFFER, *mFishVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Fish1));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Fish1), 4, GL_FLOAT, GL_FALSE, sizeof(FishVertex), (void *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Fish2));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Fish2), 4, GL_FLOAT, GL_FALSE, sizeof(FishVertex), (void *)(4 * sizeof(float)));
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Fish3));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Fish3), 4, GL_FLOAT, GL_FALSE, sizeof(FishVertex), (void *)(8 * sizeof(float)));
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Initialize AM Bomb Implosion VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mAMBombPreImplosionVAO = tmpGLuint;

    glBindVertexArray(*mAMBombPreImplosionVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mAMBombPreImplosionVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::AMBombPreImplosion1));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::AMBombPreImplosion1), 4, GL_FLOAT, GL_FALSE, sizeof(AMBombPreImplosionVertex), (void *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::AMBombPreImplosion2));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::AMBombPreImplosion2), 2, GL_FLOAT, GL_FALSE, sizeof(AMBombPreImplosionVertex), (void *)(4 * sizeof(float)));
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
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::CrossOfLight1), 4, GL_FLOAT, GL_FALSE, sizeof(CrossOfLightVertex), (void *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::CrossOfLight2));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::CrossOfLight2), 1, GL_FLOAT, GL_FALSE, sizeof(CrossOfLightVertex), (void *)(4 * sizeof(float)));
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Initialize Rain VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mRainVAO = tmpGLuint;

    glBindVertexArray(*mRainVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mRainVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Rain));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Rain), 2, GL_FLOAT, GL_FALSE, sizeof(RainVertex), (void *)0);
    CheckOpenGLError();

    // Upload quad
    {
        RainVertex rainVertices[6]{
            {-1.0, 1.0},
            {-1.0, -1.0},
            {1.0, 1.0},
            {-1.0, -1.0},
            {1.0, 1.0},
            {1.0, -1.0}
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(RainVertex) * 6, rainVertices, GL_STATIC_DRAW);
        CheckOpenGLError();
    }

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
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::WorldBorder), 4, GL_FLOAT, GL_FALSE, sizeof(WorldBorderVertex), (void *)0);
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Set generic linear texture in our shaders
    //

    mShaderManager.ActivateTexture<ProgramParameterType::GenericLinearTexturesAtlasTexture>();

    glBindTexture(GL_TEXTURE_2D, globalRenderContext.GetGenericLinearTextureAtlasOpenGLHandle());
    CheckOpenGLError();

    auto const & worldBorderAtlasFrameMetadata = mGenericLinearTextureAtlasMetadata.GetFrameMetadata(GenericLinearTextureGroups::WorldBorder, 0);

    mShaderManager.ActivateProgram<ProgramType::WorldBorder>();
    mShaderManager.SetTextureParameters<ProgramType::WorldBorder>();
    mShaderManager.SetProgramParameter<ProgramType::WorldBorder, ProgramParameterType::AtlasTile1Dx>(
        1.0f / static_cast<float>(worldBorderAtlasFrameMetadata.FrameMetadata.Size.Width),
        1.0f / static_cast<float>(worldBorderAtlasFrameMetadata.FrameMetadata.Size.Height));
    mShaderManager.SetProgramParameter<ProgramType::WorldBorder, ProgramParameterType::AtlasTile1LeftBottomTextureCoordinates>(
        worldBorderAtlasFrameMetadata.TextureCoordinatesBottomLeft.x,
        worldBorderAtlasFrameMetadata.TextureCoordinatesBottomLeft.y);
    mShaderManager.SetProgramParameter<ProgramType::WorldBorder, ProgramParameterType::AtlasTile1Size>(
        worldBorderAtlasFrameMetadata.TextureSpaceWidth,
        worldBorderAtlasFrameMetadata.TextureSpaceHeight);


    //
    // Set noise textures in our shaders
    //

    // Noise 2

    mShaderManager.ActivateTexture<ProgramParameterType::NoiseTexture2>();

    glBindTexture(GL_TEXTURE_2D, globalRenderContext.GetNoiseTextureOpenGLHandle(1));
    CheckOpenGLError();

    mShaderManager.ActivateProgram<ProgramType::Lightning>();
    mShaderManager.SetTextureParameters<ProgramType::Lightning>();


    //
    // Set initial values of non-render parameters from which
    // other parameters are calculated
    //
}

WorldRenderContext::~WorldRenderContext()
{
}

void WorldRenderContext::InitializeCloudTextures(ResourceLocator const & resourceLocator)
{
    // Load texture database
    auto cloudTextureDatabase = TextureDatabase<Render::CloudTextureDatabaseTraits>::Load(
        resourceLocator.GetTexturesRootFolderPath());

    // Create atlas
    auto cloudTextureAtlas = TextureAtlasBuilder<CloudTextureGroups>::BuildAtlas(
        cloudTextureDatabase,
        AtlasOptions::None,
        [](float, ProgressMessageType) {});

    LogMessage("Cloud texture atlas size: ", cloudTextureAtlas.AtlasData.Size.ToString());

    mShaderManager.ActivateTexture<ProgramParameterType::CloudsAtlasTexture>();

    // Create OpenGL handle
    GLuint tmpGLuint;
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
    mCloudTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata<CloudTextureGroups>>(cloudTextureAtlas.Metadata);

    // Set texture in shader
    mShaderManager.ActivateProgram<ProgramType::Clouds>();
    mShaderManager.SetTextureParameters<ProgramType::Clouds>();
}

void WorldRenderContext::InitializeWorldTextures(ResourceLocator const & resourceLocator)
{
    // Load texture database
    auto worldTextureDatabase = TextureDatabase<Render::WorldTextureDatabaseTraits>::Load(
        resourceLocator.GetTexturesRootFolderPath());

    // Ocean

    mOceanTextureFrameSpecifications = worldTextureDatabase.GetGroup(WorldTextureGroups::Ocean).GetFrameSpecifications();

    // Create list of available textures for user
    for (size_t i = 0; i < mOceanTextureFrameSpecifications.size(); ++i)
    {
        auto const & tfs = mOceanTextureFrameSpecifications[i];

        auto textureThumbnail = ImageFileTools::LoadImageRgbaAndResize(
            tfs.FilePath,
            ThumbnailSize);

        assert(static_cast<size_t>(tfs.Metadata.FrameId.FrameIndex) == mOceanAvailableThumbnails.size());

        mOceanAvailableThumbnails.emplace_back(
            tfs.Metadata.FrameName,
            std::move(textureThumbnail));
    }

    // Land

    mLandTextureFrameSpecifications = worldTextureDatabase.GetGroup(WorldTextureGroups::Land).GetFrameSpecifications();

    // Create list of available textures for user
    for (size_t i = 0; i < mLandTextureFrameSpecifications.size(); ++i)
    {
        auto const & tfs = mLandTextureFrameSpecifications[i];

        auto textureThumbnail = ImageFileTools::LoadImageRgbaAndResize(
            tfs.FilePath,
            ThumbnailSize);

        assert(static_cast<size_t>(tfs.Metadata.FrameId.FrameIndex) == mLandAvailableThumbnails.size());

        mLandAvailableThumbnails.emplace_back(
            tfs.Metadata.FrameName,
            std::move(textureThumbnail));
    }
}

void WorldRenderContext::InitializeFishTextures(ResourceLocator const & resourceLocator)
{
    // Load texture database
    auto fishTextureDatabase = TextureDatabase<Render::FishTextureDatabaseTraits>::Load(
        resourceLocator.GetTexturesRootFolderPath());

    // Create atlas
    auto fishTextureAtlas = TextureAtlasBuilder<FishTextureGroups>::BuildAtlas(
        fishTextureDatabase,
        AtlasOptions::None,
        [](float, ProgressMessageType) {});

    LogMessage("Fish texture atlas size: ", fishTextureAtlas.AtlasData.Size.ToString());

    mShaderManager.ActivateTexture<ProgramParameterType::FishesAtlasTexture>();

    // Create OpenGL handle
    GLuint tmpGLuint;
    glGenTextures(1, &tmpGLuint);
    mFishTextureAtlasOpenGLHandle = tmpGLuint;

    // Bind texture atlas
    glBindTexture(GL_TEXTURE_2D, *mFishTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    GameOpenGL::UploadMipmappedPowerOfTwoTexture(
        std::move(fishTextureAtlas.AtlasData),
        fishTextureAtlas.Metadata.GetMaxDimension());

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckOpenGLError();

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Store metadata
    mFishTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata<FishTextureGroups>>(fishTextureAtlas.Metadata);

    // Set texture in shader
    mShaderManager.ActivateProgram<ProgramType::Fishes>();
    mShaderManager.SetTextureParameters<ProgramType::Fishes>();
}

//////////////////////////////////////////////////////////////////////////////////

void WorldRenderContext::UploadStart()
{
    // At this moment we know there are no pending draw's,
    // so GPU buffers are free to be used

    // Reset AM bomb pre-implosions, they are uploaded as needed
    mAMBombPreImplosionVertexBuffer.clear();

    // Reset crosses of light, they are uploaded as needed
    mCrossOfLightVertexBuffer.clear();
}

void WorldRenderContext::UploadStarsStart(size_t starCount)
{
    //
    // Stars are sticky: we upload them once in a while and
    // continue drawing the same buffer
    //

    mStarVertexBuffer.reset(starCount);
    mIsStarVertexBufferDirty = true;
}

void WorldRenderContext::UploadStarsEnd()
{
    // Nop
}

void WorldRenderContext::UploadLightningsStart(size_t lightningCount)
{
    //
    // Lightnings are not sticky: we upload them at each frame,
    // though they will be empty most of the time
    //

    mLightningVertexBuffer.reset_fill(6 * lightningCount);

    mBackgroundLightningVertexCount = 0;
    mForegroundLightningVertexCount = 0;
}

void WorldRenderContext::UploadLightningsEnd()
{
    // Nop
}

void WorldRenderContext::UploadCloudsStart(size_t cloudCount)
{
    //
    // Clouds are not sticky: we upload them at each frame
    //

    mCloudVertexBuffer.reset(6 * cloudCount);
}

void WorldRenderContext::UploadCloudsEnd()
{
    // Nop
}

void WorldRenderContext::UploadLandStart(size_t slices)
{
    //
    // Last segments are not sticky: we upload them at each frame
    //

    mLandSegmentBuffer.reset(slices + 1);
}

void WorldRenderContext::UploadLandEnd()
{
    // Nop
}

void WorldRenderContext::UploadOceanStart(size_t slices)
{
    //
    // Ocean segments are not sticky: we upload them at each frame
    //

    mOceanSegmentBuffer.reset(6 * slices + 1);
}

void WorldRenderContext::UploadOceanEnd()
{
    // Nop
}

void WorldRenderContext::UploadFishesStart(size_t fishCount)
{
    //
    // Fishes are not sticky: we upload them at each frame
    //

    mFishVertexBuffer.reset(6 * fishCount);
}

void WorldRenderContext::UploadFishesEnd()
{
    // Nop
}

void WorldRenderContext::UploadEnd()
{
    // Nop
}

void WorldRenderContext::ProcessParameterChanges(RenderParameters const & renderParameters)
{
    if (renderParameters.IsViewDirty)
    {
        ApplyViewModelChanges(renderParameters);
    }

    if (renderParameters.IsCanvasSizeDirty)
    {
        ApplyCanvasSizeChanges(renderParameters);
    }

    if (renderParameters.IsEffectiveAmbientLightIntensityDirty)
    {
        ApplyEffectiveAmbientLightIntensityChanges(renderParameters);
    }

    if (renderParameters.IsOceanDarkeningRateDirty)
    {
        ApplyOceanDarkeningRateChanges(renderParameters);
    }

    if (renderParameters.AreOceanRenderParametersDirty)
    {
        ApplyOceanRenderParametersChanges(renderParameters);
    }

    if (renderParameters.IsOceanTextureIndexDirty)
    {
        ApplyOceanTextureIndexChanges(renderParameters);
    }

    if (renderParameters.AreLandRenderParametersDirty)
    {
        ApplyLandRenderParametersChanges(renderParameters);
    }

    if (renderParameters.IsLandTextureIndexDirty)
    {
        ApplyLandTextureIndexChanges(renderParameters);
    }
}

void WorldRenderContext::RenderPrepareStars(RenderParameters const & /*renderParameters*/)
{
    if (mIsStarVertexBufferDirty)
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mStarVBO);

        if (mStarVBOAllocatedVertexSize != mStarVertexBuffer.size())
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ARRAY_BUFFER, mStarVertexBuffer.size() * sizeof(StarVertex), mStarVertexBuffer.data(), GL_STATIC_DRAW);
            CheckOpenGLError();

            mStarVBOAllocatedVertexSize = mStarVertexBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ARRAY_BUFFER, 0, mStarVertexBuffer.size() * sizeof(StarVertex), mStarVertexBuffer.data());
            CheckOpenGLError();
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        mIsStarVertexBufferDirty = false;
    }
}

void WorldRenderContext::RenderDrawStars(RenderParameters const & /*renderParameters*/)
{
    if (mStarVertexBuffer.size() > 0)
    {
        glBindVertexArray(*mStarVAO);

        mShaderManager.ActivateProgram<ProgramType::Stars>();

        glPointSize(0.5f);

        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(mStarVertexBuffer.size()));
        CheckOpenGLError();

        glBindVertexArray(0);
    }
}

void WorldRenderContext::RenderPrepareLightnings(RenderParameters const & /*renderParameters*/)
{
    if (!mLightningVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mLightningVBO);

        if (mLightningVertexBuffer.size() > mLightningVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ARRAY_BUFFER, mLightningVertexBuffer.size() * sizeof(LightningVertex), mLightningVertexBuffer.data(), GL_STREAM_DRAW);
            CheckOpenGLError();

            mLightningVBOAllocatedVertexSize = mLightningVertexBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ARRAY_BUFFER, 0, mLightningVertexBuffer.size() * sizeof(LightningVertex), mLightningVertexBuffer.data());
            CheckOpenGLError();
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void WorldRenderContext::RenderPrepareClouds(RenderParameters const & /*renderParameters*/)
{
    glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);

    if (mCloudVertexBuffer.size() > mCloudVBOAllocatedVertexSize)
    {
        // Re-allocate VBO buffer and upload
        glBufferData(GL_ARRAY_BUFFER, mCloudVertexBuffer.size() * sizeof(CloudVertex), mCloudVertexBuffer.data(), GL_STREAM_DRAW);
        CheckOpenGLError();

        mCloudVBOAllocatedVertexSize = mCloudVertexBuffer.size();
    }
    else
    {
        // No size change, just upload VBO buffer
        glBufferSubData(GL_ARRAY_BUFFER, 0, mCloudVertexBuffer.size() * sizeof(CloudVertex), mCloudVertexBuffer.data());
        CheckOpenGLError();
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void WorldRenderContext::RenderDrawCloudsAndBackgroundLightnings(RenderParameters const & renderParameters)
{
    ////////////////////////////////////////////////////
    // Draw background clouds, iff there are background lightnings
    ////////////////////////////////////////////////////

    // The number of clouds we want to draw *over* background
    // lightnings
    size_t constexpr CloudsOverLightnings = 5;
    GLsizei cloudsOverLightningVertexStart = 0;

    if (mBackgroundLightningVertexCount > 0
        && mCloudVertexBuffer.size() > 6 * CloudsOverLightnings)
    {
        glBindVertexArray(*mCloudVAO);

        mShaderManager.ActivateProgram<ProgramType::Clouds>();

        if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
            glLineWidth(0.1f);

        cloudsOverLightningVertexStart = static_cast<GLsizei>(mCloudVertexBuffer.size()) - (6 * CloudsOverLightnings);

        glDrawArrays(GL_TRIANGLES, 0, cloudsOverLightningVertexStart);
        CheckOpenGLError();
    }

    ////////////////////////////////////////////////////
    // Draw background lightnings
    ////////////////////////////////////////////////////

    if (mBackgroundLightningVertexCount > 0)
    {
        glBindVertexArray(*mLightningVAO);

        mShaderManager.ActivateProgram<ProgramType::Lightning>();

        glDrawArrays(GL_TRIANGLES,
            0,
            static_cast<GLsizei>(mBackgroundLightningVertexCount));
        CheckOpenGLError();
    }

    ////////////////////////////////////////////////////
    // Draw foreground clouds
    ////////////////////////////////////////////////////

    if (mCloudVertexBuffer.size() > static_cast<size_t>(cloudsOverLightningVertexStart))
    {
        glBindVertexArray(*mCloudVAO);

        mShaderManager.ActivateProgram<ProgramType::Clouds>();

        if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
            glLineWidth(0.1f);

        glDrawArrays(GL_TRIANGLES, cloudsOverLightningVertexStart, static_cast<GLsizei>(mCloudVertexBuffer.size()) - cloudsOverLightningVertexStart);
        CheckOpenGLError();
    }

    ////////////////////////////////////////////////////

    glBindVertexArray(0);
}

void WorldRenderContext::RenderPrepareOcean(RenderParameters const & /*renderParameters*/)
{
    glBindBuffer(GL_ARRAY_BUFFER, *mOceanSegmentVBO);

    if (mOceanSegmentVBOAllocatedVertexSize != mOceanSegmentBuffer.size())
    {
        // Re-allocate VBO buffer and upload
        glBufferData(GL_ARRAY_BUFFER, mOceanSegmentBuffer.size() * sizeof(OceanSegment), mOceanSegmentBuffer.data(), GL_STREAM_DRAW);
        CheckOpenGLError();

        mOceanSegmentVBOAllocatedVertexSize = mOceanSegmentBuffer.size();
    }
    else
    {
        // No size change, just upload VBO buffer
        glBufferSubData(GL_ARRAY_BUFFER, 0, mOceanSegmentBuffer.size() * sizeof(OceanSegment), mOceanSegmentBuffer.data());
        CheckOpenGLError();
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void WorldRenderContext::RenderDrawOcean(bool opaquely, RenderParameters const & renderParameters)
{
    float const transparency = opaquely ? 0.0f : renderParameters.OceanTransparency;

    glBindVertexArray(*mOceanVAO);

    switch (renderParameters.OceanRenderMode)
    {
        case OceanRenderModeType::Depth:
        {
            mShaderManager.ActivateProgram<ProgramType::OceanDepth>();
            mShaderManager.SetProgramParameter<ProgramType::OceanDepth, ProgramParameterType::OceanTransparency>(
                transparency);

            break;
        }

        case OceanRenderModeType::Flat:
        {
            mShaderManager.ActivateProgram<ProgramType::OceanFlat>();
            mShaderManager.SetProgramParameter<ProgramType::OceanFlat, ProgramParameterType::OceanTransparency>(
                transparency);

            break;
        }

        case OceanRenderModeType::Texture:
        {
            mShaderManager.ActivateProgram<ProgramType::OceanTexture>();
            mShaderManager.SetProgramParameter<ProgramType::OceanTexture, ProgramParameterType::OceanTransparency>(
                transparency);

            break;
        }
    }

    if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
        glLineWidth(0.1f);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mOceanSegmentBuffer.size()));

    glBindVertexArray(0);
}

void WorldRenderContext::RenderPrepareOceanFloor(RenderParameters const & /*renderParameters*/)
{
    glBindBuffer(GL_ARRAY_BUFFER, *mLandSegmentVBO);

    if (mLandSegmentVBOAllocatedVertexSize != mLandSegmentBuffer.size())
    {
        // Re-allocate VBO buffer and upload
        glBufferData(GL_ARRAY_BUFFER, mLandSegmentBuffer.size() * sizeof(LandSegment), mLandSegmentBuffer.data(), GL_STREAM_DRAW);
        CheckOpenGLError();

        mLandSegmentVBOAllocatedVertexSize = mLandSegmentBuffer.size();
    }
    else
    {
        // No size change, just upload VBO buffer
        glBufferSubData(GL_ARRAY_BUFFER, 0, mLandSegmentBuffer.size() * sizeof(LandSegment), mLandSegmentBuffer.data());
        CheckOpenGLError();
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void WorldRenderContext::RenderDrawOceanFloor(RenderParameters const & renderParameters)
{
    glBindVertexArray(*mLandVAO);

    switch (renderParameters.LandRenderMode)
    {
        case LandRenderModeType::Flat:
        {
            mShaderManager.ActivateProgram<ProgramType::LandFlat>();
            break;
        }

        case LandRenderModeType::Texture:
        {
            mShaderManager.ActivateProgram<ProgramType::LandTexture>();
            break;
        }
    }

    if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
        glLineWidth(0.1f);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mLandSegmentBuffer.size()));

    glBindVertexArray(0);
}

void WorldRenderContext::RenderPrepareFishes(RenderParameters const & /*renderParameters*/)
{
    glBindBuffer(GL_ARRAY_BUFFER, *mFishVBO);

    if (mFishVertexBuffer.size() > mFishVBOAllocatedVertexSize)
    {
        // Re-allocate VBO buffer and upload
        glBufferData(GL_ARRAY_BUFFER, mFishVertexBuffer.size() * sizeof(FishVertex), mFishVertexBuffer.data(), GL_STREAM_DRAW);
        CheckOpenGLError();

        mFishVBOAllocatedVertexSize = mFishVertexBuffer.size();
    }
    else
    {
        // No size change, just upload VBO buffer
        glBufferSubData(GL_ARRAY_BUFFER, 0, mFishVertexBuffer.size() * sizeof(FishVertex), mFishVertexBuffer.data());
        CheckOpenGLError();
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void WorldRenderContext::RenderDrawFishes(RenderParameters const & /*renderParameters*/)
{
    if (mFishVertexBuffer.size() > 0)
    {
        glBindVertexArray(*mFishVAO);

        mShaderManager.ActivateProgram<ProgramType::Fishes>();

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mFishVertexBuffer.size()));
        CheckOpenGLError();

        glBindVertexArray(0);
    }
}

void WorldRenderContext::RenderPrepareAMBombPreImplosions(RenderParameters const & /*renderParameters*/)
{
    if (!mAMBombPreImplosionVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mAMBombPreImplosionVBO);

        if (mAMBombPreImplosionVertexBuffer.size() > mAMBombPreImplosionVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ARRAY_BUFFER, mAMBombPreImplosionVertexBuffer.size() * sizeof(AMBombPreImplosionVertex), mAMBombPreImplosionVertexBuffer.data(), GL_STREAM_DRAW);
            CheckOpenGLError();

            mAMBombPreImplosionVBOAllocatedVertexSize = mAMBombPreImplosionVertexBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ARRAY_BUFFER, 0, mAMBombPreImplosionVertexBuffer.size() * sizeof(AMBombPreImplosionVertex), mAMBombPreImplosionVertexBuffer.data());
            CheckOpenGLError();
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void WorldRenderContext::RenderDrawAMBombPreImplosions(RenderParameters const & /*renderParameters*/)
{
    if (!mAMBombPreImplosionVertexBuffer.empty())
    {
        glBindVertexArray(*mAMBombPreImplosionVAO);

        mShaderManager.ActivateProgram<ProgramType::AMBombPreImplosion>();

        assert((mAMBombPreImplosionVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mAMBombPreImplosionVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void WorldRenderContext::RenderPrepareCrossesOfLight(RenderParameters const & /*renderParameters*/)
{
    if (!mCrossOfLightVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mCrossOfLightVBO);

        if (mCrossOfLightVertexBuffer.size() > mCrossOfLightVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ARRAY_BUFFER, mCrossOfLightVertexBuffer.size() * sizeof(CrossOfLightVertex), mCrossOfLightVertexBuffer.data(), GL_STREAM_DRAW);
            CheckOpenGLError();

            mCrossOfLightVBOAllocatedVertexSize = mCrossOfLightVertexBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ARRAY_BUFFER, 0, mCrossOfLightVertexBuffer.size() * sizeof(CrossOfLightVertex), mCrossOfLightVertexBuffer.data());
            CheckOpenGLError();
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void WorldRenderContext::RenderDrawCrossesOfLight(RenderParameters const & /*renderParameters*/)
{
    if (!mCrossOfLightVertexBuffer.empty())
    {
        glBindVertexArray(*mCrossOfLightVAO);

        mShaderManager.ActivateProgram<ProgramType::CrossOfLight>();

        assert((mCrossOfLightVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mCrossOfLightVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void WorldRenderContext::RenderDrawForegroundLightnings(RenderParameters const & /*renderParameters*/)
{
    if (mForegroundLightningVertexCount > 0)
    {
        glBindVertexArray(*mLightningVAO);

        mShaderManager.ActivateProgram<ProgramType::Lightning>();

        glDrawArrays(GL_TRIANGLES,
            static_cast<GLsizei>(mLightningVertexBuffer.size() - mForegroundLightningVertexCount),
            static_cast<GLsizei>(mForegroundLightningVertexCount));
        CheckOpenGLError();

        glBindVertexArray(0);
    }
}

void WorldRenderContext::RenderPrepareRain(RenderParameters const & /*renderParameters*/)
{
    if (mIsRainDensityDirty || mRainDensity != 0.0f)
    {
        mShaderManager.ActivateProgram<ProgramType::Rain>();

        if (mIsRainDensityDirty)
        {
            // Set parameter
            mShaderManager.SetProgramParameter<ProgramType::Rain, ProgramParameterType::RainDensity>(
                mRainDensity);

            mIsRainDensityDirty = false; // Uploaded flag
        }

        if (mRainDensity != 0.0f)
        {
            // Set time parameter
            mShaderManager.SetProgramParameter<ProgramParameterType::Time>(
                ProgramType::Rain,
                GameWallClock::GetInstance().NowAsFloat());
        }
    }
}

void WorldRenderContext::RenderDrawRain(RenderParameters const & /*renderParameters*/)
{
    if (mRainDensity != 0.0f)
    {
        glBindVertexArray(*mRainVAO);

        mShaderManager.ActivateProgram(ProgramType::Rain);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        CheckOpenGLError();

        glBindVertexArray(0);
    }
}

void WorldRenderContext::RenderDrawWorldBorder(RenderParameters const & /*renderParameters*/)
{
    if (mWorldBorderVertexBuffer.size() > 0)
    {
        //
        // Render
        //

        glBindVertexArray(*mWorldBorderVAO);

        mShaderManager.ActivateProgram<ProgramType::WorldBorder>();

        assert((mWorldBorderVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mWorldBorderVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

////////////////////////////////////////////////////////////////////////////////////

void WorldRenderContext::ApplyViewModelChanges(RenderParameters const & renderParameters)
{
    //
    // Update ortho matrix in all programs
    //

    constexpr float ZFar = 1000.0f;
    constexpr float ZNear = 1.0f;

    ViewModel::ProjectionMatrix globalOrthoMatrix;
    renderParameters.View.CalculateGlobalOrthoMatrix(ZFar, ZNear, globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::LandFlat>();
    mShaderManager.SetProgramParameter<ProgramType::LandFlat, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::LandTexture>();
    mShaderManager.SetProgramParameter<ProgramType::LandTexture, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::OceanDepth>();
    mShaderManager.SetProgramParameter<ProgramType::OceanDepth, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::OceanFlat>();
    mShaderManager.SetProgramParameter<ProgramType::OceanFlat, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::OceanTexture>();
    mShaderManager.SetProgramParameter<ProgramType::OceanTexture, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::Fishes>();
    mShaderManager.SetProgramParameter<ProgramType::Fishes, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::AMBombPreImplosion>();
    mShaderManager.SetProgramParameter<ProgramType::AMBombPreImplosion, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::CrossOfLight>();
    mShaderManager.SetProgramParameter<ProgramType::CrossOfLight, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::WorldBorder>();
    mShaderManager.SetProgramParameter<ProgramType::WorldBorder, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    //
    // Recalculate world border
    //

    RecalculateWorldBorder(renderParameters);
}

void WorldRenderContext::ApplyCanvasSizeChanges(RenderParameters const & renderParameters)
{
    auto const & view = renderParameters.View;

    // Set shader parameters
    mShaderManager.ActivateProgram<ProgramType::CrossOfLight>();
    mShaderManager.SetProgramParameter<ProgramType::CrossOfLight, ProgramParameterType::ViewportSize>(
        static_cast<float>(view.GetCanvasWidth()),
        static_cast<float>(view.GetCanvasHeight()));
}

void WorldRenderContext::ApplyEffectiveAmbientLightIntensityChanges(RenderParameters const & renderParameters)
{
    // Set parameters in all programs

    mShaderManager.ActivateProgram<ProgramType::Stars>();
    mShaderManager.SetProgramParameter<ProgramType::Stars, ProgramParameterType::StarTransparency>(
        pow(std::max(0.0f, 1.0f - renderParameters.EffectiveAmbientLightIntensity), 3.0f));

    mShaderManager.ActivateProgram<ProgramType::Clouds>();
    mShaderManager.SetProgramParameter<ProgramType::Clouds, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::Lightning>();
    mShaderManager.SetProgramParameter<ProgramType::Lightning, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::LandFlat>();
    mShaderManager.SetProgramParameter<ProgramType::LandFlat, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::LandTexture>();
    mShaderManager.SetProgramParameter<ProgramType::LandTexture, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::OceanDepth>();
    mShaderManager.SetProgramParameter<ProgramType::OceanDepth, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::OceanFlat>();
    mShaderManager.SetProgramParameter<ProgramType::OceanFlat, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::OceanTexture>();
    mShaderManager.SetProgramParameter<ProgramType::OceanTexture, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::Fishes>();
    mShaderManager.SetProgramParameter<ProgramType::Fishes, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::Rain>();
    mShaderManager.SetProgramParameter<ProgramType::Rain, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::WorldBorder>();
    mShaderManager.SetProgramParameter<ProgramType::WorldBorder, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);
}

void WorldRenderContext::ApplyOceanDarkeningRateChanges(RenderParameters const & renderParameters)
{
    // Set parameter in all programs

    float const rate = renderParameters.OceanDarkeningRate / 50.0f;

    mShaderManager.ActivateProgram<ProgramType::LandTexture>();
    mShaderManager.SetProgramParameter<ProgramType::LandTexture, ProgramParameterType::OceanDarkeningRate>(
        rate);

    mShaderManager.ActivateProgram<ProgramType::OceanDepth>();
    mShaderManager.SetProgramParameter<ProgramType::OceanDepth, ProgramParameterType::OceanDarkeningRate>(
        rate);

    mShaderManager.ActivateProgram<ProgramType::OceanTexture>();
    mShaderManager.SetProgramParameter<ProgramType::OceanTexture, ProgramParameterType::OceanDarkeningRate>(
        rate);

    mShaderManager.ActivateProgram<ProgramType::Fishes>();
    mShaderManager.SetProgramParameter<ProgramType::Fishes, ProgramParameterType::OceanDarkeningRate>(
        rate);
}

void WorldRenderContext::ApplyOceanRenderParametersChanges(RenderParameters const & renderParameters)
{
    // Set ocean parameters in all water programs

    auto const depthColorStart = renderParameters.DepthOceanColorStart.toVec3f();
    mShaderManager.ActivateProgram<ProgramType::OceanDepth>();
    mShaderManager.SetProgramParameter<ProgramType::OceanDepth, ProgramParameterType::OceanDepthColorStart>(
        depthColorStart.x,
        depthColorStart.y,
        depthColorStart.z);

    auto const depthColorEnd = renderParameters.DepthOceanColorEnd.toVec3f();
    mShaderManager.ActivateProgram<ProgramType::OceanDepth>();
    mShaderManager.SetProgramParameter<ProgramType::OceanDepth, ProgramParameterType::OceanDepthColorEnd>(
        depthColorEnd.x,
        depthColorEnd.y,
        depthColorEnd.z);

    auto const flatColor = renderParameters.FlatOceanColor.toVec3f();
    mShaderManager.ActivateProgram<ProgramType::OceanFlat>();
    mShaderManager.SetProgramParameter<ProgramType::OceanFlat, ProgramParameterType::OceanFlatColor>(
        flatColor.x,
        flatColor.y,
        flatColor.z);
}

void WorldRenderContext::ApplyOceanTextureIndexChanges(RenderParameters const & renderParameters)
{
    //
    // Reload the ocean texture
    //

    // Destroy previous texture
    mOceanTextureOpenGLHandle.reset();

    // Clamp the texture index
    auto clampedOceanTextureIndex = std::min(renderParameters.OceanTextureIndex, mOceanTextureFrameSpecifications.size() - 1);

    // Load texture image
    auto oceanTextureFrame = mOceanTextureFrameSpecifications[clampedOceanTextureIndex].LoadFrame();

    // Activate texture
    mShaderManager.ActivateTexture<ProgramParameterType::OceanTexture>();

    // Create texture
    GLuint tmpGLuint;
    glGenTextures(1, &tmpGLuint);
    mOceanTextureOpenGLHandle = tmpGLuint;

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
    mShaderManager.ActivateProgram<ProgramType::OceanTexture>();
    mShaderManager.SetProgramParameter<ProgramType::OceanTexture, ProgramParameterType::TextureScaling>(
        1.0f / oceanTextureFrame.Metadata.WorldWidth,
        1.0f / oceanTextureFrame.Metadata.WorldHeight);
    mShaderManager.SetTextureParameters<ProgramType::OceanTexture>();
}

void WorldRenderContext::ApplyLandRenderParametersChanges(RenderParameters const & renderParameters)
{
    // Set land parameters in all land programs

    auto const flatColor = renderParameters.FlatLandColor.toVec3f();
    mShaderManager.ActivateProgram<ProgramType::LandFlat>();
    mShaderManager.SetProgramParameter<ProgramType::LandFlat, ProgramParameterType::LandFlatColor>(
        flatColor.x,
        flatColor.y,
        flatColor.z);
}

void WorldRenderContext::ApplyLandTextureIndexChanges(RenderParameters const & renderParameters)
{
    //
    // Reload the land texture
    //

    // Destroy previous texture
    mLandTextureOpenGLHandle.reset();

    // Clamp the texture index
    auto clampedLandTextureIndex = std::min(renderParameters.LandTextureIndex, mLandTextureFrameSpecifications.size() - 1);

    // Load texture image
    auto landTextureFrame = mLandTextureFrameSpecifications[clampedLandTextureIndex].LoadFrame();

    // Activate texture
    mShaderManager.ActivateTexture<ProgramParameterType::LandTexture>();

    // Create texture
    GLuint tmpGLuint;
    glGenTextures(1, &tmpGLuint);
    mLandTextureOpenGLHandle = tmpGLuint;

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
    mShaderManager.ActivateProgram<ProgramType::LandTexture>();
    mShaderManager.SetProgramParameter<ProgramType::LandTexture, ProgramParameterType::TextureScaling>(
        1.0f / landTextureFrame.Metadata.WorldWidth,
        1.0f / landTextureFrame.Metadata.WorldHeight);
    mShaderManager.SetTextureParameters<ProgramType::LandTexture>();
}

template <typename TVertexBuffer>
static void EmplaceWorldBorderQuad(
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

void WorldRenderContext::RecalculateWorldBorder(RenderParameters const & renderParameters)
{
    auto const & viewModel = renderParameters.View;

    ImageSize const & worldBorderTextureSize =
        mGenericLinearTextureAtlasMetadata.GetFrameMetadata(GenericLinearTextureGroups::WorldBorder, 0)
        .FrameMetadata.Size;

    // Calculate width and height, in world coordinates, of the world border, under the constraint
    // that we want to ensure that the texture is rendered with half of its original pixel size
    float const worldBorderWorldWidth = viewModel.PixelWidthToWorldWidth(static_cast<float>(worldBorderTextureSize.Width)) / 2.0f;
    float const worldBorderWorldHeight = viewModel.PixelHeightToWorldHeight(static_cast<float>(worldBorderTextureSize.Height)) / 2.0f;

    // Max coordinates in texture space (e.g. 3.0 means three frames); note that the texture bottom-left origin
    // already starts at a dead pixel (0.5/size)
    float const textureSpaceWidth =
        GameParameters::MaxWorldWidth / worldBorderWorldWidth
        - 1.0f / static_cast<float>(worldBorderTextureSize.Width);
    float const textureSpaceHeight = GameParameters::MaxWorldHeight / worldBorderWorldHeight
        - 1.0f / static_cast<float>(worldBorderTextureSize.Height);


    //
    // Check which sides of the border we need to draw
    //

    mWorldBorderVertexBuffer.clear();

    // Left
    if (-GameParameters::HalfMaxWorldWidth + worldBorderWorldWidth >= viewModel.GetVisibleWorld().TopLeft.x)
    {
        EmplaceWorldBorderQuad(
            // Top-left
            -GameParameters::HalfMaxWorldWidth,
            GameParameters::HalfMaxWorldHeight,
            0.0f,
            textureSpaceHeight,
            // Bottom-right
            -GameParameters::HalfMaxWorldWidth + worldBorderWorldWidth,
            -GameParameters::HalfMaxWorldHeight,
            1.0f,
            0.0f,
            mWorldBorderVertexBuffer);
    }

    // Right
    if (GameParameters::HalfMaxWorldWidth - worldBorderWorldWidth <= viewModel.GetVisibleWorld().BottomRight.x)
    {
        EmplaceWorldBorderQuad(
            // Top-left
            GameParameters::HalfMaxWorldWidth - worldBorderWorldWidth,
            GameParameters::HalfMaxWorldHeight,
            0.0f,
            textureSpaceHeight,
            // Bottom-right
            GameParameters::HalfMaxWorldWidth,
            -GameParameters::HalfMaxWorldHeight,
            1.0f,
            0.0f,
            mWorldBorderVertexBuffer);
    }

    // Top
    if (GameParameters::HalfMaxWorldHeight - worldBorderWorldHeight <= viewModel.GetVisibleWorld().TopLeft.y)
    {
        EmplaceWorldBorderQuad(
            // Top-left
            -GameParameters::HalfMaxWorldWidth,
            GameParameters::HalfMaxWorldHeight,
            0.0f,
            1.0f,
            // Bottom-right
            GameParameters::HalfMaxWorldWidth,
            GameParameters::HalfMaxWorldHeight - worldBorderWorldHeight,
            textureSpaceWidth,
            0.0f,
            mWorldBorderVertexBuffer);
    }

    // Bottom
    if (-GameParameters::HalfMaxWorldHeight + worldBorderWorldHeight >= viewModel.GetVisibleWorld().BottomRight.y)
    {
        EmplaceWorldBorderQuad(
            // Top-left
            -GameParameters::HalfMaxWorldWidth,
            -GameParameters::HalfMaxWorldHeight + worldBorderWorldHeight,
            0.0f,
            1.0f,
            // Bottom-right
            GameParameters::HalfMaxWorldWidth,
            -GameParameters::HalfMaxWorldHeight,
            textureSpaceWidth,
            0.0f,
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
    }
}

}