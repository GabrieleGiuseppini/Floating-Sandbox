/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderContext.h"

#include <Game/ImageFileTools.h>

#include <GameCore/GameChronometer.h>
#include <GameCore/GameException.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/ImageTools.h>
#include <GameCore/Log.h>

#include <cstring>

namespace Render {

ImageSize constexpr ThumbnailSize(32, 32);

RenderContext::RenderContext(
    ImageSize const & initialCanvasSize,
    std::function<void()> makeRenderContextCurrentFunction,
    std::function<void()> swapRenderBuffersFunction,
    PerfStats & perfStats,
    ResourceLocator const & resourceLocator,
    ProgressCallback const & progressCallback)
    // Thread
    : mRenderThread()
    , mLastRenderUploadEndCompletionIndicator()
    , mLastRenderDrawCompletionIndicator()
    // Buffers
    , mStarVertexBuffer()
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
    , mAMBombPreImplosionVertexBuffer()
    , mAMBombPreImplosionVBO()
    , mAMBombPreImplosionVBOAllocatedVertexSize(0u)
    , mCrossOfLightVertexBuffer()
    , mCrossOfLightVBO()
    , mCrossOfLightVBOAllocatedVertexSize(0u)
    , mHeatBlasterFlameVBO()
    , mFireExtinguisherSprayVBO()
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
    , mAMBombPreImplosionVAO()
    , mCrossOfLightVAO()
    , mHeatBlasterFlameVAO()
    , mFireExtinguisherSprayVAO()
    , mRainVAO()
    , mWorldBorderVAO()
    // Textures
    , mCloudTextureAtlasOpenGLHandle()
    , mCloudTextureAtlasMetadata()
    , mUploadedWorldTextureManager()
    , mOceanTextureFrameSpecifications()
    , mOceanTextureOpenGLHandle()
    , mLoadedOceanTextureIndex(std::numeric_limits<size_t>::max())
    , mLandTextureFrameSpecifications()
    , mLandTextureOpenGLHandle()
    , mLoadedLandTextureIndex(std::numeric_limits<size_t>::max())
    , mGenericLinearTextureAtlasOpenGLHandle()
    , mGenericLinearTextureAtlasMetadata()
    , mGenericMipMappedTextureAtlasOpenGLHandle()
    , mGenericMipMappedTextureAtlasMetadata()
    , mExplosionTextureAtlasOpenGLHandle()
    , mExplosionTextureAtlasMetadata()
    , mUploadedNoiseTexturesManager()
    // Ships
    , mShips()
    // HeatBlaster
    , mHeatBlasterFlameShaderToRender()
    // Fire extinguisher
    , mFireExtinguisherSprayShaderToRender()
    // Rendering externals
    , mSwapRenderBuffersFunction(swapRenderBuffersFunction)
    // Managers
    , mShaderManager()
    , mNotificationRenderContext()    
    // Render settings
    , mRenderSettings(initialCanvasSize)
    // TODOOLD
    , mOceanTransparency(0.8125f)
    , mOceanDarkeningRate(0.356993f)
    , mOceanRenderMode(OceanRenderModeType::Texture)
    , mOceanAvailableThumbnails()
    , mSelectedOceanTextureIndex(0) // Wavy Clear Thin
    , mDepthOceanColorStart(0x4a, 0x84, 0x9f)
    , mDepthOceanColorEnd(0x00, 0x00, 0x00)
    , mFlatOceanColor(0x00, 0x3d, 0x99)
    , mLandRenderMode(LandRenderModeType::Texture)
    , mLandAvailableThumbnails()
    , mSelectedLandTextureIndex(3) // Rock Coarse 3
    , mFlatLandColor(0x72, 0x46, 0x05)
    //
    , mFlatLampLightColor(0xff, 0xff, 0xbf)
    , mDefaultWaterColor(0x00, 0x00, 0xcc)
    , mShowShipThroughOcean(false)
    , mWaterContrast(0.71875f)
    , mWaterLevelOfDetail(0.6875f)
    , mVectorFieldLengthMultiplier(1.0f)
    , mShowStressedSprings(false)
    , mDrawHeatOverlay(false)
    , mHeatOverlayTransparency(0.1875f)
    , mShipFlameSizeAdjustment(1.0f)
    // Statistics
    , mPerfStats(perfStats)
    , mRenderStats()
{
    progressCallback(0.0f, "Initializing OpenGL...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            //
            // Initialize OpenGL
            //

            // Make render context current
            makeRenderContextCurrentFunction();

            // Initialize OpenGL
            GameOpenGL::InitOpenGL();

            // Initialize the shared texture unit once and for all
            mShaderManager->ActivateTexture<ProgramParameterType::SharedTexture>();
        });

