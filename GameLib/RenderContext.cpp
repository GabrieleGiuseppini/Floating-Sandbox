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
    vec4f const & ropeColour,
    ProgressCallback const & progressCallback)
    : mShaderManager()
    , mTextureRenderManager()
    , mTextRenderContext()
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
    // Sea water
    , mWaterElementBuffer()
    , mCurrentWaterElementCount(0u)
    , mWaterElementCount(0u)
    , mWaterVBO()
    // Ships
    , mShips()
    , mRopeColour(ropeColour)
    , mGenericTextureAtlasOpenGLHandle()
    , mGenericTextureAtlasMetadata()
    // Cross of light
    , mCrossOfLightBuffer()
    , mCrossOfLightVBO()
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
    , mWireframeMode(false)
{
    static constexpr float TextureProgressSteps = 1.0f /*cloud*/ + 10.0f;
    static constexpr float TotalProgressSteps = 5.0f + TextureProgressSteps;

    GLuint tmpGLuint;

    //
    // Init OpenGL
    //

    GameOpenGL::InitOpenGL();

    // Activate texture unit 0
    glActiveTexture(GL_TEXTURE0);


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
            progressCallback((1.0f + progress) / TotalProgressSteps, message);
        });



    //
    // Load texture database
    //

    progressCallback(2.0f / TotalProgressSteps, "Loading textures...");

    TextureDatabase textureDatabase = resourceLoader.LoadTextureDatabase(
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback((2.0f + progress) / TotalProgressSteps, "Loading textures...");
        });

    // Create texture render manager
    mTextureRenderManager = std::make_unique<TextureRenderManager>();


    //
    // Create cloud texture atlas
    //

    TextureAtlasBuilder cloudAtlasBuilder;
    cloudAtlasBuilder.Add(textureDatabase.GetGroup(TextureGroupType::Cloud));

    TextureAtlas cloudTextureAtlas = cloudAtlasBuilder.BuildAtlas(
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback((3.0f + progress) / TotalProgressSteps, "Loading textures...");
        });

    // Create OpenGL handle
    GLuint openGLHandle;
    glGenTextures(1, &openGLHandle);
    mCloudTextureAtlasOpenGLHandle = openGLHandle;

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

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    // Store metadata
    mCloudTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata>(cloudTextureAtlas.Metadata);


    //
    // Create generic texture atlas
    //
    // Atlas-ize all textures EXCEPT the following:
    // - Land, Water: we need these to be wrapping
    // - Clouds: we keep these separate, we have to rebind anyway
    //

    TextureAtlasBuilder genericTextureAtlasBuilder;
    for (auto const & group : textureDatabase.GetGroups())
    { 
        if (TextureGroupType::Land != group.Group
            && TextureGroupType::Water != group.Group
            && TextureGroupType::Cloud != group.Group)
        {
            genericTextureAtlasBuilder.Add(group);
        }
    }

    TextureAtlas genericTextureAtlas = genericTextureAtlasBuilder.BuildAtlas(
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback((3.0f + 1.0f + progress * (TextureProgressSteps - 1.0f)) / TotalProgressSteps, "Loading textures...");
        });

    LogMessage("Generic texture atlas size: ", genericTextureAtlas.AtlasData.Size.Width, "x", genericTextureAtlas.AtlasData.Size.Height);

    // Create OpenGL handle
    glGenTextures(1, &openGLHandle);
    mGenericTextureAtlasOpenGLHandle = openGLHandle;

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mGenericTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    // Upload atlas texture
    GameOpenGL::UploadMipmappedTexture(
        genericTextureAtlas.Metadata,
        std::move(genericTextureAtlas.AtlasData));

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CheckOpenGLError();

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    // Store metadata
    mGenericTextureAtlasMetadata = std::make_unique<TextureAtlasMetadata>(genericTextureAtlas.Metadata);



    //
    // Upload non-atlas textures
    //    

    mTextureRenderManager->UploadMipmappedGroup(
        textureDatabase.GetGroup(TextureGroupType::Land),
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback((3.0f + TextureProgressSteps + progress) / TotalProgressSteps, "Loading textures...");
        });

    mTextureRenderManager->UploadMipmappedGroup(
        textureDatabase.GetGroup(TextureGroupType::Water),
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback((4.0f + TextureProgressSteps + progress) / TotalProgressSteps, "Loading textures...");
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

    // Associate water vertex attribute with this VBO and describe it
    // (it's fully dedicated)
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::WaterAttribute), (2 + 1), GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void*)0);


    //
    // Initialize cross of light 
    //

    // Create VBO    
    glGenBuffers(1, &tmpGLuint);
    mCrossOfLightVBO = tmpGLuint;


    //
    // Initialize global settings
    //

    // Set anti-aliasing for lines and polygons
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    // Enable blend for alpha transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


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
    UpdateCanvasSize();
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
            mGenericTextureAtlasOpenGLHandle,
            *mGenericTextureAtlasMetadata,
            mOrthoMatrix,
            mVisibleWorldHeight,
            mVisibleWorldWidth,
            mCanvasToVisibleWorldHeightRatio,
            mAmbientLightIntensity,
            mWaterLevelOfDetail,
            mShipRenderMode,
            mVectorFieldRenderMode,
            mShowStressedSprings,
            mWireframeMode));
}

