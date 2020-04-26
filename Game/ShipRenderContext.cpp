/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-03-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipRenderContext.h"

#include "GameParameters.h"

#include <GameCore/GameException.h>
#include <GameCore/GameMath.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/Log.h>

#include <cstring>

namespace Render {

// Base dimensions of flame quads
static float constexpr BasisHalfFlameQuadWidth = 9.5f * 2.0f;
static float constexpr BasisFlameQuadHeight = 7.5f * 2.0f;


ShipRenderContext::ShipRenderContext(
    ShipId shipId,
    size_t shipCount,
    size_t pointCount,
    RgbaImageData shipTexture,
    ShipDefinition::TextureOriginType /*textureOrigin*/,
    ShaderManager<ShaderManagerTraits> & shaderManager,
    TextureAtlasMetadata<ExplosionTextureGroups> const & explosionTextureAtlasMetadata,
    TextureAtlasMetadata<GenericLinearTextureGroups> const & genericLinearTextureAtlasMetadata,
    TextureAtlasMetadata<GenericMipMappedTextureGroups> const & genericMipMappedTextureAtlasMetadata,
    RenderStatistics & renderStatistics,
    ViewModel const & viewModel,
    float effectiveAmbientLightIntensity,
    vec4f const & lampLightColor,
    vec4f const & waterColor,
    float waterContrast,
    float waterLevelOfDetail,
    ShipRenderMode shipRenderMode,
    DebugShipRenderMode debugShipRenderMode,
    VectorFieldRenderMode vectorFieldRenderMode,
    bool showStressedSprings,
    bool drawHeatOverlay,
    float heatOverlayTransparency,
    ShipFlameRenderMode shipFlameRenderMode,
    float shipFlameSizeAdjustment)
    : mShipId(shipId)
    , mShipCount(shipCount)
    , mPointCount(pointCount)
    , mMaxMaxPlaneId(0)
    // Buffers
    , mPointAttributeGroup1Buffer()
    , mPointAttributeGroup1VBO()
    , mPointAttributeGroup2Buffer()
    , mPointAttributeGroup2VBO()
    , mPointColorVBO()
    , mPointTemperatureVBO()
    //
    , mStressedSpringElementBuffer()
    , mStressedSpringElementVBO()
    //
    , mFlameVertexBuffer()
    , mFlameVertexBufferAllocatedSize(0)
    , mFlameBackgroundCount(0)
    , mFlameForegroundCount(0)
    , mFlameVertexVBO()
    , mWindSpeedMagnitudeRunningAverage(0.0f)
    , mCurrentWindSpeedMagnitudeAverage(-1.0f) // Make sure we update the param right away
    //
    , mExplosionPlaneVertexBuffers()
    , mExplosionTotalPlaneVertexCount(0)
    , mExplosionVBO()
    , mExplosionVBOAllocatedVertexCount(0)
    //
    , mSparkleVertexBuffer()
    , mSparkleVertexVBO()
    //
    , mAirBubbleVertexBuffer()
    , mGenericMipMappedTexturePlaneVertexBuffers()
    , mGenericMipMappedTextureTotalPlaneVertexCount(0)
    , mGenericMipMappedTextureVBO()
    , mGenericMipMappedTextureVBOAllocatedVertexCount(0)
    //
    , mHighlightVertexBuffers()
    , mHighlightVertexVBO()
    //
    , mVectorArrowVertexBuffer()
    , mVectorArrowVBO()
    , mVectorArrowColor()
    // Element (index) buffers
    , mPointElementBuffer()
    , mEphemeralPointElementBuffer()
    , mSpringElementBuffer()
    , mRopeElementBuffer()
    , mTriangleElementBuffer()
    , mElementVBO()
    , mPointElementVBOStartIndex(0)
    , mEphemeralPointElementVBOStartIndex(0)
    , mSpringElementVBOStartIndex(0)
    , mRopeElementVBOStartIndex(0)
    , mTriangleElementVBOStartIndex(0)
    // VAOs
    , mShipVAO()
    , mFlameVAO()
    , mExplosionVAO()
    , mSparkleVAO()
    , mGenericMipMappedTextureVAO()
    , mVectorArrowVAO()
    // Textures
    , mShipTextureOpenGLHandle()
    , mStressedSpringTextureOpenGLHandle()
    , mExplosionTextureAtlasMetadata(explosionTextureAtlasMetadata)
    , mGenericLinearTextureAtlasMetadata(genericLinearTextureAtlasMetadata)
    , mGenericMipMappedTextureAtlasMetadata(genericMipMappedTextureAtlasMetadata)
    // Managers
    , mShaderManager(shaderManager)
    // Parameters
    , mViewModel(viewModel)
    , mEffectiveAmbientLightIntensity(effectiveAmbientLightIntensity)
    , mLampLightColor(lampLightColor)
    , mWaterColor(waterColor)
    , mWaterContrast(waterContrast)
    , mWaterLevelOfDetail(waterLevelOfDetail)
    , mShipRenderMode(shipRenderMode)
    , mDebugShipRenderMode(debugShipRenderMode)
    , mVectorFieldRenderMode(vectorFieldRenderMode)
    , mShowStressedSprings(showStressedSprings)
    , mDrawHeatOverlay(drawHeatOverlay)
    , mHeatOverlayTransparency(heatOverlayTransparency)
    , mShipFlameRenderMode(shipFlameRenderMode)
    , mShipFlameSizeAdjustment(shipFlameSizeAdjustment)
    , mHalfFlameQuadWidth(0.0f) // Will be calculated
    , mFlameQuadHeight(0.0f) // Will be calculated
    // Statistics
    , mRenderStatistics(renderStatistics)
{
    GLuint tmpGLuint;

    // Clear errors
    glGetError();


    //
    // Initialize buffers
    //

    GLuint vbos[11];
    glGenBuffers(11, vbos);
    CheckOpenGLError();

    mPointAttributeGroup1VBO = vbos[0];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup1VBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(vec4f), nullptr, GL_STREAM_DRAW);
    mPointAttributeGroup1Buffer.reset(new vec4f[pointCount]);
    std::memset(mPointAttributeGroup1Buffer.get(), 0, pointCount * sizeof(vec4f));

