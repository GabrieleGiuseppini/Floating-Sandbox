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
    // Sea water
    , mWaterElementBuffer()
    , mCurrentWaterElementCount(0u)
    , mWaterElementCount(0u)
    , mWaterVBO()
    // Ships
    , mShips()
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
    , mWaterContrast(0.6875f)
    , mWaterLevelOfDetail(0.6875f)
    , mShipRenderMode(ShipRenderMode::Texture)
    , mVectorFieldRenderMode(VectorFieldRenderMode::None)
    , mVectorFieldLengthMultiplier(1.0f)
    , mShowStressedSprings(false)
    , mWireframeMode(false)
    // Statistics
    , mRenderStatistics()
{
    static constexpr float GenericTextureProgressSteps = 10.0f;
    static constexpr float CloudTextureProgressSteps = 4.0f;

    // Shaders, TextRenderContext, TextureDatabase, GenericTextureAtlas, Clouds, Land, Water
    static constexpr float TotalProgressSteps = 3.0f + GenericTextureProgressSteps + CloudTextureProgressSteps + 2.0f;

    GLuint tmpGLuint;


    //
    // Init OpenGL
    //

    GameOpenGL::InitOpenGL();

    // Activate shared texture unit
    mShaderManager->ActivateTexture<ProgramParameterType::SharedTexture>();


    //
    // Load shader manager
    //

    progressCallback(0.0f, "Loading shaders...");

    ShaderManager<ShaderManagerTraits>::GlobalParameters globalParameters;

    mShaderManager = ShaderManager<ShaderManagerTraits>::CreateInstance(resourceLoader, globalParameters);


    //
    // Initialize text render context
    //

    mTextRenderContext = std::make_unique<TextRenderContext>(
        resourceLoader,
        *(mShaderManager.get()),
        mCanvasWidth,
        mCanvasHeight,
        mAmbientLightIntensity,
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
    // Create generic texture atlas
    //
    // Atlas-ize all textures EXCEPT the following:
    // - Land, Water: we need these to be wrapping
    // - Clouds: we keep these separate, we have to rebind anyway
    //

    mShaderManager->ActivateTexture<ProgramParameterType::GenericTexturesAtlasTexture>();

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
    GameOpenGL::UploadMipmappedTexture(
        genericTextureAtlas.Metadata,
        std::move(genericTextureAtlas.AtlasData));

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
    mShaderManager->ActivateProgram<ProgramType::GenericTextures>();
    mShaderManager->SetTextureParameters<ProgramType::GenericTextures>();


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
    mShaderManager->ActivateProgram<ProgramType::Land>();
    mShaderManager->SetProgramParameter<ProgramType::Land, ProgramParameterType::TextureScaling>(
            1.0f / landTextureMetadata.WorldWidth,
            1.0f / landTextureMetadata.WorldHeight);
    mShaderManager->SetTextureParameters<ProgramType::Land>();

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mLandVBO = tmpGLuint;



    //
    // Initialize water
    //

    mShaderManager->ActivateTexture<ProgramParameterType::WaterTexture>();

    mTextureRenderManager->UploadMipmappedGroup(
        textureDatabase.GetGroup(TextureGroupType::Water),
        GL_LINEAR_MIPMAP_NEAREST,
        [&progressCallback](float progress, std::string const &)
        {
            progressCallback((3.0f + GenericTextureProgressSteps + CloudTextureProgressSteps + 1.0f + progress) / TotalProgressSteps, "Loading textures...");
        });

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, mTextureRenderManager->GetOpenGLHandle(TextureGroupType::Water, 0));
    CheckOpenGLError();

    // Set hardcoded parameters
    auto const & waterTextureMetadata = textureDatabase.GetFrameMetadata(TextureGroupType::Water, 0);
    mShaderManager->ActivateProgram<ProgramType::Water>();
    mShaderManager->SetProgramParameter<ProgramType::Water, ProgramParameterType::TextureScaling>(
            1.0f / waterTextureMetadata.WorldWidth,
            1.0f / waterTextureMetadata.WorldHeight);
    mShaderManager->SetTextureParameters<ProgramType::Water>();

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

    // Set anti-aliasing for lines
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

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
    UpdateWaterContrast();
    UpdateWaterLevelOfDetail();
    UpdateShipRenderMode();
    UpdateVectorFieldRenderMode();
    UpdateShowStressedSprings();

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
    ImageData texture,
    ShipDefinition::TextureOriginType textureOrigin)
{
    assert(shipId == mShips.size() + 1);
    (void)shipId;

    // Add the ship
    mShips.emplace_back(
        new ShipRenderContext(
            pointCount,
            std::move(texture),
            textureOrigin,
            *mShaderManager,
            mGenericTextureAtlasOpenGLHandle,
            *mGenericTextureAtlasMetadata,
            mRenderStatistics,
            mOrthoMatrix,
            mVisibleWorldHeight,
            mVisibleWorldWidth,
            mCanvasToVisibleWorldHeightRatio,
            mAmbientLightIntensity,
            mWaterContrast,
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

    // Reset stats
    mRenderStatistics.Reset();
}

void RenderContext::UploadStarsStart(size_t starCount)
{
    mStarElementBuffer.clear();
    mStarElementBuffer.reserve(starCount);
}

void RenderContext::UploadStarsEnd()
{
    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mStarVBO);
    CheckOpenGLError();

    // Upload buffer
    glBufferData(GL_ARRAY_BUFFER, mStarElementBuffer.size() * sizeof(StarElement), mStarElementBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
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
    // (we know we'll be using before CrossOfLight - which is the only subsequent user of this attribute,
    //  so we can describe it now and avoid a bind later)
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

    // No need to bind VBO - we've done that at UploadLandAndWaterEnd(),
    // and we know nothing's been intervening

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

    mShaderManager->ActivateProgram<ProgramType::Stars>();
    mShaderManager->SetProgramParameter<ProgramType::Stars, ProgramParameterType::StarTransparency>(
        pow(std::max(0.0f, 1.0f - mAmbientLightIntensity), 3.0f));

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

    // Update text context
    mTextRenderContext->UpdateAmbientLightIntensity(mAmbientLightIntensity);
}

void RenderContext::UpdateSeaWaterTransparency()
{
    // Set parameter in all programs

    mShaderManager->ActivateProgram<ProgramType::Water>();
    mShaderManager->SetProgramParameter<ProgramType::Water, ProgramParameterType::WaterTransparency>(
        mSeaWaterTransparency);
}

void RenderContext::UpdateWaterContrast()
{
    // Set parameter in all ships

    for (auto & s : mShips)
    {
        s->UpdateWaterContrast(mWaterContrast);
    }
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