    progressCallback(0.1f, "Loading shaders...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            //
            // Load shader manager
            //

            mShaderManager = ShaderManager<ShaderManagerTraits>::CreateInstance(resourceLocator.GetRenderShadersRootPath());
        });

    progressCallback(0.3f, "Initializing buffers...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            InitializeBuffersAndVAOs();
        });

    progressCallback(0.4f, "Loading cloud texture atlas...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            InitializeCloudTextures(resourceLocator);
        });

    progressCallback(0.5f, "Loading world textures...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            InitializeWorldTextures(resourceLocator);
        });

    progressCallback(0.6f, "Loading generic textures...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            InitializeGenericTextures(resourceLocator);
        });

    progressCallback(0.7f, "Loading explosion textures...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            InitializeExplosionTextures(resourceLocator);
        });

    progressCallback(0.8f, "Loading fonts...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            //
            // Initialize notification render context
            //

            mNotificationRenderContext = std::make_unique<NotificationRenderContext>(
                resourceLocator,
                *mShaderManager,
                *mGenericLinearTextureAtlasMetadata,                
                mRenderSettings.View.GetCanvasWidth(),
                mRenderSettings.View.GetCanvasHeight(),
                mRenderSettings.EffectiveAmbientLightIntensity);
        });

    progressCallback(0.9f, "Initializing graphics...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            //
            // Initialize global OpenGL settings
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
            // Update settings for initial values
            //

            ProcessSettingChanges(mRenderSettings);

            // TODOOLD
            OnOceanTransparencyUpdated();
            OnOceanDarkeningRateUpdated();
            OnOceanRenderParametersUpdated();
            OnOceanTextureIndexUpdated();
            OnLandRenderParametersUpdated();
            OnLandTextureIndexUpdated();
            // Ship
            OnFlatLampLightColorUpdated();
            OnDefaultWaterColorUpdated();
            OnWaterContrastUpdated();
            OnWaterLevelOfDetailUpdated();
            OnShowStressedSpringsUpdated();
            OnDrawHeatOverlayUpdated();
            OnHeatOverlayTransparencyUpdated();
            OnShipFlameSizeAdjustmentUpdated();


            //
            // Flush all pending operations
            //

            glFinish();
        });

    progressCallback(1.0f, "Initializing settings...");
}

RenderContext::~RenderContext()
{
    // Wait for an eventual pending render
    // (this destructor may only be invoked between two cycles,
    // hence knowing that there's no more render's is enough to ensure
    // nothing is using OpenGL at this moment)
    if (!!mLastRenderDrawCompletionIndicator)
    {
        mLastRenderDrawCompletionIndicator->Wait();
        mLastRenderDrawCompletionIndicator.reset();
    }
}

//////////////////////////////////////////////////////////////////////////////////

void RenderContext::RebindContext(std::function<void()> rebindContextFunction)
{
    mRenderThread.RunSynchronously(std::move(rebindContextFunction));
}

void RenderContext::Reset()
{
    // Clear ships
    mShips.clear();
}

void RenderContext::ValidateShipTexture(RgbaImageData const & texture) const
{
    // Check texture against max texture size
    if (texture.Size.Width > GameOpenGL::MaxTextureSize
        || texture.Size.Height > GameOpenGL::MaxTextureSize)
    {
        throw GameException("We are sorry, but this ship's texture image is too large for your graphics card.");
    }
}

void RenderContext::AddShip(
    ShipId shipId,
    size_t pointCount,
    RgbaImageData texture)
{
    //
    // Validate ship
    //

    ValidateShipTexture(texture);

    //
    // Add ship
    //

    assert(shipId == mShips.size());

    size_t const newShipCount = mShips.size() + 1;
    
    // Tell all ships
    for (auto const & ship : mShips)
    {
        ship->SetShipCount(newShipCount);
    }

    // Add the ship - synchronously
    mRenderThread.RunSynchronously(
        [&]()
        {
            mShips.emplace_back(
                new ShipRenderContext(
                    shipId,
                    pointCount,
                    newShipCount,
                    std::move(texture),
                    *mShaderManager,
                    *mExplosionTextureAtlasMetadata,
                    *mGenericLinearTextureAtlasMetadata,
                    *mGenericMipMappedTextureAtlasMetadata,
                    mRenderSettings,
                    // TODOOLD
                    CalculateLampLightColor(),
                    CalculateWaterColor(),
                    mWaterContrast,
                    mWaterLevelOfDetail,
                    mShowStressedSprings,
                    mDrawHeatOverlay,
                    mHeatOverlayTransparency,
                    mShipFlameSizeAdjustment));
        });
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

    int const canvasWidth = mRenderSettings.View.GetCanvasWidth();
    int const canvasHeight = mRenderSettings.View.GetCanvasHeight();

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

void RenderContext::UpdateStart()
{
    // If there's a pending RenderUploadEnd, wait for it so we
    // know that CPU buffers are safe to be used
    if (!!mLastRenderUploadEndCompletionIndicator)
    {
        auto const waitStart = GameChronometer::now();

        mLastRenderUploadEndCompletionIndicator->Wait();
        mLastRenderUploadEndCompletionIndicator.reset();

        mPerfStats.TotalWaitForRenderUploadDuration.Update(GameChronometer::now() - waitStart);
    }
}

void RenderContext::UpdateEnd()
{
    // Nop
}

///////////////////////////////////////////////////////////////////////////////////////////

void RenderContext::RenderStart()
{
    // Cleanup an eventual pending RenderUploadEnd - may be left behind if
    // this cycle did not do an Update
    mLastRenderUploadEndCompletionIndicator.reset();
}

void RenderContext::UploadStart()
{
    // Wait for an eventual pending RenderDraw, so that we know
    // GPU buffers are free to be used
    if (!!mLastRenderDrawCompletionIndicator)
    {
        auto const waitStart = GameChronometer::now();

        mLastRenderDrawCompletionIndicator->Wait();
        mLastRenderDrawCompletionIndicator.reset();

        mPerfStats.TotalWaitForRenderDrawDuration.Update(GameChronometer::now() - waitStart);
    }

    // Reset AM bomb pre-implosions, they are uploaded as needed
    mAMBombPreImplosionVertexBuffer.clear();

    // Reset crosses of light, they are uploaded as needed
    mCrossOfLightVertexBuffer.clear();

    // Reset HeatBlaster flame, it's uploaded as needed
    mHeatBlasterFlameShaderToRender.reset();

    // Reset fire extinguisher spray, it's uploaded as needed
    mFireExtinguisherSprayShaderToRender.reset();
}

void RenderContext::UploadStarsStart(size_t starCount)
{
    //
    // Stars are sticky: we upload them once in a while and
    // continue drawing the same buffer
    //

    mStarVertexBuffer.reset(starCount);
    mIsStarVertexBufferDirty = true;
}

void RenderContext::UploadStarsEnd()
{
    // Nop
}

void RenderContext::UploadLightningsStart(size_t lightningCount)
{
    //
    // Lightnings are not sticky: we upload them at each frame,
    // though they will be empty most of the time
    //

    mLightningVertexBuffer.reset_fill(6 * lightningCount);

    mBackgroundLightningVertexCount = 0;
    mForegroundLightningVertexCount = 0;
}

void RenderContext::UploadLightningsEnd()
{
    // If there has been a change, upload it now - asynchronously, on render thread - as
    // this buffer is used by mulitple render steps and thus we don't have a single user
    // who might upload it at the moment it's needed

    if (!mLightningVertexBuffer.empty())
    {
        mRenderThread.QueueTask(
            [this]()
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
            });
    }
}

