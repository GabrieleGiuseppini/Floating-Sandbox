/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL.h"
#include "GameTypes.h"
#include "ImageData.h"
#include "ProgressCallback.h"
#include "ResourceLoader.h"
#include "RotatedTextureRenderInfo.h"
#include "ShipRenderContext.h"
#include "SysSpecifics.h"
#include "Vectors.h"

#include <array>
#include <cassert>
#include <memory>
#include <string>
#include <vector>

class RenderContext
{
public:

    RenderContext(
        ResourceLoader & resourceLoader,
        vec3f const & ropeColour,
        ProgressCallback const & progressCallback);
    
    ~RenderContext();

public:

    float GetZoom() const
    {
        return mZoom;
    }

    void SetZoom(float zoom)
    {
        mZoom = zoom;

        UpdateVisibleWorldCoordinates();
        UpdateOrthoMatrix();        
    }

    void AdjustZoom(float amount)
    {
        mZoom *= amount;

        UpdateVisibleWorldCoordinates();
        UpdateOrthoMatrix();
    }

    vec2f GetCameraWorldPosition() const
    {
        return vec2f(mCamX, mCamY);
    }

    void SetCameraWorldPosition(vec2f const & pos)
    {
        mCamX = pos.x;
        mCamY = pos.y;

        UpdateVisibleWorldCoordinates();
        UpdateOrthoMatrix();
    }

    void AdjustCameraWorldPosition(vec2f const & worldOffset)
    {
        mCamX += worldOffset.x;
        mCamY += worldOffset.y;

        UpdateVisibleWorldCoordinates();
        UpdateOrthoMatrix();
    }

    int GetCanvasSizeWidth() const
    {
        return mCanvasWidth;
    }

    int GetCanvasSizeHeight() const
    {
        return mCanvasHeight;
    }

    void SetCanvasSize(int width, int height)
    {
        mCanvasWidth = width;
        mCanvasHeight = height;

        glViewport(0, 0, mCanvasWidth, mCanvasHeight);

        UpdateVisibleWorldCoordinates();
        UpdateOrthoMatrix();
    }

    float GetVisibleWorldWidth() const
    {
        return mVisibleWorldWidth;
    }

    float GetVisibleWorldHeight() const
    {
        return mVisibleWorldHeight;
    }

    float GetAmbientLightIntensity() const
    {
        return mAmbientLightIntensity;
    }

    void SetAmbientLightIntensity(float intensity)
    {
        mAmbientLightIntensity = intensity;

        UpdateAmbientLightIntensity();
    }

    float GetWaterTransparency() const
    {
        return mWaterTransparency;
    }

    void SetWaterTransparency(float transparency)
    {
        mWaterTransparency = transparency;
    }

    bool GetShowShipThroughWater() const
    {
        return mShowShipThroughWater;
    }

    void SetShowShipThroughWater(bool showShipThroughWater)
    {
        mShowShipThroughWater = showShipThroughWater;
    }


    //
    // Ship rendering
    //

    ShipRenderMode GetShipRenderMode() const
    {
        return mShipRenderMode;
    }

    void SetShipRenderMode(ShipRenderMode shipRenderMode) 
    {
        mShipRenderMode = shipRenderMode;
    }

    bool GetShowStressedSprings() const
    {
        return mShowStressedSprings;
    }

    void SetShowStressedSprings(bool showStressedSprings)
    {
        mShowStressedSprings = showStressedSprings;
    }


    //
    // Screen <-> World transformations
    //

    inline vec2 ScreenToWorld(vec2 const & screenCoordinates)
    {
        return vec2(
            (screenCoordinates.x / static_cast<float>(mCanvasWidth) - 0.5f) * mVisibleWorldWidth + mCamX,
            (screenCoordinates.y / static_cast<float>(mCanvasHeight) - 0.5f) * -mVisibleWorldHeight + mCamY);
    }

    inline vec2 ScreenOffsetToWorldOffset(vec2 const & screenOffset)
    {
        return vec2(
            screenOffset.x / static_cast<float>(mCanvasWidth) * mVisibleWorldWidth,
            screenOffset.y / static_cast<float>(mCanvasHeight) * -mVisibleWorldHeight);
    }

public:

    void Reset();

    void AddShip(
        int shipId,
        std::optional<ImageData> texture);

public:

    void RenderStart();