    mPointAttributeGroup2VBO = vbos[1];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup2VBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(vec4f), nullptr, GL_STREAM_DRAW);
    mPointAttributeGroup2Buffer.reset(new vec4f[pointCount]);
    std::memset(mPointAttributeGroup2Buffer.get(), 0, pointCount * sizeof(vec4f));

    mPointColorVBO = vbos[2];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointColorVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(vec4f), nullptr, GL_STATIC_DRAW);

    mPointTemperatureVBO = vbos[3];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointTemperatureVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(float), nullptr, GL_STREAM_DRAW);

    mStressedSpringElementVBO = vbos[4];
    mStressedSpringElementBuffer.reserve(1024); // Arbitrary

    mFlameVertexVBO = vbos[5];

    mExplosionVBO = vbos[6];
    glBindBuffer(GL_ARRAY_BUFFER, *mExplosionVBO);
    mExplosionVBOAllocatedVertexCount = 10 * 6; // Initial guess, might get more
    glBufferData(GL_ARRAY_BUFFER, mExplosionVBOAllocatedVertexCount * sizeof(ExplosionVertex), nullptr, GL_STREAM_DRAW);

    mSparkleVertexVBO = vbos[7];
    mSparkleVertexBuffer.reserve(256); // Arbitrary

    mGenericMipMappedTextureVBO = vbos[8];
    glBindBuffer(GL_ARRAY_BUFFER, *mGenericMipMappedTextureVBO);
    mGenericMipMappedTextureVBOAllocatedVertexCount = GameParameters::MaxEphemeralParticles * 6; // Initial guess, might get more
    glBufferData(GL_ARRAY_BUFFER, mGenericMipMappedTextureVBOAllocatedVertexCount * sizeof(GenericTextureVertex), nullptr, GL_STREAM_DRAW);

    mHighlightVertexVBO = vbos[9];

    mVectorArrowVBO = vbos[10];

    glBindBuffer(GL_ARRAY_BUFFER, 0);


    //
    // Initialize element (index) buffers
    //

    glGenBuffers(1, &tmpGLuint);
    mElementVBO = tmpGLuint;

    mPointElementBuffer.reserve(pointCount);
    mEphemeralPointElementBuffer.reserve(GameParameters::MaxEphemeralParticles);
    mSpringElementBuffer.reserve(pointCount * GameParameters::MaxSpringsPerPoint);
    mRopeElementBuffer.reserve(pointCount); // Arbitrary
    mTriangleElementBuffer.reserve(pointCount * GameParameters::MaxTrianglesPerPoint);


    //
    // Initialize Ship VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mShipVAO = tmpGLuint;

        glBindVertexArray(*mShipVAO);
        CheckOpenGLError();

        //
        // Describe vertex attributes
        //

        glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup1VBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::ShipPointAttributeGroup1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointAttributeGroup1), 4, GL_FLOAT, GL_FALSE, sizeof(vec4f), (void*)(0));
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup2VBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::ShipPointAttributeGroup2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointAttributeGroup2), 4, GL_FLOAT, GL_FALSE, sizeof(vec4f), (void*)(0));
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, *mPointColorVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::ShipPointColor));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointColor), 4, GL_FLOAT, GL_FALSE, sizeof(vec4f), (void*)(0));
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, *mPointTemperatureVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::ShipPointTemperature));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointTemperature), 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)(0));
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //
        // Associate element VBO
        //

        // NOTE: Intel drivers have a bug in the VAO ARB: they do not store the ELEMENT_ARRAY_BUFFER binding
        // in the VAO. So we won't associate the element VBO here, but rather before the drawing call.
        ////glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mPointElementVBO);
        ////CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize Flame VAO's
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mFlameVAO = tmpGLuint;

        glBindVertexArray(*mFlameVAO);

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mFlameVertexVBO);
        static_assert(sizeof(FlameVertex) == (4 + 2) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Flame1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Flame1), 4, GL_FLOAT, GL_FALSE, sizeof(FlameVertex), (void*)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Flame2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Flame2), 2, GL_FLOAT, GL_FALSE, sizeof(FlameVertex), (void*)((4) * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize Explosion VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mExplosionVAO = tmpGLuint;

        glBindVertexArray(*mExplosionVAO);

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mExplosionVBO);
        static_assert(sizeof(ExplosionVertex) == (4 + 4 + 2) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Explosion1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Explosion1), 4, GL_FLOAT, GL_FALSE, sizeof(ExplosionVertex), (void*)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Explosion2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Explosion2), 4, GL_FLOAT, GL_FALSE, sizeof(ExplosionVertex), (void*)((4) * sizeof(float)));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Explosion3));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Explosion3), 2, GL_FLOAT, GL_FALSE, sizeof(ExplosionVertex), (void*)((4 + 4) * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize Sparkle VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mSparkleVAO = tmpGLuint;

        glBindVertexArray(*mSparkleVAO);

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mSparkleVertexVBO);
        static_assert(sizeof(SparkleVertex) == (4 + 4) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Sparkle1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Sparkle1), 4, GL_FLOAT, GL_FALSE, sizeof(SparkleVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Sparkle2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Sparkle2), 4, GL_FLOAT, GL_FALSE, sizeof(SparkleVertex), (void *)((4) * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize GenericMipMappedTexture VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mGenericMipMappedTextureVAO = tmpGLuint;

        glBindVertexArray(*mGenericMipMappedTextureVAO);

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mGenericMipMappedTextureVBO);
        static_assert(sizeof(GenericTextureVertex) == (4 + 4 + 3) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTexture1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTexture1), 4, GL_FLOAT, GL_FALSE, sizeof(GenericTextureVertex), (void*)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTexture2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTexture2), 4, GL_FLOAT, GL_FALSE, sizeof(GenericTextureVertex), (void*)((4) * sizeof(float)));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTexture3));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTexture3), 3, GL_FLOAT, GL_FALSE, sizeof(GenericTextureVertex), (void*)((4 + 4) * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize Highlight VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mHighlightVAO = tmpGLuint;

        glBindVertexArray(*mHighlightVAO);

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mHighlightVertexVBO);
        static_assert(sizeof(HighlightVertex) == (2 + 2 + 3 + 1 + 1) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Highlight1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Highlight1), 4, GL_FLOAT, GL_FALSE, sizeof(HighlightVertex), (void *)((0) * sizeof(float)));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Highlight2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Highlight2), 4, GL_FLOAT, GL_FALSE, sizeof(HighlightVertex), (void *)((4) * sizeof(float)));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Highlight3));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Highlight3), 1, GL_FLOAT, GL_FALSE, sizeof(HighlightVertex), (void *)((4 + 4) * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize VectorArrow VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mVectorArrowVAO = tmpGLuint;

        glBindVertexArray(*mVectorArrowVAO);
        CheckOpenGLError();

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mVectorArrowVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::VectorArrow));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::VectorArrow), 3, GL_FLOAT, GL_FALSE, sizeof(vec3f), (void*)(0));
        CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize Ship texture
    //

    glGenTextures(1, &tmpGLuint);
    mShipTextureOpenGLHandle = tmpGLuint;

    // Bind texture
    mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();
    glBindTexture(GL_TEXTURE_2D, *mShipTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadMipmappedTexture(std::move(shipTexture));

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    CheckOpenGLError();

    // Set filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Set texture parameter
    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetTextureParameters<ProgramType::ShipSpringsTexture>();
    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetTextureParameters<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetTextureParameters<ProgramType::ShipTrianglesTexture>();
    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetTextureParameters<ProgramType::ShipTrianglesTextureWithTemperature>();

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);


    //
    // Initialize StressedSpring texture
    //

    glGenTextures(1, &tmpGLuint);
    mStressedSpringTextureOpenGLHandle = tmpGLuint;

    // Bind texture
    mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();
    glBindTexture(GL_TEXTURE_2D, *mStressedSpringTextureOpenGLHandle);
    CheckOpenGLError();

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    CheckOpenGLError();

    // Set filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Make texture data
    unsigned char buf[] = {
        239, 16, 39, 255,       255, 253, 181,  255,    239, 16, 39, 255,
        255, 253, 181, 255,     239, 16, 39, 255,       255, 253, 181,  255,
        239, 16, 39, 255,       255, 253, 181,  255,    239, 16, 39, 255
    };

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 3, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    CheckOpenGLError();

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);


    //
    // Set parameters to initial values
    //

    OnViewModelUpdated();

    OnEffectiveAmbientLightIntensityUpdated();
    OnLampLightColorUpdated();
    OnWaterColorUpdated();
    OnWaterContrastUpdated();
    OnWaterLevelOfDetailUpdated();
    OnHeatOverlayTransparencyUpdated();
    OnShipFlameSizeAdjustmentUpdated();
}

