/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-03-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipRenderContext.h"

#include "GameParameters.h"

#include <GameCore/GameException.h>
#include <GameCore/GameMath.h>
#include <GameCore/Log.h>

namespace Render {

ShipRenderContext::ShipRenderContext(
    ShipId shipId,
    size_t shipCount,
    size_t pointCount,
    RgbaImageData texture,
    ShipDefinition::TextureOriginType /*textureOrigin*/,
    ShaderManager<ShaderManagerTraits> & shaderManager,
    GameOpenGLTexture & textureAtlasOpenGLHandle,
    TextureAtlasMetadata const & textureAtlasMetadata,
    RenderStatistics & renderStatistics,
    ViewModel const & viewModel,
    float ambientLightIntensity,
    float waterContrast,
    float waterLevelOfDetail,
    ShipRenderMode shipRenderMode,
    DebugShipRenderMode debugShipRenderMode,
    VectorFieldRenderMode vectorFieldRenderMode,
    bool showStressedSprings)
    : mShipId(shipId)
    , mShipCount(shipCount)
    , mMaxMaxPlaneId(0)
    , mShaderManager(shaderManager)
    , mRenderStatistics(renderStatistics)
    // Parameters
    , mViewModel(viewModel)
    , mAmbientLightIntensity(ambientLightIntensity)
    , mWaterContrast(waterContrast)
    , mWaterLevelOfDetail(waterLevelOfDetail)
    , mShipRenderMode(shipRenderMode)
    , mDebugShipRenderMode(debugShipRenderMode)
    , mVectorFieldRenderMode(vectorFieldRenderMode)
    , mShowStressedSprings(showStressedSprings)
    // Textures
    , mElementShipTexture()
    , mElementStressedSpringTexture()
    // Points
    , mPointCount(pointCount)
    , mPointPositionVBO()
    , mPointLightVBO()
    , mPointWaterVBO()
    , mPointColorVBO()
    , mPointPlaneIdVBO()
    , mPointElementTextureCoordinatesVBO()
    // Generic Textures
    , mTextureAtlasOpenGLHandle(textureAtlasOpenGLHandle)
    , mTextureAtlasMetadata(textureAtlasMetadata)
    , mGenericTexturePlanes()
    , mGenericTextureMaxPlaneVertexBufferSize(0)
    , mGenericTextureRenderPolygonVertexAllocatedSize(0)
    , mGenericTextureRenderPolygonVertexVBO()
    // Elements
    , mPointElementBuffer()
    , mPointElementVBO()
    , mSpringElementBuffer()
    , mSpringElementVBO()
    , mRopeElementBuffer()
    , mRopeElementVBO()
    , mTriangleElementBuffer()
    , mTriangleElementVBO()
    , mStressedSpringElementBuffer()
    , mStressedSpringElementVBO()
    // Ephemeral points
    , mEphemeralPoints()
    , mEphemeralPointVBO()
    // Vectors
    , mVectorArrowPointPositionBuffer()
    , mVectorArrowPointPositionVBO()
    , mVectorArrowColor()
{
    GLuint tmpGLuint;

    // Clear errors
    glGetError();


    //
    // Create and pre-allocate point VBOs
    //

    GLuint pointVBOs[6];
    glGenBuffers(6, pointVBOs);

    mPointPositionVBO = pointVBOs[0];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointPositionVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(vec2f), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::ShipPointPosition));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointPosition), 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void*)(0));
    CheckOpenGLError();

    mPointLightVBO = pointVBOs[1];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointLightVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::ShipPointLight));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointLight), 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)(0));
    CheckOpenGLError();

    mPointWaterVBO = pointVBOs[2];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointWaterVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::ShipPointWater));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointWater), 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)(0));
    CheckOpenGLError();

    mPointColorVBO = pointVBOs[3];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointColorVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(vec4f), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::ShipPointColor));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointColor), 4, GL_FLOAT, GL_FALSE, sizeof(vec4f), (void*)(0));
    CheckOpenGLError();

    mPointPlaneIdVBO = pointVBOs[4];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointPlaneIdVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(PlaneId), nullptr, GL_STATIC_DRAW);
    static_assert(sizeof(PlaneId) == sizeof(uint32_t)); // GL_UNSIGNED_INT
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::ShipPointPlaneId));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointPlaneId), 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(PlaneId), (void*)(0));
    CheckOpenGLError();

    mPointElementTextureCoordinatesVBO = pointVBOs[5];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointElementTextureCoordinatesVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(vec2f), nullptr, GL_STATIC_DRAW);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::ShipPointTextureCoordinates));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointTextureCoordinates), 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void*)(0));
    CheckOpenGLError();

    glBindBuffer(GL_ARRAY_BUFFER, 0);



    //
    // Create and upload ship texture
    //

    glGenTextures(1, &tmpGLuint);
    mElementShipTexture = tmpGLuint;

    // Bind texture
    mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();
    glBindTexture(GL_TEXTURE_2D, *mElementShipTexture);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadMipmappedTexture(std::move(texture));

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    CheckOpenGLError();

    // Set filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);


    //
    // Create stressed spring texture
    //

    // Create texture name
    glGenTextures(1, &tmpGLuint);
    mElementStressedSpringTexture = tmpGLuint;

    // Bind texture
    mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();
    glBindTexture(GL_TEXTURE_2D, *mElementStressedSpringTexture);
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
    // Initialize generic textures
    //

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mGenericTextureRenderPolygonVertexVBO = tmpGLuint;

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mGenericTextureRenderPolygonVertexVBO);
    CheckOpenGLError();

    // Describe vertex buffer
    static_assert(sizeof(TextureRenderPolygonVertex) == (4 + 4 + 3) * sizeof(float));
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::GenericTexturePackedData1));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::GenericTexturePackedData1), 4, GL_FLOAT, GL_FALSE, sizeof(TextureRenderPolygonVertex), (void*)0);
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::GenericTexturePackedData2));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::GenericTexturePackedData2), 4, GL_FLOAT, GL_FALSE, sizeof(TextureRenderPolygonVertex), (void*)((4) * sizeof(float)));
    glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::GenericTexturePackedData3));
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::GenericTexturePackedData3), 3, GL_FLOAT, GL_FALSE, sizeof(TextureRenderPolygonVertex), (void*)((4 + 4) * sizeof(float)));
    CheckOpenGLError();


    //
    // Initialize elements
    //

    GLuint elementVBOs[5];
    glGenBuffers(5, elementVBOs);


    mPointElementBuffer.reserve(pointCount);
    mPointElementVBO = elementVBOs[0];

    mSpringElementBuffer.reserve(pointCount * GameParameters::MaxSpringsPerPoint);
    mSpringElementVBO = elementVBOs[1];

    mRopeElementBuffer.reserve(pointCount); // Arbitrary
    mRopeElementVBO = elementVBOs[2];

    mTriangleElementBuffer.reserve(pointCount * GameParameters::MaxTrianglesPerPoint);
    mTriangleElementVBO = elementVBOs[3];

    mStressedSpringElementBuffer.reserve(1000); // Arbitrary
    mStressedSpringElementVBO = elementVBOs[4];



    //
    // Initialize ephemeral points
    //

    mEphemeralPoints.reserve(GameParameters::MaxEphemeralParticles);

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mEphemeralPointVBO = tmpGLuint;



    //
    // Initialize vector field
    //

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mVectorArrowPointPositionVBO = tmpGLuint;



    //
    // Set parameters to initial values
    //

    OnViewModelUpdated();

    OnAmbientLightIntensityUpdated();
    OnWaterContrastUpdated();
    OnWaterLevelOfDetailUpdated();
}