    //
    // Clouds
    //

    void RenderCloudsStart(size_t clouds);

    inline void RenderCloud(
        float virtualX,
        float virtualY,
        float scale)
    {
        //
        // We use Normalized Device Coordinates here
        //

        //
        // Roll coordinates into a 3.0 X 2.0 view,
        // then take the central slice and map it into NDC ((-1,-1) X (1,1))
        //

        float rolledX = fmodf(virtualX, 3.0f);
        if (rolledX < 0.0f)
            rolledX += 3.0f;
        float mappedX = -1.0f + 2.0f * (rolledX - 1.0f);

        float rolledY = fmodf(virtualY, 2.0f);
        if (rolledY < 0.0f)
            rolledY += 2.0f;
        float mappedY = -1.0f + 2.0f * (rolledY - 0.5f);

        assert(mCloudBufferSize + 1u <= mCloudBufferMaxSize);
        CloudElement * cloudElement = &(mCloudBuffer[mCloudBufferSize]);

        // Calculate texture dimensions in NDC, as proportion in 1280 X 1024
        // (totally arbitrary, a texture this size would fill the entire screen)
        float textureTileNdcW = static_cast<float>(mCloudTextureSizes[GetCloudTextureIndex(mCloudBufferSize)].Width) / 1280.0f;
        float textureTileNdcH = static_cast<float>(mCloudTextureSizes[GetCloudTextureIndex(mCloudBufferSize)].Height) / 1024.0f;

        float leftX = mappedX - textureTileNdcW * scale / 2.0f;
        float rightX = mappedX + textureTileNdcW * scale / 2.0f;
        float topY = mappedY - textureTileNdcH * scale / 2.0f;
        float bottomY = mappedY + textureTileNdcH * scale / 2.0f;
        
        cloudElement->ndcXTopLeft = leftX;
        cloudElement->ndcYTopLeft = topY;
        cloudElement->ndcTextureXTopLeft = 0.0f;
        cloudElement->ndcTextureYTopLeft = 0.0f;

        cloudElement->ndcXBottomLeft = leftX;
        cloudElement->ndcYBottomLeft = bottomY;
        cloudElement->ndcTextureXBottomLeft = 0.0f;
        cloudElement->ndcTextureYBottomLeft = 1.0f;

        cloudElement->ndcXTopRight = rightX;
        cloudElement->ndcYTopRight = topY;
        cloudElement->ndcTextureXTopRight = 1.0f;
        cloudElement->ndcTextureYTopRight = 0.0f;

        cloudElement->ndcXBottomRight = rightX;
        cloudElement->ndcYBottomRight = bottomY;
        cloudElement->ndcTextureXBottomRight = 1.0f;
        cloudElement->ndcTextureYBottomRight = 1.0f;

        ++mCloudBufferSize;
    }

    void RenderCloudsEnd();


    //
    // Land and Water
    //

    void UploadLandAndWaterStart(size_t slices);

    inline void UploadLandAndWater(
        float x,
        float yLand,
        float yWater,
        float restWaterHeight)
    {
        assert(mLandBufferMaxSize == mWaterBufferMaxSize);
        assert(mLandBufferMaxSize > 0);

        float const worldBottom = mCamY - (mVisibleWorldHeight / 2.0f);

        //
        // Store Land element
        //

        assert(mLandBufferSize + 1u <= mLandBufferMaxSize);
        LandElement * landElement = &(mLandBuffer[mLandBufferSize]);

        landElement->x1 = x;
        landElement->y1 = yLand;
        landElement->x2 = x;
        landElement->y2 = worldBottom;

        ++mLandBufferSize;


        //
        // Store water element
        //

        assert(mWaterBufferSize + 1u <= mWaterBufferMaxSize);
        WaterElement * waterElement = &(mWaterBuffer[mWaterBufferSize]);

        waterElement->x1 = x;
        waterElement->y1 = yWater > yLand ? yWater : yLand; // Make sure that islands are not covered in water!
        waterElement->textureY1 = restWaterHeight;

        waterElement->x2 = x;
        waterElement->y2 = yLand;
        waterElement->textureY2 = 0.0f;

        ++mWaterBufferSize;
    }

    void UploadLandAndWaterEnd();

    void RenderLand();

    void RenderWater();


    /////////////////////////////////////////////////////////////////////////
    // Ships
    /////////////////////////////////////////////////////////////////////////