void RenderContext::UploadCloudsStart(size_t cloudCount)
{
    //
    // Clouds are not sticky: we upload them at each frame
    //

    mCloudVertexBuffer.reset(6 * cloudCount);
}

void RenderContext::UploadCloudsEnd()
{
    // Nop
}

void RenderContext::UploadLandStart(size_t slices)
{
    //
    // Last segments are not sticky: we upload them at each frame
    //

    mLandSegmentBuffer.reset(slices + 1);
}

void RenderContext::UploadLandEnd()
{
    // Nop
}

void RenderContext::UploadOceanStart(size_t slices)
{
    //
    // Ocean segments are not sticky: we upload them at each frame
    //

    mOceanSegmentBuffer.reset(6 * slices + 1);
}

void RenderContext::UploadOceanEnd()
{
    // Nop
}

void RenderContext::UploadEnd()
{
    // Queue an indicator here, so we may wait for it
    // when we want to touch CPU buffers again
    assert(!mLastRenderUploadEndCompletionIndicator);
    mLastRenderUploadEndCompletionIndicator = mRenderThread.QueueSynchronizationPoint();
}

void RenderContext::Draw()
{
    assert(!mLastRenderDrawCompletionIndicator);

    // Render asynchronously; we will wait for this render to complete
    // when we want to touch GPU buffers again.
    //
    // Take a copy of the current render parameters and clean its dirtyness
    mLastRenderDrawCompletionIndicator = mRenderThread.QueueTask(
        [this, renderSettings = mRenderSettings.Snapshot()]() mutable
        {
            auto const startTime = GameChronometer::now();

            RenderStatistics renderStats;

            //
            // Initialize
            //

            // Process changes to settings
            ProcessSettingChanges(renderSettings);

            // Set polygon mode
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            // Clear canvas - and depth buffer
            vec3f const clearColor = renderSettings.FlatSkyColor.toVec3f() * renderSettings.EffectiveAmbientLightIntensity;
            glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Debug mode
            if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            //
            // World
            //

            RenderStars(renderSettings);

            RenderCloudsAndBackgroundLightnings(renderSettings);

            // Render ocean opaquely, over sky
            RenderOcean(true, renderSettings);

            glEnable(GL_DEPTH_TEST); // Required by ships

            for (auto const & ship : mShips)
            {
                ship->Draw(renderSettings, renderStats);
            }

            glDisable(GL_DEPTH_TEST);

            // Render ocean transparently, over ship, unless disabled
            if (!mShowShipThroughOcean)
            {
                RenderOcean(false, renderSettings);
            }

            //
            // Misc
            //

            RenderOceanFloor(renderSettings);

            RenderAMBombPreImplosions(renderSettings);

            RenderCrossesOfLight(renderSettings);

            RenderHeatBlasterFlame(renderSettings);

            RenderFireExtinguisherSpray(renderSettings);

            RenderForegroundLightnings(renderSettings);

            RenderRain(renderSettings);

            RenderWorldBorder(renderSettings);

            mNotificationRenderContext->Draw();

            // Flip the back buffer onto the screen
            mSwapRenderBuffersFunction();

            // Update stats
            mPerfStats.TotalRenderDrawDuration.Update(GameChronometer::now() - startTime);
            mRenderStats.store(renderStats);
        });
}

void RenderContext::RenderEnd()
{
    // Nop
}

////////////////////////////////////////////////////////////////////////////////////

