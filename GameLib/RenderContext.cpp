/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderContext.h"

#include "GameException.h"
#include "Log.h"

#include <cstring>

namespace Render {

RenderContext::RenderContext(
    ResourceLoader & resourceLoader,
    vec3f const & ropeColour,
    ProgressCallback const & progressCallback)
    : mShaderManager()
    , mTextureRenderManager()
    , mTextRenderContext()
    // Clouds
    , mCloudBuffer()
    , mCloudBufferSize(0u)
    , mCloudBufferMaxSize(0u)    
    , mCloudVBO()
    // Land
    , mLandBuffer()
    , mLandBufferSize(0u)
    , mLandBufferMaxSize(0u)
    , mLandVBO()
    // Sea water
    , mWaterBuffer()
    , mWaterBufferSize(0u)
    , mWaterBufferMaxSize(0u)
    , mWaterVBO()
    // Ships
    , mShips()
    , mRopeColour(ropeColour)
    // Render parameters
    , mZoom(1.0f)
    , mCamX(0.0f)
    , mCamY(0.0f)
    , mCanvasWidth(100)
    , mCanvasHeight(100)
    , mAmbientLightIntensity(1.0f)
    , mSeaWaterTransparency(0.8125f)
    , mShowShipThroughSeaWater(false)
    , mWaterLevelOfDetail(0.6875f)
    , mShipRenderMode(ShipRenderMode::Texture)
    , mVectorFieldRenderMode(VectorFieldRenderMode::None)
    , mVectorFieldLengthMultiplier(1.0f)
    , mShowStressedSprings(false)
{
    static constexpr float TextureLoadSteps = 11.0f;

    GLuint tmpGLuint;

    //
    // Init OpenGL
    //

    GameOpenGL::InitOpenGL();


    //
    // Load shader manager
    //

    progressCallback(0.0f, "Loading shaders...");

    ShaderManager<ShaderManagerTraits>::GlobalParameters globalParameters(
        ropeColour); 

    mShaderManager = ShaderManager<ShaderManagerTraits>::CreateInstance(resourceLoader, globalParameters);


    //
    // Initialize text render context
    //

    mTextRenderContext = std::make_unique<TextRenderContext>(
        resourceLoader,
        *(mShaderManager.get()),
        mCanvasWidth,
        mCanvasHeight,
        [&progressCallback](float progress, std::string const & message)
        {
            progressCallback(0.125f + 0.125f * progress, message);
        });



    //
    // Load texture database
    //

    progressCallback(0.25f, "Loading textures...");

    TextureDatabase textureDatabase = resourceLoader.LoadTextureDatabase(
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback(0.25f + progress / 4.0f, "Loading textures...");
        });

    mTextureRenderManager = std::make_unique<TextureRenderManager>();



    //
    // Upload textures
    //

    // Activate texture unit 0
    glActiveTexture(GL_TEXTURE0);

    mCloudTextureCount = textureDatabase.GetGroup(TextureGroupType::Cloud).GetFrameCount();

