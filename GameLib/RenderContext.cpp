/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-02-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderContext.h"

#include "GameException.h"

#include <cstring>

RenderContext::RenderContext(
    ResourceLoader & resourceLoader,
    vec3f const & ropeColour,
    ProgressCallback const & progressCallback)
    : mShaderManager()
    , mTextureRenderManager()
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

    ShaderManager<Render::ShaderManagerTraits>::GlobalParameters globalParameters(
        ropeColour); 

    mShaderManager = ShaderManager<Render::ShaderManagerTraits>::CreateInstance(resourceLoader, globalParameters);



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

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);

    // Enable vertex arrays for this VBO
    glEnableVertexAttribArray(static_cast<GLuint>(Render::VertexAttributeType::SharedPosition));
    glEnableVertexAttribArray(static_cast<GLuint>(Render::VertexAttributeType::SharedTextureCoordinates));




    //
    // Initialize land 
    //

    // Set hardcoded parameters
    auto const & landTextureMetadata = textureDatabase.GetFrameMetadata(TextureGroupType::Land, 0);
    mShaderManager->ActivateProgram<Render::ProgramType::Land>();
    mShaderManager->SetProgramParameter<Render::ProgramType::Land, Render::ProgramParameterType::TextureScaling>(
            1.0f / landTextureMetadata.WorldWidth,
            1.0f / landTextureMetadata.WorldHeight);


    // Create VBO    
    glGenBuffers(1, &tmpGLuint);
    mLandVBO = tmpGLuint;

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);
    
    // Enable vertex arrays for this VBO
    glEnableVertexAttribArray(static_cast<GLuint>(Render::VertexAttributeType::SharedPosition));



    //
    // Initialize water
    //

    // Set hardcoded parameters
    auto const & waterTextureMetadata = textureDatabase.GetFrameMetadata(TextureGroupType::Water, 0);
    mShaderManager->ActivateProgram<Render::ProgramType::Water>();
    mShaderManager->SetProgramParameter<Render::ProgramType::Water, Render::ProgramParameterType::TextureScaling>(
            1.0f / waterTextureMetadata.WorldWidth,
            1.0f / waterTextureMetadata.WorldHeight);

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mWaterVBO = tmpGLuint;

    // TODOTEST: this is repeated down
    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mWaterVBO);

    // Associate WaterPosition vertex attribute with this VBO
    glVertexAttribPointer(static_cast<GLuint>(Render::VertexAttributeType::WaterPosition), 2, GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void*)0);

    // Enable vertex arrays for this VBO
    glEnableVertexAttribArray(static_cast<GLuint>(Render::VertexAttributeType::WaterPosition));
    glEnableVertexAttribArray(static_cast<GLuint>(Render::VertexAttributeType::Shared1XFloat));



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

    // TODO: might also reset all the other render contextes, once we have refactored
    // them out and stored them as uq_ptr's
}

void RenderContext::AddShip(
    int shipId,
    std::optional<ImageData> texture)
{   
    assert(shipId == mShips.size());

    // Add the ship    
    mShips.emplace_back(
        new ShipRenderContext(
            std::move(texture), 
            mRopeColour,
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
    mShaderManager->ActivateProgram<Render::ProgramType::MatteWater>();

    // Set parameters
    // TODO: move up
    mShaderManager->SetProgramParameter<Render::ProgramType::MatteWater, Render::ProgramParameterType::MatteColor>(
        1.0f, 1.0f, 1.0f, 1.0f);

    // Disable writing to the color buffer
    glColorMask(false, false, false, false);

    // Write all one's to stencil buffer
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    ////// TODOTEST
    ////// Bind VBO
    ////glBindBuffer(GL_ARRAY_BUFFER, *mWaterVBO);
    ////// Associate WaterPosition vertex attribute with this VBO
    ////glVertexAttribPointer(static_cast<GLuint>(Render::VertexAttributeType::WaterPosition), 2, GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void*)0);
    ////glEnableVertexAttribArray(static_cast<GLuint>(Render::VertexAttributeType::WaterPosition));

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
    mShaderManager->ActivateProgram<Render::ProgramType::Clouds>();

    // Bind cloud VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);

    // Describe buffer
    glVertexAttribPointer(static_cast<GLuint>(Render::VertexAttributeType::SharedPosition), 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)0);
    glVertexAttribPointer(static_cast<GLuint>(Render::VertexAttributeType::SharedTextureCoordinates), 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)(2 * sizeof(float)));

    // Upload cloud buffer     
    glBufferData(GL_ARRAY_BUFFER, mCloudBufferSize * sizeof(CloudElement), mCloudBuffer.get(), GL_DYNAMIC_DRAW);

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
    // Upload land buffer
    //

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);

    // Upload land buffer
    glBufferData(GL_ARRAY_BUFFER, mLandBufferSize * sizeof(LandElement), mLandBuffer.get(), GL_DYNAMIC_DRAW);



    //
    // Upload water buffer
    //

    assert(mWaterBufferSize == mWaterBufferMaxSize);

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mWaterVBO);

    // Upload water buffer
    glBufferData(GL_ARRAY_BUFFER, mWaterBufferSize * sizeof(WaterElement), mWaterBuffer.get(), GL_DYNAMIC_DRAW);
}

