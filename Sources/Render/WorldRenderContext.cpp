/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-07-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "WorldRenderContext.h"

#include <Core/GameWallClock.h>
#include <Core/ImageTools.h>
#include <Core/Log.h>

#include <cstring>
#include <limits>

ImageSize constexpr ThumbnailSize(32, 32);

WorldRenderContext::WorldRenderContext(
    IAssetManager const & assetManager,
    ShaderManager<GameShaderSet::ShaderSet> & shaderManager,
    GlobalRenderContext & globalRenderContext)
    : mAssetManager(assetManager)
    , mShaderManager(shaderManager)
    , mGlobalRenderContext(globalRenderContext)
    // Buffers and parameters
    , mSkyVBO()
    , mStarVertexBuffer()
    , mDirtyStarsCount(0)
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
    , mCloudNormalizedViewCamY(0.0f)
    , mLandSegmentBuffer()
    , mLandSegmentVBO()
    , mLandSegmentVBOAllocatedVertexSize(0u)
    , mOceanBasicSegmentBuffer()
    , mOceanBasicSegmentVBO()
    , mOceanBasicSegmentVBOAllocatedVertexSize(0u)
    , mOceanDetailedSegmentBuffer()
    , mOceanDetailedSegmentVBO()
    , mOceanDetailedSegmentVBOAllocatedVertexSize(0u)
    , mFishVertexBuffer()
    , mFishVBO()
    , mFishVBOAllocatedVertexSize(0u)
    , mAMBombPreImplosionVertexBuffer()
    , mAMBombPreImplosionVBO()
    , mAMBombPreImplosionVBOAllocatedVertexSize(0u)
    , mCrossOfLightVertexBuffer()
    , mCrossOfLightVBO()
    , mCrossOfLightVBOAllocatedVertexSize(0u)
    , mAABBVertexBuffer()
    , mAABBVBO()
    , mAABBVBOAllocatedVertexSize(0u)
    , mStormAmbientDarkening(0.0f)
    , mRainVBO()
    , mRainDensity(0.0)
    , mIsRainDensityDirty(true)
    , mRainWindSpeedMagnitude(0.0f)
    , mIsRainWindSpeedMagnitudeDirty(true)
    , mWorldBorderVertexBuffer()
    , mWorldBorderVBO()
    // VAOs
    , mSkyVAO()
    , mStarVAO()
    , mLightningVAO()
    , mCloudVAO()
    , mLandVAO()
    , mOceanBasicVAO()
    , mOceanDetailedVAO()
    , mFishVAO()
    , mAMBombPreImplosionVAO()
    , mCrossOfLightVAO()
    , mAABBVAO()
    , mRainVAO()
    , mWorldBorderVAO()
    // Textures
    , mCloudTextureAtlasMetadata()
    , mCloudTextureAtlasOpenGLHandle()
    , mCloudShadowsTextureOpenGLHandle()
    , mCloudShadowsTextureSize(0)
    , mHasCloudShadowsTextureBeenAllocated(false)
    , mOceanTextureFrameSpecifications()
    , mOceanTextureOpenGLHandle()
    , mLandTextureFrameSpecifications()
    , mLandTextureOpenGLHandle()
    , mFishTextureAtlasMetadata()
    , mFishTextureAtlasOpenGLHandle()
    , mGenericLinearTextureAtlasMetadata(globalRenderContext.GetGenericLinearTextureAtlasMetadata())
    // Thumbnails
    , mOceanAvailableThumbnails()
    , mLandAvailableThumbnails()
    // Parameters
    , mSunRaysInclination(1.0f)
    , mIsSunRaysInclinationDirty(1.0f)
{
    GLuint tmpGLuint;

    //
    // Initialize buffers
    //

    GLuint vbos[13];
    glGenBuffers(13, vbos);
    mSkyVBO = vbos[0];
    mStarVBO = vbos[1];
    mLightningVBO = vbos[2];
    mCloudVBO = vbos[3];
    mLandSegmentVBO = vbos[4];
    mOceanBasicSegmentVBO = vbos[5];
    mOceanDetailedSegmentVBO = vbos[6];
    mFishVBO = vbos[7];
    mAMBombPreImplosionVBO = vbos[8];
    mCrossOfLightVBO = vbos[9];
    mAABBVBO = vbos[10];
    mRainVBO = vbos[11];
    mWorldBorderVBO = vbos[12];

    //
    // Initialize Sky VAO
    //

    {

        glGenVertexArrays(1, &tmpGLuint);
        mSkyVAO = tmpGLuint;

        glBindVertexArray(*mSkyVAO);
        CheckOpenGLError();

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mSkyVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Sky));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Sky), 2, GL_FLOAT, GL_FALSE, sizeof(SkyVertex), (void *)0);
        CheckOpenGLError();

        // Upload whole screen NDC quad
        {
            SkyVertex skyVertices[6]{
                {-1.0, 1.0},
                {-1.0, -1.0},
                {1.0, 1.0},
                {-1.0, -1.0},
                {1.0, 1.0},
                {1.0, -1.0}
            };

            glBufferData(GL_ARRAY_BUFFER, sizeof(SkyVertex) * 6, skyVertices, GL_STATIC_DRAW);
            CheckOpenGLError();
        }

        glBindVertexArray(0);

        // Set texture parameters
        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::Sky>();
        mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::Sky>();
    }


    //
    // Initialize Star VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mStarVAO = tmpGLuint;

    glBindVertexArray(*mStarVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mStarVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Star));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Star), 3, GL_FLOAT, GL_FALSE, sizeof(StarVertex), (void *)0);
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
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Lightning1));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Lightning1), 4, GL_FLOAT, GL_FALSE, sizeof(LightningVertex), (void *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Lightning2));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Lightning2), 3, GL_FLOAT, GL_FALSE, sizeof(LightningVertex), (void *)(4 * sizeof(float)));
    CheckOpenGLError();

    glBindVertexArray(0);

    // Set texture parameters
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::Lightning>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::Lightning>();


    //
    // Initialize Cloud VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mCloudVAO = tmpGLuint;

    glBindVertexArray(*mCloudVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    static_assert(sizeof(CloudVertex) == (4 + 4 + 1) * sizeof(float));
    glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Cloud1));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Cloud1), 4, GL_FLOAT, GL_FALSE, sizeof(CloudVertex), (void *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Cloud2));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Cloud2), 4, GL_FLOAT, GL_FALSE, sizeof(CloudVertex), (void *)(4 * sizeof(float)));
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Cloud3));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Cloud3), 1, GL_FLOAT, GL_FALSE, sizeof(CloudVertex), (void *)((4 + 4) * sizeof(float)));
    CheckOpenGLError();

    // NOTE: Intel drivers have a bug in the VAO ARB: they do not store the ELEMENT_ARRAY_BUFFER binding
    // in the VAO. So we won't associate the element VBO here, but rather before each drawing call.
    ////mGlobalRenderContext.GetElementIndices().Bind()

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
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Land));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Land), 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    CheckOpenGLError();

    glBindVertexArray(0);

    // Set (noise) texture parameters
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandFlatDetailed>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::LandFlatDetailed>();
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandTextureDetailed>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::LandTextureDetailed>();


    //
    // Initialize Ocean Basic VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mOceanBasicVAO = tmpGLuint;

    glBindVertexArray(*mOceanBasicVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    static_assert(sizeof(OceanBasicSegment) == 3 * 2 * sizeof(float));
    glBindBuffer(GL_ARRAY_BUFFER, *mOceanBasicSegmentVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::OceanBasic));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::OceanBasic), (2 + 1), GL_FLOAT, GL_FALSE, sizeof(OceanBasicSegment) / 2, (void *)0);
    CheckOpenGLError();

    glBindVertexArray(0);

    // Set texture parameters
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthBasic>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::OceanDepthBasic>();
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureBasic>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::OceanTextureBasic>();



    //
    // Initialize Ocean Detailed VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mOceanDetailedVAO = tmpGLuint;

    glBindVertexArray(*mOceanDetailedVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    static_assert(sizeof(OceanDetailedSegment) / 2 == 7 * sizeof(float));
    glBindBuffer(GL_ARRAY_BUFFER, *mOceanDetailedSegmentVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::OceanDetailed1));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::OceanDetailed1), 3, GL_FLOAT, GL_FALSE, sizeof(OceanDetailedSegment) / 2, (void *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::OceanDetailed2));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::OceanDetailed2), 4, GL_FLOAT, GL_FALSE, sizeof(OceanDetailedSegment) / 2, (void *)(3 * sizeof(float)));
    CheckOpenGLError();

    glBindVertexArray(0);

    // Set texture parameters

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatDetailedBackground>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::OceanFlatDetailedBackground>();
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatDetailedForeground>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::OceanFlatDetailedForeground>();

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthDetailedBackground>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::OceanDepthDetailedBackground>();
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthDetailedForeground>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::OceanDepthDetailedForeground>();

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureDetailedBackground>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::OceanTextureDetailedBackground>();
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureDetailedForeground>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::OceanTextureDetailedForeground>();

    //
    // Initialize Fish VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mFishVAO = tmpGLuint;

    glBindVertexArray(*mFishVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    static_assert(sizeof(FishVertex) == 14 * sizeof(float));
    glBindBuffer(GL_ARRAY_BUFFER, *mFishVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Fish1));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Fish1), 4, GL_FLOAT, GL_FALSE, sizeof(FishVertex), (void *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Fish2));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Fish2), 4, GL_FLOAT, GL_FALSE, sizeof(FishVertex), (void *)(4 * sizeof(float)));
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Fish3));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Fish3), 4, GL_FLOAT, GL_FALSE, sizeof(FishVertex), (void *)(8 * sizeof(float)));
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Fish4));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Fish4), 2, GL_FLOAT, GL_FALSE, sizeof(FishVertex), (void *)(12 * sizeof(float)));
    CheckOpenGLError();

    // NOTE: Intel drivers have a bug in the VAO ARB: they do not store the ELEMENT_ARRAY_BUFFER binding
    // in the VAO. So we won't associate the element VBO here, but rather before each drawing call.
    ////mGlobalRenderContext.GetElementIndices().Bind()

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
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::AMBombPreImplosion1));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::AMBombPreImplosion1), 4, GL_FLOAT, GL_FALSE, sizeof(AMBombPreImplosionVertex), (void *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::AMBombPreImplosion2));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::AMBombPreImplosion2), 2, GL_FLOAT, GL_FALSE, sizeof(AMBombPreImplosionVertex), (void *)(4 * sizeof(float)));
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
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::CrossOfLight1));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::CrossOfLight1), 4, GL_FLOAT, GL_FALSE, sizeof(CrossOfLightVertex), (void *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::CrossOfLight2));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::CrossOfLight2), 1, GL_FLOAT, GL_FALSE, sizeof(CrossOfLightVertex), (void *)(4 * sizeof(float)));
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Initialize AABB VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mAABBVAO = tmpGLuint;

    glBindVertexArray(*mAABBVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    static_assert(sizeof(AABBVertex) == 6 * sizeof(float));
    glBindBuffer(GL_ARRAY_BUFFER, *mAABBVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::AABB1));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::AABB1), 4, GL_FLOAT, GL_FALSE, sizeof(AABBVertex), (void *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::AABB2));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::AABB2), 2, GL_FLOAT, GL_FALSE, sizeof(AABBVertex), (void *)(4 * sizeof(float)));
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Initialize Rain VAO
    //

    {

        glGenVertexArrays(1, &tmpGLuint);
        mRainVAO = tmpGLuint;

        glBindVertexArray(*mRainVAO);
        CheckOpenGLError();

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mRainVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Rain));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::Rain), 2, GL_FLOAT, GL_FALSE, sizeof(RainVertex), (void *)0);
        CheckOpenGLError();

        // Upload whole screen NDC quad
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
    }


    //
    // Initialize WorldBorder VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mWorldBorderVAO = tmpGLuint;

    glBindVertexArray(*mWorldBorderVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mWorldBorderVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::WorldBorder));
    glVertexAttribPointer(static_cast<GLuint>(GameShaderSet::VertexAttributeKind::WorldBorder), 4, GL_FLOAT, GL_FALSE, sizeof(WorldBorderVertex), (void *)0);
    CheckOpenGLError();

    glBindVertexArray(0);

    //
    // Initialize cloud shadows
    //

    {
        glGenTextures(1, &tmpGLuint);
        mCloudShadowsTextureOpenGLHandle = tmpGLuint;

        // Bind texture
        mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::SharedTexture>();
        glBindTexture(GL_TEXTURE_1D, *mCloudShadowsTextureOpenGLHandle);
        CheckOpenGLError();

        // Set repeat mode
        glTexParameteri(GL_TEXTURE_1D, 0x2802, GL_CLAMP_TO_EDGE);
        CheckOpenGLError();

        // Set filtering
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CheckOpenGLError();

        // Unbind texture
        glBindTexture(GL_TEXTURE_1D, 0);
    }

    //
    // Set generic linear texture in our shaders
    //

    mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::GenericLinearTexturesAtlasTexture>();

    glBindTexture(GL_TEXTURE_2D, globalRenderContext.GetGenericLinearTextureAtlasOpenGLHandle());
    CheckOpenGLError();
}

