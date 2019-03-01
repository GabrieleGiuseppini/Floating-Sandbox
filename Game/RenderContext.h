/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "RenderCore.h"
#include "ResourceLoader.h"
#include "ShipDefinition.h"
#include "ShipRenderContext.h"
#include "TextRenderContext.h"
#include "TextureAtlas.h"
#include "TextureRenderManager.h"
#include "ViewModel.h"

#include <GameOpenGL/GameOpenGL.h>
#include <GameOpenGL/ShaderManager.h>

#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/ProgressCallback.h>

#include <GameCore/SysSpecifics.h>
#include <GameCore/Vectors.h>

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
        ProgressCallback const & progressCallback);

    ~RenderContext();

public:

    //
    // World and view properties
    //

    float GetZoom() const
    {
        return mViewModel.GetZoom();
    }

    void SetZoom(float zoom)
    {
        mViewModel.SetZoom(zoom);

        OnViewModelUpdated();
    }

    void AdjustZoom(float amount)
    {
        mViewModel.AdjustZoom(amount);

        OnViewModelUpdated();
    }

    vec2f GetCameraWorldPosition() const
    {
        return mViewModel.GetCameraWorldPosition();
    }

    void SetCameraWorldPosition(vec2f const & pos)
    {
        mViewModel.SetCameraWorldPosition(pos);

        OnViewModelUpdated();
    }

    void AdjustCameraWorldPosition(vec2f const & offset)
    {
        mViewModel.AdjustCameraWorldPosition(offset);

        OnViewModelUpdated();
    }

    int GetCanvasWidth() const
    {
        return mViewModel.GetCanvasWidth();
    }

    int GetCanvasHeight() const
    {
        return mViewModel.GetCanvasHeight();
    }

    void SetCanvasSize(int width, int height)
    {
        mViewModel.SetCanvasSize(width, height);

        glViewport(0, 0, mViewModel.GetCanvasWidth(), mViewModel.GetCanvasHeight());

        mTextRenderContext->UpdateCanvasSize(mViewModel.GetCanvasWidth(), mViewModel.GetCanvasHeight());

        OnViewModelUpdated();
    }

    float GetVisibleWorldWidth() const
    {
        return mViewModel.GetVisibleWorldWidth();
    }

    float GetVisibleWorldHeight() const
    {
        return mViewModel.GetVisibleWorldHeight();
    }

    //

    float GetAmbientLightIntensity() const
    {
        return mAmbientLightIntensity;
    }

    void SetAmbientLightIntensity(float intensity)
    {
        mAmbientLightIntensity = intensity;

        OnAmbientLightIntensityUpdated();
    }

    float GetSeaWaterTransparency() const
    {
        return mSeaWaterTransparency;
    }

    void SetSeaWaterTransparency(float transparency)
    {
        mSeaWaterTransparency = transparency;

        OnSeaWaterTransparencyUpdated();
    }

    bool GetShowShipThroughSeaWater() const
    {
        return mShowShipThroughSeaWater;
    }

    void SetShowShipThroughSeaWater(bool showShipThroughSeaWater)
    {
        mShowShipThroughSeaWater = showShipThroughSeaWater;
    }

    float GetWaterContrast() const
    {
        return mWaterContrast;
    }

    void SetWaterContrast(float contrast)
    {
        mWaterContrast = contrast;

        OnWaterContrastUpdated();
    }

    float GetWaterLevelOfDetail() const
    {
        return mWaterLevelOfDetail;
    }

    void SetWaterLevelOfDetail(float levelOfDetail)
    {
        mWaterLevelOfDetail = levelOfDetail;

        OnWaterLevelOfDetailUpdated();
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

        OnShipRenderModeUpdated();
    }

    DebugShipRenderMode GetDebugShipRenderMode() const
    {
        return mDebugShipRenderMode;
    }

    void SetDebugShipRenderMode(DebugShipRenderMode debugShipRenderMode)
    {
        mDebugShipRenderMode = debugShipRenderMode;

        OnDebugShipRenderModeUpdated();
    }

    VectorFieldRenderMode GetVectorFieldRenderMode() const
    {
        return mVectorFieldRenderMode;
    }

    void SetVectorFieldRenderMode(VectorFieldRenderMode vectorFieldRenderMode)
    {
        mVectorFieldRenderMode = vectorFieldRenderMode;

        OnVectorFieldRenderModeUpdated();
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

        OnShowStressedSpringsUpdated();
    }

    //
    // Screen <-> World transformations
    //

    inline vec2f ScreenToWorld(vec2f const & screenCoordinates)
    {
        return mViewModel.ScreenToWorld(screenCoordinates);
    }

    inline vec2f ScreenOffsetToWorldOffset(vec2f const & screenOffset)
    {
        return mViewModel.ScreenOffsetToWorldOffset(screenOffset);
    }

    //
    // Statistics
    //

    RenderStatistics const & GetStatistics() const
    {
        return mRenderStatistics;
    }