    mTextureRenderManager->UploadGroup(
        textureDatabase.GetGroup(TextureGroupType::Cloud),
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback(0.5f + progress / (2.0f * TextureLoadSteps), "Loading textures...");
        });

    mTextureRenderManager->UploadMipmappedGroup(
        textureDatabase.GetGroup(TextureGroupType::Land),
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback(0.5f + (1.0f + progress) / (2.0f * TextureLoadSteps), "Loading textures...");
        });

    mTextureRenderManager->UploadGroup(
        textureDatabase.GetGroup(TextureGroupType::Water),
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback(0.5f + (2.0f + progress) / (2.0f * TextureLoadSteps), "Loading textures...");
        });

    mTextureRenderManager->UploadMipmappedGroup(
        textureDatabase.GetGroup(TextureGroupType::PinnedPoint),
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback(0.5f + (3.0f + progress) / (2.0f * TextureLoadSteps), "Loading textures...");
        });

    mTextureRenderManager->UploadMipmappedGroup(
        textureDatabase.GetGroup(TextureGroupType::RcBomb),
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback(0.5f + (4.0f + progress) / (2.0f * TextureLoadSteps), "Loading textures...");
        });

    mTextureRenderManager->UploadMipmappedGroup(
        textureDatabase.GetGroup(TextureGroupType::RcBombExplosion),
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback(0.5f + (5.0f + progress) / (2.0f * TextureLoadSteps), "Loading textures...");
        });

    mTextureRenderManager->UploadMipmappedGroup(
        textureDatabase.GetGroup(TextureGroupType::RcBombPing),
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback(0.5f + (6.0f + progress) / (2.0f * TextureLoadSteps), "Loading textures...");
        });

    mTextureRenderManager->UploadMipmappedGroup(
        textureDatabase.GetGroup(TextureGroupType::TimerBomb),
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback(0.5f + (7.0f + progress) / (2.0f * TextureLoadSteps), "Loading textures...");
        });

    mTextureRenderManager->UploadMipmappedGroup(
        textureDatabase.GetGroup(TextureGroupType::TimerBombDefuse),
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback(0.5f + (8.0f + progress) / (2.0f * TextureLoadSteps), "Loading textures...");
        });

    mTextureRenderManager->UploadMipmappedGroup(
        textureDatabase.GetGroup(TextureGroupType::TimerBombExplosion),
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback(0.5f + (9.0f + progress) / (2.0f * TextureLoadSteps), "Loading textures...");
        });

    mTextureRenderManager->UploadMipmappedGroup(
        textureDatabase.GetGroup(TextureGroupType::TimerBombFuse),
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback(0.5f + (10.0f + progress) / (2.0f * TextureLoadSteps), "Loading textures...");
        });
    

    //
    // Initialize clouds 
    //

    // Create VBO    
    glGenBuffers(1, &tmpGLuint);
    mCloudVBO = tmpGLuint;



    //
    // Initialize land 
    //

    // Set hardcoded parameters
    auto const & landTextureMetadata = textureDatabase.GetFrameMetadata(TextureGroupType::Land, 0);
    mShaderManager->ActivateProgram<ProgramType::Land>();
    mShaderManager->SetProgramParameter<ProgramType::Land, ProgramParameterType::TextureScaling>(
            1.0f / landTextureMetadata.WorldWidth,
            1.0f / landTextureMetadata.WorldHeight);

    // Create VBO    
    glGenBuffers(1, &tmpGLuint);
    mLandVBO = tmpGLuint;



    //
    // Initialize water
    //

    // Set hardcoded parameters
    auto const & waterTextureMetadata = textureDatabase.GetFrameMetadata(TextureGroupType::Water, 0);
    mShaderManager->ActivateProgram<ProgramType::Water>();
    mShaderManager->SetProgramParameter<ProgramType::Water, ProgramParameterType::TextureScaling>(
            1.0f / waterTextureMetadata.WorldWidth,
            1.0f / waterTextureMetadata.WorldHeight);

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mWaterVBO = tmpGLuint;

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mWaterVBO);

    // Associate WaterPosition vertex attribute with this VBO
    // (the other attribute is shared, hence we'll associate it later)
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::WaterPosition), 2, GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void*)0);



    //
    // Initialize ortho matrix
    //

    for (size_t r = 0; r < 4; ++r)
    {
        for (size_t c = 0; c < 4; ++c)
        {
            mOrthoMatrix[r][c] = 0.0f;
        }
    }


    //
    // Update parameters
    //

    UpdateOrthoMatrix();
    UpdateVisibleWorldCoordinates();
    UpdateAmbientLightIntensity();
    UpdateSeaWaterTransparency();
    UpdateWaterLevelOfDetail();
    UpdateShipRenderMode();
    UpdateVectorFieldRenderMode();
    UpdateShowStressedSprings();

    //
    // Flush all pending operations
    //

    glFlush();


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
    int shipId,
    size_t pointCount,
    std::optional<ImageData> texture)
{   
    assert(shipId == mShips.size());
    (void)shipId;

    // Add the ship    
    mShips.emplace_back(
        new ShipRenderContext(
            pointCount,
            std::move(texture), 
            *mShaderManager,
            *mTextureRenderManager,
            mOrthoMatrix,
            mVisibleWorldHeight,
            mVisibleWorldWidth,
            mCanvasToVisibleWorldHeightRatio,
            mAmbientLightIntensity,
            mWaterLevelOfDetail,
            mShipRenderMode,
            mVectorFieldRenderMode,
            mShowStressedSprings));
}

//////////////////////////////////////////////////////////////////////////////////

void RenderContext::RenderStart()
{
    // Set anti-aliasing for lines and polygons
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    // Set polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Enable blend for alpha transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable stencil test    
    glEnable(GL_STENCIL_TEST);    

    // Clear canvas 
    static const vec3f ClearColorBase(0.529f, 0.808f, 0.980f); // (cornflower blue)
    vec3f const clearColor = ClearColorBase * mAmbientLightIntensity;
    glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glStencilMask(0x00);

    // Communicate start to child contextes
    mTextRenderContext->RenderStart();
}

