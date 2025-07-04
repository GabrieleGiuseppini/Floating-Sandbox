/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderContext.h"

#include <Core/GameChronometer.h>
#include <Core/GameException.h>
#include <Core/Log.h>
#include <Core/SysSpecifics.h>
#include <Core/ThreadManager.h>

#include <cstring>

namespace /*anonymous*/ {

    bool CalculateDoInvokeGlFinish(std::optional<bool> doForceNoGlFinish)
    {
        if (doForceNoGlFinish.has_value())
        {
            // Use override as-is
            return !(*doForceNoGlFinish);
        }
        else
        {
            return !GameOpenGL::AvoidGlFinish;
        }
    }
}

RenderContext::RenderContext(
    RenderDeviceProperties const & renderDeviceProperties,
    FloatSize const & maxWorldSize,
    TextureAtlas<GameTextureDatabases::NpcTextureDatabase> && npcTextureAtlas,
    PerfStats & perfStats,
    ThreadManager & threadManager,
    IAssetManager const & assetManager,
    ProgressCallback const & progressCallback)
    : mDoInvokeGlFinish(false) // Will be recalculated
    // Thread
    , mRenderThread(ThreadManager::ThreadTaskKind::Render, "FS RenderThread", 0, threadManager.IsRenderingMultiThreaded(), threadManager)
    , mLastRenderUploadEndCompletionIndicator()
    , mLastRenderDrawCompletionIndicator()
    // Shader manager
    , mShaderManager()
    // Child contextes
    , mGlobalRenderContext()
    , mWorldRenderContext()
    , mShips()
    , mNotificationRenderContext()
    // Non-render parameters
    , mAmbientLightIntensity(1.0f)
    , mMoonlightColor(0x17, 0x3d, 0x5b)
    , mDoMoonlight(true)
    , mShipFlameSizeAdjustment(1.0f)
    , mShipDefaultWaterColor(0x00, 0x00, 0xcc)
    , mVectorFieldRenderMode(VectorFieldRenderModeType::None)
    , mVectorFieldLengthMultiplier(1.0f)
    // Render state
    , mLampToolToSet(vec4f(0.0f, 0.0f, 0.0f, 0.0f)) // Turned off
    // Rendering externals
    , mMakeRenderContextCurrentFunction(renderDeviceProperties.MakeRenderContextCurrentFunction)
    , mSwapRenderBuffersFunction(renderDeviceProperties.SwapRenderBuffersFunction)
    // Render parameters
    , mRenderParameters(
        maxWorldSize,
        renderDeviceProperties.InitialCanvasSize,
        renderDeviceProperties.LogicalToPhysicalDisplayFactor)
    // State
    , mWindSpeedMagnitudeRunningAverage(0.0f)
    , mCurrentWindSpeedMagnitude(0.0f)
    // Statistics
    , mPerfStats(perfStats)
    , mRenderStats()
{
    progressCallback(0.0f, ProgressMessageType::InitializingOpenGL);

    mRenderThread.RunSynchronously(
        [&, doForceNoGlFinish = renderDeviceProperties.DoForceNoGlFinish]()
        {
            //
            // Initialize OpenGL
            //

            // Make render context current - invoke from this thread
            mMakeRenderContextCurrentFunction();

            // Initialize OpenGL
            GameOpenGL::InitOpenGL();

            mDoInvokeGlFinish = CalculateDoInvokeGlFinish(doForceNoGlFinish);
            LogMessage("RenderContext: DoInvokeGlFinish=", mDoInvokeGlFinish);

            // Initialize the shared texture unit once and for all
            mShaderManager->ActivateTexture<GameShaderSets::ProgramParameterKind::SharedTexture>();
            glEnable(GL_TEXTURE_1D);
            glEnable(GL_TEXTURE_2D);

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

            // Set depth test parameters for when we'll need them
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LEQUAL);

            // Enable point sprite
            // (https://community.khronos.org/t/understanding-gl-pointcoord-always-0/70368/9)
            glEnable(0x8861); // GL_POINT_SPRITE
            glGetError(); // Eat error code just in case
        });

    progressCallback(0.05f, ProgressMessageType::LoadingShaders);

    mRenderThread.RunSynchronously(
        [&]()
        {
            //
            // Load shader manager
            //

            LogMessage("Initializing shaders...");

            mShaderManager = ShaderManager<GameShaderSets::ShaderSet>::CreateInstance(assetManager, SimpleProgressCallback::Dummy());

            LogMessage("...shaders initialized.");
        });

    progressCallback(0.1f, ProgressMessageType::InitializingNoise);

    mRenderThread.RunSynchronously(
        [&]()
        {
            mGlobalRenderContext = std::make_unique<GlobalRenderContext>(assetManager , *mShaderManager);

            mGlobalRenderContext->InitializeNoiseTextures();
        });

    progressCallback(0.15f, ProgressMessageType::LoadingGenericTextures);

    mRenderThread.RunSynchronously(
        [&]()
        {
            mGlobalRenderContext->InitializeGenericTextures();
        });

    progressCallback(0.2f, ProgressMessageType::LoadingExplosionTextureAtlas);

    mRenderThread.RunSynchronously(
        [&]()
        {
            mGlobalRenderContext->InitializeExplosionTextures();
        });

    mRenderThread.RunSynchronously(
        [&]()
        {
            mGlobalRenderContext->InitializeNpcTextures(std::move(npcTextureAtlas)); // Safe as it's synchronous
        });

    mRenderThread.RunSynchronously(
        [&]()
        {
            mWorldRenderContext = std::make_unique<WorldRenderContext>(
                assetManager,
                *mShaderManager,
                *mGlobalRenderContext);
        });

    progressCallback(0.45f, ProgressMessageType::LoadingCloudTextureAtlas);

    mRenderThread.RunSynchronously(
        [&]()
        {
            mWorldRenderContext->InitializeCloudTextures();
        });

    progressCallback(0.65f, ProgressMessageType::LoadingFishTextureAtlas);

    mRenderThread.RunSynchronously(
        [&]()
        {
            mWorldRenderContext->InitializeFishTextures();
        });

    progressCallback(0.7f, ProgressMessageType::LoadingWorldTextures);

    mRenderThread.RunSynchronously(
        [&]()
        {
            mWorldRenderContext->InitializeWorldTextures();
        });

    progressCallback(0.8f, ProgressMessageType::LoadingFonts);

    mRenderThread.RunSynchronously(
        [&]()
        {
            //
            // Initialize notification render context
            //

            mNotificationRenderContext = std::make_unique<NotificationRenderContext>(
                assetManager,
                *mShaderManager,
                *mGlobalRenderContext);
        });

    progressCallback(0.9f, ProgressMessageType::InitializingGraphics);

    mRenderThread.RunSynchronously(
        [&]()
        {
            //
            // Set initial values of non-render parameters from which
            // other parameters are calculated
            //

            SetAmbientLightIntensity(mAmbientLightIntensity);
            SetMoonlightColor(mMoonlightColor);
            SetDoMoonlight(mDoMoonlight);
            SetShipDefaultWaterColor(mShipDefaultWaterColor);


            //
            // Update parameters for initial values
            //

            {
                auto const initialRenderParameters = mRenderParameters.TakeSnapshotAndClear();

                ProcessParameterChanges(initialRenderParameters);

                mGlobalRenderContext->ProcessParameterChanges(initialRenderParameters);

                mWorldRenderContext->ProcessParameterChanges(initialRenderParameters);

                mNotificationRenderContext->ProcessParameterChanges(initialRenderParameters);
            }


            if (mDoInvokeGlFinish)
            {
                //
                // Flush all pending operations
                //

                glFinish();
            }
        });

    progressCallback(1.0f, ProgressMessageType::InitializingGraphics);
}

