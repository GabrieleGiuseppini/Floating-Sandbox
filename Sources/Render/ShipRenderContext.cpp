/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-03-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipRenderContext.h"

#include <Core/GameMath.h>
#include <Core/GameWallClock.h>
#include <Core/Log.h>

#include <algorithm>
#include <cstring>

ShipRenderContext::ShipRenderContext(
    ShipId shipId,
    size_t pointCount,
    size_t shipCount,
    size_t maxEphemeralParticles,
    size_t maxSpringsPerPoint,
    RgbaImageData exteriorViewImage,
    RgbaImageData interiorViewImage,
    ShaderManager<GameShaderSets::ShaderSet> & shaderManager,
    GlobalRenderContext & globalRenderContext,
    RenderParameters const & renderParameters,
    float shipFlameSizeAdjustment,
    float vectorFieldLengthMultiplier)
    : mShaderManager(shaderManager)
    , mGlobalRenderContext(globalRenderContext)
    //
    , mShipId(shipId)
    , mPointCount(pointCount)
    , mShipCount(shipCount)
    , mMaxMaxPlaneId(0)
    , mIsViewModelDirty(false)
    // Buffers
    , mPointAttributeGroup1Buffer()
    , mPointAttributeGroup1VBO()
    , mPointAttributeGroup2Buffer()
    , mPointAttributeGroup2VBO()
    , mPointColorVBO()
    , mPointTemperatureVBO()
    , mPointStressVBO()
    , mPointAuxiliaryDataVBO()
    , mPointFrontierColorVBO()
    //
    , mStressedSpringElementBuffer()
    , mStressedSpringElementVBO()
    , mStressedSpringElementVBOAllocatedElementSize(0u)
    //
    , mFrontierEdgeElementBuffer()
    , mIsFrontierEdgeElementBufferDirty(true)
    , mFrontierEdgeElementVBO()
    , mFrontierEdgeElementVBOAllocatedElementSize(0u)
    //
    , mNpcPositionBuffer()
    , mNpcPositionVBO()
    , mNpcPositionVBOAllocatedVertexSize(0)
    , mNpcAttributesVertexBuffer()
    , mNpcAttributesVertexVBO()
    , mNpcAttributesVertexVBOAllocatedVertexSize(0)
    , mNpcQuadRoleVertexBuffer()
    , mNpcQuadRoleVertexVBO()
    , mNpcQuadRoleVertexVBOAllocatedVertexSize(0)
    //
    , mElectricSparkVertexBuffer()
    , mElectricSparkVBO()
    , mElectricSparkVBOAllocatedVertexSize(0u)
    //
    , mFlameVertexBuffer()
    , mFlameBackgroundCount(0u)
    , mFlameForegroundCount(0u)
    , mFlameVBO()
    , mFlameVBOAllocatedVertexSize(0u)
    //
    , mJetEngineFlameVertexBuffer()
    , mJetEngineFlameVBO()
    , mJetEngineFlameVBOAllocatedVertexSize(0u)
    //
    , mExplosionPlaneVertexBuffers()
    , mExplosionTotalVertexCount(0u)
    , mExplosionVBO()
    , mExplosionVBOAllocatedVertexSize(0u)
    //
    , mSparkleVertexBuffer()
    , mSparkleVBO()
    , mSparkleVBOAllocatedVertexSize(0u)
    //
    , mGenericMipMappedTextureAirBubbleVertexBuffer()
    , mGenericMipMappedTexturePlaneVertexBuffers()
    , mGenericMipMappedTextureTotalVertexCount(0u)
    , mGenericMipMappedTextureVBO()
    , mGenericMipMappedTextureVBOAllocatedVertexSize(0u)
    //
    , mHighlightVertexBuffers()
    , mHighlightVBO()
    , mHighlightVBOAllocatedVertexSize(0u)
    //
    , mVectorArrowVertexBuffer()
    , mVectorArrowVBO()
    , mVectorArrowVBOAllocatedVertexSize(0u)
    , mVectorArrowColor(0.0f, 0.0f, 0.0f, 1.0f)
    , mIsVectorArrowColorDirty(true)
    //
    , mCenterVertexBuffer()
    , mIsCenterVertexBufferDirty(true)
    , mCenterVBO()
    , mCenterVBOAllocatedVertexSize(0u)
    //
    , mPointToPointArrowVertexBuffer()
    , mIsPointToPointArrowsVertexBufferDirty(true)
    , mPointToPointArrowVBO()
    , mPointToPointArrowVBOAllocatedVertexSize(0u)
    // Element (index) buffers
    , mPointElementBuffer()
    , mEphemeralPointElementBuffer()
    , mSpringElementBuffer()
    , mRopeElementBuffer()
    , mTriangleElementBuffer()
    , mAreElementBuffersDirty(true)
    , mElementVBO()
    , mElementVBOAllocatedIndexSize(0u)
    , mPointElementVBOStartIndex(0)
    , mEphemeralPointElementVBOStartIndex(0)
    , mSpringElementVBOStartIndex(0)
    , mRopeElementVBOStartIndex(0)
    , mTriangleElementVBOStartIndex(0)
    // VAOs
    , mShipVAO()
    , mNpcTextureAndQuadFlatVAO()
    , mNpcQuadWithRolesVAO()
    , mElectricSparkVAO()
    , mFlameVAO()
    , mJetEngineFlameVAO()
    , mExplosionVAO()
    , mSparkleVAO()
    , mGenericMipMappedTextureVAO()
    , mVectorArrowVAO()
    , mCenterVAO()
    , mPointToPointArrowVAO()
    // Ship structure programs
    , mShipPointsProgram(GameShaderSets::ProgramKind::ShipPointsColor) // Will be recalculated
    , mShipRopesProgram(GameShaderSets::ProgramKind::ShipRopes) // Will be recalculated
    , mShipSpringsProgram(GameShaderSets::ProgramKind::ShipSpringsColor) // Will be recalculated
    , mShipTrianglesProgram(GameShaderSets::ProgramKind::ShipTrianglesColor) // Will be recalculated
    // Textures
    , mExteriorViewImage(std::move(exteriorViewImage))
    , mInteriorViewImage(std::move(interiorViewImage))
    , mShipViewModeType(ShipViewModeType::Exterior) // Will be recalculated
    , mShipTextureOpenGLHandle()
    , mStressedSpringTextureOpenGLHandle()
    , mExplosionTextureAtlasMetadata(globalRenderContext.GetExplosionTextureAtlasMetadata())
    , mGenericLinearTextureAtlasMetadata(globalRenderContext.GetGenericLinearTextureAtlasMetadata())
    , mGenericMipMappedTextureAtlasMetadata(globalRenderContext.GetGenericMipMappedTextureAtlasMetadata())
    // Calculated parameters - all of these will be calculated later
    , mPointSize(0.0f)
    // Non-render parameters - all of these will be calculated later
    , mShipFlameHalfQuadWidth(0.0f)
    , mShipFlameQuadHeight(0.0f)
    , mNpcFlameHalfQuadWidth(BasisNpcFlameHalfQuadWidth) // No adjustment at the time of writing
    , mNpcFlameQuadHeight(BasisNpcFlameQuadHeight) // No adjustment at the time of writing
    , mVectorFieldLengthMultiplier(0.0f)
{
    GLuint tmpGLuint;

    // Clear errors
    glGetError();


    //
    // Initialize buffers
    //

    GLuint vbos[22];
    glGenBuffers(22, vbos);
    CheckOpenGLError();

    mPointAttributeGroup1VBO = vbos[0];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup1VBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(vec4f), nullptr, GL_STREAM_DRAW);
    mPointAttributeGroup1Buffer.reset(pointCount);
    std::fill(
        mPointAttributeGroup1Buffer.data(),
        mPointAttributeGroup1Buffer.data() + pointCount,
        vec4f::zero());

    mPointAttributeGroup2VBO = vbos[1];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup2VBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(vec4f), nullptr, GL_STREAM_DRAW);
    mPointAttributeGroup2Buffer.reset_full(pointCount);

    mPointColorVBO = vbos[2];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointColorVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(vec4f), nullptr, GL_STATIC_DRAW);

    mPointTemperatureVBO = vbos[3];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointTemperatureVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(float), nullptr, GL_STREAM_DRAW);

    mPointStressVBO = vbos[4];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointStressVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(float), nullptr, GL_STREAM_DRAW);

    mPointAuxiliaryDataVBO = vbos[5];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointAuxiliaryDataVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(float), nullptr, GL_STREAM_DRAW);

    mPointFrontierColorVBO = vbos[6];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointFrontierColorVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(ColorWithProgress), nullptr, GL_STATIC_DRAW);

    mStressedSpringElementVBO = vbos[7];
    mStressedSpringElementBuffer.reserve(1024); // Arbitrary

    mFrontierEdgeElementVBO = vbos[8];

    mNpcPositionVBO = vbos[9];
    mNpcAttributesVertexVBO = vbos[10];
    mNpcQuadRoleVertexVBO = vbos[11];

    mElectricSparkVBO = vbos[12];

    mFlameVBO = vbos[13];

    mJetEngineFlameVBO = vbos[14];

    mExplosionVBO = vbos[15];

    mSparkleVBO = vbos[16];
    mSparkleVertexBuffer.reserve(256); // Arbitrary

    mGenericMipMappedTextureVBO = vbos[17];

    mHighlightVBO = vbos[18];

    mVectorArrowVBO = vbos[19];

    mCenterVBO = vbos[20];

    mPointToPointArrowVBO = vbos[21];

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Initialize element (index) buffers
    //

    glGenBuffers(1, &tmpGLuint);
    mElementVBO = tmpGLuint;

    mPointElementBuffer.reserve(pointCount);
    mEphemeralPointElementBuffer.reset(maxEphemeralParticles);
    mSpringElementBuffer.reserve(pointCount * maxSpringsPerPoint);
    mRopeElementBuffer.reserve(pointCount); // Arbitrary
    // Nothing for mTriangleElementBuffer, will resize as needed

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
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipPointAttributeGroup1));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipPointAttributeGroup1), 4, GL_FLOAT, GL_FALSE, sizeof(vec4f), (void*)(0));
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup2VBO);
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipPointAttributeGroup2));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipPointAttributeGroup2), 4, GL_FLOAT, GL_FALSE, sizeof(vec4f), (void*)(0));
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, *mPointColorVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipPointColor));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipPointColor), 4, GL_FLOAT, GL_FALSE, sizeof(vec4f), (void*)(0));
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, *mPointTemperatureVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipPointTemperature));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipPointTemperature), 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)(0));
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, *mPointStressVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipPointStress));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipPointStress), 1, GL_FLOAT, GL_FALSE, sizeof(float), (void *)(0));
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, *mPointAuxiliaryDataVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipPointAuxiliaryData));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipPointAuxiliaryData), 1, GL_FLOAT, GL_FALSE, sizeof(float), (void *)(0));
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, *mPointFrontierColorVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipPointFrontierColor));
        static_assert(sizeof(ColorWithProgress) == 4 * sizeof(float));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipPointFrontierColor), 4, GL_FLOAT, GL_FALSE, sizeof(ColorWithProgress), (void *)(0));
        CheckOpenGLError();

        //
        // Associate element VBO
        //

        // NOTE: Intel drivers have a bug in the VAO ARB: they do not store the ELEMENT_ARRAY_BUFFER binding
        // in the VAO. So we won't associate the element VBO here, but rather before each drawing call.
        ////glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mPointElementVBO);
        ////CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize NPC Texture Quad VAO
    //

    {
        // We use these two same VBOs in two different VAO's, as we need to use different VAOs
        // because one buffer is not used in one VAO and thus might not be allocated - which
        // doesn't fly with AMD video cards (thanks Bazzichetto!)
        auto const describeCommonVbosFunc = [&]()
        {
            glBindBuffer(GL_ARRAY_BUFFER, *mNpcPositionVBO);
            static_assert(sizeof(Quad::V.TopLeft) == (2) * sizeof(float));
            glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::NpcAttributeGroup1));
            glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::NpcAttributeGroup1), 2, GL_FLOAT, GL_FALSE, sizeof(Quad::V.TopLeft), (void *)(0));
            CheckOpenGLError();

            glBindBuffer(GL_ARRAY_BUFFER, *mNpcAttributesVertexVBO);
            static_assert(sizeof(NpcAttributesVertex) == (4 + 2 + 1) * sizeof(float));
            glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::NpcAttributeGroup2));
            glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::NpcAttributeGroup2), 4, GL_FLOAT, GL_FALSE, sizeof(NpcAttributesVertex), (void *)(0));
            glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::NpcAttributeGroup3));
            glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::NpcAttributeGroup3), 3, GL_FLOAT, GL_FALSE, sizeof(NpcAttributesVertex), (void *)(4 * sizeof(float)));
            CheckOpenGLError();

            //
            // Associate element VBO
            //

            // NOTE: Intel drivers have a bug in the VAO ARB: they do not store the ELEMENT_ARRAY_BUFFER binding
            // in the VAO. So we won't associate the element VBO here, but rather before each drawing call.
            ////mGlobalRenderContext.GetElementIndices().Bind()
        };

        glGenVertexArrays(1, &tmpGLuint);
        mNpcTextureAndQuadFlatVAO = tmpGLuint;

        glBindVertexArray(*mNpcTextureAndQuadFlatVAO);
        CheckOpenGLError();

        describeCommonVbosFunc();

        glGenVertexArrays(1, &tmpGLuint);
        mNpcQuadWithRolesVAO = tmpGLuint;

        glBindVertexArray(*mNpcQuadWithRolesVAO);
        CheckOpenGLError();

        describeCommonVbosFunc();

        glBindBuffer(GL_ARRAY_BUFFER, *mNpcQuadRoleVertexVBO);
        static_assert(sizeof(NpcQuadRoleVertex) == (3) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::NpcAttributeGroup4));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::NpcAttributeGroup4), 3, GL_FLOAT, GL_FALSE, sizeof(NpcQuadRoleVertex), (void *)(0));
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize Electric Spark VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mElectricSparkVAO = tmpGLuint;

        glBindVertexArray(*mElectricSparkVAO);
        CheckOpenGLError();

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mElectricSparkVBO);
        static_assert(sizeof(ElectricSparkVertex) == (2 + 1 + 1) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ElectricSpark1));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ElectricSpark1), 4, GL_FLOAT, GL_FALSE, sizeof(ElectricSparkVertex), (void *)(0));
        CheckOpenGLError();

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
        glBindBuffer(GL_ARRAY_BUFFER, *mFlameVBO);
        static_assert(sizeof(FlameVertex) == (4 + 3) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Flame1));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Flame1), 4, GL_FLOAT, GL_FALSE, sizeof(FlameVertex), (void*)0);
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Flame2));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Flame2), 3, GL_FLOAT, GL_FALSE, sizeof(FlameVertex), (void*)((4) * sizeof(float)));
        CheckOpenGLError();

        // NOTE: Intel drivers have a bug in the VAO ARB: they do not store the ELEMENT_ARRAY_BUFFER binding
        // in the VAO. So we won't associate the element VBO here, but rather before each drawing call.
        ////mGlobalRenderContext.GetElementIndices().Bind()

        glBindVertexArray(0);
    }

    // Set texture parameters
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipFlamesBackground>();
    mShaderManager.SetTextureParameters<GameShaderSets::ProgramKind::ShipFlamesBackground>();
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipFlamesForeground>();
    mShaderManager.SetTextureParameters<GameShaderSets::ProgramKind::ShipFlamesForeground>();


    //
    // Initialize Jet Engine Flame VAO's
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mJetEngineFlameVAO = tmpGLuint;

        glBindVertexArray(*mJetEngineFlameVAO);

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mJetEngineFlameVBO);
        static_assert(sizeof(JetEngineFlameVertex) == (4 + 2) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::JetEngineFlame1));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::JetEngineFlame1), 4, GL_FLOAT, GL_FALSE, sizeof(JetEngineFlameVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::JetEngineFlame2));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::JetEngineFlame2), 2, GL_FLOAT, GL_FALSE, sizeof(JetEngineFlameVertex), (void *)((4) * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    // Set texture parameters
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipJetEngineFlames>();
    mShaderManager.SetTextureParameters<GameShaderSets::ProgramKind::ShipJetEngineFlames>();


    //
    // Initialize Explosion VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mExplosionVAO = tmpGLuint;

        glBindVertexArray(*mExplosionVAO);

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mExplosionVBO);
        static_assert(sizeof(ExplosionVertex) == (4 + 4 + 3) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Explosion1));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Explosion1), 4, GL_FLOAT, GL_FALSE, sizeof(ExplosionVertex), (void*)0);
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Explosion2));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Explosion2), 4, GL_FLOAT, GL_FALSE, sizeof(ExplosionVertex), (void*)((4) * sizeof(float)));
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Explosion3));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Explosion3), 3, GL_FLOAT, GL_FALSE, sizeof(ExplosionVertex), (void*)((4 + 4) * sizeof(float)));
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
        glBindBuffer(GL_ARRAY_BUFFER, *mSparkleVBO);
        static_assert(sizeof(SparkleVertex) == (4 + 2) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Sparkle1));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Sparkle1), 4, GL_FLOAT, GL_FALSE, sizeof(SparkleVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Sparkle2));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Sparkle2), 2, GL_FLOAT, GL_FALSE, sizeof(SparkleVertex), (void *)((4) * sizeof(float)));
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
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipGenericMipMappedTexture1));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipGenericMipMappedTexture1), 4, GL_FLOAT, GL_FALSE, sizeof(GenericTextureVertex), (void*)0);
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipGenericMipMappedTexture2));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipGenericMipMappedTexture2), 4, GL_FLOAT, GL_FALSE, sizeof(GenericTextureVertex), (void*)((4) * sizeof(float)));
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipGenericMipMappedTexture3));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::ShipGenericMipMappedTexture3), 3, GL_FLOAT, GL_FALSE, sizeof(GenericTextureVertex), (void*)((4 + 4) * sizeof(float)));
        CheckOpenGLError();

        //
        // Associate element VBO
        //

        // NOTE: Intel drivers have a bug in the VAO ARB: they do not store the ELEMENT_ARRAY_BUFFER binding
        // in the VAO. So we won't associate the element VBO here, but rather before each drawing call.
        ////mGlobalRenderContext.GetElementIndices().Bind()

        glBindVertexArray(0);

        //
        // Initialize buffers
        //

        mGenericMipMappedTextureAirBubbleVertexBuffer.reset(maxEphemeralParticles * 4);
    }


    //
    // Initialize Highlight VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mHighlightVAO = tmpGLuint;

        glBindVertexArray(*mHighlightVAO);

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mHighlightVBO);
        static_assert(sizeof(HighlightVertex) == (2 + 2 + 3 + 1 + 1) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Highlight1));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Highlight1), 4, GL_FLOAT, GL_FALSE, sizeof(HighlightVertex), (void *)((0) * sizeof(float)));
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Highlight2));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Highlight2), 4, GL_FLOAT, GL_FALSE, sizeof(HighlightVertex), (void *)((4) * sizeof(float)));
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Highlight3));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Highlight3), 1, GL_FLOAT, GL_FALSE, sizeof(HighlightVertex), (void *)((4 + 4) * sizeof(float)));
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
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::VectorArrow));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::VectorArrow), 3, GL_FLOAT, GL_FALSE, sizeof(vec3f), (void*)(0));
        CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize Center VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mCenterVAO = tmpGLuint;

        glBindVertexArray(*mCenterVAO);
        CheckOpenGLError();

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mCenterVBO);
        static_assert(sizeof(CenterVertex) == (2 + 2 + 1) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Center1));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Center1), 4, GL_FLOAT, GL_FALSE, sizeof(CenterVertex), (void *)(0));
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Center2));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::Center2), 1, GL_FLOAT, GL_FALSE, sizeof(CenterVertex), (void *)((2 + 2) * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize PointToPointArrow VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mPointToPointArrowVAO = tmpGLuint;

        glBindVertexArray(*mPointToPointArrowVAO);
        CheckOpenGLError();

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mPointToPointArrowVBO);
        static_assert(sizeof(PointToPointArrowVertex) == (2 + 1 + 3) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::PointToPointArrow1));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::PointToPointArrow1), 3, GL_FLOAT, GL_FALSE, sizeof(PointToPointArrowVertex), (void *)(0));
        glEnableVertexAttribArray(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::PointToPointArrow2));
        glVertexAttribPointer(static_cast<GLuint>(GameShaderSets::VertexAttributeKind::PointToPointArrow2), 3, GL_FLOAT, GL_FALSE, sizeof(PointToPointArrowVertex), (void *)((3) * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize StressedSpring texture
    //

    {
        glGenTextures(1, &tmpGLuint);
        mStressedSpringTextureOpenGLHandle = tmpGLuint;

        // Bind texture
        mShaderManager.ActivateTexture<GameShaderSets::ProgramParameterKind::SharedTexture>();
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
    }


    //
    // Set initial values of non-render parameters from which
    // other parameters are calculated
    //

    SetShipFlameSizeAdjustment(shipFlameSizeAdjustment);
    SetVectorFieldLengthMultiplier(vectorFieldLengthMultiplier);


    //
    // Update parameters for initial values
    //

    ApplyShipViewModeChanges(renderParameters);
    ApplyShipStructureRenderModeChanges(renderParameters);
    ApplyViewModelChanges(renderParameters);
    ApplyEffectiveAmbientLightIntensityChanges(renderParameters);
    ApplyDepthDarkeningSensitivityChanges(renderParameters);
    ApplySkyChanges(renderParameters);
    ApplyFlatLampLightColorChanges(renderParameters);
    ApplyShipFlameRenderParameterChanges(renderParameters);
    ApplyWaterColorChanges(renderParameters);
    ApplyWaterContrastChanges(renderParameters);
    ApplyWaterLevelOfDetailChanges(renderParameters);
    ApplyHeatSensitivityChanges(renderParameters);
    ApplyStressRenderModeChanges(renderParameters);
    ApplyNpcRenderModeChanges(renderParameters);
}

ShipRenderContext::~ShipRenderContext()
{
}

//////////////////////////////////////////////////////////////////////////////////

void ShipRenderContext::UploadStart(PlaneId maxMaxPlaneId)
{
    //
    // Reset explosions, sparkles, air bubbles, generic textures, highlights,
    // vector arrows;
    // they are all uploaded as needed
    //

    {
        size_t const newSize = static_cast<size_t>(maxMaxPlaneId) + 1u;
        assert(mExplosionPlaneVertexBuffers.size() <= newSize);

        size_t const clearCount = mExplosionPlaneVertexBuffers.size();
        for (size_t i = 0; i < clearCount; ++i)
        {
            mExplosionPlaneVertexBuffers[i].vertexBuffer.clear();
        }

        if (newSize != mExplosionPlaneVertexBuffers.size())
            mExplosionPlaneVertexBuffers.resize(newSize);
    }


    mSparkleVertexBuffer.clear();

    {
        // Air bubbles

        mGenericMipMappedTextureAirBubbleVertexBuffer.clear();

        // Generic mip-mapped

        size_t const newSize = static_cast<size_t>(maxMaxPlaneId) + 1u;
        assert(mGenericMipMappedTexturePlaneVertexBuffers.size() <= newSize);

        size_t const clearCount = mGenericMipMappedTexturePlaneVertexBuffers.size();
        for (size_t i = 0; i < clearCount; ++i)
        {
            mGenericMipMappedTexturePlaneVertexBuffers[i].vertexBuffer.clear();
        }

        if (newSize != mGenericMipMappedTexturePlaneVertexBuffers.size())
            mGenericMipMappedTexturePlaneVertexBuffers.resize(newSize);
    }

    for (size_t i = 0; i <= static_cast<size_t>(HighlightModeType::_Last); ++i)
    {
        mHighlightVertexBuffers[i].clear();
    }

    mVectorArrowVertexBuffer.clear();

    //
    // Check if the max max plane ID has changed
    //

    if (maxMaxPlaneId != mMaxMaxPlaneId)
    {
        // Update value
        mMaxMaxPlaneId = maxMaxPlaneId;
        mIsViewModelDirty = true;
    }
}

void ShipRenderContext::UploadPointImmutableAttributes(vec2f const * textureCoordinates)
{
    // Uploaded only once, but we treat them as if they could
    // be uploaded any time

    // Interleave texture coordinates into AttributeGroup1 buffer
    vec4f * restrict pDst = mPointAttributeGroup1Buffer.data();
    vec2f const * restrict pSrc = textureCoordinates;
    for (size_t i = 0; i < mPointCount; ++i)
    {
        pDst[i].z = pSrc[i].x;
        pDst[i].w = pSrc[i].y;
    }
}

void ShipRenderContext::UploadPointMutableAttributesStart()
{
    // Nop
}

void ShipRenderContext::UploadPointMutableAttributes(
    vec2f const * position,
    float const * light,
    float const * water)
{
    // Uploaded at each cycle

    // Interleave positions into AttributeGroup1 buffer, and
    // light and water into AttributeGroup2 buffer
    vec2f const * const restrict pSrc1 = position;
    float const * const restrict pSrc2 = light;
    float const * const restrict pSrc3 = water;
    vec4f * restrict const pDst1 = mPointAttributeGroup1Buffer.data();
    vec4f * restrict const pDst2 = mPointAttributeGroup2Buffer.data();
    for (size_t i = 0; i < mPointCount; ++i)
    {
        pDst1[i].x = pSrc1[i].x;
        pDst1[i].y = pSrc1[i].y;

        pDst2[i].x = pSrc2[i];
        pDst2[i].y = pSrc3[i];
    }
}

void ShipRenderContext::UploadPointMutableAttributesPlaneId(
    float const * planeId,
    size_t startDst,
    size_t count)
{
    // Uploaded sparingly, but we treat them as if they could
    // be uploaded at any time

    // Interleave plane ID into AttributeGroup2 buffer
    assert(startDst + count <= mPointCount);
    vec4f * restrict pDst = &(mPointAttributeGroup2Buffer.data()[startDst]);
    float const * restrict pSrc = planeId;
    for (size_t i = 0; i < count; ++i)
        pDst[i].z = pSrc[i];
}

void ShipRenderContext::UploadPointMutableAttributesDecay(
    float const * decay,
    size_t startDst,
    size_t count)
{
    // Uploaded sparingly, but we treat them as if they could
    // be uploaded at any time

    // Interleave decay into AttributeGroup2 buffer
    assert(startDst + count <= mPointCount);
    vec4f * restrict pDst = &(mPointAttributeGroup2Buffer.data()[startDst]);
    float const * restrict pSrc = decay;
    for (size_t i = 0; i < count; ++i)
        pDst[i].w = pSrc[i];
}

void ShipRenderContext::UploadPointMutableAttributesEnd()
{
    // Nop
}

void ShipRenderContext::UploadPointColors(
    vec4f const * color,
    size_t startDst,
    size_t count)
{
    // Uploaded sparingly

    // We've been invoked on the render thread

    //
    // Upload color range
    //

    assert(startDst + count <= mPointCount);

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
    // We've been invoked on the render thread

    //
    // Upload temperature range
    //

    assert(startDst + count <= mPointCount);

    glBindBuffer(GL_ARRAY_BUFFER, *mPointTemperatureVBO);

    glBufferSubData(GL_ARRAY_BUFFER, startDst * sizeof(float), count * sizeof(float), temperature);
    CheckOpenGLError();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadPointStress(
    float const * stress,
    size_t startDst,
    size_t count)
{
    // We've been invoked on the render thread

    //
    // Upload stress range
    //

    assert(startDst + count <= mPointCount);

    glBindBuffer(GL_ARRAY_BUFFER, *mPointStressVBO);

    glBufferSubData(GL_ARRAY_BUFFER, startDst * sizeof(float), count * sizeof(float), stress);
    CheckOpenGLError();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadPointAuxiliaryData(
    float const * auxiliaryData,
    size_t startDst,
    size_t count)
{
    // We've been invoked on the render thread

    //
    // Upload aux data
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mPointAuxiliaryDataVBO);

    glBufferSubData(GL_ARRAY_BUFFER, startDst * sizeof(float), count * sizeof(float), auxiliaryData);
    CheckOpenGLError();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadPointFrontierColors(ColorWithProgress const * colors)
{
    // Uploaded sparingly

    // We've been invoked on the render thread

    glBindBuffer(GL_ARRAY_BUFFER, *mPointFrontierColorVBO);

    glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(ColorWithProgress), colors);
    CheckOpenGLError();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadElementsStart()
{
    // Elements are uploaded sparingly

    // Empty all buffers - except triangles - as elements will be completely re-populated soon
    // (with a yet-unknown quantity of elements);
    //
    // If the client does not upload new triangles, it means we have to reuse the last known set

    mPointElementBuffer.clear();
    mSpringElementBuffer.clear();
    mRopeElementBuffer.clear();
    mAreElementBuffersDirty = true;
}

void ShipRenderContext::UploadElementTrianglesStart(size_t trianglesCount)
{
    // Client wants to upload a new set of triangles
    //
    // No need to clear, we'll repopulate everything

    mTriangleElementBuffer.reset_full(trianglesCount);
}

void ShipRenderContext::UploadElementTrianglesEnd()
{
    // Nop
}

void ShipRenderContext::UploadElementsEnd()
{
    // Nop
}

void ShipRenderContext::UploadElementStressedSpringsStart()
{
    //
    // Stressed springs are not sticky: we upload them at each frame,
    // though they will be empty most of the time
    //

    mStressedSpringElementBuffer.clear();
}

void ShipRenderContext::UploadElementStressedSpringsEnd()
{
    // Nop
}

void ShipRenderContext::UploadElementFrontierEdgesStart(size_t edgesCount)
{
    //
    // Frontier points are sticky: we upload them once in a while and reuse
    // them as needed
    //

    // No need to clear, we'll repopulate everything
    mFrontierEdgeElementBuffer.reset(edgesCount);
    mIsFrontierEdgeElementBufferDirty = true;
}

void ShipRenderContext::UploadElementFrontierEdgesEnd()
{
    // Nop
}

void ShipRenderContext::UploadNpcsStart(size_t maxQuadCount)
{
    //
    // NPC quads are not sticky: we upload them at each frame
    //

    //
    // Prepare buffers and indices
    //

    mNpcPositionBuffer.reset(maxQuadCount);
    mNpcAttributesVertexBuffer.reset(maxQuadCount * 4);
    mNpcQuadRoleVertexBuffer.reset(maxQuadCount * 4);

    mGlobalRenderContext.GetElementIndices().EnsureSize(maxQuadCount);
}

void ShipRenderContext::UploadNpcsEnd()
{
    // Nop
}

void ShipRenderContext::UploadElectricSparksStart(size_t count)
{
    //
    // Electric sparks are not sticky: we upload them at each frame
    //

    mElectricSparkVertexBuffer.reset(6 * count);
}

void ShipRenderContext::UploadElectricSparksEnd()
{
    // Nop
}

void ShipRenderContext::UploadFlamesStart(size_t count)
{
    //
    // Flames are not sticky: we upload them at each frame,
    // though they will be empty most of the time
    //

    mFlameVertexBuffer.reset(4 * count);

    mFlameBackgroundCount = 0;
    mFlameForegroundCount = 0;

    mGlobalRenderContext.GetElementIndices().EnsureSize(count);
}

void ShipRenderContext::UploadFlamesEnd()
{
    assert((mFlameBackgroundCount + mFlameForegroundCount) * 4u == mFlameVertexBuffer.size());

    // Nop
}

void ShipRenderContext::UploadJetEngineFlamesStart()
{
    //
    // Jet engine flames are not sticky: we upload them at each frame,
    // though they will be empty most of the time
    //

    mJetEngineFlameVertexBuffer.clear();
}

void ShipRenderContext::UploadJetEngineFlamesEnd()
{
    // Nop
}

void ShipRenderContext::UploadElementEphemeralPointsStart()
{
    // Client wants to upload a new set of ephemeral point elements

    // Empty buffer
    mEphemeralPointElementBuffer.clear();

    mAreElementBuffersDirty = true;
}

void ShipRenderContext::UploadElementEphemeralPointsEnd()
{
    // Nop
}

void ShipRenderContext::UploadVectorsStart(
    size_t maxCount,
    vec4f const & color)
{
    mVectorArrowVertexBuffer.reserve(maxCount * 3 * 2);

    if (color != mVectorArrowColor)
    {
        mVectorArrowColor = color;

        mIsVectorArrowColorDirty = true;
    }
}

void ShipRenderContext::UploadVectorsEnd()
{
    // Nop
}


void ShipRenderContext::UploadCentersStart(size_t count)
{
    //
    // Centers are are sticky as long as start() is not invoked
    //

    mCenterVertexBuffer.clear();
    mCenterVertexBuffer.reserve(count);

    mIsCenterVertexBufferDirty = true;
}

void ShipRenderContext::UploadCentersEnd()
{
    // Nop
}

void ShipRenderContext::UploadPointToPointArrowsStart(size_t count)
{
    //
    // Point-to-point arrows are sticky as long as start() is not invoked
    //

    mPointToPointArrowVertexBuffer.clear();
    mPointToPointArrowVertexBuffer.reserve(count);

    mIsPointToPointArrowsVertexBufferDirty = true;
}

void ShipRenderContext::UploadPointToPointArrowsEnd()
{
    // Nop
}

void ShipRenderContext::UploadEnd()
{
    // Nop
}

void ShipRenderContext::ProcessParameterChanges(RenderParameters const & renderParameters)
{
    if (renderParameters.IsShipViewModeDirty)
    {
        ApplyShipViewModeChanges(renderParameters);
    }

    if (renderParameters.AreShipStructureRenderModeSelectorsDirty)
    {
        ApplyShipStructureRenderModeChanges(renderParameters); // Also selects shaders for following functions to set parameters on
    }

    if (renderParameters.IsViewDirty
        || mIsViewModelDirty
        || renderParameters.AreShipStructureRenderModeSelectorsDirty)
    {
        ApplyViewModelChanges(renderParameters);
        mIsViewModelDirty = false;
    }

    if (renderParameters.IsEffectiveAmbientLightIntensityDirty
        || renderParameters.IsShipAmbientLightSensitivityDirty
        || renderParameters.AreShipStructureRenderModeSelectorsDirty)
    {
        ApplyEffectiveAmbientLightIntensityChanges(renderParameters);
    }

    if (renderParameters.IsShipDepthDarkeningSensitivityDirty)
    {
        ApplyDepthDarkeningSensitivityChanges(renderParameters);
    }

    if (renderParameters.IsSkyDirty
        || renderParameters.AreShipStructureRenderModeSelectorsDirty)
    {
        ApplySkyChanges(renderParameters);
    }

    if (renderParameters.IsFlatLampLightColorDirty
        || renderParameters.AreShipStructureRenderModeSelectorsDirty)
    {
        ApplyFlatLampLightColorChanges(renderParameters);
    }

    if (renderParameters.AreShipFlameRenderParametersDirty)
    {
        ApplyShipFlameRenderParameterChanges(renderParameters);
    }

    if (renderParameters.IsShipWaterColorDirty
        || renderParameters.AreShipStructureRenderModeSelectorsDirty)
    {
        ApplyWaterColorChanges(renderParameters);
    }

    if (renderParameters.IsShipWaterContrastDirty
        || renderParameters.AreShipStructureRenderModeSelectorsDirty)
    {
        ApplyWaterContrastChanges(renderParameters);
    }

    if (renderParameters.IsShipWaterLevelOfDetailDirty
        || renderParameters.AreShipStructureRenderModeSelectorsDirty)
    {
        ApplyWaterLevelOfDetailChanges(renderParameters);
    }

    if (renderParameters.IsHeatSensitivityDirty
        || renderParameters.AreShipStructureRenderModeSelectorsDirty)
    {
        ApplyHeatSensitivityChanges(renderParameters);
    }

    if (renderParameters.AreShipStructureRenderModeSelectorsDirty)
    {
        ApplyStressRenderModeChanges(renderParameters);
    }

    if (renderParameters.AreNpcRenderParametersDirty)
    {
        ApplyNpcRenderModeChanges(renderParameters);
    }
}

void ShipRenderContext::RenderPrepare(RenderParameters const & renderParameters)
{
    // We've been invoked on the render thread

    //
    // Upload Point AttributeGroup1 buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup1VBO);

    glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(vec4f), mPointAttributeGroup1Buffer.data());
    CheckOpenGLError();

    //
    // Upload Point AttributeGroup2 buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup2VBO);

    glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(vec4f), mPointAttributeGroup2Buffer.data());
    CheckOpenGLError();

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Upload element buffers, if needed
    //

    if (mAreElementBuffersDirty)
    {
        //
        // Upload all elements to the VBO, remembering the starting VBO index
        // of each element type which we'll need at primitives' render time
        //

        // Note: byte-granularity indices
        mTriangleElementVBOStartIndex = 0;
        mRopeElementVBOStartIndex = mTriangleElementVBOStartIndex + mTriangleElementBuffer.size() * sizeof(TriangleElement);
        mSpringElementVBOStartIndex = mRopeElementVBOStartIndex + mRopeElementBuffer.size() * sizeof(LineElement);
        mPointElementVBOStartIndex = mSpringElementVBOStartIndex + mSpringElementBuffer.size() * sizeof(LineElement);
        mEphemeralPointElementVBOStartIndex = mPointElementVBOStartIndex + mPointElementBuffer.size() * sizeof(PointElement);
        size_t requiredIndexSize = mEphemeralPointElementVBOStartIndex + mEphemeralPointElementBuffer.size() * sizeof(PointElement);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mElementVBO);

        if (mElementVBOAllocatedIndexSize != requiredIndexSize)
        {
            // Re-allocate VBO buffer
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, requiredIndexSize, nullptr, GL_STATIC_DRAW);
            CheckOpenGLError();

            mElementVBOAllocatedIndexSize = requiredIndexSize;
        }

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

        // Upload ephemeral points
        glBufferSubData(
            GL_ELEMENT_ARRAY_BUFFER,
            mEphemeralPointElementVBOStartIndex,
            mEphemeralPointElementBuffer.size() * sizeof(PointElement),
            mEphemeralPointElementBuffer.data());

        CheckOpenGLError();

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        mAreElementBuffersDirty = false;
    }

    //
    // Prepare flames
    //

    RenderPrepareFlames();

    //
    // Prepare jet engine flames
    //

    RenderPrepareJetEngineFlames();

    //
    // Prepare stressed springs
    //

    if (renderParameters.ShowStressedSprings
        && !mStressedSpringElementBuffer.empty())
    {
        //
        // Upload buffer
        //

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mStressedSpringElementVBO);

        if (mStressedSpringElementBuffer.size() > mStressedSpringElementVBOAllocatedElementSize)
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, mStressedSpringElementBuffer.size() * sizeof(LineElement), mStressedSpringElementBuffer.data(), GL_STREAM_DRAW);
            CheckOpenGLError();

            mStressedSpringElementVBOAllocatedElementSize = mStressedSpringElementBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, mStressedSpringElementBuffer.size() * sizeof(LineElement), mStressedSpringElementBuffer.data());
            CheckOpenGLError();
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    //
    // Prepare frontiers
    //

    if (renderParameters.ShowFrontiers)
    {
        //
        // Upload buffer
        //

        if (mIsFrontierEdgeElementBufferDirty)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mFrontierEdgeElementVBO);

            if (mFrontierEdgeElementBuffer.size() > mFrontierEdgeElementVBOAllocatedElementSize)
            {
                // Re-allocate VBO buffer and upload
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, mFrontierEdgeElementBuffer.size() * sizeof(LineElement), mFrontierEdgeElementBuffer.data(), GL_STATIC_DRAW);
                CheckOpenGLError();

                mFrontierEdgeElementVBOAllocatedElementSize = mFrontierEdgeElementBuffer.size();
            }
            else
            {
                // No size change, just upload VBO buffer
                glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, mFrontierEdgeElementBuffer.size() * sizeof(LineElement), mFrontierEdgeElementBuffer.data());
                CheckOpenGLError();
            }

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            mIsFrontierEdgeElementBufferDirty = false;
        }

        //
        // Set progress
        //

        mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipFrontierEdges>();
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipFrontierEdges, GameShaderSets::ProgramParameterKind::Time>(GameWallClock::GetInstance().ContinuousNowAsFloat());
    }

    //
    // Prepare NPCs
    //

    RenderPrepareNpcs(renderParameters);

    //
    // Prepare electric sparks
    //

    RenderPrepareElectricSparks(renderParameters);

    //
    // Prepare sparkles
    //

    RenderPrepareSparkles(renderParameters);

    //
    // Prepare generic mipmapped textures
    //

    RenderPrepareGenericMipMappedTextures(renderParameters);

    //
    // Prepare explosions
    //

    RenderPrepareExplosions(renderParameters);

    //
    // Prepare highlights
    //

    RenderPrepareHighlights(renderParameters);

    //
    // Prepare vectors
    //

    RenderPrepareVectorArrows(renderParameters);

    //
    // Prepare centers
    //

    RenderPrepareCenters(renderParameters);

    //
    // Prepare point-to-point arrows
    //

    RenderPreparePointToPointArrows(renderParameters);
}