void RenderContext::RenderCloudsStart(size_t clouds)
{
    if (clouds != mCloudBufferMaxSize)
    {
        // Realloc
        mCloudBuffer.reset();
        mCloudBuffer.reset(new CloudElement[clouds]);
        mCloudBufferMaxSize = clouds;
    }

    mCloudBufferSize = 0u;
}

void RenderContext::RenderCloudsEnd()
{
    //
    // Draw water stencil
    //

    // Use matte water program
    mShaderManager->ActivateProgram<ProgramType::MatteWater>();

    // Disable writing to the color buffer
    glColorMask(false, false, false, false);

    // Write all one's to stencil buffer
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    // Disable vertex attribute 0, as we don't use it
    glDisableVertexAttribArray(0);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mWaterBufferSize));    

    // Don't write anything to stencil buffer now
    glStencilMask(0x00);

    // Re-enable writing to the color buffer
    glColorMask(true, true, true, true);
    


    //
    // Draw clouds
    //

    assert(mCloudBufferSize == mCloudBufferMaxSize);

    // Use program
    mShaderManager->ActivateProgram<ProgramType::Clouds>();

    // Bind cloud VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);
    CheckOpenGLError();

    // Describe shared attribute indices
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::SharedAttribute1), 4, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)0);
    CheckOpenGLError();

    // Enable vertex attribute 0
    glEnableVertexAttribArray(0);

    // Upload cloud buffer     
    glBufferData(GL_ARRAY_BUFFER, mCloudBufferSize * sizeof(CloudElement), mCloudBuffer.get(), GL_DYNAMIC_DRAW);
    CheckOpenGLError();

    // Enable stenciling - only draw where there are no 1's
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

    // Draw
    for (size_t c = 0; c < mCloudBufferSize; ++c)
    {
        // Bind Texture
        glBindTexture(
            GL_TEXTURE_2D, 
            mTextureRenderManager->GetOpenGLHandle(
                TextureGroupType::Cloud, 
                static_cast<TextureFrameIndex>(c % mCloudTextureCount)));
        CheckOpenGLError();

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, static_cast<GLint>(4 * c), 4);
    }

    // Disable stenciling - draw always
    glStencilFunc(GL_ALWAYS, 0, 0x00);
}

void RenderContext::UploadLandAndWaterStart(size_t slices)
{
    //
    // Prepare land buffer
    //

    if (slices + 1 != mLandBufferMaxSize)
    {
        // Realloc
        mLandBuffer.reset();
        mLandBuffer.reset(new LandElement[slices + 1]);
        mLandBufferMaxSize = slices + 1;
    }

    mLandBufferSize = 0u;

    //
    // Prepare water buffer
    //

    if (slices + 1 != mWaterBufferMaxSize)
    {
        // Realloc
        mWaterBuffer.reset();
        mWaterBuffer.reset(new WaterElement[slices + 1]);
        mWaterBufferMaxSize = slices + 1;
    }

    mWaterBufferSize = 0u;
}

void RenderContext::UploadLandAndWaterEnd()
{
    //
    // We upload the land buffer later, when we need it.
    // However, we need the water buffer also for clouds, hence we upload it now
    //

    assert(mWaterBufferSize == mWaterBufferMaxSize);

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mWaterVBO);
    CheckOpenGLError();

    // Upload water buffer
    glBufferData(GL_ARRAY_BUFFER, mWaterBufferSize * sizeof(WaterElement), mWaterBuffer.get(), GL_DYNAMIC_DRAW);
    CheckOpenGLError();
}

void RenderContext::RenderLand()
{
    // Use program
    mShaderManager->ActivateProgram<ProgramType::Land>();

    // Bind texture
    glBindTexture(
        GL_TEXTURE_2D,
        mTextureRenderManager->GetOpenGLHandle(TextureGroupType::Land, 0));
    CheckOpenGLError();

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);
    CheckOpenGLError();

    // Upload land buffer
    glBufferData(GL_ARRAY_BUFFER, mLandBufferSize * sizeof(LandElement), mLandBuffer.get(), GL_DYNAMIC_DRAW);
    CheckOpenGLError();

    // Describe shared attribute indices
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::SharedAttribute1), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    CheckOpenGLError();

    // Enable vertex attribute 0
    glEnableVertexAttribArray(0);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mLandBufferSize));
}