RenderContext::~RenderContext()
{
    LogMessage("RenderContext::~RenderContext()");

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

void RenderContext::RebindContext()
{
    mRenderThread.RunSynchronously(
        [&]()
        {
            mMakeRenderContextCurrentFunction();
        });
}

void RenderContext::Reset()
{
    // Ship's destructors do OpenGL cleanups, hence we
    // want to clear the vector on the rendering thread
    // (synchronously)
    mRenderThread.RunSynchronously(
        [&]()
        {
            // Clear ships
            mShips.clear();

            // Notify other layers
            mWorldRenderContext->OnReset(mRenderParameters);
        });

    // Reset state
    mWindSpeedMagnitudeRunningAverage.Reset(0.0f);
}

void RenderContext::ValidateShipTexture(RgbaImageData const & texture) const
{
    // Check texture against max texture size
    if (texture.Size.width > GameOpenGL::MaxTextureSize
        || texture.Size.height > GameOpenGL::MaxTextureSize)
    {
        throw GameException("We are sorry, but this ship's texture image is too large for your graphics card. The texture size is " +
            texture.Size.ToString() + " while the maximum supported by your graphics cards is " + ImageSize(GameOpenGL::MaxTextureSize, GameOpenGL::MaxTextureSize).ToString());
    }
}

void RenderContext::AddShip(
    ShipId shipId,
    size_t pointCount,
    size_t maxEphemeralParticles,
    size_t maxSpringsPerPoint,
    RgbaImageData exteriorTextureImage,
    RgbaImageData interiorViewImage)
{
    //
    // Validate ship
    //

    ValidateShipTexture(exteriorTextureImage);
    ValidateShipTexture(interiorViewImage);

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
                    maxEphemeralParticles,
                    maxSpringsPerPoint,
                    std::move(exteriorTextureImage),
                    std::move(interiorViewImage),
                    *mShaderManager,
                    *mGlobalRenderContext,
                    mRenderParameters,
                    mShipFlameSizeAdjustment,
                    mVectorFieldLengthMultiplier));
        });
}