void ShipRenderContext::RenderDraw(
    RenderParameters const & renderParameters,
    RenderStatistics & renderStats)
{
    // We've been invoked on the render thread

    //
    // Set gross noise in the noise texture unit, as all our shaders require that one
    //

    mShaderManager.ActivateTexture<GameShaderSets::ProgramParameterKind::NoiseTexture>();
    glBindTexture(GL_TEXTURE_2D, mGlobalRenderContext.GetNoiseTextureOpenGLHandle(NoiseType::Gross));

    //
    // Render background flames
    //

    if (renderParameters.DrawFlames)
    {
        RenderDrawFlames<GameShaderSets::ProgramKind::ShipFlamesBackground>(
            0,
            mFlameBackgroundCount,
            renderStats);
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

        mShaderManager.ActivateTexture<GameShaderSets::ProgramParameterKind::SharedTexture>();
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

        if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe
            || renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Decay
            || renderParameters.DebugShipRenderMode == DebugShipRenderModeType::InternalPressure
            || renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Strength
            || renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Structure
            || renderParameters.DebugShipRenderMode == DebugShipRenderModeType::None)
        {
            if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Decay)
            {
                mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipTrianglesDecay>();
            }
            else if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::InternalPressure)
            {
                mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipTrianglesInternalPressure>();
            }
            else if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Strength)
            {
                mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipTrianglesStrength>();
            }
            else
            {
                mShaderManager.ActivateProgram(mShipTrianglesProgram);
            }

            if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
                glLineWidth(0.1f);

            // Draw!
            glDrawElements(
                GL_TRIANGLES,
                static_cast<GLsizei>(3 * mTriangleElementBuffer.size()),
                GL_UNSIGNED_INT,
                (GLvoid *)mTriangleElementVBOStartIndex);

            // Update stats
            renderStats.LastRenderedShipTriangles += mTriangleElementBuffer.size();
        }

        //
        // Set line width, for ropes and springs
        //

        glLineWidth(renderParameters.View.WorldOffsetToPhysicalDisplayOffset(0.1f * 2.0f));

        //
        // Draw ropes, unless it's a debug mode that doesn't want them
        //
        // Note: when DebugRenderMode is springs|edgeSprings, ropes would all be uploaded
        // as springs.
        //

        if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Structure
            || renderParameters.DebugShipRenderMode == DebugShipRenderModeType::None)
        {
            mShaderManager.ActivateProgram(mShipRopesProgram);

            glDrawElements(
                GL_LINES,
                static_cast<GLsizei>(2 * mRopeElementBuffer.size()),
                GL_UNSIGNED_INT,
                (GLvoid *)mRopeElementVBOStartIndex);

            // Update stats
            renderStats.LastRenderedShipRopes += mRopeElementBuffer.size();
        }

        //
        // Draw springs
        //
        // We draw springs when:
        // - DebugRenderMode is springs|edgeSprings, in which case we use colors - so to show
        //   structural springs -, or
        // - DebugRenderMode is structure, in which case we use colors - so to draw 1D chains -, or
        // - DebugRenderMode is none, in which case we use texture - so to draw 1D chains and edge springs
        // - DebugRenderMode is decay|internalPressure|strength, in which case we use the special rendering
        //
        // Note: when DebugRenderMode is springs|edgeSprings, ropes would all be here.
        //

        if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Springs
            || renderParameters.DebugShipRenderMode == DebugShipRenderModeType::EdgeSprings
            || renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Structure
            || renderParameters.DebugShipRenderMode == DebugShipRenderModeType::None
            || renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Decay
            || renderParameters.DebugShipRenderMode == DebugShipRenderModeType::InternalPressure
            || renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Strength)
        {
            if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Decay)
            {
                mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSpringsDecay>();
            }
            else if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::InternalPressure)
            {
                mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSpringsInternalPressure>();
            }
            else if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Strength)
            {
                mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSpringsStrength>();
            }
            else
            {
                mShaderManager.ActivateProgram(mShipSpringsProgram);
            }

            glDrawElements(
                GL_LINES,
                static_cast<GLsizei>(2 * mSpringElementBuffer.size()),
                GL_UNSIGNED_INT,
                (GLvoid *)mSpringElementVBOStartIndex);

            // Update stats
            renderStats.LastRenderedShipSprings += mSpringElementBuffer.size();
        }

        //
        // Draw stressed springs
        //

        if (renderParameters.ShowStressedSprings
            && !mStressedSpringElementBuffer.empty())
        {
            mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipStressedSprings>();

            // Bind stressed spring element VBO
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mStressedSpringElementVBO);

            // Bind stressed spring texture
            mShaderManager.ActivateTexture<GameShaderSets::ProgramParameterKind::SharedTexture>();
            glBindTexture(GL_TEXTURE_2D, *mStressedSpringTextureOpenGLHandle);
            CheckOpenGLError();

            // Draw
            glDrawElements(
                GL_LINES,
                static_cast<GLsizei>(2 * mStressedSpringElementBuffer.size()),
                GL_UNSIGNED_INT,
                (GLvoid *)0);

            // Bind again ship element VBO
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mElementVBO);
        }

        //
        // Draw frontiers
        //

        if (renderParameters.ShowFrontiers
            && !mFrontierEdgeElementBuffer.empty())
        {
            mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipFrontierEdges>();

            glLineWidth(4.2f);

            // Bind frontier edge element VBO
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mFrontierEdgeElementVBO);

            // Draw
            glDrawElements(
                GL_LINES,
                static_cast<GLsizei>(2 * mFrontierEdgeElementBuffer.size()),
                GL_UNSIGNED_INT,
                (GLvoid *)0);

            // Bind again ship element VBO
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mElementVBO);
        }

        //
        // Draw points (orphaned/all non-ephemerals, and ephemerals)
        //

        if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Points
            || renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Structure
            || renderParameters.DebugShipRenderMode == DebugShipRenderModeType::None)
        {
            size_t const totalPoints = mPointElementBuffer.size() + mEphemeralPointElementBuffer.size();

            if (totalPoints > 0)
            {
                mShaderManager.ActivateProgram(mShipPointsProgram);

                glPointSize(mPointSize);

                glDrawElements(
                    GL_POINTS,
                    static_cast<GLsizei>(totalPoints),
                    GL_UNSIGNED_INT,
                    (GLvoid *)mPointElementVBOStartIndex);

                // Update stats
                renderStats.LastRenderedShipPoints += totalPoints;
            }
        }

        // We are done with the ship VAO
        glBindVertexArray(0);
    }

    //
    // Render electric sparks
    //

    RenderDrawElectricSparks(renderParameters);

    //
    // Render sparkles
    //

    RenderDrawSparkles(renderParameters);

    //
    // Render generic textures
    //

    RenderDrawGenericMipMappedTextures(renderParameters, renderStats);

    //
    // Render foreground flames
    //

    if (renderParameters.DrawFlames)
    {
        RenderDrawFlames<GameShaderSets::ProgramKind::ShipFlamesForeground>(
            mFlameBackgroundCount,
            mFlameForegroundCount,
            renderStats);
    }

    //
    // Render jet engine flames
    //

    RenderDrawJetEngineFlames();

    //
    // Render NPCs
    //

    RenderDrawNpcs(renderParameters);

    //
    // Render explosions
    //

    if (renderParameters.DrawExplosions)
    {
        RenderDrawExplosions(renderParameters);
    }

    //
    // Render highlights
    //

    RenderDrawHighlights(renderParameters);

    //
    // Render vectors
    //

    RenderDrawVectorArrows(renderParameters);

    //
    // Render centers
    //

    RenderDrawCenters(renderParameters);

    //
    // Render point-to-point arrows
    //

    RenderDrawPointToPointArrows(renderParameters);

    //
    // Update stats
    //

    renderStats.LastRenderedShipPlanes += mMaxMaxPlaneId + 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ShipRenderContext::RenderPrepareNpcs(RenderParameters const & renderParameters)
{
    //
    // Upload buffers, if needed
    //

    assert(mNpcPositionBuffer.size() * 4 == mNpcAttributesVertexBuffer.size());
    assert((renderParameters.NpcRenderMode != NpcRenderModeType::QuadWithRoles && mNpcQuadRoleVertexBuffer.size() == 0)
        || (renderParameters.NpcRenderMode == NpcRenderModeType::QuadWithRoles && mNpcQuadRoleVertexBuffer.size() == mNpcAttributesVertexBuffer.size()));

    if (!mNpcPositionBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mNpcPositionVBO);
        if (mNpcPositionBuffer.size() > mNpcPositionVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ARRAY_BUFFER, sizeof(Quad) * mNpcPositionBuffer.size(), mNpcPositionBuffer.data(), GL_STREAM_DRAW);
            CheckOpenGLError();

            mNpcPositionVBOAllocatedVertexSize = mNpcPositionBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Quad) * mNpcPositionBuffer.size(), mNpcPositionBuffer.data());
            CheckOpenGLError();
        }

        glBindBuffer(GL_ARRAY_BUFFER, *mNpcAttributesVertexVBO);
        if (mNpcAttributesVertexBuffer.size() > mNpcAttributesVertexVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ARRAY_BUFFER, sizeof(NpcAttributesVertex) * mNpcAttributesVertexBuffer.size(), mNpcAttributesVertexBuffer.data(), GL_STREAM_DRAW);
            CheckOpenGLError();

            mNpcAttributesVertexVBOAllocatedVertexSize = mNpcAttributesVertexBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(NpcAttributesVertex) * mNpcAttributesVertexBuffer.size(), mNpcAttributesVertexBuffer.data());
            CheckOpenGLError();
        }

        if (renderParameters.NpcRenderMode == NpcRenderModeType::QuadWithRoles)
        {
            glBindBuffer(GL_ARRAY_BUFFER, *mNpcQuadRoleVertexVBO);
            if (mNpcQuadRoleVertexBuffer.size() > mNpcQuadRoleVertexVBOAllocatedVertexSize)
            {
                // Re-allocate VBO buffer and upload
                glBufferData(GL_ARRAY_BUFFER, sizeof(NpcQuadRoleVertex) * mNpcQuadRoleVertexBuffer.size(), mNpcQuadRoleVertexBuffer.data(), GL_STREAM_DRAW);
                CheckOpenGLError();

                mNpcQuadRoleVertexVBOAllocatedVertexSize = mNpcQuadRoleVertexBuffer.size();
            }
            else
            {
                // No size change, just upload VBO buffer
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(NpcQuadRoleVertex) * mNpcQuadRoleVertexBuffer.size(), mNpcQuadRoleVertexBuffer.data());
                CheckOpenGLError();
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void ShipRenderContext::RenderDrawNpcs(RenderParameters const & renderParameters)
{
    if (!mNpcPositionBuffer.empty())
    {
        switch (renderParameters.NpcRenderMode)
        {
            case NpcRenderModeType::Texture:
            {
                glBindVertexArray(*mNpcTextureAndQuadFlatVAO);
                mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipNpcsTexture>();
                break;
            }

            case NpcRenderModeType::QuadWithRoles:
            {
                glBindVertexArray(*mNpcQuadWithRolesVAO);
                mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipNpcsQuadWithRoles>();
                break;
            }

            case NpcRenderModeType::QuadFlat:
            {
                glBindVertexArray(*mNpcTextureAndQuadFlatVAO);
                mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipNpcsQuadFlat>();
                break;
            }
        }

        // Intel bug: cannot associate with VAO
        mGlobalRenderContext.GetElementIndices().Bind();

        if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
            glLineWidth(0.1f);

        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(mNpcPositionBuffer.size() * 6),
            GL_UNSIGNED_INT,
            (GLvoid *)0);

        glBindVertexArray(0);
    }
}