void RenderContext::RenderLand()
{
    // Use program
    mShaderManager->ActivateProgram<Render::ProgramType::Land>();

    // Bind texture
    glBindTexture(
        GL_TEXTURE_2D,
        mTextureRenderManager->GetOpenGLHandle(TextureGroupType::Land, 0));

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);

    // Describe buffer
    glVertexAttribPointer(static_cast<GLuint>(Render::VertexAttributeType::SharedPosition), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mLandBufferSize));
}

void RenderContext::RenderWater()
{
    // Use program
    mShaderManager->ActivateProgram<Render::ProgramType::Water>();

    // Bind texture
    glBindTexture(
        GL_TEXTURE_2D, 
        mTextureRenderManager->GetOpenGLHandle(TextureGroupType::Water, 0));

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mWaterVBO);

    // Associate Shared1XFloat vertex attribute with this VBO
    glVertexAttribPointer(static_cast<GLuint>(Render::VertexAttributeType::Shared1XFloat), 1, GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void*)(2 * sizeof(float)));

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mWaterBufferSize));
}

void RenderContext::RenderEnd()
{
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

    // Update all ships
    for (auto & ship : mShips)
    {
        ship->UpdateOrthoMatrix(mOrthoMatrix);
    }

    // Set parameters in all programs

    mShaderManager->ActivateProgram<Render::ProgramType::Land>();
    mShaderManager->SetProgramParameter<Render::ProgramType::Land, Render::ProgramParameterType::OrthoMatrix>(
        mOrthoMatrix);

    mShaderManager->ActivateProgram<Render::ProgramType::Water>();
    mShaderManager->SetProgramParameter<Render::ProgramType::Water, Render::ProgramParameterType::OrthoMatrix>(
        mOrthoMatrix);

    mShaderManager->ActivateProgram<Render::ProgramType::MatteWater>();
    mShaderManager->SetProgramParameter<Render::ProgramType::MatteWater, Render::ProgramParameterType::OrthoMatrix>(
        mOrthoMatrix);

    mShaderManager->ActivateProgram<Render::ProgramType::Matte>();
    mShaderManager->SetProgramParameter<Render::ProgramType::Matte, Render::ProgramParameterType::OrthoMatrix>(
        mOrthoMatrix);
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
    // Update all ships
    for (auto & ship : mShips)
    {
        ship->UpdateAmbientLightIntensity(mAmbientLightIntensity);
    }

    // Set parameters in all programs

    mShaderManager->ActivateProgram<Render::ProgramType::Clouds>();
    mShaderManager->SetProgramParameter<Render::ProgramType::Clouds, Render::ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager->ActivateProgram<Render::ProgramType::Land>();
    mShaderManager->SetProgramParameter<Render::ProgramType::Land, Render::ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager->ActivateProgram<Render::ProgramType::Water>();
    mShaderManager->SetProgramParameter<Render::ProgramType::Water, Render::ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);
}

void RenderContext::UpdateSeaWaterTransparency()
{
    // Set parameter in all programs

    mShaderManager->ActivateProgram<Render::ProgramType::Water>();
    mShaderManager->SetProgramParameter<Render::ProgramType::Water, Render::ProgramParameterType::WaterTransparency>(
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