ShipRenderContext::~ShipRenderContext()
{
}

void ShipRenderContext::UpdateOrthoMatrices()
{
    //
    // Each plane Z segment is divided into 7 layers, one for each type of rendering we do for a ship:
    //      - 0: Ropes (always behind)
    //      - 1: Springs
    //      - 2: Triangles
    //          - Triangles are always drawn temporally before ropes and springs though, to avoid anti-aliasing issues
    //      - 3: Stressed springs
    //      - 4: Points
    //      - 5: Generic textures
    //      - 6: Vectors
    //

    constexpr float ShipRegionZStart = 1.0f;
    constexpr float ShipRegionZWidth = -2.0f;

    constexpr int NLayers = 7;

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


    //
    // Layer 1: Springs
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

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 2: Triangles
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

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 3: Stressed Springs
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

    mShaderManager.ActivateProgram<ProgramType::ShipStressedSprings>();
    mShaderManager.SetProgramParameter<ProgramType::ShipStressedSprings, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 4: Points
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

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 5: Generic Textures
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

    mShaderManager.ActivateProgram<ProgramType::ShipGenericTextures>();
    mShaderManager.SetProgramParameter<ProgramType::ShipGenericTextures, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 6: Vectors
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

    mShaderManager.ActivateProgram<ProgramType::ShipVectors>();
    mShaderManager.SetProgramParameter<ProgramType::ShipVectors, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);
}

