/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "RenderCore.h"
#include "ShipDefinition.h"
#include "TextureAtlas.h"
#include "ViewModel.h"

#include <GameOpenGL/GameOpenGL.h>
#include <GameOpenGL/ShaderManager.h>

#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/SysSpecifics.h>
#include <GameCore/Vectors.h>

#include <array>
#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Render {

class ShipRenderContext
{
public:

    ShipRenderContext(
        ShipId shipId,
        size_t shipCount,
        size_t pointCount,
        RgbaImageData texture,
        ShipDefinition::TextureOriginType textureOrigin,
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
        bool showStressedSprings);

    ~ShipRenderContext();

public:

    void OnViewModelUpdated()
    {
        // Recalculate ortho matrices
        UpdateOrthoMatrices();
    }

    void SetShipCount(size_t shipCount)
    {
        mShipCount = shipCount;

        // Recalculate ortho matrices
        UpdateOrthoMatrices();
    }

    void SetAmbientLightIntensity(float ambientLightIntensity)
    {
        mAmbientLightIntensity = ambientLightIntensity;

        // Set parameters
        OnAmbientLightIntensityUpdated();
    }

    void SetWaterContrast(float waterContrast)
    {
        mWaterContrast = waterContrast;

        // Set parameters
        OnWaterContrastUpdated();
    }

    void SetWaterLevelThreshold(float waterLevelOfDetail)
    {
        mWaterLevelOfDetail = waterLevelOfDetail;

        // Set parameters
        OnWaterLevelOfDetailUpdated();
    }

    void SetShipRenderMode(ShipRenderMode shipRenderMode)
    {
        mShipRenderMode = shipRenderMode;
    }

    void SetDebugShipRenderMode(DebugShipRenderMode debugShipRenderMode)
    {
        mDebugShipRenderMode = debugShipRenderMode;
    }

    void SetVectorFieldRenderMode(VectorFieldRenderMode vectorFieldRenderMode)
    {
        mVectorFieldRenderMode = vectorFieldRenderMode;
    }

    void SetShowStressedSprings(bool showStressedSprings)
    {
        mShowStressedSprings = showStressedSprings;
    }

public:

    void RenderStart();

    //
    // Points
    //

    void UploadPointImmutableGraphicalAttributes(
        vec4f const * color,
        vec2f const * textureCoordinates);

    void UploadShipPointColorRange(
        vec4f const * color,
        size_t startIndex,
        size_t count);

    void UploadPoints(
        vec2f const * position,
        float const * light,
        float const * water);

    void UploadPointPlaneIds(
        PlaneId const * planeId,
        PlaneId maxMaxPlaneId);


    //
    // Triangle Elements
    //

    void UploadElementTrianglesStart(size_t trianglesCount);

    inline void UploadElementTriangle(
        size_t triangleIndex,
        int pointIndex1,
        int pointIndex2,
        int pointIndex3)
    {
        assert(triangleIndex < mTriangleElementBuffer.size());

        TriangleElement & triangleElement = mTriangleElementBuffer[triangleIndex];

        triangleElement.pointIndex1 = pointIndex1;
        triangleElement.pointIndex2 = pointIndex2;
        triangleElement.pointIndex3 = pointIndex3;
    }

    void UploadElementTrianglesEnd();

    //
    // Other Elements
    //

    void UploadElementsStart();

    inline void UploadElementPoint(int pointIndex)
    {
        mPointElementBuffer.emplace_back();
        PointElement & pointElement = mPointElementBuffer.back();

        pointElement.pointIndex = pointIndex;
    }

    inline void UploadElementSpring(
        int pointIndex1,
        int pointIndex2)
    {
        mSpringElementBuffer.emplace_back();
        SpringElement & springElement = mSpringElementBuffer.back();

        springElement.pointIndex1 = pointIndex1;
        springElement.pointIndex2 = pointIndex2;
    }

    inline void UploadElementRope(
        int pointIndex1,
        int pointIndex2)
    {
        mRopeElementBuffer.emplace_back();
        RopeElement & ropeElement = mRopeElementBuffer.back();

        ropeElement.pointIndex1 = pointIndex1;
        ropeElement.pointIndex2 = pointIndex2;
    }


    void UploadElementsEnd();

    //
    // Stressed springs
    //

    void UploadElementStressedSpringsStart();

    inline void UploadElementStressedSpring(
        int pointIndex1,
        int pointIndex2)
    {
        mStressedSpringElementBuffer.emplace_back();
        StressedSpringElement & stressedSpringElement = mStressedSpringElementBuffer.back();

        stressedSpringElement.pointIndex1 = pointIndex1;
        stressedSpringElement.pointIndex2 = pointIndex2;
    }

