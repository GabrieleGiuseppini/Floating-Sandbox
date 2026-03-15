/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderContext.h"

#include <Core/GameChronometer.h>
#include <Core/GameExceptions.h>
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
    // Inner context
    , mInnerContext(new InnerContext())
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
            mInnerContext->ShaderManager->ActivateTexture<GameShaderSets::ProgramParameterKind::SharedTexture>();
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

            mInnerContext->ShaderManager = ShaderManager<GameShaderSets::ShaderSet>::CreateInstance(assetManager, SimpleProgressCallback::Dummy());

            LogMessage("...shaders initialized.");
        });

    progressCallback(0.1f, ProgressMessageType::InitializingNoise);

    mRenderThread.RunSynchronously(
        [&]()
        {
            mInnerContext->GlobalRenderContext = std::make_unique<GlobalRenderContext>(assetManager , *mInnerContext->ShaderManager);

            mInnerContext->GlobalRenderContext->InitializeNoiseTextures();
        });

    progressCallback(0.15f, ProgressMessageType::LoadingGenericTextures);

    mRenderThread.RunSynchronously(
        [&]()
        {
            mInnerContext->GlobalRenderContext->InitializeGenericTextures();
        });

    progressCallback(0.2f, ProgressMessageType::LoadingExplosionTextureAtlas);

    mRenderThread.RunSynchronously(
        [&]()
        {
            mInnerContext->GlobalRenderContext->InitializeExplosionTextures();
        });

    mRenderThread.RunSynchronously(
        [&]()
        {
            mInnerContext->GlobalRenderContext->InitializeNpcTextures(std::move(npcTextureAtlas)); // Safe as it's synchronous
        });

    mRenderThread.RunSynchronously(
        [&]()
        {
            mInnerContext->WorldRenderContext = std::make_unique<WorldRenderContext>(
                assetManager,
                *mInnerContext->ShaderManager,
                *mInnerContext->GlobalRenderContext);
        });

    progressCallback(0.45f, ProgressMessageType::LoadingCloudTextureAtlas);

    mRenderThread.RunSynchronously(
        [&]()
        {
            mInnerContext->WorldRenderContext->InitializeCloudTextures();
        });

    progressCallback(0.65f, ProgressMessageType::LoadingFishTextureAtlas);

    mRenderThread.RunSynchronously(
        [&]()
        {
            mInnerContext->WorldRenderContext->InitializeFishTextures();
        });

    progressCallback(0.7f, ProgressMessageType::LoadingWorldTextures);

    mRenderThread.RunSynchronously(
        [&]()
        {
            mInnerContext->WorldRenderContext->InitializeWorldTextures();
        });

    progressCallback(0.8f, ProgressMessageType::LoadingFonts);

    mRenderThread.RunSynchronously(
        [&]()
        {
            //
            // Initialize notification render context
            //

            mInnerContext->NotificationRenderContext = std::make_unique<NotificationRenderContext>(
                assetManager,
                *mInnerContext->ShaderManager,
                *mInnerContext->GlobalRenderContext);
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

                mInnerContext->GlobalRenderContext->ProcessParameterChanges(initialRenderParameters);

                mInnerContext->WorldRenderContext->ProcessParameterChanges(initialRenderParameters);

                mInnerContext->NotificationRenderContext->ProcessParameterChanges(initialRenderParameters);
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

    // Now zapp all OpenGL machinery - on the rendering thread as
    // that's where the current OpenGL is bound to
    mRenderThread.RunSynchronously(
        [&]()
        {
            mInnerContext.reset();
        });
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
            mInnerContext->Ships.clear();

            // Notify other layers
            mInnerContext->WorldRenderContext->OnReset(mRenderParameters);
        });
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

    assert(shipId == mInnerContext->Ships.size());

    size_t const newShipCount = mInnerContext->Ships.size() + 1;

    // Tell all ships
    for (auto const & ship : mInnerContext->Ships)
    {
        ship->SetShipCount(newShipCount);
    }

    // Add the ship - synchronously
    mRenderThread.RunSynchronously(
        [&]()
        {
            mInnerContext->Ships.emplace_back(
                new ShipRenderContext(
                    shipId,
                    pointCount,
                    newShipCount,
                    maxEphemeralParticles,
                    maxSpringsPerPoint,
                    std::move(exteriorTextureImage),
                    std::move(interiorViewImage),
                    *mInnerContext->ShaderManager,
                    *mInnerContext->GlobalRenderContext,
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

    mInnerContext->WorldRenderContext->UploadStart();

    mInnerContext->NotificationRenderContext->UploadStart();
}

void RenderContext::UploadEnd()
{
    mInnerContext->WorldRenderContext->UploadEnd();

    mInnerContext->NotificationRenderContext->UploadEnd();

    // Queue an indicator here, so we may wait for it
    // when we want to touch CPU buffers again
    assert(!mLastRenderUploadEndCompletionIndicator);
    mLastRenderUploadEndCompletionIndicator = mRenderThread.QueueSynchronizationPoint();
}

void RenderContext::Draw(float currentSimulationTime)
{
    assert(!mLastRenderDrawCompletionIndicator);

    // Render asynchronously; we will wait for this render to complete
    // when we want to touch GPU buffers again.
    //
    // Take a copy of the current render parameters and clean its dirtyness, and of the current render state
    mLastRenderDrawCompletionIndicator = mRenderThread.QueueTask(
        [this, renderParameters = mRenderParameters.TakeSnapshotAndClear(), lampToolToSet = mLampToolToSet, currentSimulationTime = currentSimulationTime]() mutable
        {
            auto const startTime = GameChronometer::Now();

            RenderStatistics renderStats;

            //
            // Process changes to parameters
            //

            {
                ProcessParameterChanges(renderParameters);

                mInnerContext->GlobalRenderContext->ProcessParameterChanges(renderParameters);

                mInnerContext->WorldRenderContext->ProcessParameterChanges(renderParameters);

                for (auto const & ship : mInnerContext->Ships)
                {
                    ship->ProcessParameterChanges(renderParameters);
                }

                mInnerContext->NotificationRenderContext->ProcessParameterChanges(renderParameters);
            }

            //
            // Prepare
            //

            {
                if (lampToolToSet)
                {
                    mInnerContext->ShaderManager->SetProgramParameterInAllShaders<GameShaderSets::ProgramParameterKind::LampToolAttributes>(*lampToolToSet);
                }

                mInnerContext->GlobalRenderContext->RenderPrepareStart();

                mInnerContext->WorldRenderContext->RenderPrepareStars(renderParameters);

                mInnerContext->WorldRenderContext->RenderPrepareLightnings(renderParameters);

                mInnerContext->WorldRenderContext->RenderPrepareClouds(renderParameters);

                mInnerContext->WorldRenderContext->RenderPrepareOcean(renderParameters);

                for (auto const & ship : mInnerContext->Ships)
                {
                    ship->RenderPrepare(renderParameters);
                }

                mInnerContext->WorldRenderContext->RenderPrepareOceanFloor(renderParameters);

                mInnerContext->WorldRenderContext->RenderPrepareFishes(renderParameters);

                mInnerContext->WorldRenderContext->RenderPrepareUnderwaterPlants(currentSimulationTime, renderParameters);

                mInnerContext->WorldRenderContext->RenderPrepareAntiGravityFields(currentSimulationTime, renderParameters);

                mInnerContext->WorldRenderContext->RenderPrepareTornadoes(currentSimulationTime, renderParameters);

                mInnerContext->WorldRenderContext->RenderPrepareAMBombPreImplosions(renderParameters);

                mInnerContext->WorldRenderContext->RenderPrepareCrossesOfLight(renderParameters);

                mInnerContext->WorldRenderContext->RenderPrepareRain(renderParameters);

                mInnerContext->WorldRenderContext->RenderPrepareAABBs(renderParameters);

                mInnerContext->NotificationRenderContext->RenderPrepare();

                mInnerContext->WorldRenderContext->RenderPrepareEnd();

                mInnerContext->GlobalRenderContext->RenderPrepareEnd(); // Updates global element indices

                // Update stats
                mPerfStats.Update<PerfMeasurement::TotalUploadRenderDraw>(GameChronometer::Now() - startTime);
            }

            //
            // Render
            //

            {
                mInnerContext->WorldRenderContext->RenderDrawSky(renderParameters); // Acts as canvas clear

                mInnerContext->WorldRenderContext->RenderDrawStars(renderParameters);

                mInnerContext->WorldRenderContext->RenderDrawCloudsAndBackgroundLightnings(renderParameters);

                // Render ocean opaquely, over sky
                mInnerContext->WorldRenderContext->RenderDrawOcean(true, renderParameters);

                // Render tornadoes, in the background
                mInnerContext->WorldRenderContext->RenderDrawTornadoes(DepthKindType::Background, renderParameters);

                glEnable(GL_DEPTH_TEST); // Required by ships

                for (auto const & ship : mInnerContext->Ships)
                {
                    ship->RenderDraw(renderParameters, renderStats);
                }

                glDisable(GL_DEPTH_TEST);

                mInnerContext->WorldRenderContext->RenderDrawUnderwaterPlants(renderParameters); // To be covered by ocean floor and last ocean layer

                mInnerContext->WorldRenderContext->RenderDrawOceanFloor(renderParameters);

                mInnerContext->WorldRenderContext->RenderDrawFishes(renderParameters);

                // Render ocean transparently, over the rest of the world, unless disabled
                if (!renderParameters.ShowShipThroughOcean)
                {
                    mInnerContext->WorldRenderContext->RenderDrawOcean(false, renderParameters);
                }

                mInnerContext->WorldRenderContext->RenderDrawAntiGravityFields(renderParameters);

                // Render tornadoes, in the foreground
                mInnerContext->WorldRenderContext->RenderDrawTornadoes(DepthKindType::Foreground, renderParameters);

                mInnerContext->WorldRenderContext->RenderDrawAMBombPreImplosions(renderParameters);

                mInnerContext->WorldRenderContext->RenderDrawCrossesOfLight(renderParameters);

                mInnerContext->WorldRenderContext->RenderDrawForegroundLightnings(renderParameters);

                mInnerContext->WorldRenderContext->RenderDrawRain(renderParameters);

                mInnerContext->WorldRenderContext->RenderDrawAABBs(renderParameters);

                mInnerContext->WorldRenderContext->RenderDrawWorldBorder(renderParameters);

                mInnerContext->NotificationRenderContext->RenderDraw();
            }

            //
            // Wrap up
            //

            if (mDoInvokeGlFinish)
            {
                // Flush all pending operations
                //
                // Note: if removed, we pay the synchronization price at the next following render call,
                // hence better to do it now so we know we've concluded a cycle
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