    //
    // Ship Points
    //

    void UploadShipPointImmutableGraphicalAttributes(
        int shipId,
        size_t count,
        vec3f const * restrict color,
        vec2f const * restrict textureCoordinates)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadPointImmutableGraphicalAttributes(
            count,
            color,
            textureCoordinates);
    }

    void UploadShipPoints(
        int shipId,
        size_t count,
        vec2f const * restrict position,
        float const * restrict light,
        float const * restrict water)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadPoints(
            count,
            position,
            light,
            water);
    }

    //
    // Ship elements (points, springs, ropes, and triangles)
    //

    inline void UploadShipElementsStart(
        int shipId, 
        std::vector<std::size_t> const & connectedComponentsMaxSizes)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementsStart(connectedComponentsMaxSizes);
    }

    inline void UploadShipElementPoint(
        int shipId,
        int shipPointIndex,
        ConnectedComponentId connectedComponentId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementPoint(
            shipPointIndex,
            connectedComponentId);
    }

    inline void UploadShipElementSpring(
        int shipId,
        int shipPointIndex1,
        int shipPointIndex2,
        ConnectedComponentId connectedComponentId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementSpring(
            shipPointIndex1,
            shipPointIndex2,
            connectedComponentId);
    }

    inline void UploadShipElementRope(
        int shipId,
        int shipPointIndex1,
        int shipPointIndex2,
        ConnectedComponentId connectedComponentId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementRope(
            shipPointIndex1,
            shipPointIndex2,
            connectedComponentId);
    }

    inline void UploadShipElementTriangle(
        int shipId,
        int shipPointIndex1,
        int shipPointIndex2,
        int shipPointIndex3,
        ConnectedComponentId connectedComponentId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementTriangle(
            shipPointIndex1,
            shipPointIndex2,
            shipPointIndex3,
            connectedComponentId);
    }

    inline void UploadShipElementsEnd(int shipId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementsEnd();
    }

    inline void UploadShipElementStressedSpringsStart(int shipId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementStressedSpringsStart();
    }

    inline void UploadShipElementStressedSpring(
        int shipId,
        int shipPointIndex1,
        int shipPointIndex2,
        ConnectedComponentId connectedComponentId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementStressedSpring(
            shipPointIndex1,
            shipPointIndex2,
            connectedComponentId);
    }

    void UploadShipElementStressedSpringsEnd(int shipId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementStressedSpringsEnd();
    }

    inline void UploadShipElementPinnedPointsStart(
        int shipId,
        size_t count)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementPinnedPointsStart(count);
    }

    inline void UploadShipElementPinnedPoint(
        int shipId,
        float x,
        float y,
        ConnectedComponentId connectedComponentId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementPinnedPoint(
            x,
            y,
            connectedComponentId);
    }

    inline void UploadShipElementPinnedPointsEnd(int shipId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementPinnedPointsEnd();
    }

    inline void UploadShipElementBombsStart(
        int shipId,
        size_t count)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementBombsStart(count);
    }

    inline void UploadShipElementBomb(
        int shipId,
        BombType bombType,
        RotatedTextureRenderInfo const & renderInfo,
        std::optional<uint32_t> lightedFrameIndex,
        std::optional<uint32_t> unlightedFrameIndex,
        ConnectedComponentId connectedComponentId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementBomb(
            bombType,
            renderInfo,
            lightedFrameIndex,
            unlightedFrameIndex,
            connectedComponentId);
    }

    inline void UploadShipElementBombsEnd(int shipId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementBombsEnd();
    }

    //
    // Lamps
    //

    void UploadLampsStart(
        int shipId,
        size_t connectedComponents)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadLampsStart(connectedComponents);
    }

    void UploadLamp(
        int shipId,
        float x,
        float y,
        float lightIntensity,
        ConnectedComponentId connectedComponentId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadLamp(
            x,
            y,
            lightIntensity,
            connectedComponentId);
    }

    void UploadLampsEnd(int shipId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadLampsEnd();
    }

    void RenderShip(int shipId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->Render(
            mShipRenderMode,
            mShowStressedSprings);
    }


    //
    // Final
    //

    void RenderEnd();


private:
    
    void UpdateOrthoMatrix();

    void UpdateVisibleWorldCoordinates();

    void UpdateAmbientLightIntensity();