    void UploadElementStressedSpringsEnd();


    //
    // Generic textures
    //

    inline void UploadGenericTextureRenderSpecification(
        ConnectedComponentId connectedComponentId,
        TextureFrameId const & textureFrameId,
        vec2f const & position)
    {
        UploadGenericTextureRenderSpecification(
            connectedComponentId,
            textureFrameId,
            position,
            1.0f,
            0.0f,
            1.0f);
    }

    inline void UploadGenericTextureRenderSpecification(
        ConnectedComponentId connectedComponentId,
        TextureFrameId const & textureFrameId,
        vec2f const & position,
        float scale,
        vec2f const & rotationBase,
        vec2f const & rotationOffset,
        float alpha)
    {
        UploadGenericTextureRenderSpecification(
            connectedComponentId,
            textureFrameId,
            position,
            scale,
            rotationBase.angle(rotationOffset),
            alpha);
    }

    inline void UploadGenericTextureRenderSpecification(
        ConnectedComponentId connectedComponentId,
        TextureFrameId const & textureFrameId,
        vec2f const & position,
        float scale,
        float angle,
        float alpha)
    {
        size_t const connectedComponentIndex = connectedComponentId;

        assert(connectedComponentIndex < mGenericTextureConnectedComponents.size());

        // Get this connected component's vertex buffer
        auto & vertexBuffer = mGenericTextureConnectedComponents[connectedComponentIndex].VertexBuffer;

        //
        // Populate the texture quad
        //

        TextureAtlasFrameMetadata const & frame = mTextureAtlasMetadata.GetFrameMetadata(textureFrameId);

        float const leftX = -frame.FrameMetadata.AnchorWorldX;
        float const rightX = frame.FrameMetadata.WorldWidth - frame.FrameMetadata.AnchorWorldX;
        float const topY = frame.FrameMetadata.WorldHeight - frame.FrameMetadata.AnchorWorldY;
        float const bottomY = -frame.FrameMetadata.AnchorWorldY;

        float const lightSensitivity =
            frame.FrameMetadata.HasOwnAmbientLight ? 0.0f : 1.0f;

        // Append vertices - two triangles

        // Triangle 1

        // Top-left
        vertexBuffer.emplace_back(
            position,
            vec2f(leftX, topY),
            vec2f(frame.TextureCoordinatesBottomLeft.x, frame.TextureCoordinatesTopRight.y),
            scale,
            angle,
            alpha,
            lightSensitivity);

        // Top-Right
        vertexBuffer.emplace_back(
            position,
            vec2f(rightX, topY),
            frame.TextureCoordinatesTopRight,
            scale,
            angle,
            alpha,
            lightSensitivity);

        // Bottom-left
        vertexBuffer.emplace_back(
            position,
            vec2f(leftX, bottomY),
            frame.TextureCoordinatesBottomLeft,
            scale,
            angle,
            alpha,
            lightSensitivity);

        // Triangle 2

        // Top-Right
        vertexBuffer.emplace_back(
            position,
            vec2f(rightX, topY),
            frame.TextureCoordinatesTopRight,
            scale,
            angle,
            alpha,
            lightSensitivity);

        // Bottom-left
        vertexBuffer.emplace_back(
            position,
            vec2f(leftX, bottomY),
            frame.TextureCoordinatesBottomLeft,
            scale,
            angle,
            alpha,
            lightSensitivity);

        // Bottom-right
        vertexBuffer.emplace_back(
            position,
            vec2f(rightX, bottomY),
            vec2f(frame.TextureCoordinatesTopRight.x, frame.TextureCoordinatesBottomLeft.y),
            scale,
            angle,
            alpha,
            lightSensitivity);

        // Update max size among all connected components
        mGenericTextureMaxVertexBufferSize = std::max(mGenericTextureMaxVertexBufferSize, vertexBuffer.size());
    }


    //
    // Ephemeral points
    //

    void UploadEphemeralPointsStart();

    inline void UploadEphemeralPoint(
        int pointIndex)
    {
        mEphemeralPoints.emplace_back();

        mEphemeralPoints.back().pointIndex = pointIndex;
    }

    void UploadEphemeralPointsEnd();


    //
    // Vectors
    //

    void UploadVectors(
        size_t count,
        vec2f const * position,
        vec2f const * vector,
        float lengthAdjustment,
        vec4f const & color);

    void RenderEnd();

private:

    void UpdateOrthoMatrices();
    void OnAmbientLightIntensityUpdated();
    void OnWaterContrastUpdated();
    void OnWaterLevelOfDetailUpdated();

    // TODO: this will go
    struct GenericTextureConnectedComponentData;

    void RenderPointElements();