void RenderContext::InitializeBuffersAndVAOs()
{
    GLuint tmpGLuint;

    //
    // Initialize buffers
    //

    GLuint vbos[11];
    glGenBuffers(11, vbos);
    mStarVBO = vbos[0];
    mLightningVBO = vbos[1];
    mCloudVBO = vbos[2];
    mLandSegmentVBO = vbos[3];
    mOceanSegmentVBO = vbos[4];
    mAMBombPreImplosionVBO = vbos[5];
    mCrossOfLightVBO = vbos[6];
    mHeatBlasterFlameVBO = vbos[7];
    mFireExtinguisherSprayVBO = vbos[8];
    mRainVBO = vbos[9];
    mWorldBorderVBO = vbos[10];


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
    glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Cloud1));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Cloud1), 4, GL_FLOAT, GL_FALSE, sizeof(CloudVertex), (void *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Cloud2));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Cloud2), 1, GL_FLOAT, GL_FALSE, sizeof(CloudVertex), (void *)(4 * sizeof(float)));
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
    // Initialize HeatBlaster flame VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mHeatBlasterFlameVAO = tmpGLuint;

    glBindVertexArray(*mHeatBlasterFlameVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mHeatBlasterFlameVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::HeatBlasterFlame));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::HeatBlasterFlame), 4, GL_FLOAT, GL_FALSE, sizeof(HeatBlasterFlameVertex), (void *)0);
    CheckOpenGLError();

    glBindVertexArray(0);


    //
    // Initialize Fire Extinguisher Spray VAO
    //

    glGenVertexArrays(1, &tmpGLuint);
    mFireExtinguisherSprayVAO = tmpGLuint;

    glBindVertexArray(*mFireExtinguisherSprayVAO);
    CheckOpenGLError();

    // Describe vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, *mFireExtinguisherSprayVBO);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::FireExtinguisherSpray));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::FireExtinguisherSpray), 4, GL_FLOAT, GL_FALSE, sizeof(FireExtinguisherSprayVertex), (void *)0);
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
}

void RenderContext::InitializeCloudTextures(ResourceLocator const & resourceLocator)
{
    // Load texture database
    auto cloudTextureDatabase = TextureDatabase<Render::CloudTextureDatabaseTraits>::Load(
        resourceLocator.GetTexturesRootFolderPath());

    // Create atlas
    auto cloudTextureAtlas = TextureAtlasBuilder<CloudTextureGroups>::BuildAtlas(
        cloudTextureDatabase,
        AtlasOptions::None,
        [](float, std::string const &) {});

    LogMessage("Cloud texture atlas size: ", cloudTextureAtlas.AtlasData.Size.ToString());

    mShaderManager->ActivateTexture<ProgramParameterType::CloudsAtlasTexture>();

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
    mShaderManager->ActivateProgram<ProgramType::Clouds>();
    mShaderManager->SetTextureParameters<ProgramType::Clouds>();
}

void RenderContext::InitializeWorldTextures(ResourceLocator const & resourceLocator)
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