void RenderContext::RenderWater()
{
    // Use program
    mShaderManager->ActivateProgram<ProgramType::Water>();

    // Bind texture
    glBindTexture(
        GL_TEXTURE_2D, 
        mTextureRenderManager->GetOpenGLHandle(TextureGroupType::Water, 0));
    CheckOpenGLError();

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mWaterVBO);
    CheckOpenGLError();

    // Describe shared attribute indices
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::SharedAttribute1), 1, GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void*)(2 * sizeof(float)));
    CheckOpenGLError();

    // Enable vertex attribute 0
    glEnableVertexAttribArray(0);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mWaterBufferSize));
}

void RenderContext::RenderEnd()
{
    // Communicate end to child contextes
    mTextRenderContext->RenderEnd();

    glFlush();
}

////////////////////////////////////////////////////////////////////////////////////

void RenderContext::UpdateOrthoMatrix()
{
    static constexpr float zFar = 1000.0f;
    static constexpr float zNear = 1.0f;

    // Calculate new matrix
    mOrthoMatrix[0][0] = 2.0f / mVisibleWorldWidth;
    mOrthoMatrix[1][1] = 2.0f / mVisibleWorldHeight;
    mOrthoMatrix[2][2] = -2.0f / (zFar - zNear);
    mOrthoMatrix[3][0] = -2.0f * mCamX / mVisibleWorldWidth;
    mOrthoMatrix[3][1] = -2.0f * mCamY / mVisibleWorldHeight;
    mOrthoMatrix[3][2] = -(zFar + zNear) / (zFar - zNear);
    mOrthoMatrix[3][3] = 1.0f;

    // Set parameters in all programs

    mShaderManager->ActivateProgram<ProgramType::Land>();
    mShaderManager->SetProgramParameter<ProgramType::Land, ProgramParameterType::OrthoMatrix>(
        mOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::Water>();
    mShaderManager->SetProgramParameter<ProgramType::Water, ProgramParameterType::OrthoMatrix>(
        mOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::MatteWater>();
    mShaderManager->SetProgramParameter<ProgramType::MatteWater, ProgramParameterType::OrthoMatrix>(
        mOrthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::Matte>();
    mShaderManager->SetProgramParameter<ProgramType::Matte, ProgramParameterType::OrthoMatrix>(
        mOrthoMatrix);

    // Update all ships
    for (auto & ship : mShips)
    {
        ship->UpdateOrthoMatrix(mOrthoMatrix);
    }
}

void RenderContext::UpdateVisibleWorldCoordinates()
{
    // Calculate new dimensions
    mVisibleWorldHeight = 2.0f * 70.0f / (mZoom + 0.001f);
    mVisibleWorldWidth = static_cast<float>(mCanvasWidth) / static_cast<float>(mCanvasHeight) * mVisibleWorldHeight;
    mCanvasToVisibleWorldHeightRatio = static_cast<float>(mCanvasHeight) / mVisibleWorldHeight;

    // Update all ships
    for (auto & ship : mShips)
    {
        ship->UpdateVisibleWorldCoordinates(
            mVisibleWorldHeight,
            mVisibleWorldWidth,
            mCanvasToVisibleWorldHeightRatio);
    }
}

void RenderContext::UpdateAmbientLightIntensity()
{
    // Set parameters in all programs

    mShaderManager->ActivateProgram<ProgramType::Clouds>();
    mShaderManager->SetProgramParameter<ProgramType::Clouds, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::Land>();
    mShaderManager->SetProgramParameter<ProgramType::Land, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager->ActivateProgram<ProgramType::Water>();
    mShaderManager->SetProgramParameter<ProgramType::Water, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    // Update all ships
    for (auto & ship : mShips)
    {
        ship->UpdateAmbientLightIntensity(mAmbientLightIntensity);
    }
}

void RenderContext::UpdateSeaWaterTransparency()
{
    // Set parameter in all programs

    mShaderManager->ActivateProgram<ProgramType::Water>();
    mShaderManager->SetProgramParameter<ProgramType::Water, ProgramParameterType::WaterTransparency>(
        mSeaWaterTransparency);
}

void RenderContext::UpdateWaterLevelOfDetail()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->UpdateWaterLevelThreshold(mWaterLevelOfDetail);
    }
}

void RenderContext::UpdateShipRenderMode()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->UpdateShipRenderMode(mShipRenderMode);
    }
}

void RenderContext::UpdateVectorFieldRenderMode()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->UpdateVectorFieldRenderMode(mVectorFieldRenderMode);
    }
}

void RenderContext::UpdateShowStressedSprings()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->UpdateShowStressedSprings(mShowStressedSprings);
    }
}

}