WorldRenderContext::~WorldRenderContext()
{
}

void WorldRenderContext::InitializeCloudTextures()
{
    // Load atlas
    auto cloudTextureAtlas = TextureAtlas<GameTextureDatabases::CloudTextureDatabase>::Deserialize(mAssetManager);

    LogMessage("Cloud texture atlas size: ", cloudTextureAtlas.Image.Size);

    mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::CloudsAtlasTexture>();

    // Create OpenGL handle
    GLuint tmpGLuint;
    glGenTextures(1, &tmpGLuint);
    mCloudTextureAtlasOpenGLHandle = tmpGLuint;

    // Bind texture atlas
    glBindTexture(GL_TEXTURE_2D, *mCloudTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    GameOpenGL::UploadTexture(std::move(cloudTextureAtlas.Image));

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckOpenGLError();

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Store metadata
    mCloudTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata<GameTextureDatabases::CloudTextureDatabase>>(cloudTextureAtlas.Metadata);

    // Set textures in shader
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::CloudsBasic>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::CloudsBasic>();
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::CloudsDetailed>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::CloudsDetailed>();
}

void WorldRenderContext::InitializeWorldTextures()
{
    // Load texture database
    auto worldTextureDatabase = TextureDatabase<GameTextureDatabases::WorldTextureDatabase>::Load(mAssetManager);

    // Ocean

    mOceanTextureFrameSpecifications = worldTextureDatabase.GetGroup(GameTextureDatabases::WorldTextureGroups::Ocean).GetFrameSpecifications();

    // Create list of available textures for user
    for (size_t i = 0; i < mOceanTextureFrameSpecifications.size(); ++i)
    {
        auto const & tfs = mOceanTextureFrameSpecifications[i];

        auto originalTextureImage = mAssetManager.LoadTextureDatabaseFrameRGBA(
            GameTextureDatabases::WorldTextureDatabase::DatabaseName,
            tfs.RelativePath);
        auto textureThumbnail = ImageTools::Resize(
            originalTextureImage,
            originalTextureImage.Size.ShrinkToFit(ThumbnailSize),
            ImageTools::FilterKind::Bilinear);

        assert(static_cast<size_t>(tfs.Metadata.FrameId.FrameIndex) == mOceanAvailableThumbnails.size());

        mOceanAvailableThumbnails.emplace_back(
            tfs.Metadata.DisplayName,
            std::move(textureThumbnail));
    }

    // Land

    mLandTextureFrameSpecifications = worldTextureDatabase.GetGroup(GameTextureDatabases::WorldTextureGroups::Land).GetFrameSpecifications();

    // Create list of available textures for user
    for (size_t i = 0; i < mLandTextureFrameSpecifications.size(); ++i)
    {
        auto const & tfs = mLandTextureFrameSpecifications[i];

        auto originalTextureImage = mAssetManager.LoadTextureDatabaseFrameRGBA(
            GameTextureDatabases::WorldTextureDatabase::DatabaseName,
            tfs.RelativePath);
        auto textureThumbnail = ImageTools::Resize(
            originalTextureImage,
            originalTextureImage.Size.ShrinkToFit(ThumbnailSize),
            ImageTools::FilterKind::Bilinear);

        assert(static_cast<size_t>(tfs.Metadata.FrameId.FrameIndex) == mLandAvailableThumbnails.size());

        mLandAvailableThumbnails.emplace_back(
            tfs.Metadata.DisplayName,
            std::move(textureThumbnail));
    }
}

void WorldRenderContext::InitializeFishTextures()
{
    // Load texture database
    auto fishTextureDatabase = TextureDatabase<GameTextureDatabases::FishTextureDatabase>::Load(mAssetManager);

    // Create atlas
    auto fishTextureAtlas = TextureAtlasBuilder<GameTextureDatabases::FishTextureDatabase>::BuildAtlas(
        fishTextureDatabase,
        TextureAtlasOptions::MipMappable,
        mAssetManager,
        [](float, ProgressMessageType) {});

    LogMessage("Fish texture atlas size: ", fishTextureAtlas.Image.Size);

    mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::FishesAtlasTexture>();

    // Create OpenGL handle
    GLuint tmpGLuint;
    glGenTextures(1, &tmpGLuint);
    mFishTextureAtlasOpenGLHandle = tmpGLuint;

    // Bind texture atlas
    glBindTexture(GL_TEXTURE_2D, *mFishTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    assert(fishTextureAtlas.Metadata.IsSuitableForMipMapping());
    GameOpenGL::UploadMipmappedAtlasTexture(
        std::move(fishTextureAtlas.Image),
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
    mFishTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata<GameTextureDatabases::FishTextureDatabase>>(fishTextureAtlas.Metadata);

    // Set textures in shader
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::FishesBasic>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::FishesBasic>();
    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::FishesDetailed>();
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::FishesDetailed>();
}

void WorldRenderContext::OnReset(RenderParameters const & renderParameters)
{
    // Invoked on rendering thread

    if (renderParameters.LandRenderDetail == LandRenderDetailType::Detailed)
    {
        // Re-generate noise
        mGlobalRenderContext.RegeneratePerlin_8_1024_073_Noise();
    }
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

    // Reset AABBs, they are uploaded as needed
    mAABBVertexBuffer.clear();
}

void WorldRenderContext::UploadStarsStart(
    size_t uploadCount,
    size_t totalCount)
{
    //
    // Stars are sticky: we upload them once in a while and
    // continue drawing the same buffer, eventually updating
    // a prefix of it
    //

    mStarVertexBuffer.ensure_size_full(totalCount);
    mDirtyStarsCount = uploadCount;
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

    mLightningVertexBuffer.reset_full(6 * lightningCount);

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

    mCloudVertexBuffer.reset(4 * cloudCount);

    mGlobalRenderContext.GetElementIndices().EnsureSize(cloudCount);
}

void WorldRenderContext::UploadCloudsEnd()
{
    // Nop
}

void WorldRenderContext::UploadCloudShadows(
    float const * shadowBuffer,
    size_t shadowSampleCount)
{
    // We've been invoked on the render thread

    mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::SharedTexture>();
    glBindTexture(GL_TEXTURE_1D, *mCloudShadowsTextureOpenGLHandle);
    if (!mHasCloudShadowsTextureBeenAllocated)
    {
        glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, static_cast<GLsizei>(shadowSampleCount), 0, GL_RED, GL_FLOAT, shadowBuffer);
        mHasCloudShadowsTextureBeenAllocated = true;
    }
    else
    {
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, static_cast<GLsizei>(shadowSampleCount), GL_RED, GL_FLOAT, shadowBuffer);
    }

    CheckOpenGLError();
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

void WorldRenderContext::UploadOceanBasicStart(size_t slices)
{
    //
    // Ocean segments are not sticky: we upload them at each frame
    //

    mOceanBasicSegmentBuffer.reset(slices + 1);
}

void WorldRenderContext::UploadOceanBasicEnd()
{
    // Nop
}

void WorldRenderContext::UploadOceanDetailedStart(size_t slices)
{
    //
    // Ocean segments are not sticky: we upload them at each frame
    //

    mOceanDetailedSegmentBuffer.reset(slices + 1);
}

void WorldRenderContext::UploadOceanDetailedEnd()
{
    // Nop
}

void WorldRenderContext::UploadFishesStart(size_t fishCount)
{
    //
    // Fishes are not sticky: we upload them at each frame
    //

    mFishVertexBuffer.reset(4 * fishCount);

    mGlobalRenderContext.GetElementIndices().EnsureSize(fishCount);
}

void WorldRenderContext::UploadFishesEnd()
{
    // Nop
}

void WorldRenderContext::UploadAABBsStart(size_t aabbCount)
{
    //
    // AABBs are not sticky: we upload them at each frame
    //

    mAABBVertexBuffer.reset(8 * aabbCount);
}

void WorldRenderContext::UploadAABBsEnd()
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

    if (renderParameters.IsSkyDirty)
    {
        ApplySkyChanges(renderParameters);
    }

    if (renderParameters.IsOceanDepthDarkeningRateDirty)
    {
        ApplyOceanDepthDarkeningRateChanges(renderParameters);
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
    if (mDirtyStarsCount > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mStarVBO);

        if (mStarVBOAllocatedVertexSize != mStarVertexBuffer.size())
        {
            // Re-allocate VBO buffer and upload entire buffer
            glBufferData(GL_ARRAY_BUFFER, mStarVertexBuffer.size() * sizeof(StarVertex), mStarVertexBuffer.data(), GL_STATIC_DRAW);
            CheckOpenGLError();

            mStarVBOAllocatedVertexSize = mStarVertexBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer prefix
            glBufferSubData(GL_ARRAY_BUFFER, 0, mDirtyStarsCount * sizeof(StarVertex), mStarVertexBuffer.data());
            CheckOpenGLError();
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        mDirtyStarsCount = 0;
    }
}

void WorldRenderContext::RenderDrawSky(RenderParameters const & renderParameters)
{
    //
    // First step in pipeline, as it implicitly or explicitly clears the canvas
    //

    if (renderParameters.DoCrepuscularGradient
        && renderParameters.DebugShipRenderMode != DebugShipRenderModeType::Wireframe)
    {
        // Use shader - it'll clear canvas

        glBindVertexArray(*mSkyVAO);

        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::Sky>();

        glDrawArrays(GL_TRIANGLES, 0, 6);
        CheckOpenGLError();

        glBindVertexArray(0);

        // Clear depth buffer
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    else
    {
        // Clear canvas - and depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

void WorldRenderContext::RenderDrawStars(RenderParameters const & /*renderParameters*/)
{
    if (mStarVertexBuffer.size() > 0)
    {
        glBindVertexArray(*mStarVAO);

        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::Stars>();

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

        if (mLightningVertexBuffer.max_size() > mLightningVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ARRAY_BUFFER, mLightningVertexBuffer.max_size() * sizeof(LightningVertex), mLightningVertexBuffer.data(), GL_STREAM_DRAW);
            CheckOpenGLError();

            mLightningVBOAllocatedVertexSize = mLightningVertexBuffer.max_size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ARRAY_BUFFER, 0, mLightningVertexBuffer.max_size() * sizeof(LightningVertex), mLightningVertexBuffer.data());
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

    bool const areCloudsHighQuality = (renderParameters.CloudRenderDetail == CloudRenderDetailType::Detailed);

    assert((mCloudVertexBuffer.size() % 4) == 0);
    size_t const elementIndexCount = mCloudVertexBuffer.size() / 4 * 6; // 4 vertices -> 6 element indices

    // The number of clouds we want to draw *over* background
    // lightnings
    size_t constexpr CloudsOverLightnings = 5;
    GLsizei cloudsOverLightningElementIndexStart = 0;

    if (mBackgroundLightningVertexCount > 0
        && mCloudVertexBuffer.size() > 4 * CloudsOverLightnings)
    {
        glBindVertexArray(*mCloudVAO);

        // Intel bug: cannot associate with VAO
        mGlobalRenderContext.GetElementIndices().Bind();

        if (areCloudsHighQuality)
        {
            mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::CloudsDetailed>();

            mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::NoiseTexture>();
            glBindTexture(GL_TEXTURE_2D, mGlobalRenderContext.GetNoiseTextureOpenGLHandle(NoiseType::Perlin_4_32_043));
        }
        else
        {
            mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::CloudsBasic>();
        }

        if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
            glLineWidth(0.1f);

        cloudsOverLightningElementIndexStart = static_cast<GLsizei>(elementIndexCount) - (6 * CloudsOverLightnings);

        glDrawElements(
            GL_TRIANGLES,
            cloudsOverLightningElementIndexStart,
            GL_UNSIGNED_INT,
            (GLvoid *)0);

        CheckOpenGLError();
    }

    ////////////////////////////////////////////////////
    // Draw background lightnings
    ////////////////////////////////////////////////////

    if (mBackgroundLightningVertexCount > 0)
    {
        glBindVertexArray(*mLightningVAO);

        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::Lightning>();

        mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::NoiseTexture>();
        glBindTexture(GL_TEXTURE_2D, mGlobalRenderContext.GetNoiseTextureOpenGLHandle(NoiseType::Gross));

        glDrawArrays(GL_TRIANGLES,
            0,
            static_cast<GLsizei>(mBackgroundLightningVertexCount));

        CheckOpenGLError();
    }

    ////////////////////////////////////////////////////
    // Draw foreground clouds
    ////////////////////////////////////////////////////

    if (elementIndexCount > static_cast<size_t>(cloudsOverLightningElementIndexStart))
    {
        glBindVertexArray(*mCloudVAO);

        // Intel bug: cannot associate with VAO
        mGlobalRenderContext.GetElementIndices().Bind();

        if (areCloudsHighQuality)
        {
            mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::CloudsDetailed>();

            mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::NoiseTexture>();
            glBindTexture(GL_TEXTURE_2D, mGlobalRenderContext.GetNoiseTextureOpenGLHandle(NoiseType::Perlin_4_32_043));
        }
        else
        {
            mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::CloudsBasic>();
        }

        if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
            glLineWidth(0.1f);

        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(elementIndexCount) - cloudsOverLightningElementIndexStart,
            GL_UNSIGNED_INT,
            (GLvoid *)(static_cast<size_t>(cloudsOverLightningElementIndexStart) * sizeof(int)));

        CheckOpenGLError();
    }

    ////////////////////////////////////////////////////

    glBindVertexArray(0);
}

void WorldRenderContext::RenderPrepareOcean(RenderParameters const & renderParameters)
{
    //
    // Buffers
    //

    switch (renderParameters.OceanRenderDetail)
    {
        case OceanRenderDetailType::Basic:
        {
            glBindBuffer(GL_ARRAY_BUFFER, *mOceanBasicSegmentVBO);

            if (mOceanBasicSegmentVBOAllocatedVertexSize != mOceanBasicSegmentBuffer.size())
            {
                // Re-allocate VBO buffer and upload
                glBufferData(GL_ARRAY_BUFFER, mOceanBasicSegmentBuffer.size() * sizeof(OceanBasicSegment), mOceanBasicSegmentBuffer.data(), GL_STREAM_DRAW);
                CheckOpenGLError();

                mOceanBasicSegmentVBOAllocatedVertexSize = mOceanBasicSegmentBuffer.size();
            }
            else
            {
                // No size change, just upload VBO buffer
                glBufferSubData(GL_ARRAY_BUFFER, 0, mOceanBasicSegmentBuffer.size() * sizeof(OceanBasicSegment), mOceanBasicSegmentBuffer.data());
                CheckOpenGLError();
            }

            break;
        }

        case OceanRenderDetailType::Detailed:
        {
            glBindBuffer(GL_ARRAY_BUFFER, *mOceanDetailedSegmentVBO);

            if (mOceanDetailedSegmentVBOAllocatedVertexSize != mOceanDetailedSegmentBuffer.size())
            {
                // Re-allocate VBO buffer and upload
                glBufferData(GL_ARRAY_BUFFER, mOceanDetailedSegmentBuffer.size() * sizeof(OceanDetailedSegment), mOceanDetailedSegmentBuffer.data(), GL_STREAM_DRAW);
                CheckOpenGLError();

                mOceanDetailedSegmentVBOAllocatedVertexSize = mOceanDetailedSegmentBuffer.size();
            }
            else
            {
                // No size change, just upload VBO buffer
                glBufferSubData(GL_ARRAY_BUFFER, 0, mOceanDetailedSegmentBuffer.size() * sizeof(OceanDetailedSegment), mOceanDetailedSegmentBuffer.data());
                CheckOpenGLError();
            }

            break;
        };
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Parameters
    //

    if (mIsSunRaysInclinationDirty)
    {
        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthDetailedBackground>();
        mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthDetailedBackground, GameShaderSet::ProgramParameterKind::SunRaysInclination>(
            mSunRaysInclination);

        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthDetailedForeground>();
        mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthDetailedForeground, GameShaderSet::ProgramParameterKind::SunRaysInclination>(
            mSunRaysInclination);

        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatDetailedBackground>();
        mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanFlatDetailedBackground, GameShaderSet::ProgramParameterKind::SunRaysInclination>(
            mSunRaysInclination);

        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatDetailedForeground>();
        mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanFlatDetailedForeground, GameShaderSet::ProgramParameterKind::SunRaysInclination>(
            mSunRaysInclination);

        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureDetailedBackground>();
        mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanTextureDetailedBackground, GameShaderSet::ProgramParameterKind::SunRaysInclination>(
            mSunRaysInclination);

        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureDetailedForeground>();
        mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanTextureDetailedForeground, GameShaderSet::ProgramParameterKind::SunRaysInclination>(
            mSunRaysInclination);


        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::FishesDetailed>();
        mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::FishesDetailed, GameShaderSet::ProgramParameterKind::SunRaysInclination>(
            mSunRaysInclination);


        mIsSunRaysInclinationDirty = false;
    }
}

void WorldRenderContext::RenderDrawOcean(bool opaquely, RenderParameters const & renderParameters)
{
    float const transparency = opaquely ? 0.0f : renderParameters.OceanTransparency;

    switch (renderParameters.OceanRenderDetail)
    {
        case OceanRenderDetailType::Basic:
        {
            glBindVertexArray(*mOceanBasicVAO);

            switch (renderParameters.OceanRenderMode)
            {
                case OceanRenderModeType::Depth:
                {
                    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthBasic>();
                    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthBasic, GameShaderSet::ProgramParameterKind::OceanTransparency>(
                        transparency);

                    mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::NoiseTexture>();
                    glBindTexture(GL_TEXTURE_2D, mGlobalRenderContext.GetNoiseTextureOpenGLHandle(NoiseType::Fine));

                    break;
                }

                case OceanRenderModeType::Flat:
                {
                    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatBasic>();
                    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanFlatBasic, GameShaderSet::ProgramParameterKind::OceanTransparency>(
                        transparency);

                    break;
                }

                case OceanRenderModeType::Texture:
                {
                    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureBasic>();
                    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanTextureBasic, GameShaderSet::ProgramParameterKind::OceanTransparency>(
                        transparency);

                    break;
                }
            }

            if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
                glLineWidth(0.1f);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mOceanBasicSegmentBuffer.size()));

            break;
        }

        case OceanRenderDetailType::Detailed:
        {
            // Bind cloud shadows texture

            mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::SharedTexture>();
            glBindTexture(GL_TEXTURE_1D, *mCloudShadowsTextureOpenGLHandle);

            // Draw background if drawing opaquely, else foreground

            glBindVertexArray(*mOceanDetailedVAO);

            switch (renderParameters.OceanRenderMode)
            {
                case OceanRenderModeType::Depth:
                {
                    GameShaderSet::ProgramKind const oceanShader = opaquely ? GameShaderSet::ProgramKind::OceanDepthDetailedBackground : GameShaderSet::ProgramKind::OceanDepthDetailedForeground;

                    mShaderManager.ActivateProgram(oceanShader);
                    mShaderManager.SetProgramParameter<GameShaderSet::ProgramParameterKind::OceanTransparency>(
                        oceanShader,
                        transparency);

                    mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::NoiseTexture>();
                    glBindTexture(GL_TEXTURE_2D, mGlobalRenderContext.GetNoiseTextureOpenGLHandle(NoiseType::Fine));

                    break;
                }

                case OceanRenderModeType::Flat:
                {
                    GameShaderSet::ProgramKind const oceanShader = opaquely ? GameShaderSet::ProgramKind::OceanFlatDetailedBackground : GameShaderSet::ProgramKind::OceanFlatDetailedForeground;

                    mShaderManager.ActivateProgram(oceanShader);
                    mShaderManager.SetProgramParameter<GameShaderSet::ProgramParameterKind::OceanTransparency>(
                        oceanShader,
                        transparency);

                    mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::NoiseTexture>();
                    glBindTexture(GL_TEXTURE_2D, mGlobalRenderContext.GetNoiseTextureOpenGLHandle(NoiseType::Fine));

                    break;
                }

                case OceanRenderModeType::Texture:
                {
                    GameShaderSet::ProgramKind const oceanShader = opaquely ? GameShaderSet::ProgramKind::OceanTextureDetailedBackground : GameShaderSet::ProgramKind::OceanTextureDetailedForeground;

                    mShaderManager.ActivateProgram(oceanShader);
                    mShaderManager.SetProgramParameter<GameShaderSet::ProgramParameterKind::OceanTransparency>(
                        oceanShader,
                        transparency);

                    break;
                }
            }

            if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
                glLineWidth(0.1f);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mOceanDetailedSegmentBuffer.size()));

            break;
        }
    }

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
    bool isHighQuality = false;
    switch (renderParameters.LandRenderDetail)
    {
        case LandRenderDetailType::Basic:
        {
            isHighQuality = false;
            break;
        }

        case LandRenderDetailType::Detailed:
        {
            isHighQuality = true;
            break;
        }
    }

    glBindVertexArray(*mLandVAO);

    switch (renderParameters.LandRenderMode)
    {
        case LandRenderModeType::Flat:
        {
            if (isHighQuality)
                mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandFlatDetailed>();
            else
                mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandFlatBasic>();
            break;
        }

        case LandRenderModeType::Texture:
        {
            if (isHighQuality)
                mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandTextureDetailed>();
            else
                mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandTextureBasic>();
            break;
        }
    }

    if (isHighQuality)
    {
        // Activate noise texture
        mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::NoiseTexture>();
        glBindTexture(GL_TEXTURE_2D, mGlobalRenderContext.GetNoiseTextureOpenGLHandle(NoiseType::Perlin_8_1024_073));
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

void WorldRenderContext::RenderDrawFishes(RenderParameters const & renderParameters)
{
    if (mFishVertexBuffer.size() > 0)
    {
        glBindVertexArray(*mFishVAO);

        // Intel bug: cannot associate with VAO
        mGlobalRenderContext.GetElementIndices().Bind();

        switch (renderParameters.OceanRenderDetail)
        {
            case OceanRenderDetailType::Basic:
            {
                mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::FishesBasic>();

                break;
            }

            case OceanRenderDetailType::Detailed:
            {
                mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::FishesDetailed>();

                mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::SharedTexture>();
                glBindTexture(GL_TEXTURE_1D, *mCloudShadowsTextureOpenGLHandle);

                break;
            }
        }

        mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::NoiseTexture>();
        glBindTexture(GL_TEXTURE_2D, mGlobalRenderContext.GetNoiseTextureOpenGLHandle(NoiseType::Fine));

        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(mFishVertexBuffer.size() / 4 * 6),
            GL_UNSIGNED_INT,
            (GLvoid *)0);
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

        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::AMBombPreImplosion>();

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

        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::CrossOfLight>();

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

        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::Lightning>();

        mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::NoiseTexture>();
        glBindTexture(GL_TEXTURE_2D, mGlobalRenderContext.GetNoiseTextureOpenGLHandle(NoiseType::Gross));

        glDrawArrays(GL_TRIANGLES,
            static_cast<GLsizei>(mLightningVertexBuffer.max_size() - mForegroundLightningVertexCount),
            static_cast<GLsizei>(mForegroundLightningVertexCount));
        CheckOpenGLError();

        glBindVertexArray(0);
    }
}

void WorldRenderContext::RenderPrepareRain(RenderParameters const & /*renderParameters*/)
{
    if (mIsRainDensityDirty || mRainDensity != 0.0f)
    {
        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::Rain>();

        if (mIsRainDensityDirty)
        {
            float const actualRainDensity = std::sqrt(mRainDensity); // Focus

            // Set parameter
            mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::Rain, GameShaderSet::ProgramParameterKind::RainDensity>(
                actualRainDensity);

            mIsRainDensityDirty = false; // Uploaded
        }

        if (mIsRainWindSpeedMagnitudeDirty)
        {
            float const rainAngle = SmoothStep(
                30.0f,
                250.0f,
                std::abs(mRainWindSpeedMagnitude))
                * ((mRainWindSpeedMagnitude < 0.0f) ? -1.0f : 1.0f)
                * 0.8f;

            // Set parameter
            mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::Rain, GameShaderSet::ProgramParameterKind::RainAngle>(
                rainAngle);

            mIsRainWindSpeedMagnitudeDirty = false; // Uploaded
        }

        if (mRainDensity != 0.0f)
        {
            // Set time parameter
            mShaderManager.SetProgramParameter<GameShaderSet::ProgramParameterKind::Time>(
                GameShaderSet::ProgramKind::Rain,
                GameWallClock::GetInstance().NowAsFloat());
        }
    }
}

void WorldRenderContext::RenderDrawRain(RenderParameters const & /*renderParameters*/)
{
    if (mRainDensity != 0.0f)
    {
        glBindVertexArray(*mRainVAO);

        mShaderManager.ActivateProgram(GameShaderSet::ProgramKind::Rain);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        CheckOpenGLError();

        glBindVertexArray(0);
    }
}

void WorldRenderContext::RenderPrepareAABBs(RenderParameters const & /*renderParameters*/)
{
    if (!mAABBVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mAABBVBO);

        if (mAABBVertexBuffer.size() > mAABBVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ARRAY_BUFFER, mAABBVertexBuffer.size() * sizeof(AABBVertex), mAABBVertexBuffer.data(), GL_STREAM_DRAW);
            CheckOpenGLError();

            mAABBVBOAllocatedVertexSize = mAABBVertexBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ARRAY_BUFFER, 0, mAABBVertexBuffer.size() * sizeof(AABBVertex), mAABBVertexBuffer.data());
            CheckOpenGLError();
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void WorldRenderContext::RenderDrawAABBs(RenderParameters const & /*renderParameters*/)
{
    if (!mAABBVertexBuffer.empty())
    {
        glBindVertexArray(*mAABBVAO);

        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::AABBs>();

        glLineWidth(2.0f);

        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mAABBVertexBuffer.size()));
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

        mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::WorldBorder>();

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

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandFlatBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandFlatBasic, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandFlatDetailed>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandFlatDetailed, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandTextureBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandTextureBasic, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandTextureDetailed>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandTextureDetailed, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthBasic, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthDetailedBackground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthDetailedBackground, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthDetailedForeground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthDetailedForeground, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanFlatBasic, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatDetailedBackground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanFlatDetailedBackground, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatDetailedForeground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanFlatDetailedForeground, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanTextureBasic, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureDetailedBackground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanTextureDetailedBackground, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureDetailedForeground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanTextureDetailedForeground, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::FishesBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::FishesBasic, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::FishesDetailed>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::FishesDetailed, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::AMBombPreImplosion>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::AMBombPreImplosion, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::CrossOfLight>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::CrossOfLight, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::AABBs>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::AABBs, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::WorldBorder>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::WorldBorder, GameShaderSet::ProgramParameterKind::OrthoMatrix>(
        globalOrthoMatrix);

    //
    // Freeze here view cam's y - warped so perspective is more visible at lower y
    //

    mCloudNormalizedViewCamY = 2.0f / (1.0f + std::exp(-12.0f * renderParameters.View.GetCameraWorldPosition().y / renderParameters.View.GetHalfMaxWorldHeight())) - 1.0f;


    //
    // Recalculate world border
    //

    RecalculateWorldBorder(renderParameters);
}

void WorldRenderContext::ApplyCanvasSizeChanges(RenderParameters const & renderParameters)
{
    auto const & view = renderParameters.View;

    // Set shader parameters

    vec2f const viewportSize = vec2f(
        static_cast<float>(view.GetCanvasPhysicalSize().width),
        static_cast<float>(view.GetCanvasPhysicalSize().height));

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::CrossOfLight>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::CrossOfLight, GameShaderSet::ProgramParameterKind::ViewportSize>(viewportSize);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::Rain>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::Rain, GameShaderSet::ProgramParameterKind::ViewportSize>(viewportSize);
}

void WorldRenderContext::ApplyEffectiveAmbientLightIntensityChanges(RenderParameters const & renderParameters)
{
    RecalculateClearCanvasColor(renderParameters);

    // Set parameters in all programs

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::Sky>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::Sky, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::Stars>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::Stars, GameShaderSet::ProgramParameterKind::StarTransparency>(
        pow(std::max(0.0f, 1.0f - renderParameters.EffectiveAmbientLightIntensity), 3.0f));

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::CloudsBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::CloudsBasic, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::CloudsDetailed>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::CloudsDetailed, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::Lightning>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::Lightning, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandFlatBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandFlatBasic, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandFlatDetailed>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandFlatDetailed, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandTextureBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandTextureBasic, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandTextureDetailed>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandTextureDetailed, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthBasic, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthDetailedBackground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthDetailedBackground, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthDetailedForeground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthDetailedForeground, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanFlatBasic, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatDetailedBackground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanFlatDetailedBackground, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatDetailedForeground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanFlatDetailedForeground, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanTextureBasic, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureDetailedBackground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanTextureDetailedBackground, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureDetailedForeground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanTextureDetailedForeground, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::FishesBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::FishesBasic, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::FishesDetailed>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::FishesDetailed, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::Rain>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::Rain, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::WorldBorder>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::WorldBorder, GameShaderSet::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        renderParameters.EffectiveAmbientLightIntensity);
}