void ShipRenderContext::RenderPrepareElectricSparks(RenderParameters const & /*renderParameters*/)
{
    if (!mElectricSparkVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mElectricSparkVBO);

        if (mElectricSparkVertexBuffer.size() > mElectricSparkVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ARRAY_BUFFER, mElectricSparkVertexBuffer.size() * sizeof(ElectricSparkVertex), mElectricSparkVertexBuffer.data(), GL_DYNAMIC_DRAW);
            CheckOpenGLError();

            mElectricSparkVBOAllocatedVertexSize = mElectricSparkVertexBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ARRAY_BUFFER, 0, mElectricSparkVertexBuffer.size() * sizeof(ElectricSparkVertex), mElectricSparkVertexBuffer.data());
            CheckOpenGLError();
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void ShipRenderContext::RenderDrawElectricSparks(RenderParameters const & renderParameters)
{
    if (!mElectricSparkVertexBuffer.empty())
    {
        glBindVertexArray(*mElectricSparkVAO);

        mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipElectricSparks>();

        if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
            glLineWidth(0.1f);

        assert(0 == (mElectricSparkVertexBuffer.size() % 6));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mElectricSparkVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void ShipRenderContext::RenderPrepareFlames()
{
    //
    // Upload buffers, if needed
    //

    if (!mFlameVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mFlameVBO);

        if (mFlameVertexBuffer.size() > mFlameVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ARRAY_BUFFER, mFlameVertexBuffer.size() * sizeof(FlameVertex), mFlameVertexBuffer.data(), GL_STREAM_DRAW);
            CheckOpenGLError();

            mFlameVBOAllocatedVertexSize = mFlameVertexBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ARRAY_BUFFER, 0, mFlameVertexBuffer.size() * sizeof(FlameVertex), mFlameVertexBuffer.data());
            CheckOpenGLError();
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    //
    // Set flame parameters, if we'll be drawing flames
    //

    if (mFlameBackgroundCount > 0 || mFlameForegroundCount > 0)
    {
        float const flameProgress = GameWallClock::GetInstance().NowAsFloat() * 0.345f;

        mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipFlamesBackground>();
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipFlamesBackground, GameShaderSets::ProgramParameterKind::FlameProgress>(flameProgress);

        mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipFlamesForeground>();
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipFlamesForeground, GameShaderSets::ProgramParameterKind::FlameProgress>(flameProgress);
    }
}

template<GameShaderSets::ProgramKind FlameShaderType>
void ShipRenderContext::RenderDrawFlames(
    size_t startFlameIndex,
    size_t flameCount,
    RenderStatistics & renderStats)
{
    if (flameCount > 0)
    {
        glBindVertexArray(*mFlameVAO);

        // Intel bug: cannot associate with VAO
        mGlobalRenderContext.GetElementIndices().Bind();

        mShaderManager.ActivateProgram<FlameShaderType>();

        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(flameCount * 6u),
            GL_UNSIGNED_INT,
            (GLvoid *)(static_cast<size_t>(startFlameIndex) * 6u * sizeof(int)));
        CheckOpenGLError();

        glBindVertexArray(0);

        // Update stats
        renderStats.LastRenderedShipFlames += flameCount; // # of quads
    }
}

