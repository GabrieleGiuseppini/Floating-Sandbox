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
    , mHeatBlasterFlameVBO()
    , mFireExtinguisherSprayVBO()
    // VAOs
    , mHeatBlasterFlameVAO()
    , mFireExtinguisherSprayVAO()    
    // Child contextes
    , mGlobalRenderContext()
    , mWorldRenderContext()
    , mShips()    
    , mNotificationRenderContext()
    // HeatBlaster
    , mHeatBlasterFlameShaderToRender()
    // Fire extinguisher
    , mFireExtinguisherSprayShaderToRender()
    // Non-render parameters
    , mAmbientLightIntensity(1.0f)
    , mShipFlameSizeAdjustment(1.0f)
    , mShipDefaultWaterColor(0x00, 0x00, 0xcc)
    , mVectorFieldRenderMode(VectorFieldRenderModeType::None)
    , mVectorFieldLengthMultiplier(1.0f)
    // Rendering externals
    , mSwapRenderBuffersFunction(swapRenderBuffersFunction)
    // Shader manager
    , mShaderManager()    
    // Render parameters
    , mRenderParameters(initialCanvasSize)
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

    progressCallback(0.05f, "Loading shaders...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            //
            // Load shader manager
            //

            mShaderManager = ShaderManager<ShaderManagerTraits>::CreateInstance(resourceLocator.GetRenderShadersRootPath());
        });

    progressCallback(0.1f, "Initializing noise...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            mGlobalRenderContext = std::make_unique<GlobalRenderContext>(*mShaderManager);

            mGlobalRenderContext->InitializeNoiseTextures(resourceLocator);
        });
    
    progressCallback(0.15f, "Loading generic textures...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            mGlobalRenderContext->InitializeGenericTextures(resourceLocator);
        });

    progressCallback(0.2f, "Loading explosion texture atlas...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            mGlobalRenderContext->InitializeExplosionTextures(resourceLocator);
        });

    progressCallback(0.4f, "Initializing buffers...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            InitializeBuffersAndVAOs();
        });

    progressCallback(0.45f, "Initializing world...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            mWorldRenderContext = std::make_unique<WorldRenderContext>(
                *mShaderManager,
                *mGlobalRenderContext);
        });

    progressCallback(0.5f, "Loading cloud texture atlas...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            mWorldRenderContext->InitializeCloudTextures(resourceLocator);
        });

    progressCallback(0.7f, "Loading world textures...");

    mRenderThread.RunSynchronously(
        [&]()
        {
            mWorldRenderContext->InitializeWorldTextures(resourceLocator);
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
                *mGlobalRenderContext,
                mRenderParameters.View.GetCanvasWidth(),
                mRenderParameters.View.GetCanvasHeight(),
                mRenderParameters.EffectiveAmbientLightIntensity);
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
            // Set initial values of non-render parameters from which
            // other parameters are calculated
            //

            SetAmbientLightIntensity(mAmbientLightIntensity);
            SetShipDefaultWaterColor(mShipDefaultWaterColor);


            //
            // Update parameters for initial values
            //

            {
                auto const initialRenderParameters = mRenderParameters.TakeSnapshotAndClear();

                ProcessParameterChanges(initialRenderParameters);

                mWorldRenderContext->ProcessParameterChanges(initialRenderParameters);

                // TODO: notification render context
            }


            //
            // Flush all pending operations
            //

            glFinish();
        });

    progressCallback(1.0f, "Initializing graphics...");
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
    LogMessage("TODOTEST: RenderContext::RebindContext: start");
    mRenderThread.RunSynchronously(std::move(rebindContextFunction));
    LogMessage("TODOTEST: RenderContext::RebindContext: end");
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
        });
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
                    *mGlobalRenderContext,
                    mRenderParameters,
                    mShipFlameSizeAdjustment));
        });
}

RgbImageData RenderContext::TakeScreenshot()
{
    //
    // Allocate buffer
    //

    int const canvasWidth = mRenderParameters.View.GetCanvasWidth();
    int const canvasHeight = mRenderParameters.View.GetCanvasHeight();

    auto pixelBuffer = std::make_unique<rgbColor[]>(canvasWidth * canvasHeight);

    //
    // Take screnshot - synchronously
    //

    mRenderThread.RunSynchronously(
        [&]()
        {
            //
            // Flush draw calls
            //

            glFinish();

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
        });

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

    mWorldRenderContext->UploadStart();

    // Reset HeatBlaster flame, it's uploaded as needed
    mHeatBlasterFlameShaderToRender.reset();

    // Reset fire extinguisher spray, it's uploaded as needed
    mFireExtinguisherSprayShaderToRender.reset();
}