void ShipRenderContext::OnAmbientLightIntensityUpdated()
{
    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipGenericTextures>();
    mShaderManager.SetProgramParameter<ProgramType::ShipGenericTextures, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipVectors>();
    mShaderManager.SetProgramParameter<ProgramType::ShipVectors, ProgramParameterType::AmbientLightIntensity>(
        mAmbientLightIntensity);
}

void ShipRenderContext::OnWaterContrastUpdated()
{
    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::WaterContrast>(
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

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);
}

//////////////////////////////////////////////////////////////////////////////////

void ShipRenderContext::RenderStart()
{
    //
    // Reset generic textures
    //

    mGenericTexturePlanes.clear();
    mGenericTexturePlanes.resize(mMaxMaxPlaneId + 1);
    mGenericTextureMaxPlaneVertexBufferSize = 0;
}

void ShipRenderContext::UploadPointImmutableGraphicalAttributes(vec2f const * textureCoordinates)
{
    // Upload texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, *mPointElementTextureCoordinatesVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(vec2f), textureCoordinates);
    CheckOpenGLError();
}

void ShipRenderContext::UploadShipPointColors(
    vec4f const * color,
    size_t startDst,
    size_t count)
{
    assert(startDst + count <= mPointCount);

    glBindBuffer(GL_ARRAY_BUFFER, *mPointColorVBO);
    glBufferSubData(GL_ARRAY_BUFFER, startDst * sizeof(vec4f), count * sizeof(vec4f), color);
    CheckOpenGLError();
}

void ShipRenderContext::UploadPoints(
    vec2f const * position,
    float const * light,
    float const * water)
{
    // Upload positions
    glBindBuffer(GL_ARRAY_BUFFER, *mPointPositionVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(vec2f), position);
    CheckOpenGLError();

    // Upload light
    glBindBuffer(GL_ARRAY_BUFFER, *mPointLightVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(float), light);
    CheckOpenGLError();

    // Upload water
    glBindBuffer(GL_ARRAY_BUFFER, *mPointWaterVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(float), water);
    CheckOpenGLError();
}

void ShipRenderContext::UploadPointPlaneIds(
    PlaneId const * planeId,
    size_t startDst,
    size_t count,
    PlaneId maxMaxPlaneId)
{
    assert(startDst + count <= mPointCount);

    // Upload plane IDs
    glBindBuffer(GL_ARRAY_BUFFER, *mPointPlaneIdVBO);
    glBufferSubData(GL_ARRAY_BUFFER, startDst * sizeof(PlaneId), count * sizeof(PlaneId), planeId);
    CheckOpenGLError();

    // Check if the max ever plane ID has changed
    if (maxMaxPlaneId != mMaxMaxPlaneId)
    {
        // Update value
        mMaxMaxPlaneId = maxMaxPlaneId;

        // Make room for generic textures
        mGenericTexturePlanes.resize(mMaxMaxPlaneId + 1);

        // Recalculate view model parameters
        OnViewModelUpdated();
    }
}

void ShipRenderContext::UploadElementTrianglesStart(size_t trianglesCount)
{
    // No need to clear, we'll repopulate everything
    mTriangleElementBuffer.resize(trianglesCount);
}