void ShipRenderContext::RenderPrepareJetEngineFlames()
{
    //
    // Upload buffers, if needed
    //

    if (!mJetEngineFlameVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mJetEngineFlameVBO);

        if (mJetEngineFlameVertexBuffer.size() > mJetEngineFlameVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ARRAY_BUFFER, mJetEngineFlameVertexBuffer.size() * sizeof(JetEngineFlameVertex), mJetEngineFlameVertexBuffer.data(), GL_STREAM_DRAW);
            CheckOpenGLError();

            mJetEngineFlameVBOAllocatedVertexSize = mJetEngineFlameVertexBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ARRAY_BUFFER, 0, mJetEngineFlameVertexBuffer.size() * sizeof(JetEngineFlameVertex), mJetEngineFlameVertexBuffer.data());
            CheckOpenGLError();
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //
        // Set flame parameters
        //

        float const flameProgress = GameWallClock::GetInstance().NowAsFloat();

        mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipJetEngineFlames>();
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipJetEngineFlames, GameShaderSets::ProgramParameterKind::FlameProgress>(flameProgress);
    }
}

void ShipRenderContext::RenderDrawJetEngineFlames()
{
    if (!mJetEngineFlameVertexBuffer.empty())
    {
        glBindVertexArray(*mJetEngineFlameVAO);

        mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipJetEngineFlames>();

        assert(0 == (mJetEngineFlameVertexBuffer.size() % 6));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mJetEngineFlameVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void ShipRenderContext::RenderPrepareSparkles(RenderParameters const & /*renderParameters*/)
{
    if (!mSparkleVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mSparkleVBO);

        if (mSparkleVertexBuffer.size() > mSparkleVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ARRAY_BUFFER, mSparkleVertexBuffer.size() * sizeof(SparkleVertex), mSparkleVertexBuffer.data(), GL_DYNAMIC_DRAW);
            CheckOpenGLError();

            mSparkleVBOAllocatedVertexSize = mSparkleVertexBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ARRAY_BUFFER, 0, mSparkleVertexBuffer.size() * sizeof(SparkleVertex), mSparkleVertexBuffer.data());
            CheckOpenGLError();
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void ShipRenderContext::RenderDrawSparkles(RenderParameters const & renderParameters)
{
    if (!mSparkleVertexBuffer.empty())
    {
        glBindVertexArray(*mSparkleVAO);

        mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSparkles>();

        if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
            glLineWidth(0.1f);

        assert(0 == (mSparkleVertexBuffer.size() % 6));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mSparkleVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void ShipRenderContext::RenderPrepareGenericMipMappedTextures(RenderParameters const & /*renderParameters*/)
{
    //
    // Calculate indices needed for generic mipmapped textures
    //
    // We do this here so we allow subsequent GlobalRenderContext::RenderPrepare
    // to upload indices
    //

    size_t const nonAirBubblesTotalVertexCount = std::accumulate(
        mGenericMipMappedTexturePlaneVertexBuffers.cbegin(),
        mGenericMipMappedTexturePlaneVertexBuffers.cend(),
        size_t(0),
        [](size_t const total, auto const & vec)
        {
            return total + vec.vertexBuffer.size();
        });

    assert((mGenericMipMappedTextureAirBubbleVertexBuffer.size() % 4) == 0);
    assert((nonAirBubblesTotalVertexCount % 4) == 0);

    mGenericMipMappedTextureTotalVertexCount = mGenericMipMappedTextureAirBubbleVertexBuffer.size() + nonAirBubblesTotalVertexCount;

    assert((mGenericMipMappedTextureTotalVertexCount % 4) == 0);
    mGlobalRenderContext.GetElementIndices().EnsureSize(mGenericMipMappedTextureTotalVertexCount / 4);

    if (mGenericMipMappedTextureTotalVertexCount > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mGenericMipMappedTextureVBO);

        if (mGenericMipMappedTextureTotalVertexCount > mGenericMipMappedTextureVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer
            glBufferData(GL_ARRAY_BUFFER, mGenericMipMappedTextureTotalVertexCount * sizeof(GenericTextureVertex), nullptr, GL_DYNAMIC_DRAW);
            CheckOpenGLError();

            mGenericMipMappedTextureVBOAllocatedVertexSize = mGenericMipMappedTextureTotalVertexCount;
        }

        // Map vertex buffer
        auto mappedBuffer = reinterpret_cast<uint8_t *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
        CheckOpenGLError();

        // Upload air bubbles
        if (!mGenericMipMappedTextureAirBubbleVertexBuffer.empty())
        {
            size_t const byteCopySize = mGenericMipMappedTextureAirBubbleVertexBuffer.size() * sizeof(GenericTextureVertex);
            std::memcpy(mappedBuffer, mGenericMipMappedTextureAirBubbleVertexBuffer.data(), byteCopySize);
            mappedBuffer += byteCopySize;
        }

        // Upload all planes of other textures
        for (auto const & plane : mGenericMipMappedTexturePlaneVertexBuffers)
        {
            if (!plane.vertexBuffer.empty())
            {
                size_t const byteCopySize = plane.vertexBuffer.size() * sizeof(GenericTextureVertex);
                std::memcpy(mappedBuffer, plane.vertexBuffer.data(), byteCopySize);
                mappedBuffer += byteCopySize;
            }
        }

        // Unmap vertex buffer
        glUnmapBuffer(GL_ARRAY_BUFFER);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void ShipRenderContext::RenderDrawGenericMipMappedTextures(
    RenderParameters const & renderParameters,
    RenderStatistics & renderStats)
{
    if (mGenericMipMappedTextureTotalVertexCount > 0) // Calculated at prepare() time
    {
        glBindVertexArray(*mGenericMipMappedTextureVAO);

        // Intel bug: cannot associate with VAO
        mGlobalRenderContext.GetElementIndices().Bind();

        mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipGenericMipMappedTextures>();

        if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
            glLineWidth(0.1f);

        assert(0 == (mGenericMipMappedTextureTotalVertexCount % 4));
        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(mGenericMipMappedTextureTotalVertexCount / 4 * 6),
            GL_UNSIGNED_INT,
            (GLvoid *)0);

        glBindVertexArray(0);

        // Update stats
        renderStats.LastRenderedShipGenericMipMappedTextures += mGenericMipMappedTextureTotalVertexCount / 4; // # of quads
    }
}

void ShipRenderContext::RenderPrepareExplosions(RenderParameters const & /*renderParameters*/)
{
    mExplosionTotalVertexCount = std::accumulate(
        mExplosionPlaneVertexBuffers.cbegin(),
        mExplosionPlaneVertexBuffers.cend(),
        size_t(0),
        [](size_t const total, auto const & vec)
        {
            return total + vec.vertexBuffer.size();
        });

    if (mExplosionTotalVertexCount > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mExplosionVBO);

        if (mExplosionTotalVertexCount > mExplosionVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer
            glBufferData(GL_ARRAY_BUFFER, mExplosionTotalVertexCount * sizeof(ExplosionVertex), nullptr, GL_STREAM_DRAW);
            CheckOpenGLError();

            mExplosionVBOAllocatedVertexSize = mExplosionTotalVertexCount;
        }

        // Map vertex buffer
        auto mappedBuffer = reinterpret_cast<uint8_t *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
        CheckOpenGLError();

        // Upload all planes
        for (auto const & plane : mExplosionPlaneVertexBuffers)
        {
            if (!plane.vertexBuffer.empty())
            {
                size_t const byteCopySize = plane.vertexBuffer.size() * sizeof(ExplosionVertex);
                std::memcpy(mappedBuffer, plane.vertexBuffer.data(), byteCopySize);
                mappedBuffer += byteCopySize;
            }
        }

        // Unmap vertex buffer
        glUnmapBuffer(GL_ARRAY_BUFFER);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void ShipRenderContext::RenderDrawExplosions(RenderParameters const & renderParameters)
{
    if (mExplosionTotalVertexCount > 0) // Calculated at prepare() time
    {
        glBindVertexArray(*mExplosionVAO);

        mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipExplosions>();

        if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
            glLineWidth(0.1f);

        assert(0 == (mExplosionTotalVertexCount % 6));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mExplosionTotalVertexCount));

        glBindVertexArray(0);
    }
}

void ShipRenderContext::RenderPrepareHighlights(RenderParameters const & /*renderParameters*/)
{
    for (size_t i = 0; i <= static_cast<size_t>(HighlightModeType::_Last); ++i)
    {
        if (!mHighlightVertexBuffers[i].empty())
        {
            glBindBuffer(GL_ARRAY_BUFFER, *mHighlightVBO);

            if (mHighlightVertexBuffers[i].size() > mHighlightVBOAllocatedVertexSize)
            {
                // Re-allocate VBO buffer and upload
                glBufferData(GL_ARRAY_BUFFER, mHighlightVertexBuffers[i].size() * sizeof(HighlightVertex), mHighlightVertexBuffers[i].data(), GL_DYNAMIC_DRAW);
                CheckOpenGLError();

                mHighlightVBOAllocatedVertexSize = mHighlightVertexBuffers[i].size();
            }
            else
            {
                // No size change, just upload VBO buffer
                glBufferSubData(GL_ARRAY_BUFFER, 0, mHighlightVertexBuffers[i].size() * sizeof(HighlightVertex), mHighlightVertexBuffers[i].data());
                CheckOpenGLError();
            }

            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }
}

void ShipRenderContext::RenderDrawHighlights(RenderParameters const & renderParameters)
{
    for (size_t i = 0; i <= static_cast<size_t>(HighlightModeType::_Last); ++i)
    {
        if (!mHighlightVertexBuffers[i].empty())
        {
            glBindVertexArray(*mHighlightVAO);

            switch (static_cast<HighlightModeType>(i))
            {
                case HighlightModeType::Circle:
                {
                    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipCircleHighlights>();
                    break;
                }

                case HighlightModeType::ElectricalElement:
                {
                    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipElectricalElementHighlights>();
                    break;
                }

                default:
                {
                    assert(false);
                    break;
                }
            }

            if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
                glLineWidth(0.1f);

            assert(0 == (mHighlightVertexBuffers[i].size() % 6));
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mHighlightVertexBuffers[i].size()));

            glBindVertexArray(0);
        }
    }
}

void ShipRenderContext::RenderPrepareVectorArrows(RenderParameters const & /*renderParameters*/)
{
    if (!mVectorArrowVertexBuffer.empty())
    {
        //
        // Color
        //

        if (mIsVectorArrowColorDirty)
        {
            mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipVectors>();

            mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipVectors, GameShaderSets::ProgramParameterKind::MatteColor>(mVectorArrowColor);

            mIsVectorArrowColorDirty = false;
        }

        glBindBuffer(GL_ARRAY_BUFFER, *mVectorArrowVBO);

        if (mVectorArrowVertexBuffer.size() > mVectorArrowVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ARRAY_BUFFER, mVectorArrowVertexBuffer.size() * sizeof(vec3f), mVectorArrowVertexBuffer.data(), GL_DYNAMIC_DRAW);
            CheckOpenGLError();

            mVectorArrowVBOAllocatedVertexSize = mVectorArrowVertexBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ARRAY_BUFFER, 0, mVectorArrowVertexBuffer.size() * sizeof(vec3f), mVectorArrowVertexBuffer.data());
            CheckOpenGLError();
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void ShipRenderContext::RenderDrawVectorArrows(RenderParameters const & /*renderParameters*/)
{
    if (!mVectorArrowVertexBuffer.empty())
    {
        glBindVertexArray(*mVectorArrowVAO);

        mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipVectors>();

        glLineWidth(1.0f);

        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mVectorArrowVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void ShipRenderContext::RenderPrepareCenters(RenderParameters const & /*renderParameters*/)
{
    if (mIsCenterVertexBufferDirty)
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mCenterVBO);

        if (!mCenterVertexBuffer.empty())
        {
            if (mCenterVertexBuffer.size() > mCenterVBOAllocatedVertexSize)
            {
                // Re-allocate VBO buffer and upload
                glBufferData(GL_ARRAY_BUFFER, mCenterVertexBuffer.size() * sizeof(CenterVertex), mCenterVertexBuffer.data(), GL_DYNAMIC_DRAW);
                CheckOpenGLError();

                mCenterVBOAllocatedVertexSize = mCenterVertexBuffer.size();
            }
            else
            {
                // No size change, just upload VBO buffer
                glBufferSubData(GL_ARRAY_BUFFER, 0, mCenterVertexBuffer.size() * sizeof(CenterVertex), mCenterVertexBuffer.data());
                CheckOpenGLError();
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        mIsCenterVertexBufferDirty = false;
    }
}

void ShipRenderContext::RenderDrawCenters(RenderParameters const & renderParameters)
{
    if (!mCenterVertexBuffer.empty())
    {
        glBindVertexArray(*mCenterVAO);

        mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipCenters>();

        if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
            glLineWidth(0.1f);

        assert(0 == (mCenterVertexBuffer.size() % 6));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mCenterVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void ShipRenderContext::RenderPreparePointToPointArrows(RenderParameters const & /*renderParameters*/)
{
    if (mIsPointToPointArrowsVertexBufferDirty)
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mPointToPointArrowVBO);

        if (!mPointToPointArrowVertexBuffer.empty())
        {
            if (mPointToPointArrowVertexBuffer.size() > mPointToPointArrowVBOAllocatedVertexSize)
            {
                // Re-allocate VBO buffer and upload
                glBufferData(GL_ARRAY_BUFFER, mPointToPointArrowVertexBuffer.size() * sizeof(PointToPointArrowVertex), mPointToPointArrowVertexBuffer.data(), GL_DYNAMIC_DRAW);
                CheckOpenGLError();

                mPointToPointArrowVBOAllocatedVertexSize = mPointToPointArrowVertexBuffer.size();
            }
            else
            {
                // No size change, just upload VBO buffer
                glBufferSubData(GL_ARRAY_BUFFER, 0, mPointToPointArrowVertexBuffer.size() * sizeof(PointToPointArrowVertex), mPointToPointArrowVertexBuffer.data());
                CheckOpenGLError();
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        mIsPointToPointArrowsVertexBufferDirty = false;
    }
}

void ShipRenderContext::RenderDrawPointToPointArrows(RenderParameters const & /*renderParameters*/)
{
    if (!mPointToPointArrowVertexBuffer.empty())
    {
        glBindVertexArray(*mPointToPointArrowVAO);

        mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipPointToPointArrows>();

        glLineWidth(0.5f);

        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mPointToPointArrowVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ShipRenderContext::ApplyShipViewModeChanges(RenderParameters const & renderParameters)
{
    //
    // Initialize ship texture
    //
    // We re-create the whole mipmap chain from scratch, as old cards
    // (e.g. Intel) do not like texture sizes changing for a level
    // while other levels are set
    //

    mShipTextureOpenGLHandle.reset();

    GLuint tmpGLuint;
    glGenTextures(1, &tmpGLuint);
    mShipTextureOpenGLHandle = tmpGLuint;

    // Bind texture
    mShaderManager.ActivateTexture<GameShaderSets::ProgramParameterKind::SharedTexture>();
    glBindTexture(GL_TEXTURE_2D, *mShipTextureOpenGLHandle);
    CheckOpenGLError();

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    CheckOpenGLError();

    // Set filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Upload texture mipmap chain
    switch (renderParameters.ShipViewMode)
    {
        case ShipViewModeType::Exterior:
        {
            GameOpenGL::UploadMipmappedTexture(mExteriorViewImage);

            break;
        }

        case ShipViewModeType::Interior:
        {
            GameOpenGL::UploadMipmappedTexture(mInteriorViewImage);

            break;
        }
    }

    // Set texture parameter in shaders
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSpringsTexture>();
    mShaderManager.SetTextureParameters<GameShaderSets::ProgramKind::ShipSpringsTexture>();
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSpringsTextureStress>();
    mShaderManager.SetTextureParameters<GameShaderSets::ProgramKind::ShipSpringsTextureStress>();
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSpringsTextureHeatOverlay>();
    mShaderManager.SetTextureParameters<GameShaderSets::ProgramKind::ShipSpringsTextureHeatOverlay>();
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSpringsTextureHeatOverlayStress>();
    mShaderManager.SetTextureParameters<GameShaderSets::ProgramKind::ShipSpringsTextureHeatOverlayStress>();
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSpringsTextureIncandescence>();
    mShaderManager.SetTextureParameters<GameShaderSets::ProgramKind::ShipSpringsTextureIncandescence>();
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSpringsTextureIncandescenceStress>();
    mShaderManager.SetTextureParameters<GameShaderSets::ProgramKind::ShipSpringsTextureIncandescenceStress>();
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipTrianglesTexture>();
    mShaderManager.SetTextureParameters<GameShaderSets::ProgramKind::ShipTrianglesTexture>();
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipTrianglesTextureStress>();
    mShaderManager.SetTextureParameters<GameShaderSets::ProgramKind::ShipTrianglesTextureStress>();
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipTrianglesTextureHeatOverlay>();
    mShaderManager.SetTextureParameters<GameShaderSets::ProgramKind::ShipTrianglesTextureHeatOverlay>();
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipTrianglesTextureHeatOverlayStress>();
    mShaderManager.SetTextureParameters<GameShaderSets::ProgramKind::ShipTrianglesTextureHeatOverlayStress>();
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipTrianglesTextureIncandescence>();
    mShaderManager.SetTextureParameters<GameShaderSets::ProgramKind::ShipTrianglesTextureIncandescence>();
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipTrianglesTextureIncandescenceStress>();
    mShaderManager.SetTextureParameters<GameShaderSets::ProgramKind::ShipTrianglesTextureIncandescenceStress>();

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    mShipViewModeType = renderParameters.ShipViewMode;
}

void ShipRenderContext::ApplyShipStructureRenderModeChanges(RenderParameters const & renderParameters)
{
    // Select shaders
    SelectShipPrograms(renderParameters);

    // Shader parameters will be set in shaders by ProcessParameterChanges()
}

void ShipRenderContext::ApplyViewModelChanges(RenderParameters const & renderParameters)
{
    //
    // Ortho-matrixes
    //
    // Each plane Z segment is divided into a number of layers, one for each type of rendering we do for a ship:
    //      - 0: Ropes (always behind)
    //      - 1: Flames (background, i.e. flames that are on ropes, so these are drawn *behind* triangles on same plane, like ropes are)
    //      - 2: Springs
    //      - 3: Triangles
    //          - Triangles are always drawn temporally before ropes and springs though, to avoid anti-aliasing issues
    //      - 4: Stressed springs, Frontier edges (temporally after)
    //      - 5: Points
    //      - 6: Electric sparks, Sparkles
    //      - 7: Generic textures
    //      - 8: Flames (foreground), Jet engine flames
    //      - 9: NPCs
    //      - 10: Explosions
    //      - 11: Highlights, Centers
    //      - 12: Vectors, Point-to-Point Arrows
    //

    constexpr float ShipRegionZStart = 1.0f; // Far
    constexpr float ShipRegionZWidth = -2.0f; // Near (-1)

    constexpr int NLayers = 13;

    auto const & view = renderParameters.View;

    ProjectionMatrix shipOrthoMatrix;

    //
    // Layer 0: Ropes
    //

    view.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        0,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram(mShipRopesProgram);
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        mShipRopesProgram,
        shipOrthoMatrix);

    //
    // Layer 1: Flames - background
    //

    view.UpdateShipOrthoMatrixForLayer(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        1,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipFlamesBackground>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipFlamesBackground, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 2: Springs
    //

    view.UpdateShipOrthoMatrixForLayer(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        2,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram(mShipSpringsProgram);
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        mShipSpringsProgram,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSpringsDecay>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipSpringsDecay, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSpringsInternalPressure>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipSpringsInternalPressure, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSpringsStrength>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipSpringsStrength, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 3: Triangles
    //

    view.UpdateShipOrthoMatrixForLayer(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        3,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram(mShipTrianglesProgram);
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        mShipTrianglesProgram,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipTrianglesDecay>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipTrianglesDecay, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipTrianglesInternalPressure>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipTrianglesInternalPressure, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipTrianglesStrength>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipTrianglesStrength, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 4: Stressed Springs, Frontier Edges
    //

    view.UpdateShipOrthoMatrixForLayer(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        4,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipStressedSprings>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipStressedSprings, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipFrontierEdges>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipFrontierEdges, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 5: Points
    //

    view.UpdateShipOrthoMatrixForLayer(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        5,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram(mShipPointsProgram);
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        mShipPointsProgram,
        shipOrthoMatrix);

    //
    // Layer 6: Electric Sparks, Sparkles
    //

    view.UpdateShipOrthoMatrixForLayer(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        6,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipElectricSparks>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipElectricSparks, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSparkles>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipSparkles, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 7: Generic Textures
    //

    view.UpdateShipOrthoMatrixForLayer(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        7,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipGenericMipMappedTextures>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipGenericMipMappedTextures, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 8: Flames - foreground, Jet engine flames
    //

    view.UpdateShipOrthoMatrixForLayer(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        8,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipFlamesForeground>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipFlamesForeground, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipJetEngineFlames>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipJetEngineFlames, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 9: NPCs
    //

    view.UpdateShipOrthoMatrixForLayer(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        9,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipNpcsTexture>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipNpcsTexture, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipNpcsQuadWithRoles>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipNpcsQuadWithRoles, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipNpcsQuadFlat>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipNpcsQuadFlat, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 10: Explosions
    //

    view.UpdateShipOrthoMatrixForLayer(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        10,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipExplosions>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipExplosions, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 11: Highlights, Centers
    //

    view.UpdateShipOrthoMatrixForLayer(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        11,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipElectricalElementHighlights>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipElectricalElementHighlights, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipCircleHighlights>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipCircleHighlights, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipCenters>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipCenters, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 12: Vectors, Point-to-Point Arrows
    //

    view.UpdateShipOrthoMatrixForLayer(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        12,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipVectors>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipVectors, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipPointToPointArrows>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipPointToPointArrows, GameShaderSets::ProgramParameterKind::OrthoMatrix>(
        shipOrthoMatrix);

    /////////////////////////////////////////////

    //
    // Calculated parameters
    //
    // Note: we get here when either ViewModel is dirty, or ShipParticleRenderMode is dirty
    //

    // Point size

    switch (renderParameters.ShipParticleRenderMode)
    {
        case ShipParticleRenderModeType::Fragment:
        {
            mPointSize = renderParameters.View.WorldOffsetToPhysicalDisplayOffset(0.3f);
            mShaderManager.SetProgramParameterInAllShaders<GameShaderSets::ProgramParameterKind::ShipParticleRenderMode>(0.0f);
            break;
        }

        case ShipParticleRenderModeType::Particle:
        {
            mPointSize = renderParameters.View.WorldOffsetToPhysicalDisplayOffset(1.0f);
            mShaderManager.SetProgramParameterInAllShaders<GameShaderSets::ProgramParameterKind::ShipParticleRenderMode>(1.0f);
            break;
        }
    }
}

void ShipRenderContext::ApplyEffectiveAmbientLightIntensityChanges(RenderParameters const & renderParameters)
{
    //
    // Set parameter in all programs
    //

    float const effectiveAmbientLightIntensityParamValue =
        (1.0f - renderParameters.ShipAmbientLightSensitivity)
        + renderParameters.ShipAmbientLightSensitivity * renderParameters.EffectiveAmbientLightIntensity;

    if (renderParameters.HeatRenderMode != HeatRenderModeType::HeatOverlay)
    {
        mShaderManager.ActivateProgram(mShipPointsProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::EffectiveAmbientLightIntensity>(
            mShipPointsProgram,
            effectiveAmbientLightIntensityParamValue);

        mShaderManager.ActivateProgram(mShipRopesProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::EffectiveAmbientLightIntensity>(
            mShipRopesProgram,
            effectiveAmbientLightIntensityParamValue);

        mShaderManager.ActivateProgram(mShipSpringsProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::EffectiveAmbientLightIntensity>(
            mShipSpringsProgram,
            effectiveAmbientLightIntensityParamValue);

        mShaderManager.ActivateProgram(mShipTrianglesProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::EffectiveAmbientLightIntensity>(
            mShipTrianglesProgram,
            effectiveAmbientLightIntensityParamValue);
    }

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSpringsDecay>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipSpringsDecay, GameShaderSets::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        effectiveAmbientLightIntensityParamValue);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSpringsInternalPressure>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipSpringsInternalPressure, GameShaderSets::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        effectiveAmbientLightIntensityParamValue);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipSpringsStrength>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipSpringsStrength, GameShaderSets::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        effectiveAmbientLightIntensityParamValue);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipTrianglesDecay>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipTrianglesDecay, GameShaderSets::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        effectiveAmbientLightIntensityParamValue);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipTrianglesInternalPressure>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipTrianglesInternalPressure, GameShaderSets::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        effectiveAmbientLightIntensityParamValue);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipTrianglesStrength>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipTrianglesStrength, GameShaderSets::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        effectiveAmbientLightIntensityParamValue);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipNpcsQuadFlat>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipNpcsQuadFlat, GameShaderSets::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        effectiveAmbientLightIntensityParamValue);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipNpcsQuadWithRoles>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipNpcsQuadWithRoles, GameShaderSets::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        effectiveAmbientLightIntensityParamValue);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipNpcsTexture>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipNpcsTexture, GameShaderSets::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        effectiveAmbientLightIntensityParamValue);

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipGenericMipMappedTextures>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipGenericMipMappedTextures, GameShaderSets::ProgramParameterKind::EffectiveAmbientLightIntensity>(
        effectiveAmbientLightIntensityParamValue);
}

void ShipRenderContext::ApplyDepthDarkeningSensitivityChanges(RenderParameters const & renderParameters)
{
    mShaderManager.SetProgramParameterInAllShaders<GameShaderSets::ProgramParameterKind::ShipDepthDarkeningSensitivity>(renderParameters.ShipDepthDarkeningSensitivity);
}

void ShipRenderContext::ApplySkyChanges(RenderParameters const & renderParameters)
{
    vec3f const effectiveMoonlightColor = renderParameters.EffectiveMoonlightColor.toVec3f();

    if (renderParameters.HeatRenderMode != HeatRenderModeType::HeatOverlay)
    {
        mShaderManager.ActivateProgram(mShipPointsProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::EffectiveMoonlightColor>(
            mShipPointsProgram,
            effectiveMoonlightColor);

        mShaderManager.ActivateProgram(mShipRopesProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::EffectiveMoonlightColor>(
            mShipRopesProgram,
            effectiveMoonlightColor);

        mShaderManager.ActivateProgram(mShipSpringsProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::EffectiveMoonlightColor>(
            mShipSpringsProgram,
            effectiveMoonlightColor);

        mShaderManager.ActivateProgram(mShipTrianglesProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::EffectiveMoonlightColor>(
            mShipTrianglesProgram,
            effectiveMoonlightColor);
    }

    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipGenericMipMappedTextures>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipGenericMipMappedTextures, GameShaderSets::ProgramParameterKind::EffectiveMoonlightColor>(
        effectiveMoonlightColor);
}

void ShipRenderContext::ApplyFlatLampLightColorChanges(RenderParameters const & renderParameters)
{
    //
    // Set parameter in all affected programs
    //

    vec3f const lampLightColor = renderParameters.FlatLampLightColor.toVec3f();

    mShaderManager.ActivateProgram(mShipPointsProgram);
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::LampLightColor>(
        mShipPointsProgram,
        lampLightColor);

    mShaderManager.ActivateProgram(mShipRopesProgram);
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::LampLightColor>(
        mShipRopesProgram,
        lampLightColor);

    mShaderManager.ActivateProgram(mShipSpringsProgram);
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::LampLightColor>(
        mShipSpringsProgram,
        lampLightColor);

    mShaderManager.ActivateProgram(mShipTrianglesProgram);
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::LampLightColor>(
        mShipTrianglesProgram,
        lampLightColor);


    mShaderManager.ActivateProgram(GameShaderSets::ProgramKind::ShipNpcsQuadFlat);
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipNpcsQuadFlat, GameShaderSets::ProgramParameterKind::LampLightColor>(
        lampLightColor);

    mShaderManager.ActivateProgram(GameShaderSets::ProgramKind::ShipNpcsQuadWithRoles);
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipNpcsQuadWithRoles, GameShaderSets::ProgramParameterKind::LampLightColor>(
        lampLightColor);

    mShaderManager.ActivateProgram(GameShaderSets::ProgramKind::ShipNpcsTexture);
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipNpcsTexture, GameShaderSets::ProgramParameterKind::LampLightColor>(
        lampLightColor);
}

void ShipRenderContext::ApplyShipFlameRenderParameterChanges(RenderParameters const & renderParameters)
{
    //
    // Set parameters in all affected programs
    //

    mShaderManager.ActivateProgram(GameShaderSets::ProgramKind::ShipFlamesBackground);
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipFlamesBackground, GameShaderSets::ProgramParameterKind::KaosAdjustment>(
        renderParameters.ShipFlameKaosAdjustment);

    mShaderManager.ActivateProgram(GameShaderSets::ProgramKind::ShipFlamesForeground);
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipFlamesForeground, GameShaderSets::ProgramParameterKind::KaosAdjustment>(
        renderParameters.ShipFlameKaosAdjustment);
}

void ShipRenderContext::ApplyWaterColorChanges(RenderParameters const & renderParameters)
{
    //
    // Set parameter in all affected programs
    //

    auto const waterColor = renderParameters.ShipWaterColor;

    if (renderParameters.HeatRenderMode != HeatRenderModeType::HeatOverlay)
    {
        mShaderManager.ActivateProgram(mShipPointsProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::WaterColor>(
            mShipPointsProgram,
            waterColor);

        mShaderManager.ActivateProgram(mShipRopesProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::WaterColor>(
            mShipRopesProgram,
            waterColor);

        mShaderManager.ActivateProgram(mShipSpringsProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::WaterColor>(
            mShipSpringsProgram,
            waterColor);

        mShaderManager.ActivateProgram(mShipTrianglesProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::WaterColor>(
            mShipTrianglesProgram,
            waterColor);
    }
}

void ShipRenderContext::ApplyWaterContrastChanges(RenderParameters const & renderParameters)
{
    //
    // Set parameter in all affected programs
    //

    if (renderParameters.HeatRenderMode != HeatRenderModeType::HeatOverlay)
    {
        mShaderManager.ActivateProgram(mShipPointsProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::WaterContrast>(
            mShipPointsProgram,
            renderParameters.ShipWaterContrast);

        mShaderManager.ActivateProgram(mShipRopesProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::WaterContrast>(
            mShipRopesProgram,
            renderParameters.ShipWaterContrast);

        mShaderManager.ActivateProgram(mShipSpringsProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::WaterContrast>(
            mShipSpringsProgram,
            renderParameters.ShipWaterContrast);

        mShaderManager.ActivateProgram(mShipTrianglesProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::WaterContrast>(
            mShipTrianglesProgram,
            renderParameters.ShipWaterContrast);
    }
}

void ShipRenderContext::ApplyWaterLevelOfDetailChanges(RenderParameters const & renderParameters)
{
    //
    // Set parameter in all affected programs
    //

    // Transform: 0->1 == 2.0->0.01
    float const waterLevelThreshold = 2.0f + renderParameters.ShipWaterLevelOfDetail * (-2.0f + 0.01f);

    if (renderParameters.HeatRenderMode != HeatRenderModeType::HeatOverlay)
    {
        mShaderManager.ActivateProgram(mShipPointsProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::WaterLevelThreshold>(
            mShipPointsProgram,
            waterLevelThreshold);

        mShaderManager.ActivateProgram(mShipRopesProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::WaterLevelThreshold>(
            mShipRopesProgram,
            waterLevelThreshold);

        mShaderManager.ActivateProgram(mShipSpringsProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::WaterLevelThreshold>(
            mShipSpringsProgram,
            waterLevelThreshold);

        mShaderManager.ActivateProgram(mShipTrianglesProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::WaterLevelThreshold>(
            mShipTrianglesProgram,
            waterLevelThreshold);
    }
}

void ShipRenderContext::ApplyHeatSensitivityChanges(RenderParameters const & renderParameters)
{
    //
    // Set parameter in all heat programs
    //

    // Sensitivity = 0  => Shift = 1
    // Sensitivity = 1  => Shift = 0.0001
    float const heatShift = 1.0f - renderParameters.HeatSensitivity * (1.0f - 0.0001f);

    if (renderParameters.HeatRenderMode != HeatRenderModeType::None)
    {
        mShaderManager.ActivateProgram(mShipPointsProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::HeatShift>(
            mShipPointsProgram,
            heatShift);

        mShaderManager.ActivateProgram(mShipRopesProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::HeatShift>(
            mShipRopesProgram,
            heatShift);

        mShaderManager.ActivateProgram(mShipSpringsProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::HeatShift>(
            mShipSpringsProgram,
            heatShift);

        mShaderManager.ActivateProgram(mShipTrianglesProgram);
        mShaderManager.SetProgramParameter<GameShaderSets::ProgramParameterKind::HeatShift>(
            mShipTrianglesProgram,
            heatShift);
    }
}

void ShipRenderContext::ApplyStressRenderModeChanges(RenderParameters const & renderParameters)
{
    //
    // Update stress color map
    //

    vec4f const * stressColorMap = nullptr;

    switch (renderParameters.StressRenderMode)
    {
        case StressRenderModeType::None:
        {
            // Nothing to do
            return;
        }

        case StressRenderModeType::StressOverlay:
        {
            // Symmetric left and right, transparent at center

            static std::array<vec4f, 12> constexpr StressColorMap {

                vec4f(166.0f / 255.0f, 0.0f, 0.0f, 1.0f),               // [-1.20 -> -1.00)
                vec4f(166.0f / 255.0f, 0.0f, 0.0f, 1.0f),               // [-1.00 -> -0.80)
                vec4f(166.0f / 255.0f, 130.0f / 255.0f, 0.0f, 1.0f),    // [-0.80 -> -0.60)
                vec4f(0.0f, 130.0f / 255.0f, 0.0f, 1.0f),               // [-0.60 -> -0.40)
                vec4f(0.0f, 0.0f, 94.0f / 255.0f, 1.0f),                // [-0.40 -> -0.20)
                vec4f(0.0f, 0.0f, 94.0f / 255.0f, 0.0f),                // [-0.20 ->  0.00)

                vec4f(0.0f, 0.0f, 94.0f / 255.0f, 0.0f),                // [ 0.00 ->  0.20)
                vec4f(0.0f, 0.0f, 94.0f / 255.0f, 1.0f),                // [ 0.20 ->  0.40)
                vec4f(0.0f, 130.0f / 255.0f, 0.0f, 1.0f),               // [ 0.40 ->  0.60)
                vec4f(166.0f / 255.0f, 130.0f / 255.0f, 0.0f, 1.0f),    // [ 0.60 ->  0.80)
                vec4f(166.0f / 255.0f, 0.0f, 0.0f, 1.0f),               // [ 0.80 ->  1.00)
                vec4f(166.0f / 255.0f, 0.0f, 0.0f, 1.0f)                // [ 1.00 ->  1.20)
            };

            stressColorMap = StressColorMap.data();

            break;
        }

        case StressRenderModeType::TensionOverlay:
        {
            // Opaque green at center, full red at -1.0, full blue at +1.0

            static std::array<vec4f, 12> constexpr StressColorMap {

                vec4f(166.0f / 255.0f, 0.0f, 0.0f, 1.0f),               // [-1.20 -> -1.00)
                vec4f(166.0f / 255.0f, 0.0f, 0.0f, 1.0f),               // [-1.00 -> -0.80)
                vec4f(166.0f / 255.0f, 65.0f / 255.0f, 0.0f, 1.0f),     // [-0.80 -> -0.60)
                vec4f(166.0f / 255.0f, 130.0f / 255.0f, 0.0f, 1.0f),    // [-0.60 -> -0.40)
                vec4f(83.0f / 255.0f, 130.0f / 255.0f, 0.0f, 1.0f),     // [-0.40 -> -0.20)
                vec4f(0.0f, 130.0f / 255.0f, 0.0f, 1.0f),               // [-0.20 ->  0.00)

                vec4f(0.0f, 130.0f / 255.0f, 0.0f, 1.0f),               // [ 0.00 ->  0.20)
                vec4f(0.0f, 98.0f / 255.0f, 23.0f / 255.0f, 1.0f),      // [ 0.20 ->  0.40)
                vec4f(0.0f, 66.0f / 255.0f, 46.0f / 255.0f, 1.0f),      // [ 0.40 ->  0.60)
                vec4f(0.0f, 33.0f / 255.0f, 69.0f / 255.0f, 1.0f),      // [ 0.60 ->  0.80)
                vec4f(0.0f, 0.0f, 94.0f / 255.0f, 1.0f),                // [ 0.80 ->  1.00)
                vec4f(0.0f, 0.0f, 94.0f / 255.0f, 1.0f)                 // [ 1.00 ->  1.20)
            };

            stressColorMap = StressColorMap.data();

            break;
        }
    }

    static std::array<GameShaderSets::ProgramKind, 18> constexpr StressColorMapPrograms{
        GameShaderSets::ProgramKind::ShipPointsColorStress,
        GameShaderSets::ProgramKind::ShipPointsColorHeatOverlayStress,
        GameShaderSets::ProgramKind::ShipPointsColorIncandescenceStress,
        GameShaderSets::ProgramKind::ShipRopesStress,
        GameShaderSets::ProgramKind::ShipRopesHeatOverlayStress,
        GameShaderSets::ProgramKind::ShipRopesIncandescenceStress,
        GameShaderSets::ProgramKind::ShipSpringsColorStress,
        GameShaderSets::ProgramKind::ShipSpringsColorHeatOverlayStress,
        GameShaderSets::ProgramKind::ShipSpringsColorIncandescenceStress,
        GameShaderSets::ProgramKind::ShipSpringsTextureStress,
        GameShaderSets::ProgramKind::ShipSpringsTextureHeatOverlayStress,
        GameShaderSets::ProgramKind::ShipSpringsTextureIncandescenceStress,
        GameShaderSets::ProgramKind::ShipTrianglesColorStress,
        GameShaderSets::ProgramKind::ShipTrianglesColorHeatOverlayStress,
        GameShaderSets::ProgramKind::ShipTrianglesColorIncandescenceStress,
        GameShaderSets::ProgramKind::ShipTrianglesTextureStress,
        GameShaderSets::ProgramKind::ShipTrianglesTextureHeatOverlayStress,
        GameShaderSets::ProgramKind::ShipTrianglesTextureIncandescenceStress
    };

    for (auto program : StressColorMapPrograms)
    {
        mShaderManager.ActivateProgram(program);
        mShaderManager.SetProgramParameterVec4fArray<GameShaderSets::ProgramParameterKind::StressColorMap>(
            program,
            stressColorMap,
            12);
    }
}

void ShipRenderContext::ApplyNpcRenderModeChanges(RenderParameters const & renderParameters)
{
    // Set parameter in program
    mShaderManager.ActivateProgram<GameShaderSets::ProgramKind::ShipNpcsQuadFlat>();
    mShaderManager.SetProgramParameter<GameShaderSets::ProgramKind::ShipNpcsQuadFlat, GameShaderSets::ProgramParameterKind::NpcQuadFlatColor>(
        renderParameters.NpcQuadFlatColor.toVec3f());
}

void ShipRenderContext::SelectShipPrograms(RenderParameters const & renderParameters)
{
    // Here we select a cell out of a full 3D matrix; dimensions:
    //  - Texture vs. Color (depending on DebugShipRenderMode)
    //  - None vs HeatOverlay vs. Incandescence (depending on HeatRenderMode)
    //  - None vs Stress (depending on StressRenderMode)

    bool const doStress = (renderParameters.StressRenderMode != StressRenderModeType::None);

    if (renderParameters.DebugShipRenderMode == DebugShipRenderModeType::None)
    {
        // Use texture program
        switch (renderParameters.HeatRenderMode)
        {
            case HeatRenderModeType::HeatOverlay:
            {
                if (!doStress)
                {
                    mShipPointsProgram = GameShaderSets::ProgramKind::ShipPointsColorHeatOverlay;
                    mShipRopesProgram = GameShaderSets::ProgramKind::ShipRopesHeatOverlay;
                    mShipSpringsProgram = GameShaderSets::ProgramKind::ShipSpringsTextureHeatOverlay;
                    mShipTrianglesProgram = GameShaderSets::ProgramKind::ShipTrianglesTextureHeatOverlay;
                }
                else
                {
                    mShipPointsProgram = GameShaderSets::ProgramKind::ShipPointsColorHeatOverlayStress;
                    mShipRopesProgram = GameShaderSets::ProgramKind::ShipRopesHeatOverlayStress;
                    mShipSpringsProgram = GameShaderSets::ProgramKind::ShipSpringsTextureHeatOverlayStress;
                    mShipTrianglesProgram = GameShaderSets::ProgramKind::ShipTrianglesTextureHeatOverlayStress;
                }

                break;
            }

            case HeatRenderModeType::Incandescence:
            {
                if (!doStress)
                {
                    mShipPointsProgram = GameShaderSets::ProgramKind::ShipPointsColorIncandescence;
                    mShipRopesProgram = GameShaderSets::ProgramKind::ShipRopesIncandescence;
                    mShipSpringsProgram = GameShaderSets::ProgramKind::ShipSpringsTextureIncandescence;
                    mShipTrianglesProgram = GameShaderSets::ProgramKind::ShipTrianglesTextureIncandescence;
                }
                else
                {
                    mShipPointsProgram = GameShaderSets::ProgramKind::ShipPointsColorIncandescenceStress;
                    mShipRopesProgram = GameShaderSets::ProgramKind::ShipRopesIncandescenceStress;
                    mShipSpringsProgram = GameShaderSets::ProgramKind::ShipSpringsTextureIncandescenceStress;
                    mShipTrianglesProgram = GameShaderSets::ProgramKind::ShipTrianglesTextureIncandescenceStress;
                }

                break;
            }

            case HeatRenderModeType::None:
            {
                if (!doStress)
                {
                    mShipPointsProgram = GameShaderSets::ProgramKind::ShipPointsColor;
                    mShipRopesProgram = GameShaderSets::ProgramKind::ShipRopes;
                    mShipSpringsProgram = GameShaderSets::ProgramKind::ShipSpringsTexture;
                    mShipTrianglesProgram = GameShaderSets::ProgramKind::ShipTrianglesTexture;
                }
                else
                {
                    mShipPointsProgram = GameShaderSets::ProgramKind::ShipPointsColorStress;
                    mShipRopesProgram = GameShaderSets::ProgramKind::ShipRopesStress;
                    mShipSpringsProgram = GameShaderSets::ProgramKind::ShipSpringsTextureStress;
                    mShipTrianglesProgram = GameShaderSets::ProgramKind::ShipTrianglesTextureStress;
                }

                break;
            }
        }
    }
    else
    {
        // Use color program
        switch (renderParameters.HeatRenderMode)
        {
            case HeatRenderModeType::HeatOverlay:
            {
                if (!doStress)
                {
                    mShipPointsProgram = GameShaderSets::ProgramKind::ShipPointsColorHeatOverlay;
                    mShipRopesProgram = GameShaderSets::ProgramKind::ShipRopesHeatOverlay;
                    mShipSpringsProgram = GameShaderSets::ProgramKind::ShipSpringsColorHeatOverlay;
                    mShipTrianglesProgram = GameShaderSets::ProgramKind::ShipTrianglesColorHeatOverlay;
                }
                else
                {
                    mShipPointsProgram = GameShaderSets::ProgramKind::ShipPointsColorHeatOverlayStress;
                    mShipRopesProgram = GameShaderSets::ProgramKind::ShipRopesHeatOverlayStress;
                    mShipSpringsProgram = GameShaderSets::ProgramKind::ShipSpringsColorHeatOverlayStress;
                    mShipTrianglesProgram = GameShaderSets::ProgramKind::ShipTrianglesColorHeatOverlayStress;
                }

                break;
            }

            case HeatRenderModeType::Incandescence:
            {
                if (!doStress)
                {
                    mShipPointsProgram = GameShaderSets::ProgramKind::ShipPointsColorIncandescence;
                    mShipRopesProgram = GameShaderSets::ProgramKind::ShipRopesIncandescence;
                    mShipSpringsProgram = GameShaderSets::ProgramKind::ShipSpringsColorIncandescence;
                    mShipTrianglesProgram = GameShaderSets::ProgramKind::ShipTrianglesColorIncandescence;
                }
                else
                {
                    mShipPointsProgram = GameShaderSets::ProgramKind::ShipPointsColorIncandescenceStress;
                    mShipRopesProgram = GameShaderSets::ProgramKind::ShipRopesIncandescenceStress;
                    mShipSpringsProgram = GameShaderSets::ProgramKind::ShipSpringsColorIncandescenceStress;
                    mShipTrianglesProgram = GameShaderSets::ProgramKind::ShipTrianglesColorIncandescenceStress;
                }

                break;
            }

            case HeatRenderModeType::None:
            {
                if (!doStress)
                {
                    mShipPointsProgram = GameShaderSets::ProgramKind::ShipPointsColor;
                    mShipRopesProgram = GameShaderSets::ProgramKind::ShipRopes;
                    mShipSpringsProgram = GameShaderSets::ProgramKind::ShipSpringsColor;
                    mShipTrianglesProgram = GameShaderSets::ProgramKind::ShipTrianglesColor;
                }
                else
                {
                    mShipPointsProgram = GameShaderSets::ProgramKind::ShipPointsColorStress;
                    mShipRopesProgram = GameShaderSets::ProgramKind::ShipRopesStress;
                    mShipSpringsProgram = GameShaderSets::ProgramKind::ShipSpringsColorStress;
                    mShipTrianglesProgram = GameShaderSets::ProgramKind::ShipTrianglesColorStress;
                }

                break;
            }
        }
    }
}
