/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL.h"
#include "GameTypes.h"
#include "ImageData.h"
#include "SysSpecifics.h"
#include "TextureRenderManager.h"
#include "TextureTypes.h"
#include "Vectors.h"

#include <array>
#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class ShipRenderContext
{
public:

    ShipRenderContext(
        std::optional<ImageData> texture,
        vec3f const & ropeColour,
        TextureRenderManager const & textureRenderManager,
        float const(&orthoMatrix)[4][4],
        float visibleWorldHeight,
        float visibleWorldWidth,
        float canvasToVisibleWorldHeightRatio,
        float ambientLightIntensity,
        float waterLevelOfDetail,
        ShipRenderMode shipRenderMode,
        VectorFieldRenderMode vectorFieldRenderMode,
        bool showStressedSprings);
    
    ~ShipRenderContext();

public:

    void UpdateOrthoMatrix(float const(&orthoMatrix)[4][4]);

    void UpdateVisibleWorldCoordinates(
        float visibleWorldHeight,
        float visibleWorldWidth,
        float canvasToVisibleWorldHeightRatio);

    void UpdateAmbientLightIntensity(float ambientLightIntensity);

    void UpdateWaterLevelThreshold(float waterLevelOfDetail);

    void UpdateShipRenderMode(ShipRenderMode shipRenderMode);

    void UpdateVectorFieldRenderMode(VectorFieldRenderMode vectorFieldRenderMode);

    void UpdateShowStressedSprings(bool showStressedSprings);

public:

    void RenderStart(std::vector<std::size_t> const & connectedComponentsMaxSizes);

    //
    // Points
    //

    void UploadPointImmutableGraphicalAttributes(
        size_t count,
        vec3f const * restrict color,
        vec2f const * restrict textureCoordinates);

    void UploadPoints(
        size_t count,
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
            std::nullopt);
    }

    inline void UploadGenericTextureRenderSpecification(
        ConnectedComponentId connectedComponentId,
        TextureFrameId const & textureFrameId,
        vec2f const & position,
        float scale,
        std::optional<std::pair<vec2f, vec2f>> const & orientation)
    {
        size_t const connectedComponentIndex = connectedComponentId - 1;

        assert(connectedComponentIndex < mConnectedComponentGenericTextureInfos.size());

        // Store <Index in TexturePolygonVertices, textureFrameId> in per-connected component data
        mConnectedComponentGenericTextureInfos[connectedComponentIndex].emplace_back(
            mGenericTextureRenderPolygonVertexBuffer.size(),
            textureFrameId);

        // Append vertices
        mTextureRenderManager.AddRenderPolygon(
            textureFrameId,
            position,
            scale,
            orientation,
            mGenericTextureRenderPolygonVertexBuffer);
    }



    // TODOOLD

    /*
    void UploadElementPinnedPointsStart(size_t count);

    inline void UploadElementPinnedPoint(
        float pinnedPointX,
        float pinnedPointY,
        ConnectedComponentId connectedComponentId)
    {
        size_t const connectedComponentIndex = connectedComponentId - 1;

        assert(connectedComponentIndex < mConnectedComponents.size());

        // Insert at end of this connected component's pinned points
        auto insertedIt = mPinnedPointElementBuffer.emplace(
            mPinnedPointElementBuffer.begin()
            + mConnectedComponents[connectedComponentIndex].pinnedPointElementOffset
            + mConnectedComponents[connectedComponentIndex].pinnedPointElementCount);

        PinnedPointElement & pinnedPointElement = *insertedIt;

        TextureFrameMetadata const & textureMetadata = mTextureRenderManager.GetFrameMetadata(
            TextureGroupType::PinnedPoint,
            0);

        // TODOHERE: double-check
        float leftX = pinnedPointX - textureMetadata.AnchorWorldX;
        float rightX = pinnedPointX + (textureMetadata.WorldWidth - textureMetadata.AnchorWorldX);
        float topY = pinnedPointY - textureMetadata.AnchorWorldY;
        float bottomY = pinnedPointY + (textureMetadata.WorldHeight - textureMetadata.AnchorWorldY);

        pinnedPointElement.xTopLeft = leftX;
        pinnedPointElement.yTopLeft = topY;
        pinnedPointElement.textureXTopLeft = 0.0f;
        pinnedPointElement.textureYTopLeft = 0.0f;

        pinnedPointElement.xBottomLeft = leftX;
        pinnedPointElement.yBottomLeft = bottomY;
        pinnedPointElement.textureXBottomLeft = 0.0f;
        pinnedPointElement.textureYBottomLeft = 1.0f;

        pinnedPointElement.xTopRight = rightX;
        pinnedPointElement.yTopRight = topY;
        pinnedPointElement.textureXTopRight = 1.0f;
        pinnedPointElement.textureYTopRight = 0.0f;

        pinnedPointElement.xBottomRight = rightX;
        pinnedPointElement.yBottomRight = bottomY;
        pinnedPointElement.textureXBottomRight = 1.0f;
        pinnedPointElement.textureYBottomRight = 1.0f;
        

        //
        // Adjust connected components
        //

        ++(mConnectedComponents[connectedComponentIndex].pinnedPointElementCount);

        for (auto c = connectedComponentIndex + 1; c < mConnectedComponents.size(); ++c)
        {
            ++(mConnectedComponents[c].pinnedPointElementOffset);
        }        
    }

    void UploadElementPinnedPointsEnd();


    void UploadElementBombsStart(size_t count);

    inline void UploadElementBomb(
        BombType bombType,
        RotatedTextureRenderInfo const & renderInfo,
        std::optional<TextureFrameId> lightedFrameId,
        std::optional<TextureFrameId> unlightedFrameId,
        ConnectedComponentId connectedComponentId)
    {
        size_t const connectedComponentIndex = connectedComponentId - 1;

        assert(connectedComponentIndex < mConnectedComponents.size());

        //
        // Store bomb element
        //

        // Insert at end of this connected component's bombs
        auto insertedIt = mBombElementBuffer.emplace(
            mBombElementBuffer.begin()
            + mConnectedComponents[connectedComponentIndex].bombElementOffset
            + mConnectedComponents[connectedComponentIndex].bombElementInfos.size());

        BombElement & bombElement = *insertedIt;
        
        // TODOHERE: we are doing it for both textures temporarily; this invocation will however become
        // two calls soon
        TextureFrameId textureFrameId = (!!lightedFrameId ? *lightedFrameId : *unlightedFrameId);
        TextureFrameMetadata const & textureMetadata = mTextureRenderManager.GetFrameMetadata(textureFrameId);

        // World size that the texture should be scaled to
        static float const textureTileW = textureMetadata.WorldWidth;
        static float const textureTileH = textureMetadata.WorldHeight;

        // Offsets
        // TODO: temporary, until we use Textures.json
        float offsetX = bombType == BombType::TimerBomb ? 0.12f : 0.0f;
        float offsetY = bombType == BombType::TimerBomb ? 3.12f : 0.0f;

        RotatedRectangle frameRect = renderInfo.CalculateRotatedRectangle(textureTileW, textureTileH);
        
        bombElement.xTopLeft = frameRect.TopLeft.x + offsetX;
        bombElement.yTopLeft = frameRect.TopLeft.y + offsetY;
        bombElement.textureXTopLeft = 0.0f;
        bombElement.textureYTopLeft = 0.0f;

        bombElement.xBottomLeft = frameRect.BottomLeft.x + offsetX;
        bombElement.yBottomLeft = frameRect.BottomLeft.y + offsetY;
        bombElement.textureXBottomLeft = 0.0f;
        bombElement.textureYBottomLeft = 1.0f;

        bombElement.xTopRight = frameRect.TopRight.x + offsetX;
        bombElement.yTopRight = frameRect.TopRight.y + offsetY;
        bombElement.textureXTopRight = 1.0f;
        bombElement.textureYTopRight = 0.0f;

        bombElement.xBottomRight = frameRect.BottomRight.x + offsetX;
        bombElement.yBottomRight = frameRect.BottomRight.y + offsetY;
        bombElement.textureXBottomRight = 1.0f;
        bombElement.textureYBottomRight = 1.0f;


        //
        // Store bomb info
        //

        mConnectedComponents[connectedComponentIndex].bombElementInfos.emplace_back(
            bombType,
            lightedFrameId,
            unlightedFrameId);
       

        //
        // Adjust connected components
        //

        for (auto c = connectedComponentIndex + 1; c < mConnectedComponents.size(); ++c)
        {
            ++(mConnectedComponents[c].bombElementOffset);
        }
    }

    void UploadElementBombsEnd();
    */


    //
    // Vectors
    //

    void UploadVectors(
        size_t count,
        vec2f const * restrict position,
        vec2f const * restrict vector,
        float lengthAdjustment,
        vec3f const & color);    

    void RenderEnd();

