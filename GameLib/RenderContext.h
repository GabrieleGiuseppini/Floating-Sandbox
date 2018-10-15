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
#include "RenderCore.h"
#include "ResourceLoader.h"
#include "ShaderManager.h"
#include "ShipRenderContext.h"
#include "SysSpecifics.h"
#include "TextRenderContext.h"
#include "TextureRenderManager.h"
#include "Vectors.h"

#include <array>
#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace Render {

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

        mTextRenderContext->UpdateCanvasSize(mCanvasWidth, mCanvasHeight);
        
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

    float GetSeaWaterTransparency() const
    {
        return mSeaWaterTransparency;
    }

    void SetSeaWaterTransparency(float transparency)
    {
        mSeaWaterTransparency = transparency;

        UpdateSeaWaterTransparency();
    }

    bool GetShowShipThroughSeaWater() const
    {
        return mShowShipThroughSeaWater;
    }

    void SetShowShipThroughSeaWater(bool showShipThroughSeaWater)
    {
        mShowShipThroughSeaWater = showShipThroughSeaWater;
    }

    float GetWaterLevelOfDetail() const
    {
        return mWaterLevelOfDetail;
    }

    void SetWaterLevelOfDetail(float levelOfDetail)
    {
        mWaterLevelOfDetail = levelOfDetail;

        UpdateWaterLevelOfDetail();
    }

    static constexpr float MinWaterLevelOfDetail = 0.0f;
    static constexpr float MaxWaterLevelOfDetail = 1.0f;

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

        UpdateShipRenderMode();
    }

    VectorFieldRenderMode GetVectorFieldRenderMode() const
    {
        return mVectorFieldRenderMode;
    }

    void SetVectorFieldRenderMode(VectorFieldRenderMode vectorFieldRenderMode)
    {
        mVectorFieldRenderMode = vectorFieldRenderMode;

        UpdateVectorFieldRenderMode();
    }

    float GetVectorFieldLengthMultiplier() const
    {
        return mVectorFieldLengthMultiplier;
    }

    void SetVectorFieldLengthMultiplier(float vectorFieldLengthMultiplier)
    {
        mVectorFieldLengthMultiplier = vectorFieldLengthMultiplier;
    }

    bool GetShowStressedSprings() const
    {
        return mShowStressedSprings;
    }

    void SetShowStressedSprings(bool showStressedSprings)
    {
        mShowStressedSprings = showStressedSprings;

        UpdateShowStressedSprings();
    }


    //
    // Screen <-> World transformations
    //

    inline vec2f ScreenToWorld(vec2f const & screenCoordinates)
    {
        return vec2f(
            (screenCoordinates.x / static_cast<float>(mCanvasWidth) - 0.5f) * mVisibleWorldWidth + mCamX,
            (screenCoordinates.y / static_cast<float>(mCanvasHeight) - 0.5f) * -mVisibleWorldHeight + mCamY);
    }

    inline vec2f ScreenOffsetToWorldOffset(vec2f const & screenOffset)
    {
        return vec2f(
            screenOffset.x / static_cast<float>(mCanvasWidth) * mVisibleWorldWidth,
            screenOffset.y / static_cast<float>(mCanvasHeight) * -mVisibleWorldHeight);
    }

