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

#include <Game/GameParameters.h>

#include <GameCore/BoundedVector.h>
#include <GameCore/Colors.h>
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

    float ClampZoom(float zoom) const
    {
        return mViewModel.ClampZoom(zoom);
    }

    float SetZoom(float zoom)
    {
        auto const newZoom = mViewModel.SetZoom(zoom);

        OnViewModelUpdated();

        return newZoom;
    }

    vec2f GetCameraWorldPosition() const
    {
        return mViewModel.GetCameraWorldPosition();
    }

    vec2f ClampCameraWorldPosition(vec2f const & pos) const
    {
        return mViewModel.ClampCameraWorldPosition(pos);
    }

    vec2f SetCameraWorldPosition(vec2f const & pos)
    {
        auto const newCameraWorldPosition = mViewModel.SetCameraWorldPosition(pos);

        OnViewModelUpdated();

        return newCameraWorldPosition;
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

    rgbColor const & GetFlatSkyColor() const
    {
        return mFlatSkyColor;
    }

    void SetFlatSkyColor(rgbColor const & color)
    {
        mFlatSkyColor = color;

        // No need to notify anyone
    }

    float GetAmbientLightIntensity() const
    {
        return mAmbientLightIntensity;
    }

    void SetAmbientLightIntensity(float intensity)
    {
        mAmbientLightIntensity = intensity;

        OnAmbientLightIntensityUpdated();
    }

    float GetOceanTransparency() const
    {
        return mOceanTransparency;
    }

    void SetOceanTransparency(float transparency)
    {
        mOceanTransparency = transparency;

        OnOceanTransparencyUpdated();
    }

    bool GetShowShipThroughOcean() const
    {
        return mShowShipThroughOcean;
    }

    void SetShowShipThroughOcean(bool showShipThroughOcean)
    {
        mShowShipThroughOcean = showShipThroughOcean;
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

    OceanRenderMode GetOceanRenderMode() const
    {
        return mOceanRenderMode;
    }

    void SetOceanRenderMode(OceanRenderMode oceanRenderMode)
    {
        mOceanRenderMode = oceanRenderMode;

        OnOceanRenderParametersUpdated();
    }

    rgbColor const & GetDepthOceanColorStart() const
    {
        return mDepthOceanColorStart;
    }

    void SetDepthOceanColorStart(rgbColor const & color)
    {
        mDepthOceanColorStart = color;

        OnOceanRenderParametersUpdated();
    }

    rgbColor const & GetDepthOceanColorEnd() const
    {
        return mDepthOceanColorEnd;
    }

    void SetDepthOceanColorEnd(rgbColor const & color)
    {
        mDepthOceanColorEnd = color;

        OnOceanRenderParametersUpdated();
    }

    rgbColor const & GetFlatOceanColor() const
    {
        return mFlatOceanColor;
    }

    void SetFlatOceanColor(rgbColor const & color)
    {
        mFlatOceanColor = color;

        OnOceanRenderParametersUpdated();
    }

    LandRenderMode GetLandRenderMode() const
    {
        return mLandRenderMode;
    }

    void SetLandRenderMode(LandRenderMode landRenderMode)
    {
        mLandRenderMode = landRenderMode;

        OnLandRenderParametersUpdated();
    }

    rgbColor const & GetFlatLandColor() const
    {
        return mFlatLandColor;
    }

    void SetFlatLandColor(rgbColor const & color)
    {
        mFlatLandColor = color;

        OnLandRenderParametersUpdated();
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
        mStarVertexBuffer.emplace_back(ndcX, ndcY, brightness);
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
    // Land and Ocean
    //

    void UploadLandAndOceanStart(size_t slices);

    inline void UploadLandAndOcean(
        float x,
        float yLand,
        float yOcean,
        float oceanDepth)
    {
        assert(mLandElementCount == mOceanElementCount);
        assert(mLandElementCount > 0);

        float const yVisibleWorldBottom = mViewModel.GetVisibleWorldBottomRight().y;

        //
        // Store Land element
        //

        assert(!!mLandElementBuffer);
        assert(mCurrentLandElementCount + 1u <= mLandElementCount);
        LandElement * landElement = &(mLandElementBuffer[mCurrentLandElementCount]);

        landElement->x1 = x;
        landElement->y1 = yLand > yVisibleWorldBottom ? yLand : yVisibleWorldBottom; // Clamp top up to visible bottom
        landElement->x2 = x;
        landElement->y2 = yVisibleWorldBottom;

        ++mCurrentLandElementCount;


        //
        // Store ocean element
        //

        assert(!!mOceanElementBuffer);
        assert(mCurrentOceanElementCount + 1u <= mOceanElementCount);
        OceanElement * oceanElement = &(mOceanElementBuffer[mCurrentOceanElementCount]);

        oceanElement->x1 = x;
        oceanElement->y1 = yOcean;

        oceanElement->x2 = x;
        oceanElement->y2 = yOcean > yLand ? yLand : yVisibleWorldBottom; // If land sticks out, go down to visible bottom (land is drawn last)

        switch (mOceanRenderMode)
        {
            case OceanRenderMode::Texture:
            {
                // Texture sample Y: top=oceanDepth (we use repeat mode), bottom=0.0
                oceanElement->value1 = oceanDepth;
                oceanElement->value2 = 0.0f;

                break;
            }

            case OceanRenderMode::Depth:
            {
                // Depth: top=0.0, bottom=height as fraction of maximum depth
                oceanElement->value1 = 0.0f;
                oceanElement->value2 = oceanDepth != 0.0f
                    ? abs(oceanElement->y2 - oceanElement->y1) / oceanDepth
                    : 0.0f;

                break;
            }

            case OceanRenderMode::Flat:
            {
                // Nop
                oceanElement->value1 = 0.0f;
                oceanElement->value2 = 0.0f;

                break;
            }
        }

        ++mCurrentOceanElementCount;
    }

    void UploadLandAndOceanEnd();

    void RenderLand();

    void RenderOcean();


    //
    // Crosses of light
    //

    void UploadCrossOfLight(
        vec2f const & centerPosition,
        float progress)
    {
        // Triangle 1

        mCrossOfLightBuffer.emplace_back(
            vec2f(mViewModel.GetVisibleWorldTopLeft().x, mViewModel.GetVisibleWorldBottomRight().y), // left, bottom
            centerPosition,
            progress);

        mCrossOfLightBuffer.emplace_back(
            mViewModel.GetVisibleWorldTopLeft(), // left, top
            centerPosition,
            progress);

        mCrossOfLightBuffer.emplace_back(
            mViewModel.GetVisibleWorldBottomRight(), // right, bottom
            centerPosition,
            progress);

        // Triangle 2

        mCrossOfLightBuffer.emplace_back(
            mViewModel.GetVisibleWorldTopLeft(), // left, top
            centerPosition,
            progress);

        mCrossOfLightBuffer.emplace_back(
            mViewModel.GetVisibleWorldBottomRight(), // right, bottom
            centerPosition,
            progress);

        mCrossOfLightBuffer.emplace_back(
            vec2f(mViewModel.GetVisibleWorldBottomRight().x, mViewModel.GetVisibleWorldTopLeft().y),  // right, top
            centerPosition,
            progress);
    }

    /////////////////////////////////////////////////////////////////////////
    // Ships
    /////////////////////////////////////////////////////////////////////////

    void RenderShipsStart();


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
        vec2f const * textureCoordinates)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointImmutableGraphicalAttributes(textureCoordinates);
    }

    void UploadShipPointColors(
        ShipId shipId,
        vec4f const * color,
        size_t startDst,
        size_t count)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadShipPointColors(
            color,
            startDst,
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
        size_t startDst,
        size_t count,
        PlaneId maxMaxPlaneId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointPlaneIds(
            planeId,
            startDst,
            count,
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
        PlaneId planeId,
        TextureFrameId const & textureFrameId,
        vec2f const & position)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadGenericTextureRenderSpecification(
            planeId,
            textureFrameId,
            position);
    }

    inline void UploadShipGenericTextureRenderSpecification(
        ShipId shipId,
        PlaneId planeId,
        TextureFrameId const & textureFrameId,
        vec2f const & position,
        float scale,
        vec2f const & rotationBase,
        vec2f const & rotationOffset,
        float alpha)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadGenericTextureRenderSpecification(
            planeId,
            textureFrameId,
            position,
            scale,
            rotationBase,
            rotationOffset,
            alpha);
    }

    inline void UploadShipGenericTextureRenderSpecification(
        ShipId shipId,
        PlaneId planeId,
        TextureFrameId const & textureFrameId,
        vec2f const & position,
        float scale,
        float angle,
        float alpha)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadGenericTextureRenderSpecification(
            planeId,
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
        PlaneId const * planeId,
        vec2f const * vector,
        float lengthAdjustment,
        vec4f const & color)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadVectors(
            count,
            position,
            planeId,
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
    void OnOceanTransparencyUpdated();
    void OnWaterContrastUpdated();
    void OnWaterLevelOfDetailUpdated();
    void OnShipRenderModeUpdated();
    void OnDebugShipRenderModeUpdated();
    void OnOceanRenderParametersUpdated();
    void OnLandRenderParametersUpdated();
    void OnVectorFieldRenderModeUpdated();
    void OnShowStressedSpringsUpdated();

private:

    std::unique_ptr<ShaderManager<ShaderManagerTraits>> mShaderManager;
    std::unique_ptr<TextureRenderManager> mTextureRenderManager;
    std::unique_ptr<TextRenderContext> mTextRenderContext;

    // TODOTEST: VAO TEST START

    //
    // Types
    //

#pragma pack(push)
    struct StarVertex
    {
        float ndcX;
        float ndcY;
        float brightness;

        StarVertex(
            float _ndcX,
            float _ndcY,
            float _brightness)
            : ndcX(_ndcX)
            , ndcY(_ndcY)
            , brightness(_brightness)
        {}
    };
#pragma pack(pop)

    //
    // Buffers
    //

    BoundedVector<StarVertex> mStarVertexBuffer;
    GameOpenGLVBO mStarVBO;


    //
    // VAOs
    //

    GameOpenGLVAO mStarVAO;


    // TODOTEST: OLD


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
    // Ocean
    //

#pragma pack(push)
    struct OceanElement
    {
        float x1;
        float y1;
        float value1;

        float x2;
        float y2;
        float value2;
    };
#pragma pack(pop)

    std::unique_ptr<OceanElement[]> mOceanElementBuffer;
    size_t mCurrentOceanElementCount;
    size_t mOceanElementCount;

    GameOpenGLVBO mOceanVBO;

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

    rgbColor mFlatSkyColor;
    float mAmbientLightIntensity;
    float mOceanTransparency;
    bool mShowShipThroughOcean;
    float mWaterContrast;
    float mWaterLevelOfDetail;
    ShipRenderMode mShipRenderMode;
    DebugShipRenderMode mDebugShipRenderMode;
    OceanRenderMode mOceanRenderMode;
    rgbColor mDepthOceanColorStart;
    rgbColor mDepthOceanColorEnd;
    rgbColor mFlatOceanColor;
    LandRenderMode mLandRenderMode;
    rgbColor mFlatLandColor;
    VectorFieldRenderMode mVectorFieldRenderMode;
    float mVectorFieldLengthMultiplier;
    bool mShowStressedSprings;

    //
    // Statistics
    //

    RenderStatistics mRenderStatistics;
};

}