private:
    
    struct ConnectedComponentData;
    struct GenericTextureInfo;

    void RenderPointElements(ConnectedComponentData const & connectedComponent);

    void RenderSpringElements(
        ConnectedComponentData const & connectedComponent,
        bool withTexture);

    void RenderRopeElements(ConnectedComponentData const & connectedComponent);

    void RenderTriangleElements(
        ConnectedComponentData const & connectedComponent,
        bool withTexture);

    void RenderStressedSpringElements(ConnectedComponentData const & connectedComponent);

    void RenderGenericTextures(std::vector<GenericTextureInfo> const & connectedComponent);

    void RenderVectors();

private:

    //
    // Parameters
    //

    float mCanvasToVisibleWorldHeightRatio;
    float mAmbientLightIntensity;
    float mWaterLevelThreshold;
    ShipRenderMode mShipRenderMode;
    VectorFieldRenderMode mVectorFieldRenderMode;
    bool mShowStressedSprings;

private:

    // Vertex attribute indices
    static constexpr GLuint PointPosVertexAttribute = 0;
    static constexpr GLuint PointLightVertexAttribute = 1;
    static constexpr GLuint PointWaterVertexAttribute = 2;
    static constexpr GLuint PointColorVertexAttribute = 3;
    static constexpr GLuint PointTextureCoordinatesVertexAttribute = 4;
    static constexpr GLuint GenericTexturePosVertexAttribute = 5;
    static constexpr GLuint GenericTextureTextureCoordinatesVertexAttribute = 6;
    static constexpr GLuint GenericTextureAmbientLightSensitivityVertexAttribute = 7;
    static constexpr GLuint VectorArrowPosVertexAttribute = 8;