    void RenderSpringElements(bool withTexture);

    void RenderRopeElements();

    void RenderTriangleElements(bool withTexture);

    void RenderStressedSpringElements();

    void RenderGenericTextures(GenericTextureConnectedComponentData const & connectedComponent);

    void RenderEphemeralPoints();

    void RenderVectors();

private:

    ShipId const mShipId;
    size_t mShipCount;
    PlaneId mMaxMaxPlaneId;

    //
    // Parameters
    //

    ViewModel const & mViewModel;

    float mAmbientLightIntensity;
    float mWaterContrast;
    float mWaterLevelOfDetail;
    ShipRenderMode mShipRenderMode;
    DebugShipRenderMode mDebugShipRenderMode;
    VectorFieldRenderMode mVectorFieldRenderMode;
    bool mShowStressedSprings;

private:

    ShaderManager<ShaderManagerTraits> & mShaderManager;

    RenderStatistics & mRenderStatistics;


    //
    // Textures
    //

    GameOpenGLTexture mElementShipTexture;
    GameOpenGLTexture mElementStressedSpringTexture;

    //
    // Points
    //

    size_t const mPointCount;

    GameOpenGLVBO mPointPositionVBO;
    GameOpenGLVBO mPointLightVBO;
    GameOpenGLVBO mPointWaterVBO;
    GameOpenGLVBO mPointColorVBO;
    GameOpenGLVBO mPointPlaneIdVBO;
    GameOpenGLVBO mPointElementTextureCoordinatesVBO;

    //
    // Generic Textures
    //

    GameOpenGLTexture & mTextureAtlasOpenGLHandle;
    TextureAtlasMetadata const & mTextureAtlasMetadata;

#pragma pack(push)
struct TextureRenderPolygonVertex
{
    vec2f centerPosition;
    vec2f vertexOffset;
    vec2f textureCoordinate;

    float scale;
    float angle;
    float alpha;
    float ambientLightSensitivity;

    TextureRenderPolygonVertex(
        vec2f _centerPosition,
        vec2f _vertexOffset,
        vec2f _textureCoordinate,
        float _scale,
        float _angle,
        float _alpha,
        float _ambientLightSensitivity)
        : centerPosition(_centerPosition)
        , vertexOffset(_vertexOffset)
        , textureCoordinate(_textureCoordinate)
        , scale(_scale)
        , angle(_angle)
        , alpha(_alpha)
        , ambientLightSensitivity(_ambientLightSensitivity)
    {}
};
#pragma pack(pop)

    struct GenericTextureConnectedComponentData
    {
        std::vector<TextureRenderPolygonVertex> VertexBuffer;
    };

    std::vector<GenericTextureConnectedComponentData> mGenericTextureConnectedComponents;
    size_t mGenericTextureMaxVertexBufferSize;
    size_t mGenericTextureAllocatedVertexBufferSize;

    GameOpenGLVBO mGenericTextureRenderPolygonVertexVBO;



    //
    // Elements
    //

#pragma pack(push)
    struct PointElement
    {
        int pointIndex;
    };
#pragma pack(pop)

#pragma pack(push)
    struct SpringElement
    {
        int pointIndex1;
        int pointIndex2;
    };
#pragma pack(pop)

#pragma pack(push)
    struct RopeElement
    {
        int pointIndex1;
        int pointIndex2;
    };
#pragma pack(pop)

#pragma pack(push)
    struct TriangleElement
    {
        int pointIndex1;
        int pointIndex2;
        int pointIndex3;
    };
#pragma pack(pop)

#pragma pack(push)
    struct StressedSpringElement
    {
        int pointIndex1;
        int pointIndex2;
    };
#pragma pack(pop)

    std::vector<PointElement> mPointElementBuffer;
    GameOpenGLVBO mPointElementVBO;

    std::vector<SpringElement> mSpringElementBuffer;
    GameOpenGLVBO mSpringElementVBO;

    std::vector<RopeElement> mRopeElementBuffer;
    GameOpenGLVBO mRopeElementVBO;

    std::vector<TriangleElement> mTriangleElementBuffer;
    GameOpenGLVBO mTriangleElementVBO;

    std::vector<StressedSpringElement> mStressedSpringElementBuffer;
    GameOpenGLVBO mStressedSpringElementVBO;


    //
    // Ephemeral points
    //

    std::vector<PointElement> mEphemeralPoints;

    GameOpenGLVBO mEphemeralPointVBO;


    //
    // Vectors
    //

    std::vector<vec2f> mVectorArrowPointPositionBuffer;
    GameOpenGLVBO mVectorArrowPointPositionVBO;
    vec4f mVectorArrowColor;
};

}