/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL.h"
#include "GameTypes.h"
#include "ImageData.h"
#include "RotatedTextureRenderInfo.h"
#include "SysSpecifics.h"
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
        GLuint pinnedPointTexture,
        std::vector<GLuint> rcBombTextures,
        std::vector<GLuint> timerBombTextures,
        float const(&orthoMatrix)[4][4],
        float visibleWorldHeight,
        float visibleWorldWidth,
        float canvasToVisibleWorldHeightRatio,
        float ambientLightIntensity,
        float waterLevelOfDetail);
    
    ~ShipRenderContext();

public:

    void UpdateOrthoMatrix(float const(&orthoMatrix)[4][4]);

    void UpdateVisibleWorldCoordinates(
        float visibleWorldHeight,
        float visibleWorldWidth,
        float canvasToVisibleWorldHeightRatio);

    void UpdateAmbientLightIntensity(float ambientLightIntensity);

    void UpdateWaterLevelOfDetail(float waterLevelOfDetail);

public:

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
    // Springs and triangles
    //

    void UploadElementsStart(std::vector<std::size_t> const & connectedComponentsMaxSizes);

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

        // World size that the texture should be scaled to
        static constexpr float textureTileW = 6.0f;
        static constexpr float textureTileH = 6.0f;
        
        float leftX = pinnedPointX - textureTileW / 2.0f;
        float rightX = pinnedPointX + textureTileW / 2.0f;
        float topY = pinnedPointY - textureTileH / 2.0f;
        float bottomY = pinnedPointY + textureTileH / 2.0f;

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
        std::optional<uint32_t> lightedFrameIndex,
        std::optional<uint32_t> unlightedFrameIndex,
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
       
        // World size that the texture should be scaled to
        static constexpr float textureTileW = 12.0f;
        static constexpr float textureTileH = 12.0f;

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
            lightedFrameIndex,
            unlightedFrameIndex);
       

        //
        // Adjust connected components
        //

        for (auto c = connectedComponentIndex + 1; c < mConnectedComponents.size(); ++c)
        {
            ++(mConnectedComponents[c].bombElementOffset);
        }
    }

    void UploadElementBombsEnd();


    //
    // Lamps
    //

    void UploadLampsStart(size_t connectedComponents);

    void UploadLamp(
        float x,
        float y,
        float lightIntensity,
        ConnectedComponentId connectedComponentId)
    {
        size_t const connectedComponentIndex = connectedComponentId - 1;

        assert(connectedComponentIndex < mLampBuffers.size());

        LampElement & lampElement = mLampBuffers[connectedComponentIndex].emplace_back();

        lampElement.x = x;
        lampElement.y = y;
        lampElement.lightIntensity = lightIntensity;
    }

    void UploadLampsEnd();


    /////////////////////////////////////////////////////////////

    void Render(
        ShipRenderMode renderMode,
        bool showStressedSprings);

private:

    struct ConnectedComponentData;

    void RenderPointElements(ConnectedComponentData const & connectedComponent);

    void RenderSpringElements(
        ConnectedComponentData const & connectedComponent,
        bool withTexture);

    void RenderRopeElements(ConnectedComponentData const & connectedComponent);

    void RenderTriangleElements(
        ConnectedComponentData const & connectedComponent,
        bool withTexture);

    void RenderStressedSpringElements(ConnectedComponentData const & connectedComponent);

    void RenderBombElements(ConnectedComponentData const & connectedComponent);

    void RenderPinnedPointElements(ConnectedComponentData const & connectedComponent);


private:

    float mCanvasToVisibleWorldHeightRatio;
    float mAmbientLightIntensity;
    float mWaterLevelOfDetail;

private:

    // Vertex attribute indices
    static constexpr GLuint PointPosVertexAttribute = 0;
    static constexpr GLuint PointLightVertexAttribute = 1;
    static constexpr GLuint PointWaterVertexAttribute = 2;
    static constexpr GLuint PointColorVertexAttribute = 3;
    static constexpr GLuint PointTextureCoordinatesVertexAttribute = 4;
    static constexpr GLuint PinnedPointPosVertexAttribute = 5;
    static constexpr GLuint PinnedPointTextureCoordinatesVertexAttribute = 6;
    static constexpr GLuint BombPosVertexAttribute = 7;
    static constexpr GLuint BombTextureCoordinatesVertexAttribute = 8;