void RenderContext::InitializeGenericTextures(ResourceLocator const & resourceLocator)
{
    //
    // Create generic linear texture atlas
    //

    // Load texture database
    auto genericLinearTextureDatabase = TextureDatabase<Render::GenericLinearTextureTextureDatabaseTraits>::Load(
        resourceLocator.GetTexturesRootFolderPath());

    // Create atlas
    auto genericLinearTextureAtlas = TextureAtlasBuilder<GenericLinearTextureGroups>::BuildAtlas(
        genericLinearTextureDatabase,
        AtlasOptions::None,
        [](float, std::string const &) {});

    LogMessage("Generic linear texture atlas size: ", genericLinearTextureAtlas.AtlasData.Size.ToString());

    // Activate texture
    mShaderManager->ActivateTexture<ProgramParameterType::GenericLinearTexturesAtlasTexture>();

    // Create texture OpenGL handle
    GLuint tmpGLuint;
    glGenTextures(1, &tmpGLuint);
    mGenericLinearTextureAtlasOpenGLHandle = tmpGLuint;

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mGenericLinearTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    GameOpenGL::UploadTexture(std::move(genericLinearTextureAtlas.AtlasData));

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckOpenGLError();

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Store metadata
    mGenericLinearTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata<GenericLinearTextureGroups>>(
        genericLinearTextureAtlas.Metadata);

    // Set FlamesBackground1 shader parameters
    auto const & fireAtlasFrameMetadata = mGenericLinearTextureAtlasMetadata->GetFrameMetadata(GenericLinearTextureGroups::Fire, 0);
    mShaderManager->ActivateProgram<ProgramType::ShipFlamesBackground1>();
    mShaderManager->SetTextureParameters<ProgramType::ShipFlamesBackground1>();
    mShaderManager->SetProgramParameter<ProgramType::ShipFlamesBackground1, ProgramParameterType::AtlasTile1Dx>(
        1.0f / static_cast<float>(fireAtlasFrameMetadata.FrameMetadata.Size.Width),
        1.0f / static_cast<float>(fireAtlasFrameMetadata.FrameMetadata.Size.Height));
    mShaderManager->SetProgramParameter<ProgramType::ShipFlamesBackground1, ProgramParameterType::AtlasTile1LeftBottomTextureCoordinates>(
        fireAtlasFrameMetadata.TextureCoordinatesBottomLeft.x,
        fireAtlasFrameMetadata.TextureCoordinatesBottomLeft.y);
    mShaderManager->SetProgramParameter<ProgramType::ShipFlamesBackground1, ProgramParameterType::AtlasTile1Size>(
        fireAtlasFrameMetadata.TextureSpaceWidth,
        fireAtlasFrameMetadata.TextureSpaceHeight);

    // Set FlamesForeground1 shader parameters
    mShaderManager->ActivateProgram<ProgramType::ShipFlamesForeground1>();
    mShaderManager->SetTextureParameters<ProgramType::ShipFlamesForeground1>();
    mShaderManager->SetProgramParameter<ProgramType::ShipFlamesForeground1, ProgramParameterType::AtlasTile1Dx>(
        1.0f / static_cast<float>(fireAtlasFrameMetadata.FrameMetadata.Size.Width),
        1.0f / static_cast<float>(fireAtlasFrameMetadata.FrameMetadata.Size.Height));
    mShaderManager->SetProgramParameter<ProgramType::ShipFlamesForeground1, ProgramParameterType::AtlasTile1LeftBottomTextureCoordinates>(
        fireAtlasFrameMetadata.TextureCoordinatesBottomLeft.x,
        fireAtlasFrameMetadata.TextureCoordinatesBottomLeft.y);
    mShaderManager->SetProgramParameter<ProgramType::ShipFlamesForeground1, ProgramParameterType::AtlasTile1Size>(
        fireAtlasFrameMetadata.TextureSpaceWidth,
        fireAtlasFrameMetadata.TextureSpaceHeight);

    // Set WorldBorder shader parameters
    auto const & worldBorderAtlasFrameMetadata = mGenericLinearTextureAtlasMetadata->GetFrameMetadata(GenericLinearTextureGroups::WorldBorder, 0);
    mShaderManager->ActivateProgram<ProgramType::WorldBorder>();
    mShaderManager->SetTextureParameters<ProgramType::WorldBorder>();
    mShaderManager->SetProgramParameter<ProgramType::WorldBorder, ProgramParameterType::AtlasTile1Dx>(
        1.0f / static_cast<float>(worldBorderAtlasFrameMetadata.FrameMetadata.Size.Width),
        1.0f / static_cast<float>(worldBorderAtlasFrameMetadata.FrameMetadata.Size.Height));
    mShaderManager->SetProgramParameter<ProgramType::WorldBorder, ProgramParameterType::AtlasTile1LeftBottomTextureCoordinates>(
        worldBorderAtlasFrameMetadata.TextureCoordinatesBottomLeft.x,
        worldBorderAtlasFrameMetadata.TextureCoordinatesBottomLeft.y);
    mShaderManager->SetProgramParameter<ProgramType::WorldBorder, ProgramParameterType::AtlasTile1Size>(
        worldBorderAtlasFrameMetadata.TextureSpaceWidth,
        worldBorderAtlasFrameMetadata.TextureSpaceHeight);


    //
    // Create generic mipmapped texture atlas
    //

    // Load texture database
    auto genericMipMappedTextureDatabase = TextureDatabase<Render::GenericMipMappedTextureTextureDatabaseTraits>::Load(
        resourceLocator.GetTexturesRootFolderPath());

    // Create atlas
    auto genericMipMappedTextureAtlas = TextureAtlasBuilder<GenericMipMappedTextureGroups>::BuildAtlas(
        genericMipMappedTextureDatabase,
        AtlasOptions::None,
        [](float, std::string const &) {});

    LogMessage("Generic mipmapped texture atlas size: ", genericMipMappedTextureAtlas.AtlasData.Size.ToString());

    // Activate texture
    mShaderManager->ActivateTexture<ProgramParameterType::GenericMipMappedTexturesAtlasTexture>();

    // Create texture OpenGL handle
    glGenTextures(1, &tmpGLuint);
    mGenericMipMappedTextureAtlasOpenGLHandle = tmpGLuint;

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mGenericMipMappedTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    GameOpenGL::UploadMipmappedPowerOfTwoTexture(
        std::move(genericMipMappedTextureAtlas.AtlasData),
        genericMipMappedTextureAtlas.Metadata.GetMaxDimension());

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckOpenGLError();

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Store metadata
    mGenericMipMappedTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata<GenericMipMappedTextureGroups>>(genericMipMappedTextureAtlas.Metadata);

    // Set texture in shaders
    mShaderManager->ActivateProgram<ProgramType::ShipGenericMipMappedTextures>();
    mShaderManager->SetTextureParameters<ProgramType::ShipGenericMipMappedTextures>();


    //
    // Initialize noise textures
    //

    // Load texture database
    auto noiseTextureDatabase = TextureDatabase<Render::NoiseTextureDatabaseTraits>::Load(
        resourceLocator.GetTexturesRootFolderPath());

    // Noise 1

    mShaderManager->ActivateTexture<ProgramParameterType::NoiseTexture1>();

    mUploadedNoiseTexturesManager.UploadNextFrame(
        noiseTextureDatabase.GetGroup(NoiseTextureGroups::Noise),
        0,
        GL_LINEAR);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, mUploadedNoiseTexturesManager.GetOpenGLHandle(NoiseTextureGroups::Noise, 0));
    CheckOpenGLError();

    // Set noise texture in shaders
    mShaderManager->ActivateProgram<ProgramType::ShipFlamesBackground1>();
    mShaderManager->SetTextureParameters<ProgramType::ShipFlamesBackground1>();
    mShaderManager->ActivateProgram<ProgramType::ShipFlamesBackground2>();
    mShaderManager->SetTextureParameters<ProgramType::ShipFlamesBackground2>();
    mShaderManager->ActivateProgram<ProgramType::ShipFlamesBackground3>();
    mShaderManager->SetTextureParameters<ProgramType::ShipFlamesBackground3>();
    mShaderManager->ActivateProgram<ProgramType::ShipFlamesForeground1>();
    mShaderManager->SetTextureParameters<ProgramType::ShipFlamesForeground1>();
    mShaderManager->ActivateProgram<ProgramType::ShipFlamesForeground2>();
    mShaderManager->SetTextureParameters<ProgramType::ShipFlamesForeground2>();
    mShaderManager->ActivateProgram<ProgramType::ShipFlamesForeground3>();
    mShaderManager->SetTextureParameters<ProgramType::ShipFlamesForeground3>();

    // Noise 2

    mShaderManager->ActivateTexture<ProgramParameterType::NoiseTexture2>();

    mUploadedNoiseTexturesManager.UploadNextFrame(
        noiseTextureDatabase.GetGroup(NoiseTextureGroups::Noise),
        1,
        GL_LINEAR);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, mUploadedNoiseTexturesManager.GetOpenGLHandle(NoiseTextureGroups::Noise, 1));
    CheckOpenGLError();

    // Set noise texture in shaders
    mShaderManager->ActivateProgram<ProgramType::HeatBlasterFlameCool>();
    mShaderManager->SetTextureParameters<ProgramType::HeatBlasterFlameCool>();
    mShaderManager->ActivateProgram<ProgramType::HeatBlasterFlameHeat>();
    mShaderManager->SetTextureParameters<ProgramType::HeatBlasterFlameHeat>();
    mShaderManager->ActivateProgram<ProgramType::FireExtinguisherSpray>();
    mShaderManager->SetTextureParameters<ProgramType::FireExtinguisherSpray>();
    mShaderManager->ActivateProgram<ProgramType::Lightning>();
    mShaderManager->SetTextureParameters<ProgramType::Lightning>();
}