ShipRenderContext::~ShipRenderContext()
{
}

void ShipRenderContext::OnViewModelUpdated()
{
    // Recalculate ortho matrices
    UpdateOrthoMatrices();
}

void ShipRenderContext::UpdateOrthoMatrices()
{
    //
    // Each plane Z segment is divided into a number of layers, one for each type of rendering we do for a ship:
    //      - 0: Ropes (always behind)
    //      - 1: Flames - background
    //      - 2: Springs
    //      - 3: Triangles
    //          - Triangles are always drawn temporally before ropes and springs though, to avoid anti-aliasing issues
    //      - 4: Stressed springs
    //      - 5: Points
    //      - 6: Flames - foreground
    //      - 7: Sparkles
    //      - 8: Generic textures
    //      - 9: Explosions
    //      - 10: Highlights
    //      - 11: Vectors
    //

    constexpr float ShipRegionZStart = 1.0f; // Far
    constexpr float ShipRegionZWidth = -2.0f; // Near (-1)

    constexpr int NLayers = 12;

    ViewModel::ProjectionMatrix shipOrthoMatrix;

    //
    // Layer 0: Ropes
    //

    mViewModel.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        0,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopesWithTemperature, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 1: Flames - background
    //

    mViewModel.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        1,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipFlamesBackground1>();
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesBackground1, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipFlamesBackground2>();
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesBackground2, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipFlamesBackground3>();
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesBackground3, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);


    //
    // Layer 2: Springs
    //

    mViewModel.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        2,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColorWithTemperature, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTextureWithTemperature, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 3: Triangles
    //

    mViewModel.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        3,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColorWithTemperature, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesDecay>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesDecay, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTextureWithTemperature, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 4: Stressed Springs
    //

    mViewModel.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        4,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipStressedSprings>();
    mShaderManager.SetProgramParameter<ProgramType::ShipStressedSprings, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 5: Points
    //

    mViewModel.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        5,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColorWithTemperature, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 6: Flames - foreground
    //

    mViewModel.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        6,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipFlamesForeground1>();
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesForeground1, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipFlamesForeground2>();
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesForeground2, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipFlamesForeground3>();
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesForeground3, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 7: Sparkles
    //

    mViewModel.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        7,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipSparkles>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSparkles, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 8: Generic Textures
    //

    mViewModel.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        8,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipGenericMipMappedTextures>();
    mShaderManager.SetProgramParameter<ProgramType::ShipGenericMipMappedTextures, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 9: Explosions
    //

    mViewModel.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        9,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipExplosions>();
    mShaderManager.SetProgramParameter<ProgramType::ShipExplosions, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 10: Highlights
    //

    mViewModel.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        10,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipElectricalElementHighlights>();
    mShaderManager.SetProgramParameter<ProgramType::ShipElectricalElementHighlights, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipCircleHighlights>();
    mShaderManager.SetProgramParameter<ProgramType::ShipCircleHighlights, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 11: Vectors
    //

    mViewModel.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        11,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipVectors>();
    mShaderManager.SetProgramParameter<ProgramType::ShipVectors, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);
}