RgbImageData RenderContext::TakeScreenshot()
{
    //
    // Allocate buffer
    //

    auto const canvasPhysicalSize = mRenderParameters.View.GetCanvasPhysicalSize();

    auto pixelBuffer = std::make_unique<rgbColor[]>(canvasPhysicalSize.GetLinearSize());

    //
    // Take screnshot - synchronously
    //

    mRenderThread.RunSynchronously(
        [&]()
        {
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
            glReadPixels(0, 0, canvasPhysicalSize.width, canvasPhysicalSize.height, GL_RGB, GL_UNSIGNED_BYTE, pixelBuffer.get());
            CheckOpenGLError();
        });

    return RgbImageData(
        ImageSize(canvasPhysicalSize.width, canvasPhysicalSize.height),
        std::move(pixelBuffer));
}

//////////////////////////////////////////////////////////////////////////////////

void RenderContext::UpdateStart()
{
    // If there's a pending RenderUploadEnd, wait for it so we
    // know that CPU buffers are safe to be used
    if (!!mLastRenderUploadEndCompletionIndicator)
    {
        auto const waitStart = GameChronometer::Now();

        mLastRenderUploadEndCompletionIndicator->Wait();
        mLastRenderUploadEndCompletionIndicator.reset();

        mPerfStats.Update<PerfMeasurement::TotalWaitForRenderUpload>(GameChronometer::Now() - waitStart);
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
        auto const waitStart = GameChronometer::Now();

        mLastRenderDrawCompletionIndicator->Wait();
        mLastRenderDrawCompletionIndicator.reset();

        mPerfStats.Update<PerfMeasurement::TotalWaitForRenderDraw>(GameChronometer::Now() - waitStart);
    }

    mWorldRenderContext->UploadStart();

    mNotificationRenderContext->UploadStart();
}

void RenderContext::UploadEnd()
{
    mWorldRenderContext->UploadEnd();

    mNotificationRenderContext->UploadEnd();

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
    // Take a copy of the current render parameters and clean its dirtyness, and of the current render state
    mLastRenderDrawCompletionIndicator = mRenderThread.QueueTask(
        [this, renderParameters = mRenderParameters.TakeSnapshotAndClear(), lampToolToSet = mLampToolToSet]() mutable
        {
            auto const startTime = GameChronometer::Now();

            RenderStatistics renderStats;

            //
            // Process changes to parameters
            //

            {
                ProcessParameterChanges(renderParameters);

                mGlobalRenderContext->ProcessParameterChanges(renderParameters);

                mWorldRenderContext->ProcessParameterChanges(renderParameters);

                for (auto const & ship : mShips)
                {
                    ship->ProcessParameterChanges(renderParameters);
                }

                mNotificationRenderContext->ProcessParameterChanges(renderParameters);
            }

            //
            // Prepare
            //

            {
                if (lampToolToSet)
                {
                    mShaderManager->SetProgramParameterInAllShaders<GameShaderSets::ProgramParameterKind::LampToolAttributes>(*lampToolToSet);
                }

                mGlobalRenderContext->RenderPrepareStart();

                mWorldRenderContext->RenderPrepareStars(renderParameters);

                mWorldRenderContext->RenderPrepareLightnings(renderParameters);

                mWorldRenderContext->RenderPrepareClouds(renderParameters);

                mWorldRenderContext->RenderPrepareOcean(renderParameters);

                for (auto const & ship : mShips)
                {
                    ship->RenderPrepare(renderParameters);
                }

                mWorldRenderContext->RenderPrepareOceanFloor(renderParameters);

                mWorldRenderContext->RenderPrepareFishes(renderParameters);

                mWorldRenderContext->RenderPrepareAMBombPreImplosions(renderParameters);

                mWorldRenderContext->RenderPrepareCrossesOfLight(renderParameters);

                mWorldRenderContext->RenderPrepareRain(renderParameters);

                mWorldRenderContext->RenderPrepareAABBs(renderParameters);

                mNotificationRenderContext->RenderPrepare();

                mGlobalRenderContext->RenderPrepareEnd(); // Updates global element indices

                // Update stats
                mPerfStats.Update<PerfMeasurement::TotalUploadRenderDraw>(GameChronometer::Now() - startTime);
            }

            //
            // Render
            //

            {
                mWorldRenderContext->RenderDrawSky(renderParameters); // Acts as canvas clear

                mWorldRenderContext->RenderDrawStars(renderParameters);

                mWorldRenderContext->RenderDrawCloudsAndBackgroundLightnings(renderParameters);

                // Render ocean opaquely, over sky
                mWorldRenderContext->RenderDrawOcean(true, renderParameters);

                glEnable(GL_DEPTH_TEST); // Required by ships

                for (auto const & ship : mShips)
                {
                    ship->RenderDraw(renderParameters, renderStats);
                }

                glDisable(GL_DEPTH_TEST);

                mWorldRenderContext->RenderDrawOceanFloor(renderParameters);

                mWorldRenderContext->RenderDrawFishes(renderParameters);

                // Render ocean transparently, over the rest of the world, unless disabled
                if (!renderParameters.ShowShipThroughOcean)
                {
                    mWorldRenderContext->RenderDrawOcean(false, renderParameters);
                }

                mWorldRenderContext->RenderDrawAMBombPreImplosions(renderParameters);

                mWorldRenderContext->RenderDrawCrossesOfLight(renderParameters);

                mWorldRenderContext->RenderDrawForegroundLightnings(renderParameters);

                mWorldRenderContext->RenderDrawRain(renderParameters);

                mWorldRenderContext->RenderDrawAABBs(renderParameters);

                mWorldRenderContext->RenderDrawWorldBorder(renderParameters);

                mNotificationRenderContext->RenderDraw();
            }

            //
            // Wrap up
            //

            if (mDoInvokeGlFinish)
            {
                // Flush all pending operations
                glFinish();
            }

            // Flip the back buffer onto the screen
            mSwapRenderBuffersFunction();

            // Update stats
            mPerfStats.Update<PerfMeasurement::TotalRenderDraw>(GameChronometer::Now() - startTime);
            mRenderStats.store(renderStats);
        });

    //
    // Reset render state now that it's copied into render thread
    //

    mLampToolToSet.reset();
}