private:

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
    // Elements (points, springs, ropes, triangles, stressed springs, pinned points, bombs)
    //

    GameOpenGLShaderProgram mElementColorShaderProgram;
    GLint mElementColorShaderOrthoMatrixParameter;
    GLint mElementColorShaderAmbientLightIntensityParameter;
    GLint mElementColorShaderWaterLevelOfDetailParameter;
    // TODO: @ others?

    GameOpenGLShaderProgram mElementRopeShaderProgram;
    GLint mElementRopeShaderOrthoMatrixParameter;
    GLint mElementRopeShaderAmbientLightIntensityParameter;

    GameOpenGLShaderProgram mElementTextureShaderProgram;
    GLint mElementTextureShaderOrthoMatrixParameter;
    GLint mElementTextureShaderAmbientLightIntensityParameter;

    GameOpenGLShaderProgram mElementStressedSpringShaderProgram;
    GLint mElementStressedSpringShaderOrthoMatrixParameter;

    GameOpenGLShaderProgram mElementPinnedPointShaderProgram;
    GLint mElementPinnedPointShaderOrthoMatrixParameter;
    GLint mElementPinnedPointShaderAmbientLightIntensityParameter;

    GameOpenGLShaderProgram mElementBombShaderProgram;
    GLint mElementBombShaderOrthoMatrixParameter;
    GLint mElementBombShaderAmbientLightIntensityParameter;

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

#pragma pack(push)
    struct PinnedPointElement
    {
        float xTopLeft;
        float yTopLeft;
        float textureXTopLeft;
        float textureYTopLeft;

        float xBottomLeft;
        float yBottomLeft;
        float textureXBottomLeft;
        float textureYBottomLeft;

        float xTopRight;
        float yTopRight;
        float textureXTopRight;
        float textureYTopRight;

        float xBottomRight;
        float yBottomRight;
        float textureXBottomRight;
        float textureYBottomRight;
    };
#pragma pack(pop)

#pragma pack(push)
    struct BombElement
    {
        float xTopLeft;
        float yTopLeft;
        float textureXTopLeft;
        float textureYTopLeft;

        float xBottomLeft;
        float yBottomLeft;
        float textureXBottomLeft;
        float textureYBottomLeft;

        float xTopRight;
        float yTopRight;
        float textureXTopRight;
        float textureYTopRight;

        float xBottomRight;
        float yBottomRight;
        float textureXBottomRight;
        float textureYBottomRight;
    };
#pragma pack(pop)

    struct BombElementInfo
    {
        BombType bombType;
        std::optional<uint32_t> lightedFrameIndex;
        std::optional<uint32_t> unlightedFrameIndex;

        BombElementInfo(
            BombType _bombType,
            std::optional<uint32_t> _lightedFrameIndex,
            std::optional<uint32_t> _unlightedFrameIndex)
            : bombType(_bombType)
            , lightedFrameIndex(_lightedFrameIndex)
            , unlightedFrameIndex(_unlightedFrameIndex)
        {}
    };

    //
    // All the data that belongs to a single connected component.
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

        size_t pinnedPointElementOffset;
        size_t pinnedPointElementCount;

        size_t bombElementOffset;
        std::vector<BombElementInfo> bombElementInfos;

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
            , pinnedPointElementOffset(0)
            , pinnedPointElementCount(0)
            , bombElementOffset(0)
            , bombElementInfos()
        {}
    };

    std::vector<ConnectedComponentData> mConnectedComponents;

    //
    // Pinned point data, stored globally once across all connected components.
    //

    std::vector<PinnedPointElement> mPinnedPointElementBuffer;
    GameOpenGLVBO mPinnedPointVBO;

    //
    // Bomb point data, stored globally once across all connected components.
    //

    std::vector<BombElement> mBombElementBuffer;
    GameOpenGLVBO mBombVBO;

    //
    // Textures
    //

    GameOpenGLTexture mElementTexture;
    GameOpenGLTexture mElementStressedSpringTexture;
    GLuint const mElementPinnedPointTexture;
    std::vector<GLuint> const mElementRCBombTextures;
    std::vector<GLuint> const mElementTimerBombTextures;

    //
    // Lamps
    //

#pragma pack(push)
    struct LampElement
    {
        float x;
        float y;
        float lightIntensity;
    };
#pragma pack(pop)

    std::vector<std::vector<LampElement>> mLampBuffers;
};
