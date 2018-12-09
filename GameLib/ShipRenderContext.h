/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL.h"
#include "GameTypes.h"
#include "ImageData.h"
#include "RenderCore.h"
#include "ShaderManager.h"
#include "ShipDefinition.h"
#include "SysSpecifics.h"
#include "Vectors.h"

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
        size_t pointCount,
        ImageData texture,
        ShipDefinition::TextureOriginType textureOrigin,
        ShaderManager<ShaderManagerTraits> & shaderManager,
        GameOpenGLTexture & textureAtlasOpenGLHandle,
        TextureAtlasMetadata const & textureAtlasMetadata,
        RenderStatistics & renderStatistics,
        float const(&orthoMatrix)[4][4],
        float visibleWorldHeight,
        float visibleWorldWidth,
        float canvasToVisibleWorldHeightRatio,
        float ambientLightIntensity,
        float waterContrast,
        float waterLevelOfDetail,
        ShipRenderMode shipRenderMode,
        VectorFieldRenderMode vectorFieldRenderMode,
        bool showStressedSprings,
        bool wireframeMode);

    ~ShipRenderContext();

public:

    void UpdateOrthoMatrix(float const(&orthoMatrix)[4][4]);

    void UpdateVisibleWorldCoordinates(
        float visibleWorldHeight,
        float visibleWorldWidth,
        float canvasToVisibleWorldHeightRatio);

    void UpdateAmbientLightIntensity(float ambientLightIntensity);

    void UpdateWaterContrast(float waterConstrast);

    void UpdateWaterLevelThreshold(float waterLevelOfDetail);

    void UpdateShipRenderMode(ShipRenderMode shipRenderMode);

    void UpdateVectorFieldRenderMode(VectorFieldRenderMode vectorFieldRenderMode);

    void UpdateShowStressedSprings(bool showStressedSprings);

    void UpdateWireframeMode(bool wireframeMode);

public:

    void RenderStart(std::vector<std::size_t> const & connectedComponentsMaxSizes);

    //
    // Points
    //

    void UploadPointImmutableGraphicalAttributes(
        vec4f const * restrict color,
        vec2f const * restrict textureCoordinates);

    void UploadShipPointColorRange(
        vec4f const * restrict color,
        size_t startIndex,
        size_t count);

    void UploadPoints(
        vec2f const * restrict position,
        float const * restrict light,
        float const * restrict water);


    //
    // Elements
    //

    void UploadElementsStart();

    inline void UploadElementPoint(
        int pointIndex,
        ConnectedComponentId connectedComponentId)
    {
        size_t const connectedComponentIndex = connectedComponentId - 1;

        assert(connectedComponentIndex < mConnectedComponents.size());
        assert(mConnectedComponents[connectedComponentIndex].pointElementCount + 1u <= mConnectedComponents[connectedComponentIndex].pointElementMaxCount);

        PointElement * const pointElement = &(mConnectedComponents[connectedComponentIndex].pointElementBuffer[mConnectedComponents[connectedComponentIndex].pointElementCount]);

        pointElement->pointIndex = pointIndex;

        ++(mConnectedComponents[connectedComponentIndex].pointElementCount);
    }

    inline void UploadElementSpring(
        int pointIndex1,
        int pointIndex2,
        ConnectedComponentId connectedComponentId)
    {
        size_t const connectedComponentIndex = connectedComponentId - 1;

        assert(connectedComponentIndex < mConnectedComponents.size());
        assert(mConnectedComponents[connectedComponentIndex].springElementCount + 1u <= mConnectedComponents[connectedComponentIndex].springElementMaxCount);

        SpringElement * const springElement = &(mConnectedComponents[connectedComponentIndex].springElementBuffer[mConnectedComponents[connectedComponentIndex].springElementCount]);

        springElement->pointIndex1 = pointIndex1;
        springElement->pointIndex2 = pointIndex2;

        ++(mConnectedComponents[connectedComponentIndex].springElementCount);
    }

    inline void UploadElementRope(
        int pointIndex1,
        int pointIndex2,
        ConnectedComponentId connectedComponentId)
    {
        size_t const connectedComponentIndex = connectedComponentId - 1;

        assert(connectedComponentIndex < mConnectedComponents.size());
        assert(mConnectedComponents[connectedComponentIndex].ropeElementCount + 1u <= mConnectedComponents[connectedComponentIndex].ropeElementMaxCount);

        RopeElement * const ropeElement = &(mConnectedComponents[connectedComponentIndex].ropeElementBuffer[mConnectedComponents[connectedComponentIndex].ropeElementCount]);

        ropeElement->pointIndex1 = pointIndex1;
        ropeElement->pointIndex2 = pointIndex2;

        ++(mConnectedComponents[connectedComponentIndex].ropeElementCount);
    }

    inline void UploadElementTriangle(
        int pointIndex1,
        int pointIndex2,
        int pointIndex3,
        ConnectedComponentId connectedComponentId)
    {
        size_t const connectedComponentIndex = connectedComponentId - 1;

        assert(connectedComponentIndex < mConnectedComponents.size());
        assert(mConnectedComponents[connectedComponentIndex].triangleElementCount + 1u <= mConnectedComponents[connectedComponentIndex].triangleElementMaxCount);

        TriangleElement * const triangleElement = &(mConnectedComponents[connectedComponentIndex].triangleElementBuffer[mConnectedComponents[connectedComponentIndex].triangleElementCount]);

        triangleElement->pointIndex1 = pointIndex1;
        triangleElement->pointIndex2 = pointIndex2;
        triangleElement->pointIndex3 = pointIndex3;

        ++(mConnectedComponents[connectedComponentIndex].triangleElementCount);
    }

    void UploadElementsEnd();

    void UploadElementStressedSpringsStart();

    inline void UploadElementStressedSpring(
        int pointIndex1,
        int pointIndex2,
        ConnectedComponentId connectedComponentId)
    {
        size_t const connectedComponentIndex = connectedComponentId - 1;

        assert(connectedComponentIndex < mConnectedComponents.size());
        assert(mConnectedComponents[connectedComponentIndex].stressedSpringElementCount + 1u <= mConnectedComponents[connectedComponentIndex].stressedSpringElementMaxCount);

        StressedSpringElement * const stressedSpringElement = &(mConnectedComponents[connectedComponentIndex].stressedSpringElementBuffer[mConnectedComponents[connectedComponentIndex].stressedSpringElementCount]);

        stressedSpringElement->pointIndex1 = pointIndex1;
        stressedSpringElement->pointIndex2 = pointIndex2;

        ++(mConnectedComponents[connectedComponentIndex].stressedSpringElementCount);
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
        size_t const connectedComponentIndex = connectedComponentId - 1;

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
        vec2f const * restrict position,
        vec2f const * restrict vector,
        float lengthAdjustment,
        vec4f const & color);

    void RenderEnd();