void RenderContext::InitializeExplosionTextures(ResourceLocator const & resourceLocator)
{
    // Load atlas
    TextureAtlas<ExplosionTextureGroups> explosionTextureAtlas = TextureAtlas<ExplosionTextureGroups>::Deserialize(
        ExplosionTextureDatabaseTraits::DatabaseName,
        resourceLocator.GetTexturesRootFolderPath());

    LogMessage("Explosion texture atlas size: ", explosionTextureAtlas.AtlasData.Size.ToString());

    // Activate texture
    mShaderManager->ActivateTexture<ProgramParameterType::ExplosionsAtlasTexture>();

    // Create OpenGL handle
    GLuint tmpGLuint;
    glGenTextures(1, &tmpGLuint);
    mExplosionTextureAtlasOpenGLHandle = tmpGLuint;

    // Bind texture atlas
    glBindTexture(GL_TEXTURE_2D, *mExplosionTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    GameOpenGL::UploadTexture(std::move(explosionTextureAtlas.AtlasData));

    // Set repeat mode - we want to clamp, to leverage the fact that
    // all frames are perfectly transparent at the edges
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckOpenGLError();

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Store metadata
    mExplosionTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata<ExplosionTextureGroups>>(explosionTextureAtlas.Metadata);

    // Set texture in shaders
    mShaderManager->ActivateProgram<ProgramType::ShipExplosions>();
    mShaderManager->SetTextureParameters<ProgramType::ShipExplosions>();
}

void RenderContext::RenderStars(RenderSettings const & /*renderSettings*/)
{
    //
    // Buffer
    //

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

        mIsStarVertexBufferDirty = false;

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    //
    // Render
    //

    if (mStarVertexBuffer.size() > 0)
    {
        glBindVertexArray(*mStarVAO);

        mShaderManager->ActivateProgram<ProgramType::Stars>();

        glPointSize(0.5f);

        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(mStarVertexBuffer.size()));
        CheckOpenGLError();

        glBindVertexArray(0);
    }
}

void RenderContext::RenderCloudsAndBackgroundLightnings(RenderSettings const & renderSettings)
{
    ////////////////////////////////////////////////////
    // Clouds buffer
    ////////////////////////////////////////////////////

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

        mShaderManager->ActivateProgram<ProgramType::Clouds>();

        if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
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

        mShaderManager->ActivateProgram<ProgramType::Lightning>();

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

        mShaderManager->ActivateProgram<ProgramType::Clouds>();

        if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
            glLineWidth(0.1f);

        glDrawArrays(GL_TRIANGLES, cloudsOverLightningVertexStart, static_cast<GLsizei>(mCloudVertexBuffer.size()) - cloudsOverLightningVertexStart);
        CheckOpenGLError();
    }

    ////////////////////////////////////////////////////

    glBindVertexArray(0);
}

void RenderContext::RenderOcean(bool opaquely, RenderSettings const & renderSettings)
{
    //
    // Buffer
    //

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

    //
    // Render
    //

    float const transparency = opaquely ? 0.0f : renderSettings.OceanTransparency;

    glBindVertexArray(*mOceanVAO);

    switch (mOceanRenderMode)
    {
        case OceanRenderModeType::Depth:
        {
            mShaderManager->ActivateProgram<ProgramType::OceanDepth>();
            mShaderManager->SetProgramParameter<ProgramType::OceanDepth, ProgramParameterType::OceanTransparency>(
                transparency);

            break;
        }

        case OceanRenderModeType::Flat:
        {
            mShaderManager->ActivateProgram<ProgramType::OceanFlat>();
            mShaderManager->SetProgramParameter<ProgramType::OceanFlat, ProgramParameterType::OceanTransparency>(
                transparency);

            break;
        }

        case OceanRenderModeType::Texture:
        {
            mShaderManager->ActivateProgram<ProgramType::OceanTexture>();
            mShaderManager->SetProgramParameter<ProgramType::OceanTexture, ProgramParameterType::OceanTransparency>(
                transparency);

            break;
        }
    }

    if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
        glLineWidth(0.1f);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mOceanSegmentBuffer.size()));

    glBindVertexArray(0);
}