void ShipRenderContext::UploadElementTrianglesEnd()
{
    // Upload
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mTriangleElementVBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mTriangleElementBuffer.size() * sizeof(TriangleElement), mTriangleElementBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
}

void ShipRenderContext::UploadElementsStart()
{
    // Empty all buffers, as they will be completely re-populated soon
    // (with a yet-unknown quantity of elements)
    mPointElementBuffer.clear();
    mSpringElementBuffer.clear();
    mRopeElementBuffer.clear();
    mStressedSpringElementBuffer.clear();
}

void ShipRenderContext::UploadElementsEnd()
{
    //
    // Upload all elements, except for stressed springs
    //

    // Points
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mPointElementVBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mPointElementBuffer.size() * sizeof(PointElement), mPointElementBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();

    // Springs
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mSpringElementVBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mSpringElementBuffer.size() * sizeof(SpringElement), mSpringElementBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();

    // Ropes
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mRopeElementVBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mRopeElementBuffer.size() * sizeof(RopeElement), mRopeElementBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mStressedSpringElementBuffer.size() * sizeof(StressedSpringElement), mStressedSpringElementBuffer.data(), GL_DYNAMIC_DRAW);
    CheckOpenGLError();
}

void ShipRenderContext::UploadEphemeralPointsStart()
{
    mEphemeralPoints.clear();
}

void ShipRenderContext::UploadEphemeralPointsEnd()
{
    //
    // Upload ephemeral points
    //

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mEphemeralPointVBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mEphemeralPoints.size() * sizeof(PointElement), mEphemeralPoints.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
}

void ShipRenderContext::UploadVectors(
    size_t count,
    vec2f const * position,
    PlaneId const * planeId,
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

    mVectorArrowPointPositionBuffer.clear();
    mVectorArrowPointPositionBuffer.reserve(count * 3 * 2);

    for (size_t i = 0; i < count; ++i)
    {
        float planeIdf = static_cast<float>(planeId[i]);

        // Stem
        vec2f stemEndpoint = position[i] + vector[i] * lengthAdjustment;
        mVectorArrowPointPositionBuffer.emplace_back(position[i], planeIdf);
        mVectorArrowPointPositionBuffer.emplace_back(stemEndpoint, planeIdf);

        // Left
        vec2f leftDir = vec2f(-vector[i].dot(XMatrixLeft), -vector[i].dot(YMatrixLeft)).normalise();
        mVectorArrowPointPositionBuffer.emplace_back(stemEndpoint, planeIdf);
        mVectorArrowPointPositionBuffer.emplace_back(stemEndpoint + leftDir * 0.2f, planeIdf);

        // Right
        vec2f rightDir = vec2f(-vector[i].dot(XMatrixRight), -vector[i].dot(YMatrixRight)).normalise();
        mVectorArrowPointPositionBuffer.emplace_back(stemEndpoint, planeIdf);
        mVectorArrowPointPositionBuffer.emplace_back(stemEndpoint + rightDir * 0.2f, planeIdf);
    }


    //
    // Upload buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mVectorArrowPointPositionVBO);
    glBufferData(GL_ARRAY_BUFFER, mVectorArrowPointPositionBuffer.size() * sizeof(vec3f), mVectorArrowPointPositionBuffer.data(), GL_DYNAMIC_DRAW);
    CheckOpenGLError();


    //
    // Store color
    //

    mVectorArrowColor = color;
}