void ShipRenderContext::OnEffectiveAmbientLightIntensityUpdated()
{
    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::EffectiveAmbientLightIntensity>(
        mEffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopesWithTemperature, ProgramParameterType::EffectiveAmbientLightIntensity>(
        mEffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::EffectiveAmbientLightIntensity>(
        mEffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColorWithTemperature, ProgramParameterType::EffectiveAmbientLightIntensity>(
        mEffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::EffectiveAmbientLightIntensity>(
        mEffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTextureWithTemperature, ProgramParameterType::EffectiveAmbientLightIntensity>(
        mEffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::EffectiveAmbientLightIntensity>(
        mEffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColorWithTemperature, ProgramParameterType::EffectiveAmbientLightIntensity>(
        mEffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesDecay>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesDecay, ProgramParameterType::EffectiveAmbientLightIntensity>(
        mEffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::EffectiveAmbientLightIntensity>(
        mEffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTextureWithTemperature, ProgramParameterType::EffectiveAmbientLightIntensity>(
        mEffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::EffectiveAmbientLightIntensity>(
        mEffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColorWithTemperature, ProgramParameterType::EffectiveAmbientLightIntensity>(
        mEffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipGenericMipMappedTextures>();
    mShaderManager.SetProgramParameter<ProgramType::ShipGenericMipMappedTextures, ProgramParameterType::EffectiveAmbientLightIntensity>(
        mEffectiveAmbientLightIntensity);
}

void ShipRenderContext::OnLampLightColorUpdated()
{
    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopesWithTemperature, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColorWithTemperature, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTextureWithTemperature, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColorWithTemperature, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTextureWithTemperature, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColorWithTemperature, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);
}

void ShipRenderContext::OnWaterColorUpdated()
{
    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopesWithTemperature, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColorWithTemperature, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTextureWithTemperature, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColorWithTemperature, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTextureWithTemperature, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColorWithTemperature, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);
}

void ShipRenderContext::OnWaterContrastUpdated()
{
    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopesWithTemperature, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColorWithTemperature, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTextureWithTemperature, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColorWithTemperature, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTextureWithTemperature, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColorWithTemperature, ProgramParameterType::WaterContrast>(
        mWaterContrast);
}

void ShipRenderContext::OnWaterLevelOfDetailUpdated()
{
    // Transform: 0->1 == 2.0->0.01
    float waterLevelThreshold = 2.0f + mWaterLevelOfDetail * (-2.0f + 0.01f);

    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopesWithTemperature, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColorWithTemperature, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTextureWithTemperature, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColorWithTemperature, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTextureWithTemperature, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColorWithTemperature, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);
}

void ShipRenderContext::OnHeatOverlayTransparencyUpdated()
{
    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopesWithTemperature, ProgramParameterType::HeatOverlayTransparency>(
        mHeatOverlayTransparency);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColorWithTemperature, ProgramParameterType::HeatOverlayTransparency>(
        mHeatOverlayTransparency);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTextureWithTemperature, ProgramParameterType::HeatOverlayTransparency>(
        mHeatOverlayTransparency);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColorWithTemperature, ProgramParameterType::HeatOverlayTransparency>(
        mHeatOverlayTransparency);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTextureWithTemperature, ProgramParameterType::HeatOverlayTransparency>(
        mHeatOverlayTransparency);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColorWithTemperature, ProgramParameterType::HeatOverlayTransparency>(
        mHeatOverlayTransparency);
}

void ShipRenderContext::OnShipFlameSizeAdjustmentUpdated()
{
    // Recalculate quad dimensions
    mHalfFlameQuadWidth = BasisHalfFlameQuadWidth * mShipFlameSizeAdjustment;
    mFlameQuadHeight = BasisFlameQuadHeight * mShipFlameSizeAdjustment;
}

//////////////////////////////////////////////////////////////////////////////////

void ShipRenderContext::RenderStart(PlaneId maxMaxPlaneId)
{
    //
    // Reset flames, explosions, air bubbles, generic textures, highlights
    //

    mFlameVertexBuffer.reset();
    mFlameBackgroundCount = 0u;
    mFlameForegroundCount = 0u;

    mExplosionPlaneVertexBuffers.clear();
    mExplosionPlaneVertexBuffers.resize(maxMaxPlaneId + 1);
    mExplosionTotalPlaneVertexCount = 0;

    glBindBuffer(GL_ARRAY_BUFFER, *mGenericMipMappedTextureVBO);
    mAirBubbleVertexBuffer.map(mGenericMipMappedTextureVBOAllocatedVertexCount);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    mGenericMipMappedTexturePlaneVertexBuffers.clear();
    mGenericMipMappedTexturePlaneVertexBuffers.resize(maxMaxPlaneId + 1);
    mGenericMipMappedTextureTotalPlaneVertexCount = 0;

    for (size_t i = 0; i <= static_cast<size_t>(HighlightMode::_Last); ++i)
        mHighlightVertexBuffers[i].clear();


    //
    // Check if the max ever plane ID has changed
    //

    if (maxMaxPlaneId != mMaxMaxPlaneId)
    {
        // Update value
        mMaxMaxPlaneId = maxMaxPlaneId;

        // Recalculate view model parameters
        OnViewModelUpdated();
    }
}

void ShipRenderContext::UploadPointImmutableAttributes(vec2f const * textureCoordinates)
{
    // Interleave texture coordinates into AttributeGroup1 buffer;
    // wait to upload it until we also get positions
    vec4f * restrict pDst = mPointAttributeGroup1Buffer.get();
    vec2f const * restrict pSrc = textureCoordinates;
    for (size_t i = 0; i < mPointCount; ++i)
    {
        pDst[i].z = pSrc[i].x;
        pDst[i].w = pSrc[i].y;
    }
}

void ShipRenderContext::UploadPointMutableAttributesStart()
{
}

void ShipRenderContext::UploadPointMutableAttributes(
    vec2f const * position,
    float const * light,
    float const * water,
    size_t lightAndWaterCount)
{
    // Interleave positions into AttributeGroup1 buffer
    vec4f * restrict pDst1 = mPointAttributeGroup1Buffer.get();
    vec2f const * restrict pSrc = position;
    for (size_t i = 0; i < mPointCount; ++i)
    {
        pDst1[i].x = pSrc[i].x;
        pDst1[i].y = pSrc[i].y;
    }

    // Upload AttributeGroup1 buffer
    glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup1VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(vec4f), mPointAttributeGroup1Buffer.get());
    CheckOpenGLError();

    // Interleave light and water into AttributeGroup2 buffer;
    // wait to upload it until we know whether the other attributes
    // have been uploaded (or not)
    vec4f * restrict pDst2 = mPointAttributeGroup2Buffer.get();
    float const * restrict pSrc1 = light;
    float const * restrict pSrc2 = water;
    for (size_t i = 0; i < lightAndWaterCount; ++i)
    {
        pDst2[i].x = pSrc1[i];
        pDst2[i].y = pSrc2[i];
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadPointMutableAttributesPlaneId(
    float const * planeId,
    size_t startDst,
    size_t count)
{
    // Interleave plane ID into AttributeGroup2 buffer
    vec4f * restrict pDst = &(mPointAttributeGroup2Buffer.get()[startDst]);
    float const * restrict pSrc = planeId;
    for (size_t i = 0; i < count; ++i)
        pDst[i].z = pSrc[i];
}

void ShipRenderContext::UploadPointMutableAttributesDecay(
    float const * decay,
    size_t startDst,
    size_t count)
{
    // Interleave decay into AttributeGroup2 buffer
    vec4f * restrict pDst = &(mPointAttributeGroup2Buffer.get()[startDst]);
    float const * restrict pSrc = decay;
    for (size_t i = 0; i < count; ++i)
        pDst[i].w = pSrc[i];
}

void ShipRenderContext::UploadPointMutableAttributesEnd()
{
    // Upload attribute group buffers
    glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup2VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(vec4f), mPointAttributeGroup2Buffer.get());
    CheckOpenGLError();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadPointColors(
    vec4f const * color,
    size_t startDst,
    size_t count)
{
    assert(startDst + count <= mPointCount);

    // Upload color range
    glBindBuffer(GL_ARRAY_BUFFER, *mPointColorVBO);
    glBufferSubData(GL_ARRAY_BUFFER, startDst * sizeof(vec4f), count * sizeof(vec4f), color);
    CheckOpenGLError();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadPointTemperature(
    float const * temperature,
    size_t startDst,
    size_t count)
{
    assert(startDst + count <= mPointCount);

    // Upload temperature range
    glBindBuffer(GL_ARRAY_BUFFER, *mPointTemperatureVBO);
    glBufferSubData(GL_ARRAY_BUFFER, startDst * sizeof(float), count * sizeof(float), temperature);
    CheckOpenGLError();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadElementsStart()
{
    // Empty all buffers - except triangles - as elements will be completely re-populated soon
    // (with a yet-unknown quantity of elements);
    //
    // if the client does not upload new triangles, it means we have to reuse the last known set

    mPointElementBuffer.clear();
    mSpringElementBuffer.clear();
    mRopeElementBuffer.clear();
    mStressedSpringElementBuffer.clear();
}

void ShipRenderContext::UploadElementTrianglesStart(size_t trianglesCount)
{
    // Client wants to upload a new set of triangles
    //
    // No need to clear, we'll repopulate everything

    mTriangleElementBuffer.resize(trianglesCount);
}

void ShipRenderContext::UploadElementTrianglesEnd()
{
}

void ShipRenderContext::UploadElementsEnd(bool doFinalizeEphemeralPoints)
{
    //
    // Upload all elements to the VBO, remembering the starting VBO index
    // of each element type
    //

    // Note: byte-granularity indices
    mTriangleElementVBOStartIndex = 0;
    mRopeElementVBOStartIndex = mTriangleElementVBOStartIndex + mTriangleElementBuffer.size() * sizeof(TriangleElement);
    mSpringElementVBOStartIndex = mRopeElementVBOStartIndex + mRopeElementBuffer.size() * sizeof(LineElement);
    mPointElementVBOStartIndex = mSpringElementVBOStartIndex + mSpringElementBuffer.size() * sizeof(LineElement);
    mEphemeralPointElementVBOStartIndex = mPointElementVBOStartIndex + mPointElementBuffer.size() * sizeof(PointElement);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mElementVBO);

    // Allocate whole buffer, including room for all possible ephemeral points
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        mEphemeralPointElementVBOStartIndex + GameParameters::MaxEphemeralParticles * sizeof(PointElement),
        nullptr,
        GL_STATIC_DRAW);
    CheckOpenGLError();

    // Upload triangles
    glBufferSubData(
        GL_ELEMENT_ARRAY_BUFFER,
        mTriangleElementVBOStartIndex,
        mTriangleElementBuffer.size() * sizeof(TriangleElement),
        mTriangleElementBuffer.data());

    // Upload ropes
    glBufferSubData(
        GL_ELEMENT_ARRAY_BUFFER,
        mRopeElementVBOStartIndex,
        mRopeElementBuffer.size() * sizeof(LineElement),
        mRopeElementBuffer.data());

    // Upload springs
    glBufferSubData(
        GL_ELEMENT_ARRAY_BUFFER,
        mSpringElementVBOStartIndex,
        mSpringElementBuffer.size() * sizeof(LineElement),
        mSpringElementBuffer.data());

    // Upload points
    glBufferSubData(
        GL_ELEMENT_ARRAY_BUFFER,
        mPointElementVBOStartIndex,
        mPointElementBuffer.size() * sizeof(PointElement),
        mPointElementBuffer.data());

    // Upload the ephemeral points that we know about, provided
    // that there aren't new ephemeral points coming; otherwise
    // we'll upload these later
    if (doFinalizeEphemeralPoints)
    {
        glBufferSubData(
            GL_ELEMENT_ARRAY_BUFFER,
            mEphemeralPointElementVBOStartIndex,
            mEphemeralPointElementBuffer.size() * sizeof(PointElement),
            mEphemeralPointElementBuffer.data());
    }

    CheckOpenGLError();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadElementStressedSpringsStart()
{
    // Empty buffer
    mStressedSpringElementBuffer.clear();
}

void ShipRenderContext::UploadElementStressedSpringsEnd()
{
    //
    // Upload stressed spring elements
    //

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mStressedSpringElementVBO);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        mStressedSpringElementBuffer.size() * sizeof(LineElement),
        mStressedSpringElementBuffer.data(),
        GL_STREAM_DRAW);
    CheckOpenGLError();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadFlamesStart(
    size_t count,
    float windSpeedMagnitude)
{
    //
    // Prepare buffer - map flame VBO's
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mFlameVertexVBO);

    if (count > mFlameVertexBufferAllocatedSize
        || count < mFlameVertexBufferAllocatedSize - 100)
    {
        // Reallocate
        mFlameVertexBufferAllocatedSize = ((count / 100) + 1) * 100;
        glBufferData(GL_ARRAY_BUFFER, mFlameVertexBufferAllocatedSize * 6 * sizeof(FlameVertex), nullptr, GL_STREAM_DRAW);

    }

    // Map buffer
    mFlameVertexBuffer.map_and_fill(count * 6);
    CheckOpenGLError();


    glBindBuffer(GL_ARRAY_BUFFER, 0);


    //
    // Update wind speed
    //

    float newWind = mWindSpeedMagnitudeRunningAverage.Update(windSpeedMagnitude);

    // Set wind speed magnitude parameter, if it has changed
    if (newWind != mCurrentWindSpeedMagnitudeAverage)
    {
        // Calculate wind angle: we do this here once instead of doing it for each and every pixel
        float const windRotationAngle = std::copysign(
            0.6f * SmoothStep(0.0f, 100.0f, std::abs(newWind)),
            -newWind);

        switch (mShipFlameRenderMode)
        {
            case ShipFlameRenderMode::Mode1:
            {
                mShaderManager.ActivateProgram<ProgramType::ShipFlamesBackground1>();
                mShaderManager.SetProgramParameter<ProgramType::ShipFlamesBackground1, ProgramParameterType::FlameWindRotationAngle>(
                    windRotationAngle);

                mShaderManager.ActivateProgram<ProgramType::ShipFlamesForeground1>();
                mShaderManager.SetProgramParameter<ProgramType::ShipFlamesForeground1, ProgramParameterType::FlameWindRotationAngle>(
                    windRotationAngle);

                break;
            }

            case ShipFlameRenderMode::Mode2:
            {
                mShaderManager.ActivateProgram<ProgramType::ShipFlamesBackground2>();
                mShaderManager.SetProgramParameter<ProgramType::ShipFlamesBackground2, ProgramParameterType::FlameWindRotationAngle>(
                    windRotationAngle);

                mShaderManager.ActivateProgram<ProgramType::ShipFlamesForeground2>();
                mShaderManager.SetProgramParameter<ProgramType::ShipFlamesForeground2, ProgramParameterType::FlameWindRotationAngle>(
                    windRotationAngle);

                break;
            }

            case ShipFlameRenderMode::Mode3:
            {
                mShaderManager.ActivateProgram<ProgramType::ShipFlamesBackground3>();
                mShaderManager.SetProgramParameter<ProgramType::ShipFlamesBackground3, ProgramParameterType::FlameWindRotationAngle>(
                    windRotationAngle);

                mShaderManager.ActivateProgram<ProgramType::ShipFlamesForeground3>();
                mShaderManager.SetProgramParameter<ProgramType::ShipFlamesForeground3, ProgramParameterType::FlameWindRotationAngle>(
                    windRotationAngle);

                break;
            }

            case ShipFlameRenderMode::NoDraw:
            {
                break;
            }
        }

        mCurrentWindSpeedMagnitudeAverage = newWind;
    }
}

void ShipRenderContext::UploadFlamesEnd()
{
    assert((mFlameBackgroundCount + mFlameForegroundCount) * 6u == mFlameVertexBuffer.size());

    // Unmap flame VBO's
    glBindBuffer(GL_ARRAY_BUFFER, *mFlameVertexVBO);
    mFlameVertexBuffer.unmap();
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadSparklesStart()
{
    // Empty buffer
    mSparkleVertexBuffer.clear();
}

void ShipRenderContext::UploadSparklesEnd()
{
    //
    // Upload to VBO
    //

    if (!mSparkleVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mSparkleVertexVBO);

        glBufferData(
            GL_ARRAY_BUFFER,
            mSparkleVertexBuffer.size() * sizeof(SparkleVertex),
            mSparkleVertexBuffer.data(),
            GL_DYNAMIC_DRAW);
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void ShipRenderContext::UploadElementEphemeralPointsStart()
{
    // Empty buffer
    mEphemeralPointElementBuffer.clear();
}

void ShipRenderContext::UploadElementEphemeralPointsEnd()
{
    //
    // Upload ephemeral point elements to the end of the element VBO
    //

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mElementVBO);

    glBufferSubData(
        GL_ELEMENT_ARRAY_BUFFER,
        mEphemeralPointElementVBOStartIndex,
        mEphemeralPointElementBuffer.size() * sizeof(PointElement),
        mEphemeralPointElementBuffer.data());
    CheckOpenGLError();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadVectors(
    size_t count,
    vec2f const * position,
    float const * planeId,
    vec2f const * vector,
    float lengthAdjustment,
    vec4f const & color)
{
    static float const CosAlphaLeftRight = cos(-2.f * Pi<float> / 8.f);
    static float const SinAlphaLeft = sin(-2.f * Pi<float> / 8.f);
    static float const SinAlphaRight = -SinAlphaLeft;

    static vec2f const XMatrixLeft = vec2f(CosAlphaLeftRight, SinAlphaLeft);
    static vec2f const YMatrixLeft = vec2f(-SinAlphaLeft, CosAlphaLeftRight);
    static vec2f const XMatrixRight = vec2f(CosAlphaLeftRight, SinAlphaRight);
    static vec2f const YMatrixRight = vec2f(-SinAlphaRight, CosAlphaLeftRight);

    //
    // Create buffer with endpoint positions of each segment of each arrow
    //

    mVectorArrowVertexBuffer.clear();
    mVectorArrowVertexBuffer.reserve(count * 3 * 2);

    for (size_t i = 0; i < count; ++i)
    {
        // Stem
        vec2f stemEndpoint = position[i] + vector[i] * lengthAdjustment;
        mVectorArrowVertexBuffer.emplace_back(position[i], planeId[i]);
        mVectorArrowVertexBuffer.emplace_back(stemEndpoint, planeId[i]);

        // Left
        vec2f leftDir = vec2f(-vector[i].dot(XMatrixLeft), -vector[i].dot(YMatrixLeft)).normalise();
        mVectorArrowVertexBuffer.emplace_back(stemEndpoint, planeId[i]);
        mVectorArrowVertexBuffer.emplace_back(stemEndpoint + leftDir * 0.2f, planeId[i]);

        // Right
        vec2f rightDir = vec2f(-vector[i].dot(XMatrixRight), -vector[i].dot(YMatrixRight)).normalise();
        mVectorArrowVertexBuffer.emplace_back(stemEndpoint, planeId[i]);
        mVectorArrowVertexBuffer.emplace_back(stemEndpoint + rightDir * 0.2f, planeId[i]);
    }


    //
    // Upload buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mVectorArrowVBO);
    glBufferData(GL_ARRAY_BUFFER, mVectorArrowVertexBuffer.size() * sizeof(vec3f), mVectorArrowVertexBuffer.data(), GL_DYNAMIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    //
    // Manage color
    //

    if (mVectorArrowColor != color)
    {
        mShaderManager.ActivateProgram<ProgramType::ShipVectors>();
        mShaderManager.SetProgramParameter<ProgramType::ShipVectors, ProgramParameterType::MatteColor>(
            color.x,
            color.y,
            color.z,
            color.w);

        mVectorArrowColor = color;
    }
}

void ShipRenderContext::RenderEnd()
{
    //
    // Render background flames
    //

    if (mShipFlameRenderMode == ShipFlameRenderMode::Mode1)
    {
        RenderFlames<ProgramType::ShipFlamesBackground1>(
            0,
            mFlameBackgroundCount);
    }
    else if (mShipFlameRenderMode == ShipFlameRenderMode::Mode2)
    {
        RenderFlames<ProgramType::ShipFlamesBackground2>(
            0,
            mFlameBackgroundCount);
    }
    else if (mShipFlameRenderMode == ShipFlameRenderMode::Mode3)
    {
        RenderFlames<ProgramType::ShipFlamesBackground3>(
            0,
            mFlameBackgroundCount);
    }


    //
    // Draw ship elements
    //

    glBindVertexArray(*mShipVAO);

    {
        //
        // Bind element VBO
        //
        // NOTE: Intel drivers have a bug in the VAO ARB: they do not store the ELEMENT_ARRAY_BUFFER binding
        // in the VAO
        //

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mElementVBO);


        //
        // Bind ship texture
        //

        assert(!!mShipTextureOpenGLHandle);

        mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();
        glBindTexture(GL_TEXTURE_2D, *mShipTextureOpenGLHandle);



        //
        // Draw triangles
        //
        // Best to draw triangles (temporally) before springs and ropes, otherwise
        // the latter, which use anti-aliasing, would end up being contoured with background
        // when drawn Z-ally over triangles
        //
        // Also, edge springs might just contain transparent pixels (when textured), which
        // would result in the same artifact
        //

        if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe
            || mDebugShipRenderMode == DebugShipRenderMode::Decay
            || mDebugShipRenderMode == DebugShipRenderMode::None)
        {
            if (mDebugShipRenderMode == DebugShipRenderMode::Decay)
            {
                // Use decay program
                mShaderManager.ActivateProgram<ProgramType::ShipTrianglesDecay>();
            }
            else
            {
                if (mShipRenderMode == ShipRenderMode::Texture)
                {
                    // Use texture program
                    if (mDrawHeatOverlay)
                        mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
                    else
                        mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
                }
                else
                {
                    // Use color program
                    if (mDrawHeatOverlay)
                        mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
                    else
                        mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
                }
            }

            if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
                glLineWidth(0.1f);

            // Draw!
            glDrawElements(
                GL_TRIANGLES,
                static_cast<GLsizei>(3 * mTriangleElementBuffer.size()),
                GL_UNSIGNED_INT,
                (GLvoid *)mTriangleElementVBOStartIndex);

            // Update stats
            mRenderStatistics.LastRenderedShipTriangles += mTriangleElementBuffer.size();
        }



        //
        // Set line width, for ropes and springs
        //

        glLineWidth(0.1f * 2.0f * mViewModel.GetCanvasToVisibleWorldHeightRatio());



        //
        // Draw ropes, unless it's a debug mode
        //
        // Note: when DebugRenderMode is springs|edgeSprings, ropes would all be uploaded
        // as springs.
        //

        if (mDebugShipRenderMode == DebugShipRenderMode::None)
        {
            if (mDrawHeatOverlay)
                mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
            else
                mShaderManager.ActivateProgram<ProgramType::ShipRopes>();

            glDrawElements(
                GL_LINES,
                static_cast<GLsizei>(2 * mRopeElementBuffer.size()),
                GL_UNSIGNED_INT,
                (GLvoid *)mRopeElementVBOStartIndex);

            // Update stats
            mRenderStatistics.LastRenderedShipRopes += mRopeElementBuffer.size();
        }



        //
        // Draw springs
        //
        // We draw springs when:
        // - DebugRenderMode is springs|edgeSprings, in which case we use colors - so to show
        //   structural springs -, or
        // - RenderMode is structure (so to draw 1D chains), in which case we use colors, or
        // - RenderMode is texture (so to draw 1D chains), in which case we use texture iff it is present
        //
        // Note: when DebugRenderMode is springs|edgeSprings, ropes would all be here.
        //

        if (mDebugShipRenderMode == DebugShipRenderMode::Springs
            || mDebugShipRenderMode == DebugShipRenderMode::EdgeSprings
            || (mDebugShipRenderMode == DebugShipRenderMode::None
                && (mShipRenderMode == ShipRenderMode::Structure || mShipRenderMode == ShipRenderMode::Texture)))
        {
            if (mDebugShipRenderMode == DebugShipRenderMode::None && mShipRenderMode == ShipRenderMode::Texture)
            {
                // Use texture program
                if (mDrawHeatOverlay)
                    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
                else
                    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
            }
            else
            {
                // Use color program
                if (mDrawHeatOverlay)
                    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
                else
                    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
            }

            glDrawElements(
                GL_LINES,
                static_cast<GLsizei>(2 * mSpringElementBuffer.size()),
                GL_UNSIGNED_INT,
                (GLvoid *)mSpringElementVBOStartIndex);

            // Update stats
            mRenderStatistics.LastRenderedShipSprings += mSpringElementBuffer.size();
        }



        //
        // Draw stressed springs
        //

        if (mShowStressedSprings
            && !mStressedSpringElementBuffer.empty())
        {
            mShaderManager.ActivateProgram<ProgramType::ShipStressedSprings>();

            // Bind stressed spring texture
            mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();
            glBindTexture(GL_TEXTURE_2D, *mStressedSpringTextureOpenGLHandle);
            CheckOpenGLError();

            // Bind stressed spring VBO
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mStressedSpringElementVBO);

            // Draw
            glDrawElements(
                GL_LINES,
                static_cast<GLsizei>(2 * mStressedSpringElementBuffer.size()),
                GL_UNSIGNED_INT,
                (GLvoid *)0);

            // Bind again element VBO
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mElementVBO);
        }



        //
        // Draw points (orphaned/all non-ephemerals, and ephemerals)
        //

        if (mDebugShipRenderMode == DebugShipRenderMode::None
            || mDebugShipRenderMode == DebugShipRenderMode::Points)
        {
            auto const totalPoints = mPointElementBuffer.size() + mEphemeralPointElementBuffer.size();

            if (mDrawHeatOverlay)
                mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
            else
                mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();

            glPointSize(0.3f * mViewModel.GetCanvasToVisibleWorldHeightRatio());

            glDrawElements(
                GL_POINTS,
                static_cast<GLsizei>(1 * totalPoints),
                GL_UNSIGNED_INT,
                (GLvoid *)mPointElementVBOStartIndex);

            // Update stats
            mRenderStatistics.LastRenderedShipPoints += totalPoints;
        }

        // We are done with the ship VAO
        glBindVertexArray(0);
    }



    //
    // Render foreground flames
    //

    if (mShipFlameRenderMode == ShipFlameRenderMode::Mode1)
    {
        RenderFlames<ProgramType::ShipFlamesForeground1>(
            mFlameBackgroundCount,
            mFlameForegroundCount);
    }
    else if (mShipFlameRenderMode == ShipFlameRenderMode::Mode2)
    {
        RenderFlames<ProgramType::ShipFlamesForeground2>(
            mFlameBackgroundCount,
            mFlameForegroundCount);
    }
    else if (mShipFlameRenderMode == ShipFlameRenderMode::Mode3)
    {
        RenderFlames<ProgramType::ShipFlamesForeground3>(
            mFlameBackgroundCount,
            mFlameForegroundCount);
    }



    //
    // Render sparkles
    //

    RenderSparkles();



    //
    // Render generic textures
    //

    RenderGenericMipMappedTextures();



    //
    // Render explosions
    //

    RenderExplosions();



    //
    // Render highlights
    //

    RenderHighlights();



    //
    // Render vectors, if we're asked to
    //

    if (mVectorFieldRenderMode != VectorFieldRenderMode::None)
    {
        RenderVectorArrows();
    }



    //
    // Update stats
    //

    mRenderStatistics.LastRenderedShipPlanes += mMaxMaxPlaneId + 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////

template<ProgramType ShaderProgram>
void ShipRenderContext::RenderFlames(
    size_t startFlameIndex,
    size_t flameCount)
{
    if (flameCount > 0
        && mShipFlameRenderMode != ShipFlameRenderMode::NoDraw)
    {
        glBindVertexArray(*mFlameVAO);

        mShaderManager.ActivateProgram<ShaderProgram>();

        // Set flame speed parameter
        mShaderManager.SetProgramParameter<ShaderProgram, ProgramParameterType::FlameSpeed>(
            GameWallClock::GetInstance().NowAsFloat() * 0.345f);

        // Bind VBO
        glBindBuffer(GL_ARRAY_BUFFER, *mFlameVertexVBO);

        // Render
        if (mShipFlameRenderMode == ShipFlameRenderMode::Mode1)
        {
            glDrawArrays(
                GL_TRIANGLES,
                static_cast<GLint>(startFlameIndex * 6u),
                static_cast<GLint>(flameCount * 6u));
        }
        else
        {
            glDrawArraysInstanced(
                GL_TRIANGLES,
                static_cast<GLint>(startFlameIndex * 6u),
                static_cast<GLint>(flameCount * 6u),
                2); // Without border, with border
        }

        glBindVertexArray(0);

        // Update stats
        mRenderStatistics.LastRenderedShipFlames += flameCount; // # of quads
    }
}

void ShipRenderContext::RenderSparkles()
{
    if (mSparkleVertexBuffer.size() > 0)
    {
        glBindVertexArray(*mSparkleVAO);

        mShaderManager.ActivateProgram<ProgramType::ShipSparkles>();

        if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
            glLineWidth(0.1f);

        glBindBuffer(GL_ARRAY_BUFFER, *mSparkleVertexVBO);

        assert(0 == (mSparkleVertexBuffer.size() % 6));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mSparkleVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void ShipRenderContext::RenderGenericMipMappedTextures()
{
    // Unmap generic texture VBO (which we have mapped regardless of whether or not there
    // are air bubbles)
    glBindBuffer(GL_ARRAY_BUFFER, *mGenericMipMappedTextureVBO);
    mAirBubbleVertexBuffer.unmap();

    //
    // Render
    //

    if (mAirBubbleVertexBuffer.size() > 0
        || mGenericMipMappedTextureTotalPlaneVertexCount > 0)
    {
        glBindVertexArray(*mGenericMipMappedTextureVAO);

        mShaderManager.ActivateProgram<ProgramType::ShipGenericMipMappedTextures>();

        if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
            glLineWidth(0.1f);

        // Bind VBO (need to do this after VAO change)
        glBindBuffer(GL_ARRAY_BUFFER, *mGenericMipMappedTextureVBO);


        //
        // Air bubbles
        //

        if (mAirBubbleVertexBuffer.size() > 0)
        {
            // Render
            assert(0 == (mAirBubbleVertexBuffer.size() % 6));
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mAirBubbleVertexBuffer.size()));

            // Update stats
            mRenderStatistics.LastRenderedShipGenericMipMappedTextures += mAirBubbleVertexBuffer.size() / 6; // # of quads
        }


        //
        // Generic mipmapped textures
        //

        if (mGenericMipMappedTextureTotalPlaneVertexCount > 0)
        {
            //
            // Upload vertex buffers
            //

            // (Re-)Allocate vertex buffer, if needed
            if (mGenericMipMappedTextureVBOAllocatedVertexCount < mGenericMipMappedTextureTotalPlaneVertexCount)
            {
                mGenericMipMappedTextureVBOAllocatedVertexCount = mGenericMipMappedTextureTotalPlaneVertexCount;

                glBufferData(GL_ARRAY_BUFFER, mGenericMipMappedTextureVBOAllocatedVertexCount * sizeof(GenericTextureVertex), nullptr, GL_DYNAMIC_DRAW);
                CheckOpenGLError();
            }

            // Map vertex buffer
            auto mappedBuffer = reinterpret_cast<uint8_t *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
            CheckOpenGLError();

            // Copy all buffers
            for (auto const & plane : mGenericMipMappedTexturePlaneVertexBuffers)
            {
                if (!plane.vertexBuffer.empty())
                {
                    size_t const byteCopySize = plane.vertexBuffer.size() * sizeof(GenericTextureVertex);
                    std::memcpy(mappedBuffer, plane.vertexBuffer.data(), byteCopySize);

                    // Advance
                    mappedBuffer += byteCopySize;
                }
            }

            // Unmap vertex buffer
            glUnmapBuffer(GL_ARRAY_BUFFER);


            //
            // Render
            //

            assert(0 == (mGenericMipMappedTextureTotalPlaneVertexCount % 6));
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mGenericMipMappedTextureTotalPlaneVertexCount));


            //
            // Update stats
            //

            mRenderStatistics.LastRenderedShipGenericMipMappedTextures += mGenericMipMappedTextureTotalPlaneVertexCount / 6;
        }

        glBindVertexArray(0);
    }
}

void ShipRenderContext::RenderExplosions()
{
    if (mExplosionTotalPlaneVertexCount > 0)
    {
        glBindVertexArray(*mExplosionVAO);

        mShaderManager.ActivateProgram<ProgramType::ShipExplosions>();

        if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
            glLineWidth(0.1f);

        glBindBuffer(GL_ARRAY_BUFFER, *mExplosionVBO);

        //
        // Upload to VBO
        //

        // (Re-)Allocate vertex buffer, if needed
        if (mExplosionVBOAllocatedVertexCount < mExplosionTotalPlaneVertexCount)
        {
            mExplosionVBOAllocatedVertexCount = mExplosionTotalPlaneVertexCount;

            glBufferData(GL_ARRAY_BUFFER, mExplosionVBOAllocatedVertexCount * sizeof(ExplosionVertex), nullptr, GL_DYNAMIC_DRAW);
            CheckOpenGLError();
        }

        // Map vertex buffer
        auto mappedBuffer = reinterpret_cast<uint8_t *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
        CheckOpenGLError();

        // Copy all buffers
        for (auto const & plane : mExplosionPlaneVertexBuffers)
        {
            if (!plane.vertexBuffer.empty())
            {
                size_t const byteCopySize = plane.vertexBuffer.size() * sizeof(ExplosionVertex);
                std::memcpy(mappedBuffer, plane.vertexBuffer.data(), byteCopySize);

                // Advance
                mappedBuffer += byteCopySize;
            }
        }

        // Unmap vertex buffer
        glUnmapBuffer(GL_ARRAY_BUFFER);

        //
        // Render
        //

        assert(0 == (mExplosionTotalPlaneVertexCount % 6));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mExplosionTotalPlaneVertexCount));

        glBindVertexArray(0);
    }
}

void ShipRenderContext::RenderHighlights()
{
    for (size_t i = 0; i <= static_cast<size_t>(HighlightMode::_Last); ++i)
    {
        if (!mHighlightVertexBuffers[i].empty())
        {
            //
            // Upload buffer
            //

            glBindBuffer(GL_ARRAY_BUFFER, *mHighlightVertexVBO);

            glBufferData(GL_ARRAY_BUFFER,
                sizeof(HighlightVertex) * mHighlightVertexBuffers[i].size(),
                mHighlightVertexBuffers[i].data(),
                GL_DYNAMIC_DRAW);
            CheckOpenGLError();

            glBindBuffer(GL_ARRAY_BUFFER, 0);


            //
            // Render
            //

            glBindVertexArray(*mHighlightVAO);

            switch (static_cast<HighlightMode>(i))
            {
                case HighlightMode::Circle:
                {
                    mShaderManager.ActivateProgram<ProgramType::ShipCircleHighlights>();
                    break;
                }

                case HighlightMode::ElectricalElement:
                {
                    mShaderManager.ActivateProgram<ProgramType::ShipElectricalElementHighlights>();
                    break;
                }

                default:
                {
                    assert(false);
                    break;
                }
            }

            if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
                glLineWidth(0.1f);

            assert(0 == (mHighlightVertexBuffers[i].size() % 6));
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mHighlightVertexBuffers[i].size()));

            glBindVertexArray(0);
        }
    }
}

void ShipRenderContext::RenderVectorArrows()
{
    glBindVertexArray(*mVectorArrowVAO);

    mShaderManager.ActivateProgram<ProgramType::ShipVectors>();

    glLineWidth(0.5f);

    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mVectorArrowVertexBuffer.size()));

    glBindVertexArray(0);
}

}