private:

    //
    // Textures
    //

    GameOpenGLTexture mElementShipTexture;
    GameOpenGLTexture mElementStressedSpringTexture;

    //
    // Points
    //
    
    size_t mPointCount;

    GameOpenGLVBO mPointPositionVBO;
    GameOpenGLVBO mPointLightVBO;
    GameOpenGLVBO mPointWaterVBO;
    GameOpenGLVBO mPointColorVBO;
    GameOpenGLVBO mPointElementTextureCoordinatesVBO;
    
    //
    // Elements (points, springs, ropes, triangles, stressed springs)
    //

    GameOpenGLShaderProgram mElementColorShaderProgram;
    GLint mElementColorShaderOrthoMatrixParameter;
    GLint mElementColorShaderAmbientLightIntensityParameter;
    GLint mElementColorShaderWaterLevelThresholdParameter;

    GameOpenGLShaderProgram mElementRopeShaderProgram;
    GLint mElementRopeShaderOrthoMatrixParameter;
    GLint mElementRopeShaderAmbientLightIntensityParameter;
    GLint mElementRopeShaderWaterLevelThresholdParameter;
    
    GameOpenGLShaderProgram mElementShipTextureShaderProgram;
    GLint mElementShipTextureShaderOrthoMatrixParameter;
    GLint mElementShipTextureShaderAmbientLightIntensityParameter;
    GLint mElementShipTextureShaderWaterLevelThresholdParameter;

    GameOpenGLShaderProgram mElementStressedSpringShaderProgram;
    GLint mElementStressedSpringShaderOrthoMatrixParameter;

    //
    // Generic Textures
    //

    TextureRenderManager const & mTextureRenderManager;

    struct GenericTextureInfo
    {
        size_t polygonIndex;
        TextureFrameId frameId;

        GenericTextureInfo(
            size_t _polygonIndex,
            TextureFrameId _frameId)
            : polygonIndex(_polygonIndex)
            , frameId(_frameId)
        {}
    };

    std::vector<std::vector<GenericTextureInfo>> mConnectedComponentGenericTextureInfos;

    std::vector<TextureRenderPolygonVertex> mGenericTextureRenderPolygonVertexBuffer;
    GameOpenGLVBO mGenericTextureRenderPolygonVertexVBO;
    GameOpenGLShaderProgram mGenericTextureShaderProgram;
    GLint mGenericTextureShaderOrthoMatrixParameter;
    GLint mGenericTextureShaderAmbientLightIntensityParameter;


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
    // Vectors
    //

    std::vector<vec2f> mVectorArrowPointPositionBuffer;
    GameOpenGLVBO mVectorArrowPointPositionVBO;
    GameOpenGLShaderProgram mVectorArrowShaderProgram;
    GLint mVectorArrowShaderOrthoMatrixParameter;
    GLint mVectorArrowShaderColorParameter;
};