void RenderContext::UploadEnd()
{
    mWorldRenderContext->UploadEnd();

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
        [this, renderParameters = mRenderParameters.TakeSnapshotAndClear()]() mutable
        {
            auto const startTime = GameChronometer::now();

            RenderStatistics renderStats;

            //
            // Process changes to parameters
            //

            {
                ProcessParameterChanges(renderParameters);

                mWorldRenderContext->ProcessParameterChanges(renderParameters);

                // TODO: notification render context

                for (auto const & ship : mShips)
                {
                    ship->ProcessParameterChanges(renderParameters);
                }
            }

            //
            // Initialize
            //

            // Set polygon mode
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            // Clear canvas - and depth buffer
            vec3f const clearColor = renderParameters.FlatSkyColor.toVec3f() * renderParameters.EffectiveAmbientLightIntensity;
            glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Debug mode
            if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            //
            // World
            //

            mWorldRenderContext->RenderStars(renderParameters);

            mWorldRenderContext->PrepareRenderLightnings(renderParameters);

            mWorldRenderContext->RenderCloudsAndBackgroundLightnings(renderParameters);

            // Render ocean opaquely, over sky
            mWorldRenderContext->RenderOcean(true, renderParameters);

            glEnable(GL_DEPTH_TEST); // Required by ships

            for (auto const & ship : mShips)
            {
                ship->Draw(renderParameters, renderStats);
            }

            glDisable(GL_DEPTH_TEST);

            // Render ocean transparently, over ship, unless disabled
            if (!renderParameters.ShowShipThroughOcean)
            {
                mWorldRenderContext->RenderOcean(false, renderParameters);
            }

            //
            // Misc
            //

            mWorldRenderContext->RenderOceanFloor(renderParameters);

            mWorldRenderContext->RenderAMBombPreImplosions(renderParameters);

            mWorldRenderContext->RenderCrossesOfLight(renderParameters);

            RenderHeatBlasterFlame(renderParameters);

            RenderFireExtinguisherSpray(renderParameters);

            mWorldRenderContext->RenderForegroundLightnings(renderParameters);

            mWorldRenderContext->RenderRain(renderParameters);

            mWorldRenderContext->RenderWorldBorder(renderParameters);

            mNotificationRenderContext->Draw();

            // Flip the back buffer onto the screen
            LogMessage("TODOTEST: RenderContext::Draw: Swapping buffers...");
            mSwapRenderBuffersFunction();
            LogMessage("TODOTEST: RenderContext::Draw: ...buffers swapped.");

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

    GLuint vbos[2];
    glGenBuffers(2, vbos);
    mHeatBlasterFlameVBO = vbos[0];
    mFireExtinguisherSprayVBO = vbos[1];


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
}

void RenderContext::RenderHeatBlasterFlame(RenderParameters const & /*renderParameters*/)
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

void RenderContext::RenderFireExtinguisherSpray(RenderParameters const & /*renderParameters*/)
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

void RenderContext::ProcessParameterChanges(RenderParameters const & renderParameters)
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
}

void RenderContext::ApplyViewModelChanges(RenderParameters const & renderParameters)
{
    //
    // Update ortho matrix in all programs
    //

    constexpr float ZFar = 1000.0f;
    constexpr float ZNear = 1.0f;

    ViewModel::ProjectionMatrix globalOrthoMatrix;
    renderParameters.View.CalculateGlobalOrthoMatrix(ZFar, ZNear, globalOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::HeatBlasterFlameCool>();
    mShaderManager->SetProgramParameter<ProgramType::HeatBlasterFlameCool, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::HeatBlasterFlameHeat>();
    mShaderManager->SetProgramParameter<ProgramType::HeatBlasterFlameHeat, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::FireExtinguisherSpray>();
    mShaderManager->SetProgramParameter<ProgramType::FireExtinguisherSpray, ProgramParameterType::OrthoMatrix>(
        globalOrthoMatrix);
}

void RenderContext::ApplyCanvasSizeChanges(RenderParameters const & renderParameters)
{
    auto const & view = renderParameters.View;

    // Set viewport
    glViewport(0, 0, view.GetCanvasWidth(), view.GetCanvasHeight());

    // Propagate
    mNotificationRenderContext->UpdateCanvasSize(view.GetCanvasWidth(), view.GetCanvasHeight());
}

void RenderContext::ApplyEffectiveAmbientLightIntensityChanges(RenderParameters const & renderParameters)
{
    // TODO: move upstairs - NotificationRenderContext::ProcessParameterChanges() invoked at all places
    mNotificationRenderContext->UpdateEffectiveAmbientLightIntensity(renderParameters.EffectiveAmbientLightIntensity);
}

float RenderContext::CalculateEffectiveAmbientLightIntensity(
    float ambientLightIntensity,
    float stormAmbientDarkening)
{
    return ambientLightIntensity * stormAmbientDarkening;
}

vec4f RenderContext::CalculateShipWaterColor() const
{
    switch (mRenderParameters.OceanRenderMode)
    {
        case OceanRenderModeType::Depth:
        {
            return
                (mRenderParameters.DepthOceanColorStart.toVec4f(1.0f) + mRenderParameters.DepthOceanColorEnd.toVec4f(1.0f))
                / 2.0f;
        }

        case OceanRenderModeType::Flat:
        {
            return mRenderParameters.FlatOceanColor.toVec4f(1.0f);
        }

        default:
        {
            assert(mRenderParameters.OceanRenderMode == OceanRenderModeType::Texture); // Darn VS - warns

            return mShipDefaultWaterColor.toVec4f(1.0f);
        }
    }
}

}