void WorldRenderContext::ApplySkyChanges(RenderParameters const & renderParameters)
{
    RecalculateClearCanvasColor(renderParameters);

    // Set parameters in all programs

    vec3f const effectiveMoonlightColor = renderParameters.EffectiveMoonlightColor.toVec3f();


    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::Sky>();

    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::Sky, GameShaderSet::ProgramParameterKind::CrepuscularColor>(
        renderParameters.CrepuscularColor.toVec3f());

    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::Sky, GameShaderSet::ProgramParameterKind::FlatSkyColor>(
        renderParameters.FlatSkyColor.toVec3f());

    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::Sky, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);


    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::CloudsBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::CloudsBasic, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::CloudsDetailed>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::CloudsDetailed, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanFlatBasic, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatDetailedBackground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanFlatDetailedBackground, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatDetailedForeground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanFlatDetailedForeground, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthBasic, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthDetailedBackground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthDetailedBackground, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthDetailedForeground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthDetailedForeground, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanTextureBasic, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureDetailedBackground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanTextureDetailedBackground, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureDetailedForeground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanTextureDetailedForeground, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandFlatBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandFlatBasic, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandFlatDetailed>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandFlatDetailed, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandTextureBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandTextureBasic, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandTextureDetailed>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandTextureDetailed, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::Rain>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::Rain, GameShaderSet::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);
}