void RenderContext::RenderOceanFloor(RenderSettings const & renderSettings)
{
    //
    // Buffer
    //

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

    //
    // Render
    //

    glBindVertexArray(*mLandVAO);

    switch (mLandRenderMode)
    {
        case LandRenderModeType::Flat:
        {
            mShaderManager->ActivateProgram<ProgramType::LandFlat>();
            break;
        }

        case LandRenderModeType::Texture:
        {
            mShaderManager->ActivateProgram<ProgramType::LandTexture>();
            break;
        }
    }

    if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
        glLineWidth(0.1f);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mLandSegmentBuffer.size()));

    glBindVertexArray(0);
}

void RenderContext::RenderAMBombPreImplosions(RenderSettings const & /*renderSettings*/)
{
    if (!mAMBombPreImplosionVertexBuffer.empty())
    {
        //
        // Buffer
        //

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


        //
        // Render
        //

        glBindVertexArray(*mAMBombPreImplosionVAO);

        mShaderManager->ActivateProgram<ProgramType::AMBombPreImplosion>();

        assert((mAMBombPreImplosionVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mAMBombPreImplosionVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void RenderContext::RenderCrossesOfLight(RenderSettings const & /*renderSettings*/)
{
    if (!mCrossOfLightVertexBuffer.empty())
    {
        //
        // Buffer
        //

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


        //
        // Render
        //

        glBindVertexArray(*mCrossOfLightVAO);

        mShaderManager->ActivateProgram<ProgramType::CrossOfLight>();

        assert((mCrossOfLightVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mCrossOfLightVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void RenderContext::RenderHeatBlasterFlame(RenderSettings const & /*renderSettings*/)
{
    if (!!mHeatBlasterFlameShaderToRender)
    {
        //
        // Buffer
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
}

void RenderContext::RenderFireExtinguisherSpray(RenderSettings const & /*renderSettings*/)
{
    if (!!mFireExtinguisherSprayShaderToRender)
    {
        //
        // Buffer
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

        // Draw
        assert((mFireExtinguisherSprayVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mFireExtinguisherSprayVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void RenderContext::RenderForegroundLightnings(RenderSettings const & /*renderSettings*/)
{
    if (mForegroundLightningVertexCount > 0)
    {
        glBindVertexArray(*mLightningVAO);

        mShaderManager->ActivateProgram<ProgramType::Lightning>();

        glDrawArrays(GL_TRIANGLES,
            static_cast<GLsizei>(mLightningVertexBuffer.size() - mForegroundLightningVertexCount),
            static_cast<GLsizei>(mForegroundLightningVertexCount));
        CheckOpenGLError();

        glBindVertexArray(0);
    }
}

void RenderContext::RenderRain(RenderSettings const & /*renderSettings*/)
{
    if (mIsRainDensityDirty)
    {
        // Set parameter
        mShaderManager->ActivateProgram<ProgramType::Rain>();
        mShaderManager->SetProgramParameter<ProgramType::Rain, ProgramParameterType::RainDensity>(
            mRainDensity);

        mIsRainDensityDirty = false;
    }

    if (mRainDensity != 0.0f)
    {
        glBindVertexArray(*mRainVAO);

        mShaderManager->ActivateProgram(ProgramType::Rain);

        // Set time parameter
        mShaderManager->SetProgramParameter<ProgramParameterType::Time>(
            ProgramType::Rain,
            GameWallClock::GetInstance().NowAsFloat());

        // Draw
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindVertexArray(0);
    }
}

void RenderContext::RenderWorldBorder(RenderSettings const & /*renderSettings*/)
{
    if (mWorldBorderVertexBuffer.size() > 0)
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

void RenderContext::ProcessSettingChanges(RenderSettings const & renderSettings)
{
    if (renderSettings.IsViewDirty)
    {
        ApplyViewModelChanges(renderSettings);
    }

    if (renderSettings.IsCanvasSizeDirty)
    {
        ApplyCanvasSizeChanges(renderSettings);
    }

    if (renderSettings.IsEffectiveAmbientLightIntensityDirty)
    {
        ApplyEffectiveAmbientLightIntensityChanges(renderSettings);
    }
}

void RenderContext::ApplyViewModelChanges(RenderSettings const & renderSettings)
{
    //
    // Update ortho matrix in all programs
    //

    constexpr float ZFar = 1000.0f;
    constexpr float ZNear = 1.0f;

    ViewModel::ProjectionMatrix globalOrthoMatrix;
    renderSettings.View.CalculateGlobalOrthoMatrix(ZFar, ZNear, globalOrthoMatrix);

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

    mShaderManager->ActivateProgram<ProgramType::AMBombPreImplosion>();
    mShaderManager->SetProgramParameter<ProgramType::AMBombPreImplosion, ProgramParameterType::OrthoMatrix>(
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
    // Recalculate world border
    //

    RecalculateWorldBorder(renderSettings);
}

void RenderContext::ApplyCanvasSizeChanges(RenderSettings const & renderSettings)
{
    // Set shader parameters
    mShaderManager->ActivateProgram<ProgramType::CrossOfLight>();
    mShaderManager->SetProgramParameter<ProgramType::CrossOfLight, ProgramParameterType::ViewportSize>(
        static_cast<float>(renderSettings.View.GetCanvasWidth()),
        static_cast<float>(renderSettings.View.GetCanvasHeight()));

    // Set viewport
    glViewport(0, 0, renderSettings.View.GetCanvasWidth(), renderSettings.View.GetCanvasHeight());

    // Propagate
    mNotificationRenderContext->UpdateCanvasSize(renderSettings.View.GetCanvasWidth(), renderSettings.View.GetCanvasHeight());
}

void RenderContext::ApplyEffectiveAmbientLightIntensityChanges(RenderSettings const & renderSettings)
{
    // Set parameters in all programs

    mShaderManager->ActivateProgram<ProgramType::Stars>();
    mShaderManager->SetProgramParameter<ProgramType::Stars, ProgramParameterType::StarTransparency>(
        pow(std::max(0.0f, 1.0f - renderSettings.EffectiveAmbientLightIntensity), 3.0f));

    mShaderManager->ActivateProgram<ProgramType::Clouds>();
    mShaderManager->SetProgramParameter<ProgramType::Clouds, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::Lightning>();
    mShaderManager->SetProgramParameter<ProgramType::Lightning, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::LandFlat>();
    mShaderManager->SetProgramParameter<ProgramType::LandFlat, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::LandTexture>();
    mShaderManager->SetProgramParameter<ProgramType::LandTexture, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::OceanDepth>();
    mShaderManager->SetProgramParameter<ProgramType::OceanDepth, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::OceanFlat>();
    mShaderManager->SetProgramParameter<ProgramType::OceanFlat, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::OceanTexture>();
    mShaderManager->SetProgramParameter<ProgramType::OceanTexture, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::Rain>();
    mShaderManager->SetProgramParameter<ProgramType::Rain, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::WorldBorder>();
    mShaderManager->SetProgramParameter<ProgramType::WorldBorder, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    // Update notification context
    mNotificationRenderContext->UpdateEffectiveAmbientLightIntensity(renderSettings.EffectiveAmbientLightIntensity);
}

// TODOHERE

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

        // Destroy previous texture
        mOceanTextureOpenGLHandle.reset();

        // Clamp the texture index
        mLoadedOceanTextureIndex = std::min(mSelectedOceanTextureIndex, mOceanTextureFrameSpecifications.size() - 1);

        // Load texture image
        auto oceanTextureFrame = mOceanTextureFrameSpecifications[mLoadedOceanTextureIndex].LoadFrame();

        // Activate texture
        mShaderManager->ActivateTexture<ProgramParameterType::OceanTexture>();

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

        // Destroy previous texture
        mLandTextureOpenGLHandle.reset();

        // Clamp the texture index
        mLoadedLandTextureIndex = std::min(mSelectedLandTextureIndex, mLandTextureFrameSpecifications.size() - 1);

        // Load texture image
        auto landTextureFrame = mLandTextureFrameSpecifications[mLoadedLandTextureIndex].LoadFrame();

        // Activate texture
        mShaderManager->ActivateTexture<ProgramParameterType::LandTexture>();

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
        mShaderManager->ActivateProgram<ProgramType::LandTexture>();
        mShaderManager->SetProgramParameter<ProgramType::LandTexture, ProgramParameterType::TextureScaling>(
            1.0f / landTextureFrame.Metadata.WorldWidth,
            1.0f / landTextureFrame.Metadata.WorldHeight);
        mShaderManager->SetTextureParameters<ProgramType::LandTexture>();
    }
}

void RenderContext::OnFlatLampLightColorUpdated()
{
    // Calculate new lamp light color
    auto const lampLightColor = CalculateLampLightColor();

    // Set parameter in all ships
    for (auto & s : mShips)
    {
        s->SetLampLightColor(lampLightColor);
    }
}

void RenderContext::OnDefaultWaterColorUpdated()
{
    // Calculate new water color
    auto const waterColor = CalculateWaterColor();

    // Set parameter in all ships
    for (auto & s : mShips)
    {
        s->SetWaterColor(waterColor);
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

void RenderContext::OnShipFlameSizeAdjustmentUpdated()
{
    // Set parameter in all ships
    for (auto & s : mShips)
    {
        s->SetShipFlameSizeAdjustment(mShipFlameSizeAdjustment);
    }
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

void RenderContext::RecalculateWorldBorder(RenderSettings const & renderSettings)
{
    auto const & viewModel = renderSettings.View;

    ImageSize const & worldBorderTextureSize =
        mGenericLinearTextureAtlasMetadata->GetFrameMetadata(GenericLinearTextureGroups::WorldBorder, 0)
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
    if (-GameParameters::HalfMaxWorldWidth + worldBorderWorldWidth >= viewModel.GetVisibleWorldTopLeft().x)
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
    if (GameParameters::HalfMaxWorldWidth - worldBorderWorldWidth <= viewModel.GetVisibleWorldBottomRight().x)
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
    if (GameParameters::HalfMaxWorldHeight - worldBorderWorldHeight <= viewModel.GetVisibleWorldTopLeft().y)
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
    if (-GameParameters::HalfMaxWorldHeight + worldBorderWorldHeight >= viewModel.GetVisibleWorldBottomRight().y)
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

float RenderContext::CalculateEffectiveAmbientLightIntensity(RenderSettings const & renderSettings) const
{
    return renderSettings.AmbientLightIntensity * mStormAmbientDarkening;
}

vec4f RenderContext::CalculateLampLightColor() const
{
    return mFlatLampLightColor.toVec4f(1.0f);
}

vec4f RenderContext::CalculateWaterColor() const
{
    switch (mOceanRenderMode)
    {
        case OceanRenderModeType::Depth:
        {
            return
                (mDepthOceanColorStart.toVec4f(1.0f) + mDepthOceanColorEnd.toVec4f(1.0f))
                / 2.0f;
        }

        case OceanRenderModeType::Flat:
        {
            return mFlatOceanColor.toVec4f(1.0f);
        }

        default:
        {
            assert(mOceanRenderMode == OceanRenderModeType::Texture); // Darn VS - warns

            return mDefaultWaterColor.toVec4f(1.0f);
        }
    }
}

}