public:

    void Reset();

    void AddShip(
        ShipId shipId,
        size_t pointCount,
        RgbaImageData texture,
        ShipDefinition::TextureOriginType textureOrigin);

    RgbImageData TakeScreenshot();

public:

    void RenderStart();

    //
    // Sky
    //

    void RenderSkyStart();


    void UploadStarsStart(size_t starCount);

    inline void UploadStar(
        float ndcX,
        float ndcY,
        float brightness)
    {
        mStarElementBuffer.emplace_back(ndcX, ndcY, brightness);
    }

    void UploadStarsEnd();


    void UploadCloudsStart(size_t cloudCount);

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

        size_t cloudTextureIndex = mCurrentCloudElementCount % mCloudTextureAtlasMetadata->GetFrameMetadata().size();

        auto cloudAtlasFrameMetadata = mCloudTextureAtlasMetadata->GetFrameMetadata(
            TextureGroupType::Cloud,
            static_cast<TextureFrameIndex>(cloudTextureIndex));

        float const aspectRatio = static_cast<float>(mViewModel.GetCanvasWidth()) / static_cast<float>(mViewModel.GetCanvasHeight());

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

    void UploadCloudsEnd();


    void RenderSkyEnd();


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

        float const worldBottom = mViewModel.GetCameraWorldPosition().y - (mViewModel.GetVisibleWorldHeight() / 2.0f);

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
        waterElement->y1 = yWater;
        waterElement->textureY1 = restWaterHeight;

        waterElement->x2 = x;
        waterElement->y2 = yWater > yLand ? yLand : worldBottom;
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
        float const left = -mViewModel.GetVisibleWorldWidth() / 2.0f;
        float const right = mViewModel.GetVisibleWorldWidth() / 2.0f;
        float const top = mViewModel.GetVisibleWorldHeight() / 2.0f;
        float const bottom = -mViewModel.GetVisibleWorldHeight() / 2.0f;

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

    void RenderShipsStart(size_t shipCount);


    void RenderShipStart(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->RenderStart();
    }

    //
    // Ship Points
    //

    void UploadShipPointImmutableGraphicalAttributes(
        ShipId shipId,
        vec4f const * color,
        vec2f const * textureCoordinates)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointImmutableGraphicalAttributes(
            color,
            textureCoordinates);
    }

    void UploadShipPointColorRange(
        ShipId shipId,
        vec4f const * color,
        size_t startIndex,
        size_t count)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadShipPointColorRange(
            color,
            startIndex,
            count);
    }

    void UploadShipPoints(
        ShipId shipId,
        vec2f const * position,
        float const * light,
        float const * water)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPoints(
            position,
            light,
            water);
    }

    void UploadShipPointPlaneIds(
        ShipId shipId,
        PlaneId const * planeId,
        PlaneId maxMaxPlaneId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointPlaneIds(
            planeId,
            maxMaxPlaneId);
    }

    //
    // Ship triangle elements
    //

    inline void UploadShipElementTrianglesStart(
        ShipId shipId,
        size_t trianglesCount)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementTrianglesStart(trianglesCount);
    }

    inline void UploadShipElementTriangle(
        ShipId shipId,
        size_t triangleIndex,
        int shipPointIndex1,
        int shipPointIndex2,
        int shipPointIndex3)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementTriangle(
            triangleIndex,
            shipPointIndex1,
            shipPointIndex2,
            shipPointIndex3);
    }

    inline void UploadShipElementTrianglesEnd(
        ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementTrianglesEnd();
    }

    //
    // Other ship elements (points, springs, and ropes)
    //

    inline void UploadShipElementsStart(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementsStart();
    }

    inline void UploadShipElementPoint(
        ShipId shipId,
        int shipPointIndex)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementPoint(shipPointIndex);
    }

    inline void UploadShipElementSpring(
        ShipId shipId,
        int shipPointIndex1,
        int shipPointIndex2)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementSpring(
            shipPointIndex1,
            shipPointIndex2);
    }

    inline void UploadShipElementRope(
        ShipId shipId,
        int shipPointIndex1,
        int shipPointIndex2)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementRope(
            shipPointIndex1,
            shipPointIndex2);
    }

    inline void UploadShipElementsEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementsEnd();
    }

    //
    // Ship stressed springs
    //

    inline void UploadShipElementStressedSpringsStart(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementStressedSpringsStart();
    }

    inline void UploadShipElementStressedSpring(
        ShipId shipId,
        int shipPointIndex1,
        int shipPointIndex2)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementStressedSpring(
            shipPointIndex1,
            shipPointIndex2);
    }

    void UploadShipElementStressedSpringsEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementStressedSpringsEnd();
    }

    //
    // Generic textures
    //

    inline void UploadShipGenericTextureRenderSpecification(
        ShipId shipId,
        ConnectedComponentId connectedComponentId,
        TextureFrameId const & textureFrameId,
        vec2f const & position)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadGenericTextureRenderSpecification(
            connectedComponentId,
            textureFrameId,
            position);
    }

    inline void UploadShipGenericTextureRenderSpecification(
        ShipId shipId,
        ConnectedComponentId connectedComponentId,
        TextureFrameId const & textureFrameId,
        vec2f const & position,
        float scale,
        vec2f const & rotationBase,
        vec2f const & rotationOffset,
        float alpha)
    {
        assert(shipId >= 0 && shipId < mShips.size());

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
        ShipId shipId,
        ConnectedComponentId connectedComponentId,
        TextureFrameId const & textureFrameId,
        vec2f const & position,
        float scale,
        float angle,
        float alpha)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadGenericTextureRenderSpecification(
            connectedComponentId,
            textureFrameId,
            position,
            scale,
            angle,
            alpha);
    }

    //
    // Ephemeral points
    //

    inline void UploadShipEphemeralPointsStart(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadEphemeralPointsStart();
    }

    inline void UploadShipEphemeralPoint(
        ShipId shipId,
        int pointIndex)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadEphemeralPoint(
            pointIndex);
    }

    void UploadShipEphemeralPointsEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadEphemeralPointsEnd();
    }


    //
    // Vectors
    //

    void UploadShipVectors(
        ShipId shipId,
        size_t count,
        vec2f const * position,
        vec2f const * vector,
        float lengthAdjustment,
        vec4f const & color)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadVectors(
            count,
            position,
            vector,
            lengthAdjustment * mVectorFieldLengthMultiplier,
            color);
    }

    void RenderShipEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->RenderEnd();
    }


    void RenderShipsEnd();



    //
    // Text
    //

    RenderedTextHandle AddText(
        std::vector<std::string> const & textLines,
        TextPositionType position,
        float alpha,
        FontType font)
    {
        assert(!!mTextRenderContext);
        return mTextRenderContext->AddText(
            textLines,
            position,
            alpha,
            font);
    }

    void UpdateText(
        RenderedTextHandle textHandle,
        std::vector<std::string> const & textLines,
        float alpha)
    {
        assert(!!mTextRenderContext);
        mTextRenderContext->UpdateText(
            textHandle,
            textLines,
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

    void OnViewModelUpdated();
    void OnAmbientLightIntensityUpdated();
    void OnSeaWaterTransparencyUpdated();
    void OnWaterContrastUpdated();
    void OnWaterLevelOfDetailUpdated();
    void OnShipRenderModeUpdated();
    void OnDebugShipRenderModeUpdated();
    void OnVectorFieldRenderModeUpdated();
    void OnShowStressedSpringsUpdated();

private:

    std::unique_ptr<ShaderManager<ShaderManagerTraits>> mShaderManager;
    std::unique_ptr<TextureRenderManager> mTextureRenderManager;
    std::unique_ptr<TextRenderContext> mTextRenderContext;

    //
    // Stars
    //

#pragma pack(push)
    struct StarElement
    {
        float ndcX;
        float ndcY;
        float brightness;

        StarElement(
            float _ndcX,
            float _ndcY,
            float _brightness)
            : ndcX(_ndcX)
            , ndcY(_ndcY)
            , brightness(_brightness)
        {}
    };
#pragma pack(pop)

    std::vector<StarElement> mStarElementBuffer;

    GameOpenGLVBO mStarVBO;


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

    GameOpenGLTexture mCloudTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata> mCloudTextureAtlasMetadata;

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

    GameOpenGLTexture mGenericTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata> mGenericTextureAtlasMetadata;

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

    //
    // The current render parameters
    //

    ViewModel mViewModel;

    float mAmbientLightIntensity;
    float mSeaWaterTransparency;
    bool mShowShipThroughSeaWater;
    float mWaterContrast;
    float mWaterLevelOfDetail;
    ShipRenderMode mShipRenderMode;
    DebugShipRenderMode mDebugShipRenderMode;
    VectorFieldRenderMode mVectorFieldRenderMode;
    float mVectorFieldLengthMultiplier;
    bool mShowStressedSprings;

    //
    // Statistics
    //

    RenderStatistics mRenderStatistics;
};

}