void ShipRenderContext::RenderEnd()
{
    //
    // Disable vertex attribute 0, as we won't use it in here (it's all dedicated)
    //

    glDisableVertexAttribArray(0);


    ///////////////////////////////////////////////
    //
    // Draw all layers
    //
    ///////////////////////////////////////////////

    // TODO: this will go with orphaned points rearc, will become a single Render invoked right after triangles
    //
    // Draw points
    //

    if (mDebugShipRenderMode == DebugShipRenderMode::Points)
    {
        RenderPointElements();
    }


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
        || (mDebugShipRenderMode == DebugShipRenderMode::None
            && (mShipRenderMode == ShipRenderMode::Structure || mShipRenderMode == ShipRenderMode::Texture)))
    {
        RenderTriangleElements(mShipRenderMode == ShipRenderMode::Texture);
    }



    //
    // Draw ropes, unless it's a debug mode
    //
    // Note: in springs or edge springs debug mode, all ropes are uploaded as springs
    //

    if (mDebugShipRenderMode == DebugShipRenderMode::None)
    {
        RenderRopeElements();
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
        RenderSpringElements(mDebugShipRenderMode == DebugShipRenderMode::None && mShipRenderMode == ShipRenderMode::Texture);
    }



    //
    // Draw stressed springs
    //

    if (mDebugShipRenderMode == DebugShipRenderMode::None
        && mShowStressedSprings)
    {
        RenderStressedSpringElements();
    }



    //
    // Draw ephemeral points
    //

    RenderEphemeralPoints();



    //
    // Draw Generic textures
    //

    RenderGenericTextures();



    //
    // Render vectors, if we're asked to
    //

    if (mVectorFieldRenderMode != VectorFieldRenderMode::None)
    {
        RenderVectors();
    }



    //
    // Update stats
    //

    mRenderStatistics.LastRenderedShipPlanes += mMaxMaxPlaneId + 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ShipRenderContext::RenderPointElements()
{
    // Use color program
    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();

    // Set point size
    glPointSize(0.2f * 2.0f * mViewModel.GetCanvasToVisibleWorldHeightRatio());

    // Bind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mPointElementVBO);
    CheckOpenGLError();

    // Draw
    glDrawElements(GL_POINTS, static_cast<GLsizei>(1 * mPointElementBuffer.size()), GL_UNSIGNED_INT, 0);
}

void ShipRenderContext::RenderSpringElements(bool withTexture)
{
    if (withTexture)
    {
        // Use texture program
        mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();

        // Bind texture
        mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();
        assert(!!mElementShipTexture);
        glBindTexture(GL_TEXTURE_2D, *mElementShipTexture);
        CheckOpenGLError();
    }
    else
    {
        // Use color program
        mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    }

    // Set line size
    glLineWidth(0.1f * 2.0f * mViewModel.GetCanvasToVisibleWorldHeightRatio());

    // Bind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mSpringElementVBO);
    CheckOpenGLError();

    // Draw
    glDrawElements(GL_LINES, static_cast<GLsizei>(2 * mSpringElementBuffer.size()), GL_UNSIGNED_INT, 0);

    // Update stats
    mRenderStatistics.LastRenderedShipSprings += mSpringElementBuffer.size();
}

void ShipRenderContext::RenderRopeElements()
{
    // Use rope program
    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();

    // Set line size
    glLineWidth(0.1f * 2.0f * mViewModel.GetCanvasToVisibleWorldHeightRatio());

    // Bind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mRopeElementVBO);
    CheckOpenGLError();

    // Draw
    glDrawElements(GL_LINES, static_cast<GLsizei>(2 * mRopeElementBuffer.size()), GL_UNSIGNED_INT, 0);

    // Update stats
    mRenderStatistics.LastRenderedShipRopes += mRopeElementBuffer.size();
}

void ShipRenderContext::RenderTriangleElements(bool withTexture)
{
    if (withTexture)
    {
        // Use texture program
        mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();

        // Bind texture
        mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();
        assert(!!mElementShipTexture);
        glBindTexture(GL_TEXTURE_2D, *mElementShipTexture);
    }
    else
    {
        // Use color program
        mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    }

    if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
        glLineWidth(0.1f);

    // Bind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mTriangleElementVBO);
    CheckOpenGLError();

    // Draw
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(3 * mTriangleElementBuffer.size()), GL_UNSIGNED_INT, 0);

    // Update stats
    mRenderStatistics.LastRenderedShipTriangles += mTriangleElementBuffer.size();
}