private:

    struct ConnectedComponentData;
    struct GenericTextureConnectedComponentData;

    void RenderPointElements(ConnectedComponentData const & connectedComponent);

    void RenderSpringElements(
        ConnectedComponentData const & connectedComponent,
        bool withTexture);

    void RenderRopeElements(ConnectedComponentData const & connectedComponent);

    void RenderTriangleElements(
        ConnectedComponentData const & connectedComponent,
        bool withTexture);

    void RenderStressedSpringElements(ConnectedComponentData const & connectedComponent);

    void RenderGenericTextures(GenericTextureConnectedComponentData const & connectedComponent);

    void RenderEphemeralPoints();

    void RenderVectors();

private:

    //
    // Parameters
    //

    float mCanvasToVisibleWorldHeightRatio;
    float mAmbientLightIntensity;
    float mWaterContrast;
    float mWaterLevelThreshold;
    ShipRenderMode mShipRenderMode;
    VectorFieldRenderMode mVectorFieldRenderMode;
    bool mShowStressedSprings;
    bool mWireframeMode;

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
    // Connected component data
    //

    std::vector<std::size_t> mConnectedComponentsMaxSizes;

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


    //
    // All the data that belongs to a single connected component
    //

    struct ConnectedComponentData
    {
        size_t pointElementCount;
        size_t pointElementMaxCount;
        std::unique_ptr<PointElement[]> pointElementBuffer;
        GameOpenGLVBO pointElementVBO;

        size_t springElementCount;
        size_t springElementMaxCount;
        std::unique_ptr<SpringElement[]> springElementBuffer;
        GameOpenGLVBO springElementVBO;

        size_t ropeElementCount;
        size_t ropeElementMaxCount;
        std::unique_ptr<RopeElement[]> ropeElementBuffer;
        GameOpenGLVBO ropeElementVBO;

        size_t triangleElementCount;
        size_t triangleElementMaxCount;
        std::unique_ptr<TriangleElement[]> triangleElementBuffer;
        GameOpenGLVBO triangleElementVBO;

        size_t stressedSpringElementCount;
        size_t stressedSpringElementMaxCount;
        std::unique_ptr<StressedSpringElement[]> stressedSpringElementBuffer;
        GameOpenGLVBO stressedSpringElementVBO;

        ConnectedComponentData()
            : pointElementCount(0)
            , pointElementMaxCount(0)
            , pointElementBuffer()
            , pointElementVBO()
            , springElementCount(0)
            , springElementMaxCount(0)
            , springElementBuffer()
            , springElementVBO()
            , ropeElementCount(0)
            , ropeElementMaxCount(0)
            , ropeElementBuffer()
            , ropeElementVBO()
            , triangleElementCount(0)
            , triangleElementMaxCount(0)
            , triangleElementBuffer()
            , triangleElementVBO()
            , stressedSpringElementCount(0)
            , stressedSpringElementMaxCount(0)
            , stressedSpringElementBuffer()
            , stressedSpringElementVBO()
        {}
    };

    std::vector<ConnectedComponentData> mConnectedComponents;


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