private:

    //
    // Clouds
    //

    GameOpenGLShaderProgram mCloudShaderProgram;
    GLint mCloudShaderAmbientLightIntensityParameter;

#pragma pack(push)
    struct CloudElement
    {
        float ndcXTopLeft;
        float ndcYTopLeft;
        float ndcTextureXTopLeft;
        float ndcTextureYTopLeft;

        float ndcXBottomLeft;
        float ndcYBottomLeft;
        float ndcTextureXBottomLeft;
        float ndcTextureYBottomLeft;

        float ndcXTopRight;
        float ndcYTopRight;
        float ndcTextureXTopRight;
        float ndcTextureYTopRight;

        float ndcXBottomRight;
        float ndcYBottomRight;
        float ndcTextureXBottomRight;
        float ndcTextureYBottomRight;
    };
#pragma pack(pop)

    std::unique_ptr<CloudElement[]> mCloudBuffer;
    size_t mCloudBufferSize;
    size_t mCloudBufferMaxSize;
    
    GameOpenGLVBO mCloudVBO;

    std::vector<ImageSize> mCloudTextureSizes;
    std::vector<GameOpenGLTexture> mCloudTextures;

    inline size_t GetCloudTextureIndex(size_t cloudIndex) const
    {
        return cloudIndex % mCloudTextures.size();
    }

    //
    // Land
    //

    GameOpenGLShaderProgram mLandShaderProgram;
    GLint mLandShaderAmbientLightIntensityParameter;
    GLint mLandShaderOrthoMatrixParameter;

#pragma pack(push)
    struct LandElement
    {
        float x1;
        float y1;
        float x2;
        float y2;
    };
#pragma pack(pop)

    std::unique_ptr<LandElement[]> mLandBuffer;
    size_t mLandBufferSize;
    size_t mLandBufferMaxSize;

    GameOpenGLVBO mLandVBO;
    
    GameOpenGLTexture mLandTexture;


    //
    // Water
    //

    GameOpenGLShaderProgram mWaterShaderProgram;
    GLint mWaterShaderAmbientLightIntensityParameter;
    GLint mWaterShaderWaterTransparencyParameter;
    GLint mWaterShaderOrthoMatrixParameter;

#pragma pack(push)
    struct WaterElement
    {
        float x1;
        float y1;
        float textureY1;
        
        float x2;
        float y2;
        float textureY2;
    };
#pragma pack(pop)

    std::unique_ptr<WaterElement[]> mWaterBuffer;
    size_t mWaterBufferSize;
    size_t mWaterBufferMaxSize;

    GameOpenGLVBO mWaterVBO;

    GameOpenGLTexture mWaterTexture;

    //
    // Ships
    //

    std::vector<std::unique_ptr<ShipRenderContext>> mShips;
    vec3f const mRopeColour;
    ImageSize mPinnedPointTextureSize;
    GameOpenGLTexture mPinnedPointTexture;
    std::vector<ImageSize> mRCBombTextureSizes;
    std::vector<GameOpenGLTexture> mRCBombTextures;
    std::vector<ImageSize> mTimerBombTextureSizes;
    std::vector<GameOpenGLTexture> mTimerBombTextures;


    //
    // Multi-purpose shaders
    //

    GameOpenGLShaderProgram mMatteNdcShaderProgram;
    GLint mMatteNdcShaderColorParameter;
    GameOpenGLVBO mMatteNdcVBO;

    GameOpenGLShaderProgram mMatteWorldShaderProgram;
    GLint mMatteWorldShaderColorParameter;
    GLint mMatteWorldShaderOrthoMatrixParameter;
    GameOpenGLVBO mMatteWorldVBO;

private:

    // The Ortho matrix
    float mOrthoMatrix[4][4];

    // The world coordinates of the visible portion
    float mVisibleWorldWidth;
    float mVisibleWorldHeight;
    float mCanvasToVisibleWorldHeightRatio;


    //
    // The current render parameters
    //

    float mZoom;
    float mCamX;
    float mCamY;
    int mCanvasWidth;
    int mCanvasHeight;

    float mAmbientLightIntensity;
    float mWaterTransparency;

    bool mShowShipThroughWater;
    ShipRenderMode mShipRenderMode;
    bool mShowStressedSprings;
};