void ShipRenderContext::RenderStressedSpringElements()
{
    if (!mStressedSpringElementBuffer.empty())
    {
        // Use program
        mShaderManager.ActivateProgram<ProgramType::ShipStressedSprings>();

        // Set line size
        glLineWidth(0.1f * 2.0f * mViewModel.GetCanvasToVisibleWorldHeightRatio());

        // Bind texture
        mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();
        glBindTexture(GL_TEXTURE_2D, *mElementStressedSpringTexture);
        CheckOpenGLError();

        // Bind VBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mStressedSpringElementVBO);
        CheckOpenGLError();

        // Draw
        glDrawElements(GL_LINES, static_cast<GLsizei>(2 * mStressedSpringElementBuffer.size()), GL_UNSIGNED_INT, 0);
    }
}

void ShipRenderContext::RenderGenericTextures()
{
    if (mGenericTextureMaxPlaneVertexBufferSize > 0)
    {
        // Bind VBO
        glBindBuffer(GL_ARRAY_BUFFER, *mGenericTextureRenderPolygonVertexVBO);

        // (Re-)Allocate vertex buffer, if needed
        if (mGenericTextureRenderPolygonVertexAllocatedSize != mGenericTextureMaxPlaneVertexBufferSize)
        {
            glBufferData(GL_ARRAY_BUFFER, mGenericTextureMaxPlaneVertexBufferSize * sizeof(TextureRenderPolygonVertex), nullptr, GL_DYNAMIC_DRAW);
            CheckOpenGLError();

            mGenericTextureRenderPolygonVertexAllocatedSize = mGenericTextureMaxPlaneVertexBufferSize;
        }

        //
        // Draw, separately for each plane
        //

        for (auto const & plane : mGenericTexturePlanes)
        {
            if (!plane.VertexBuffer.empty())
            {
                //
                // Upload vertex buffer
                //

                assert(plane.VertexBuffer.size() <= mGenericTextureRenderPolygonVertexAllocatedSize);

                glBufferSubData(
                    GL_ARRAY_BUFFER,
                    0,
                    plane.VertexBuffer.size() * sizeof(TextureRenderPolygonVertex),
                    plane.VertexBuffer.data());

                CheckOpenGLError();


                //
                // Render
                //

                // Use program
                mShaderManager.ActivateProgram<ProgramType::ShipGenericTextures>();

                if (mDebugShipRenderMode == DebugShipRenderMode::Wireframe)
                    glLineWidth(0.1f);

                // Draw polygons
                glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(plane.VertexBuffer.size()));

                // Update stats
                mRenderStatistics.LastRenderedShipGenericTextures += plane.VertexBuffer.size() / 6;
            }
        }
    }
}

void ShipRenderContext::RenderEphemeralPoints()
{
    if (!mEphemeralPoints.empty())
    {
        // Use color program
        mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();

        // Set point size
        glPointSize(0.3f * mViewModel.GetCanvasToVisibleWorldHeightRatio());

        // Bind VBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mEphemeralPointVBO);
        CheckOpenGLError();

        // Draw
        glDrawElements(GL_POINTS, static_cast<GLsizei>(mEphemeralPoints.size()), GL_UNSIGNED_INT, 0);

        // Update stats
        mRenderStatistics.LastRenderedShipEphemeralPoints += mEphemeralPoints.size();
    }
}

void ShipRenderContext::RenderVectors()
{
    // Use matte program
    mShaderManager.ActivateProgram<ProgramType::ShipVectors>();

    // Set line size
    glLineWidth(0.5f);

    // Set vector color
    mShaderManager.SetProgramParameter<ProgramType::ShipVectors, ProgramParameterType::MatteColor>(
        mVectorArrowColor.x,
        mVectorArrowColor.y,
        mVectorArrowColor.z,
        mVectorArrowColor.w);

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, *mVectorArrowPointPositionVBO);
    CheckOpenGLError();

    // Describe buffer
    glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::SharedAttribute0), 3, GL_FLOAT, GL_FALSE, sizeof(vec3f), (void*)(0));
    CheckOpenGLError();

    // Enable vertex attribute 0
    glEnableVertexAttribArray(0);
    CheckOpenGLError();

    // Draw
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mVectorArrowPointPositionBuffer.size()));
}

}