public:

    void Reset();

    void AddShip(
        int shipId,
        size_t pointCount,
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

        TextureFrameMetadata const & textureMetadata = mTextureRenderManager->GetFrameMetadata(
            TextureGroupType::Cloud,
            static_cast<TextureFrameIndex>(mCloudBufferSize % mCloudTextureCount));

        float const aspectRatio = static_cast<float>(mCanvasWidth) / static_cast<float>(mCanvasHeight);

        float leftX = mappedX - scale * textureMetadata.AnchorWorldX;
        float rightX = mappedX + scale * (textureMetadata.WorldWidth - textureMetadata.AnchorWorldX);        
        float topY = mappedY + scale * (textureMetadata.WorldHeight - textureMetadata.AnchorWorldY) * aspectRatio;
        float bottomY = mappedY - scale * textureMetadata.AnchorWorldY * aspectRatio;
        
        cloudElement->ndcXTopLeft = leftX;
        cloudElement->ndcYTopLeft = topY;
        cloudElement->ndcTextureXTopLeft = 0.0f;
        cloudElement->ndcTextureYTopLeft = 1.0f;

        cloudElement->ndcXBottomLeft = leftX;
        cloudElement->ndcYBottomLeft = bottomY;
        cloudElement->ndcTextureXBottomLeft = 0.0f;
        cloudElement->ndcTextureYBottomLeft = 0.0f;

        cloudElement->ndcXTopRight = rightX;
        cloudElement->ndcYTopRight = topY;
        cloudElement->ndcTextureXTopRight = 1.0f;
        cloudElement->ndcTextureYTopRight = 1.0f;

        cloudElement->ndcXBottomRight = rightX;
        cloudElement->ndcYBottomRight = bottomY;
        cloudElement->ndcTextureXBottomRight = 1.0f;
        cloudElement->ndcTextureYBottomRight = 0.0f;

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

    void RenderShipStart(
        int shipId,
        std::vector<std::size_t> const & connectedComponentsMaxSizes)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->RenderStart(connectedComponentsMaxSizes);
    }

    //
    // Ship Points
    //

    void UploadShipPointImmutableGraphicalAttributes(
        int shipId,
        vec3f const * restrict color,
        vec2f const * restrict textureCoordinates)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadPointImmutableGraphicalAttributes(
            color,
            textureCoordinates);
    }

    void UploadShipPoints(
        int shipId,
        vec2f const * restrict position,
        float const * restrict light,
        float const * restrict water)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadPoints(
            position,
            light,
            water);
    }

    //
    // Ship elements (points, springs, ropes, and triangles)
    //

    inline void UploadShipElementsStart(int shipId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementsStart();
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

    //
    // Generic textures
    //

    inline void UploadShipGenericTextureRenderSpecification(
        int shipId,
        ConnectedComponentId connectedComponentId,
        TextureFrameId const & textureFrameId,
        vec2f const & position)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadGenericTextureRenderSpecification(
            connectedComponentId,
            textureFrameId,
            position);
    }

    inline void UploadShipGenericTextureRenderSpecification(
        int shipId,
        ConnectedComponentId connectedComponentId,
        TextureFrameId const & textureFrameId,
        vec2f const & position,
        float scale,
        vec2f const & rotationBase,
        vec2f const & rotationOffset)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadGenericTextureRenderSpecification(
            connectedComponentId,
            textureFrameId,
            position,
            scale,
            rotationBase,
            rotationOffset);
    }

    inline void UploadShipGenericTextureRenderSpecification(
        int shipId,
        ConnectedComponentId connectedComponentId,
        TextureFrameId const & textureFrameId,
        vec2f const & position,
        float scale,
        float angle)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadGenericTextureRenderSpecification(
            connectedComponentId,
            textureFrameId,
            position,
            scale,
            angle);
    }

    //
    // Vectors
    //

    void UploadShipVectors(
        int shipId,
        size_t count,
        vec2f const * restrict position,
        vec2f const * restrict vector,
        float lengthAdjustment,
        vec4f const & color)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadVectors(
            count,
            position,
            vector,
            lengthAdjustment * mVectorFieldLengthMultiplier,
            color);
    }

    void RenderShipEnd(int shipId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->RenderEnd();
    }

    //
    // Text
    //

    RenderedTextHandle AddText(
        std::string const & text,
        TextPositionType position,
        float alpha,
        FontType font)
    {
        assert(!!mTextRenderContext);
        return mTextRenderContext->AddText(
            text,
            position,
            alpha,
            font);
    }

    void UpdateText(
        RenderedTextHandle textHandle,
        std::string const & text,
        float alpha)
    {
        assert(!!mTextRenderContext);
        mTextRenderContext->UpdateText(
            textHandle,
            text,
            alpha);
    }

    void ClearText(RenderedTextHandle textHandle)
    {
        assert(!!mTextRenderContext);
        mTextRenderContext->ClearText(textHandle);
    }


    //
    // Final
    //

    void RenderEnd();


private:
    
    void UpdateOrthoMatrix();
    void UpdateVisibleWorldCoordinates();
    void UpdateAmbientLightIntensity();
    void UpdateSeaWaterTransparency();
    void UpdateWaterLevelOfDetail();
    void UpdateShipRenderMode();
    void UpdateVectorFieldRenderMode();
    void UpdateShowStressedSprings();

private:

    std::unique_ptr<ShaderManager<ShaderManagerTraits>> mShaderManager;
    std::unique_ptr<TextureRenderManager> mTextureRenderManager;
    std::unique_ptr<TextRenderContext> mTextRenderContext;

    //
    // Clouds
    //

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

    size_t mCloudTextureCount;

    //
    // Land
    //

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

    //
    // Sea water
    //

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

    //
    // Ships
    //

    std::vector<std::unique_ptr<ShipRenderContext>> mShips;
    vec3f const mRopeColour;

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
    float mSeaWaterTransparency;    
    bool mShowShipThroughSeaWater;
    float mWaterLevelOfDetail;
    ShipRenderMode mShipRenderMode;
    VectorFieldRenderMode mVectorFieldRenderMode;
    float mVectorFieldLengthMultiplier;
    bool mShowStressedSprings;
};

}