void RenderContext::RenderEnd()
{
    // Nop
}

void RenderContext::WaitForPendingTasks()
{
    if (!!mLastRenderDrawCompletionIndicator)
    {
        mLastRenderDrawCompletionIndicator->Wait();
        mLastRenderDrawCompletionIndicator.reset();
    }
}

////////////////////////////////////////////////////////////////////////////////////

void RenderContext::ProcessParameterChanges(RenderParameters const & renderParameters)
{
    if (renderParameters.IsCanvasSizeDirty)
    {
        ApplyCanvasSizeChanges(renderParameters);
    }

    if (renderParameters.AreShipStructureRenderModeSelectorsDirty)
    {
        ApplyShipStructureRenderModeChanges(renderParameters);
    }
}

void RenderContext::ApplyCanvasSizeChanges(RenderParameters const & renderParameters)
{
    auto const & view = renderParameters.View;

    // Set viewport and scissor
    glViewport(0, 0, view.GetCanvasPhysicalSize().width, view.GetCanvasPhysicalSize().height);

#if FS_IS_OS_MACOS()
    // After changing the viewport, on MacOS one must also re-make the context current;
    // see https://forums.wxwidgets.org/viewtopic.php?t=41368 and
    // https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_contexts/opengl_contexts.html
    mMakeRenderContextCurrentFunction();
#endif
}

void RenderContext::ApplyShipStructureRenderModeChanges(RenderParameters const & renderParameters)
{
    // Set polygon mode
    if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

float RenderContext::CalculateEffectiveAmbientLightIntensity(
    float ambientLightIntensity,
    float stormAmbientDarkening)
{
    return ambientLightIntensity * stormAmbientDarkening;
}

rgbColor RenderContext::CalculateEffectiveMoonlightColor(
    rgbColor moonlightColor,
    bool doMoonlight)
{
    return doMoonlight
        ? moonlightColor
        : rgbColor::zero();
}

vec3f RenderContext::CalculateShipWaterColor() const
{
    switch (mRenderParameters.OceanRenderMode)
    {
        case OceanRenderModeType::Depth:
        {
            return
                (mRenderParameters.DepthOceanColorStart.toVec3f() + mRenderParameters.DepthOceanColorEnd.toVec3f())
                / 2.0f;
        }

        case OceanRenderModeType::Flat:
        {
            return mRenderParameters.FlatOceanColor.toVec3f();
        }

        default:
        {
            assert(mRenderParameters.OceanRenderMode == OceanRenderModeType::Texture); // Darn VS - warns

            return mShipDefaultWaterColor.toVec3f();
        }
    }
}