void WorldRenderContext::ApplyOceanDepthDarkeningRateChanges(RenderParameters const & renderParameters)
{
    // Set parameter in all programs

    float const rate = renderParameters.OceanDepthDarkeningRate / 50.0f;

    mShaderManager.SetProgramParameterInAllShaders<GameShaderSet::ProgramParameterKind::OceanDepthDarkeningRate>(rate);
}

void WorldRenderContext::ApplyOceanRenderParametersChanges(RenderParameters const & renderParameters)
{
    // Set ocean parameters in all water programs

    vec3f const depthColorStart = renderParameters.DepthOceanColorStart.toVec3f();

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthBasic, GameShaderSet::ProgramParameterKind::OceanDepthColorStart>(depthColorStart);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthDetailedBackground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthDetailedBackground, GameShaderSet::ProgramParameterKind::OceanDepthColorStart>(depthColorStart);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthDetailedForeground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthDetailedForeground, GameShaderSet::ProgramParameterKind::OceanDepthColorStart>(depthColorStart);

    vec3f const depthColorEnd = renderParameters.DepthOceanColorEnd.toVec3f();

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthBasic, GameShaderSet::ProgramParameterKind::OceanDepthColorEnd>(depthColorEnd);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthDetailedBackground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthDetailedBackground, GameShaderSet::ProgramParameterKind::OceanDepthColorEnd>(depthColorEnd);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanDepthDetailedForeground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanDepthDetailedForeground, GameShaderSet::ProgramParameterKind::OceanDepthColorEnd>(depthColorEnd);

    vec3f const flatColor = renderParameters.FlatOceanColor.toVec3f();

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanFlatBasic, GameShaderSet::ProgramParameterKind::OceanFlatColor>(flatColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatDetailedBackground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanFlatDetailedBackground, GameShaderSet::ProgramParameterKind::OceanFlatColor>(flatColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanFlatDetailedForeground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanFlatDetailedForeground, GameShaderSet::ProgramParameterKind::OceanFlatColor>(flatColor);
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
    auto oceanTextureFrame = mOceanTextureFrameSpecifications[clampedOceanTextureIndex].LoadFrame(mAssetManager);

    // Activate texture
    mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::OceanTexture>();

    // Create texture
    GLuint tmpGLuint;
    glGenTextures(1, &tmpGLuint);
    mOceanTextureOpenGLHandle = tmpGLuint;

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mOceanTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadMipmappedTexture(
        std::move(oceanTextureFrame.TextureData),
        GL_RGB8);

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    CheckOpenGLError();

    // Set filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Set texture and texture parameters in shaders

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanTextureBasic, GameShaderSet::ProgramParameterKind::TextureScaling>(
        1.0f / oceanTextureFrame.Metadata.WorldWidth,
        1.0f / oceanTextureFrame.Metadata.WorldHeight);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureDetailedBackground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanTextureDetailedBackground, GameShaderSet::ProgramParameterKind::TextureScaling>(
        1.0f / oceanTextureFrame.Metadata.WorldWidth,
        1.0f / oceanTextureFrame.Metadata.WorldHeight);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::OceanTextureDetailedForeground>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::OceanTextureDetailedForeground, GameShaderSet::ProgramParameterKind::TextureScaling>(
        1.0f / oceanTextureFrame.Metadata.WorldWidth,
        1.0f / oceanTextureFrame.Metadata.WorldHeight);
}

void WorldRenderContext::ApplyLandRenderParametersChanges(RenderParameters const & renderParameters)
{
    // Set land parameters in all land flat programs

    vec3f const flatColor = renderParameters.FlatLandColor.toVec3f();

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandFlatBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandFlatBasic, GameShaderSet::ProgramParameterKind::LandFlatColor>(flatColor);

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandFlatDetailed>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandFlatDetailed, GameShaderSet::ProgramParameterKind::LandFlatColor>(flatColor);
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
    auto landTextureFrame = mLandTextureFrameSpecifications[clampedLandTextureIndex].LoadFrame(mAssetManager);

    // Activate texture
    mShaderManager.ActivateTexture<GameShaderSet::ProgramParameterKind::LandTexture>();

    // Create texture
    GLuint tmpGLuint;
    glGenTextures(1, &tmpGLuint);
    mLandTextureOpenGLHandle = tmpGLuint;

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mLandTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadMipmappedTexture(
        std::move(landTextureFrame.TextureData),
        GL_RGB8);

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    CheckOpenGLError();

    // Set filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Set texture and texture parameters in all texture shaders

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandTextureBasic>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandTextureBasic, GameShaderSet::ProgramParameterKind::TextureScaling>(
        1.0f / landTextureFrame.Metadata.WorldWidth,
        1.0f / landTextureFrame.Metadata.WorldHeight);
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::LandTextureBasic>();

    mShaderManager.ActivateProgram<GameShaderSet::ProgramKind::LandTextureDetailed>();
    mShaderManager.SetProgramParameter<GameShaderSet::ProgramKind::LandTextureDetailed, GameShaderSet::ProgramParameterKind::TextureScaling>(
        1.0f / landTextureFrame.Metadata.WorldWidth,
        1.0f / landTextureFrame.Metadata.WorldHeight);
    mShaderManager.SetTextureParameters<GameShaderSet::ProgramKind::LandTextureDetailed>();
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

void WorldRenderContext::RecalculateClearCanvasColor(RenderParameters const & renderParameters)
{
    vec3f const clearColor = renderParameters.FlatSkyColor.toVec3f() * renderParameters.EffectiveAmbientLightIntensity;
    glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);
}

void WorldRenderContext::RecalculateWorldBorder(RenderParameters const & renderParameters)
{
    // Calculate width and height, in world coordinates, of the world border, under the constraint
    // that they result in the specified pixel size

    auto const & viewModel = renderParameters.View;

    static float constexpr WorldBorderPixelSize = 20.0f;
    float const worldBorderWorldSize = viewModel.PhysicalDisplayOffsetToWorldOffset(static_cast<float>(WorldBorderPixelSize));

    //
    // Check which sides - if any - of the border we need to draw
    //
    // Note: texture coord 0 is max border

    mWorldBorderVertexBuffer.clear();

    float const halfMaxWorldWidth = renderParameters.View.GetHalfMaxWorldWidth();
    float const halfMaxWorldHeight = renderParameters.View.GetHalfMaxWorldHeight();

    // Left
    if (-halfMaxWorldWidth + worldBorderWorldSize >= viewModel.GetVisibleWorld().TopLeft.x)
    {
        EmplaceWorldBorderQuad(
            // Top-left
            -halfMaxWorldWidth,
            halfMaxWorldHeight,
            0.0f,
            1.0f,
            // Bottom-right
            -halfMaxWorldWidth + worldBorderWorldSize,
            -halfMaxWorldHeight,
            1.0f,
            1.0f,
            mWorldBorderVertexBuffer);
    }

    // Right
    if (halfMaxWorldWidth - worldBorderWorldSize <= viewModel.GetVisibleWorld().BottomRight.x)
    {
        EmplaceWorldBorderQuad(
            // Top-left
            halfMaxWorldWidth - worldBorderWorldSize,
            halfMaxWorldHeight,
            1.0f,
            1.0f,
            // Bottom-right
            halfMaxWorldWidth,
            -halfMaxWorldHeight,
            0.0f,
            1.0f,
            mWorldBorderVertexBuffer);
    }

    // Top
    if (halfMaxWorldHeight - worldBorderWorldSize <= viewModel.GetVisibleWorld().TopLeft.y)
    {
        EmplaceWorldBorderQuad(
            // Top-left
            -halfMaxWorldWidth,
            halfMaxWorldHeight,
            1.0f,
            0.0f,
            // Bottom-right
            halfMaxWorldWidth,
            halfMaxWorldHeight - worldBorderWorldSize,
            1.0f,
            1.0f,
            mWorldBorderVertexBuffer);
    }

    // Bottom
    if (-halfMaxWorldHeight + worldBorderWorldSize >= viewModel.GetVisibleWorld().BottomRight.y)
    {
        EmplaceWorldBorderQuad(
            // Top-left
            -halfMaxWorldWidth,
            -halfMaxWorldHeight + worldBorderWorldSize,
            1.0f,
            1.0f,
            // Bottom-right
            halfMaxWorldWidth,
            -halfMaxWorldHeight,
            1.0f,
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
