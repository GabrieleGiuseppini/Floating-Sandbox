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
#include "TextureAtlas.h"
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
        vec4f const & ropeColour,
        ProgressCallback const & progressCallback);
    
    ~RenderContext();

public:

    //
    // World and view properties
    //

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

        UpdateCanvasSize();
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
    // Ship rendering properties
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

    bool GetWireframeMode() const
    {
        return mWireframeMode;
    }

    void SetWireframeMode(bool wireframeMode)
    {
        mWireframeMode = wireframeMode;

        UpdateWireframeMode();
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

    void RenderCloudsStart(size_t cloudCount);

    inline void UploadCloud(
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

        float rolledX = std::fmod(virtualX, 3.0f);
        if (rolledX < 0.0f)
            rolledX += 3.0f;
        float mappedX = -1.0f + 2.0f * (rolledX - 1.0f);

        float rolledY = std::fmod(virtualY, 2.0f);
        if (rolledY < 0.0f)
            rolledY += 2.0f;
        float mappedY = -1.0f + 2.0f * (rolledY - 0.5f);


        //
        // Populate entry in buffer
        //

        assert(!!mCloudElementBuffer);
        assert(mCurrentCloudElementCount + 1u <= mCloudElementCount);
        CloudElement * cloudElement = &(mCloudElementBuffer[mCurrentCloudElementCount]);

        size_t cloudTextureIndex = mCurrentCloudElementCount % mCloudTextureCount;

        auto cloudAtlasFrameMetadata = mCloudTextureAtlasMetadata->GetFrameMetadata(
            TextureGroupType::Cloud,
            static_cast<TextureFrameIndex>(cloudTextureIndex));

        float const aspectRatio = static_cast<float>(mCanvasWidth) / static_cast<float>(mCanvasHeight);

        float leftX = mappedX - scale * cloudAtlasFrameMetadata.FrameMetadata.AnchorWorldX;
        float rightX = mappedX + scale * (cloudAtlasFrameMetadata.FrameMetadata.WorldWidth - cloudAtlasFrameMetadata.FrameMetadata.AnchorWorldX);
        float topY = mappedY + scale * (cloudAtlasFrameMetadata.FrameMetadata.WorldHeight - cloudAtlasFrameMetadata.FrameMetadata.AnchorWorldY) * aspectRatio;
        float bottomY = mappedY - scale * cloudAtlasFrameMetadata.FrameMetadata.AnchorWorldY * aspectRatio;
        
        cloudElement->ndcXTopLeft1 = leftX;
        cloudElement->ndcYTopLeft1 = topY;
        cloudElement->ndcTextureXTopLeft1 = cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.x;
        cloudElement->ndcTextureYTopLeft1 = cloudAtlasFrameMetadata.TextureCoordinatesTopRight.y;

        cloudElement->ndcXBottomLeft1 = leftX;
        cloudElement->ndcYBottomLeft1 = bottomY;
        cloudElement->ndcTextureXBottomLeft1 = cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.x;
        cloudElement->ndcTextureYBottomLeft1 = cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.y;

        cloudElement->ndcXTopRight1 = rightX;
        cloudElement->ndcYTopRight1 = topY;
        cloudElement->ndcTextureXTopRight1 = cloudAtlasFrameMetadata.TextureCoordinatesTopRight.x;
        cloudElement->ndcTextureYTopRight1 = cloudAtlasFrameMetadata.TextureCoordinatesTopRight.y;

        cloudElement->ndcXBottomLeft2 = leftX;
        cloudElement->ndcYBottomLeft2 = bottomY;
        cloudElement->ndcTextureXBottomLeft2 = cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.x;
        cloudElement->ndcTextureYBottomLeft2 = cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.y;

        cloudElement->ndcXTopRight2 = rightX;
        cloudElement->ndcYTopRight2 = topY;
        cloudElement->ndcTextureXTopRight2 = cloudAtlasFrameMetadata.TextureCoordinatesTopRight.x;
        cloudElement->ndcTextureYTopRight2 = cloudAtlasFrameMetadata.TextureCoordinatesTopRight.y;

        cloudElement->ndcXBottomRight2 = rightX;
        cloudElement->ndcYBottomRight2 = bottomY;
        cloudElement->ndcTextureXBottomRight2 = cloudAtlasFrameMetadata.TextureCoordinatesTopRight.x;
        cloudElement->ndcTextureYBottomRight2 = cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.y;

        ++mCurrentCloudElementCount;
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
        assert(mLandElementCount == mWaterElementCount);
        assert(mLandElementCount > 0);

        float const worldBottom = mCamY - (mVisibleWorldHeight / 2.0f);

        //
        // Store Land element
        //

        assert(!!mLandElementBuffer);
        assert(mCurrentLandElementCount + 1u <= mLandElementCount);
        LandElement * landElement = &(mLandElementBuffer[mCurrentLandElementCount]);

        landElement->x1 = x;
        landElement->y1 = yLand;
        landElement->x2 = x;
        landElement->y2 = worldBottom;

        ++mCurrentLandElementCount;


        //
        // Store water element
        //

        assert(!!mWaterElementBuffer);
        assert(mCurrentWaterElementCount + 1u <= mWaterElementCount);
        WaterElement * waterElement = &(mWaterElementBuffer[mCurrentWaterElementCount]);

        waterElement->x1 = x;
        waterElement->y1 = yWater > yLand ? yWater : yLand; // Make sure islands are not covered in water!
        waterElement->textureY1 = restWaterHeight;

        waterElement->x2 = x;
        waterElement->y2 = yLand;
        waterElement->textureY2 = 0.0f;

        ++mCurrentWaterElementCount;
    }

    void UploadLandAndWaterEnd();

    void RenderLand();

    void RenderWater();


    //
    // Crosses of light
    //

    void UploadCrossOfLight(
        vec2f const & centerPosition,
        float progress)
    {
        float const left = -mVisibleWorldWidth / 2.0f;
        float const right = mVisibleWorldWidth / 2.0f;
        float const top = mVisibleWorldHeight / 2.0f;
        float const bottom = -mVisibleWorldHeight / 2.0f;

        // Triangle 1

        mCrossOfLightBuffer.emplace_back(
            vec2f(left, bottom),
            centerPosition,
            progress);

        mCrossOfLightBuffer.emplace_back(
            vec2f(left, top),
            centerPosition,
            progress);

        mCrossOfLightBuffer.emplace_back(
            vec2f(right, bottom),
            centerPosition,
            progress);

        // Triangle 2

        mCrossOfLightBuffer.emplace_back(
            vec2f(left, top),
            centerPosition,
            progress);

        mCrossOfLightBuffer.emplace_back(
            vec2f(right, bottom),
            centerPosition,
            progress);

        mCrossOfLightBuffer.emplace_back(
            vec2f(right, top),
            centerPosition,
            progress);
    }

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
        vec4f const * restrict color,
        vec2f const * restrict textureCoordinates)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadPointImmutableGraphicalAttributes(
            color,
            textureCoordinates);
    }

    void UploadShipPointColorRange(
        int shipId,
        vec4f const * restrict color,
        size_t startIndex,
        size_t count)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadShipPointColorRange(
            color,
            startIndex,
            count);
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

    inline void UploadShipElementEphemeralPointsStart(int shipId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementEphemeralPointsStart();
    }

    inline void UploadShipElementEphemeralPoint(
        int shipId,
        int pointIndex,
        ConnectedComponentId connectedComponentId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementEphemeralPoint(
            pointIndex,
            connectedComponentId);
    }

    void UploadShipElementEphemeralPointsEnd(int shipId)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadElementEphemeralPointsEnd();
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
        vec2f const & rotationOffset,
        float alpha)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadGenericTextureRenderSpecification(
            connectedComponentId,
            textureFrameId,
            position,
            scale,
            rotationBase,
            rotationOffset,
            alpha);
    }

    inline void UploadShipGenericTextureRenderSpecification(
        int shipId,
        ConnectedComponentId connectedComponentId,
        TextureFrameId const & textureFrameId,
        vec2f const & position,
        float scale,
        float angle,
        float alpha)
    {
        assert(shipId < mShips.size());

        mShips[shipId]->UploadGenericTextureRenderSpecification(
            connectedComponentId,
            textureFrameId,
            position,
            scale,
            angle,
            alpha);
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

    void RenderCrossesOfLight();
    
    void UpdateOrthoMatrix();
    void UpdateCanvasSize();
    void UpdateVisibleWorldCoordinates();
    void UpdateAmbientLightIntensity();
    void UpdateSeaWaterTransparency();
    void UpdateWaterLevelOfDetail();
    void UpdateShipRenderMode();
    void UpdateVectorFieldRenderMode();
    void UpdateShowStressedSprings();
    void UpdateWireframeMode();

private:

    std::unique_ptr<ShaderManager<ShaderManagerTraits>> mShaderManager;
    std::unique_ptr<TextureRenderManager> mTextureRenderManager;
    std::unique_ptr<TextRenderContext> mTextRenderContext;

    GameOpenGLTexture mCloudTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata> mCloudTextureAtlasMetadata;

    GameOpenGLTexture mGenericTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata> mGenericTextureAtlasMetadata;

    //
    // Clouds
    //

#pragma pack(push)
    // Two triangles
    struct CloudElement
    {
        float ndcXTopLeft1;
        float ndcYTopLeft1;
        float ndcTextureXTopLeft1;
        float ndcTextureYTopLeft1;

        float ndcXBottomLeft1;
        float ndcYBottomLeft1;
        float ndcTextureXBottomLeft1;
        float ndcTextureYBottomLeft1;

        float ndcXTopRight1;
        float ndcYTopRight1;
        float ndcTextureXTopRight1;
        float ndcTextureYTopRight1;

        float ndcXBottomLeft2;
        float ndcYBottomLeft2;
        float ndcTextureXBottomLeft2;
        float ndcTextureYBottomLeft2;

        float ndcXTopRight2;
        float ndcYTopRight2;
        float ndcTextureXTopRight2;
        float ndcTextureYTopRight2;

        float ndcXBottomRight2;
        float ndcYBottomRight2;
        float ndcTextureXBottomRight2;
        float ndcTextureYBottomRight2;
    };
#pragma pack(pop)

    std::unique_ptr<CloudElement[]> mCloudElementBuffer;
    size_t mCurrentCloudElementCount;
    size_t mCloudElementCount;
    
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

    std::unique_ptr<LandElement[]> mLandElementBuffer;
    size_t mCurrentLandElementCount;
    size_t mLandElementCount;

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

    std::unique_ptr<WaterElement[]> mWaterElementBuffer;
    size_t mCurrentWaterElementCount;
    size_t mWaterElementCount;

    GameOpenGLVBO mWaterVBO;

    //
    // Ships
    //

    std::vector<std::unique_ptr<ShipRenderContext>> mShips;
    vec4f const mRopeColour;

    //
    // Cross of light
    //

#pragma pack(push)
    struct CrossOfLightElement
    {
        vec2f vertex;
        vec2f centerPosition;
        float progress;

        CrossOfLightElement(
            vec2f _vertex,
            vec2f _centerPosition,
            float _progress)
            : vertex(_vertex)
            , centerPosition(_centerPosition)
            , progress(_progress)
        {}
    };
#pragma pack(pop)

    std::vector<CrossOfLightElement> mCrossOfLightBuffer;

    GameOpenGLVBO mCrossOfLightVBO;

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
    bool mWireframeMode;
};

}