//////////////////////////////////////////////////////////////////////////////////

void RenderContext::RenderStart()
{
    // Set polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Clear canvas - and stencil buffer
    static const vec3f ClearColorBase(0.529f, 0.808f, 0.980f); // (cornflower blue)
    vec3f const clearColor = ClearColorBase * mAmbientLightIntensity;
    glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);
    glClearStencil(0x00);
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (mWireframeMode)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Reset crosses of light
    mCrossOfLightBuffer.clear();

    // Communicate start to child contextes
    mTextRenderContext->RenderStart();
}

void RenderContext::RenderCloudsStart(size_t cloudCount)
{
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

void RenderContext::RenderCloudsEnd()
{
    // Enable stencil test
    glEnable(GL_STENCIL_TEST);

    ////////////////////////////////////////////////////
    // Draw water stencil
    ////////////////////////////////////////////////////

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

    // Make sure polygons are filled in any case
    if (mWireframeMode)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mWaterElementCount));    

    // Don't write anything to stencil buffer now
    glStencilMask(0x00);

    // Re-enable writing to the color buffer
    glColorMask(true, true, true, true);
    
    // Reset wireframe mode, if enabled
    if (mWireframeMode)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


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
    
    // Describe vertex attribute 0
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::SharedAttribute0), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    CheckOpenGLError();

    // Enable vertex attribute 0
    glEnableVertexAttribArray(0);

    // Enable stenciling - only draw where there are no 1's
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

    // Bind atlas texture
    glBindTexture(GL_TEXTURE_2D, *mCloudTextureAtlasOpenGLHandle);
    CheckOpenGLError();

    if (mWireframeMode)
        glLineWidth(0.1f);

    // Draw
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLint>(6 * mCloudElementCount));

    ////////////////////////////////////////////////////

    // Disable stencil test
    glDisable(GL_STENCIL_TEST);
}

void RenderContext::UploadLandAndWaterStart(size_t slices)
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
    // Prepare water buffer
    //

    if (slices + 1 != mWaterElementCount)
    {
        // Bind VBO
        glBindBuffer(GL_ARRAY_BUFFER, *mWaterVBO);
        CheckOpenGLError();

        // Realloc GPU buffer
        mWaterElementCount = slices + 1;
        glBufferData(GL_ARRAY_BUFFER, mWaterElementCount * sizeof(WaterElement), nullptr, GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        // Realloc buffer
        mWaterElementBuffer.reset(new WaterElement[mWaterElementCount]);
    }

    // Reset count of water elements
    mCurrentWaterElementCount = 0u;
}

void RenderContext::UploadLandAndWaterEnd()
{
    // Bind land VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);
    CheckOpenGLError();

    // Upload buffer
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(LandElement) * mLandElementCount, mLandElementBuffer.get());

    // Describe vertex attribute 1
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::SharedAttribute1), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    CheckOpenGLError();


    // Bind water VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mWaterVBO);
    CheckOpenGLError();

    // Upload water buffer
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(WaterElement) * mWaterElementCount, mWaterElementBuffer.get());

    // No need to describe water's vertex attribute as it is dedicated and we have described it already once and for all
}

void RenderContext::RenderLand()
{
    assert(mCurrentLandElementCount == mLandElementCount);

    // Use program
    mShaderManager->ActivateProgram<ProgramType::Land>();

    // Bind texture
    glBindTexture(
        GL_TEXTURE_2D,
        mTextureRenderManager->GetOpenGLHandle(TextureGroupType::Land, 0));
    CheckOpenGLError();

    // Disable vertex attribute 0
    glDisableVertexAttribArray(0);

    if (mWireframeMode)
        glLineWidth(0.1f);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mLandElementCount));
}

void RenderContext::RenderWater()
{
    assert(mCurrentWaterElementCount == mWaterElementCount);

    // Use program
    mShaderManager->ActivateProgram<ProgramType::Water>();

    // Bind texture
    glBindTexture(
        GL_TEXTURE_2D, 
        mTextureRenderManager->GetOpenGLHandle(TextureGroupType::Water, 0));
    CheckOpenGLError();

    // Disable vertex attribute 0
    glDisableVertexAttribArray(0);

    if (mWireframeMode)
        glLineWidth(0.1f);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mWaterElementCount));
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

    mShaderManager->ActivateProgram<ProgramType::CrossOfLight>();
    mShaderManager->SetProgramParameter<ProgramType::CrossOfLight, ProgramParameterType::OrthoMatrix>(
        mOrthoMatrix);

    // Update all ships
    for (auto & ship : mShips)
    {
        ship->UpdateOrthoMatrix(mOrthoMatrix);
    }
}

void RenderContext::UpdateCanvasSize()
{
    // Set parameters in all programs

    mShaderManager->ActivateProgram<ProgramType::CrossOfLight>();
    mShaderManager->SetProgramParameter<ProgramType::CrossOfLight, ProgramParameterType::ViewportSize>(
        static_cast<float>(mCanvasWidth),
        static_cast<float>(mCanvasHeight));
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

void RenderContext::UpdateWireframeMode()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->UpdateWireframeMode(mWireframeMode);
    }
}

}