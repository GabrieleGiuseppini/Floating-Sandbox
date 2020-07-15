/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "NotificationRenderContext.h"
#include "PerfStats.h"
#include "RenderParameters.h"
#include "RenderTypes.h"
#include "ResourceLocator.h"
#include "ShaderTypes.h"
#include "ShipRenderContext.h"
#include "TextureAtlas.h"
#include "TextureTypes.h"
#include "UploadedTextureManager.h"
#include "ViewModel.h"

#include <GameOpenGL/GameOpenGL.h>
#include <GameOpenGL/GameOpenGLMappedBuffer.h>
#include <GameOpenGL/ShaderManager.h>

#include <Game/GameParameters.h>

#include <GameCore/BoundedVector.h>
#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/ImageSize.h>
#include <GameCore/ProgressCallback.h>
#include <GameCore/SysSpecifics.h>
#include <GameCore/TaskThread.h>
#include <GameCore/Vectors.h>

#include <array>
#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Render {

class RenderContext
{
public:

    RenderContext(
        ImageSize const & initialCanvasSize,
        std::function<void()> makeRenderContextCurrentFunction,
        std::function<void()> swapRenderBuffersFunction,
        PerfStats & perfStats,
        ResourceLocator const & resourceLocator,
        ProgressCallback const & progressCallback);

    ~RenderContext();

public:

    //
    // World and view properties
    //

    float const & GetZoom() const
    {
        return mRenderParameters.View.GetZoom();
    }

    float ClampZoom(float zoom) const
    {
        return mRenderParameters.View.ClampZoom(zoom);
    }

    float const & SetZoom(float zoom)
    {
        float const & newZoom = mRenderParameters.View.SetZoom(zoom);
        mRenderParameters.IsViewDirty = true;

        return newZoom;
    }

    vec2f const & GetCameraWorldPosition() const
    {
        return mRenderParameters.View.GetCameraWorldPosition();
    }

    vec2f ClampCameraWorldPosition(vec2f const & pos) const
    {
        return mRenderParameters.View.ClampCameraWorldPosition(pos);
    }

    vec2f const & SetCameraWorldPosition(vec2f const & pos)
    {
        vec2f const & newCameraWorldPosition = mRenderParameters.View.SetCameraWorldPosition(pos);
        mRenderParameters.IsViewDirty = true;

        return newCameraWorldPosition;
    }

    int GetCanvasWidth() const
    {
        return mRenderParameters.View.GetCanvasWidth();
    }

    int GetCanvasHeight() const
    {
        return mRenderParameters.View.GetCanvasHeight();
    }

    void SetCanvasSize(int width, int height)
    {
        mRenderParameters.View.SetCanvasSize(width, height);
        mRenderParameters.IsViewDirty = true;
        mRenderParameters.IsCanvasSizeDirty = true;
    }

    void SetPixelOffset(float x, float y)
    {
        mRenderParameters.View.SetPixelOffset(x, y);
        mRenderParameters.IsViewDirty = true;
    }

    void ResetPixelOffset()
    {
        mRenderParameters.View.ResetPixelOffset();
        mRenderParameters.IsViewDirty = true;
    }

    float GetVisibleWorldWidth() const
    {
        return mRenderParameters.View.GetVisibleWorldWidth();
    }

    float GetVisibleWorldHeight() const
    {
        return mRenderParameters.View.GetVisibleWorldHeight();
    }

    float GetVisibleWorldLeft() const
    {
        return mRenderParameters.View.GetVisibleWorldTopLeft().x;
    }

    float GetVisibleWorldRight() const
    {
        return mRenderParameters.View.GetVisibleWorldBottomRight().x;
    }

    float GetVisibleWorldTop() const
    {
        return mRenderParameters.View.GetVisibleWorldTopLeft().y;
    }

    float GetVisibleWorldBottom() const
    {
        return mRenderParameters.View.GetVisibleWorldBottomRight().y;
    }

    float CalculateZoomForWorldWidth(float worldWidth) const
    {
        return mRenderParameters.View.CalculateZoomForWorldWidth(worldWidth);
    }

    float CalculateZoomForWorldHeight(float worldHeight) const
    {
        return mRenderParameters.View.CalculateZoomForWorldHeight(worldHeight);
    }

    //

    float GetAmbientLightIntensity() const
    {
        return mAmbientLightIntensity;
    }

    void SetAmbientLightIntensity(float intensity)
    {
        mAmbientLightIntensity = intensity;
        mRenderParameters.EffectiveAmbientLightIntensity = CalculateEffectiveAmbientLightIntensity();
        mRenderParameters.IsEffectiveAmbientLightIntensityDirty = true;
    }

    float GetEffectiveAmbientLightIntensity() const
    {
        return mRenderParameters.EffectiveAmbientLightIntensity;
    }

    //

    rgbColor const & GetFlatSkyColor() const
    {
        return mRenderParameters.FlatSkyColor;
    }

    void SetFlatSkyColor(rgbColor const & color)
    {
        mRenderParameters.FlatSkyColor = color;
        // No need to set dirty, this is picked up at each cycle anyway
    }

    // TODOHERE

    float GetOceanTransparency() const
    {
        return mOceanTransparency;
    }

    void SetOceanTransparency(float transparency)
    {        
        mOceanTransparency = transparency;

        OnOceanTransparencyUpdated();
    }

    float GetOceanDarkeningRate() const
    {
        return mOceanDarkeningRate;
    }

    void SetOceanDarkeningRate(float darkeningRate)
    {
        mOceanDarkeningRate = darkeningRate;

        OnOceanDarkeningRateUpdated();
    }

    bool GetShowShipThroughOcean() const
    {
        return mShowShipThroughOcean;
    }

    void SetShowShipThroughOcean(bool showShipThroughOcean)
    {
        mShowShipThroughOcean = showShipThroughOcean;
    }

    OceanRenderModeType GetOceanRenderMode() const
    {
        return mOceanRenderMode;
    }

    void SetOceanRenderMode(OceanRenderModeType oceanRenderMode)
    {
        mOceanRenderMode = oceanRenderMode;

        OnOceanRenderParametersUpdated();
    }

    std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureOceanAvailableThumbnails() const
    {
        return mOceanAvailableThumbnails;
    }

    size_t GetTextureOceanTextureIndex() const
    {
        return mSelectedOceanTextureIndex;
    }

    void SetTextureOceanTextureIndex(size_t index)
    {
        mSelectedOceanTextureIndex = index;

        OnOceanTextureIndexUpdated();
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

    LandRenderModeType GetLandRenderMode() const
    {
        return mLandRenderMode;
    }

    void SetLandRenderMode(LandRenderModeType landRenderMode)
    {
        mLandRenderMode = landRenderMode;

        OnLandRenderParametersUpdated();
    }

    std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureLandAvailableThumbnails() const
    {
        return mLandAvailableThumbnails;
    }

    size_t GetTextureLandTextureIndex() const
    {
        return mSelectedLandTextureIndex;
    }

    void SetTextureLandTextureIndex(size_t index)
    {
        mSelectedLandTextureIndex = index;

        OnLandTextureIndexUpdated();
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

    //
    // Ship rendering properties
    //

    rgbColor const & GetFlatLampLightColor() const
    {
        return mFlatLampLightColor;
    }

    void SetFlatLampLightColor(rgbColor const & color)
    {
        mFlatLampLightColor = color;

        OnFlatLampLightColorUpdated();
    }

    rgbColor const & GetDefaultWaterColor() const
    {
        return mDefaultWaterColor;
    }

    void SetDefaultWaterColor(rgbColor const & color)
    {
        mDefaultWaterColor = color;

        OnDefaultWaterColorUpdated();
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

    DebugShipRenderModeType GetDebugShipRenderMode() const
    {
        return mRenderParameters.DebugShipRenderMode;
    }

    void SetDebugShipRenderMode(DebugShipRenderModeType debugShipRenderMode)
    {
        mRenderParameters.DebugShipRenderMode = debugShipRenderMode;
    }

    VectorFieldRenderModeType GetVectorFieldRenderMode() const
    {
        return mVectorFieldRenderMode;
    }

    void SetVectorFieldRenderMode(VectorFieldRenderModeType vectorFieldRenderMode)
    {
        mVectorFieldRenderMode = vectorFieldRenderMode;
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

    bool GetDrawHeatOverlay() const
    {
        return mDrawHeatOverlay;
    }

    void SetDrawHeatOverlay(bool drawHeatOverlay)
    {
        mDrawHeatOverlay = drawHeatOverlay;

        OnDrawHeatOverlayUpdated();
    }

    float GetHeatOverlayTransparency() const
    {
        return mHeatOverlayTransparency;
    }

    void SetHeatOverlayTransparency(float transparency)
    {
        mHeatOverlayTransparency = transparency;

        OnHeatOverlayTransparencyUpdated();
    }

    ShipFlameRenderModeType GetShipFlameRenderMode() const
    {
        return mRenderParameters.ShipFlameRenderMode;
    }

    void SetShipFlameRenderMode(ShipFlameRenderModeType shipFlameRenderMode)
    {
        mRenderParameters.ShipFlameRenderMode = shipFlameRenderMode;
        // No need to notify, will be picked up
    }

    float const & GetShipFlameSizeAdjustment() const
    {
        return mShipFlameSizeAdjustment;
    }

    void SetShipFlameSizeAdjustment(float shipFlameSizeAdjustment)
    {
        mShipFlameSizeAdjustment = shipFlameSizeAdjustment;

        for (auto & s : mShips)
        {
            s->SetShipFlameSizeAdjustment(mShipFlameSizeAdjustment);
        }
    }

    static constexpr float MinShipFlameSizeAdjustment = 0.1f;
    static constexpr float MaxShipFlameSizeAdjustment = 20.0f;

    //
    // Screen <-> World transformations
    //

    inline vec2f ScreenToWorld(vec2f const & screenCoordinates) const
    {
        return mRenderParameters.View.ScreenToWorld(screenCoordinates);
    }

    inline vec2f ScreenOffsetToWorldOffset(vec2f const & screenOffset) const
    {
        return mRenderParameters.View.ScreenOffsetToWorldOffset(screenOffset);
    }

    //
    // Statistics
    //

    RenderStatistics GetStatistics() const
    {
        return mRenderStats.load();
    }

public:

    void RebindContext(std::function<void()> rebindContextFunction);

    void Reset();

    void ValidateShipTexture(RgbaImageData const & texture) const;

    void AddShip(
        ShipId shipId,
        size_t pointCount,
        RgbaImageData texture);

    RgbImageData TakeScreenshot();

public:

    void UpdateStart();

    void UpdateEnd();

    void RenderStart();

    void UploadStart();

    void UploadStarsStart(size_t starCount);

    inline void UploadStar(
        float ndcX,
        float ndcY,
        float brightness)
    {
        //
        // Populate vertex in buffer
        //

        mStarVertexBuffer.emplace_back(ndcX, ndcY, brightness);
    }

    void UploadStarsEnd();

    inline void UploadStormAmbientDarkening(float darkening)
    {
        if (darkening != mStormAmbientDarkening)
        {
            mStormAmbientDarkening = darkening;
            mRenderParameters.EffectiveAmbientLightIntensity = CalculateEffectiveAmbientLightIntensity();
            mRenderParameters.IsEffectiveAmbientLightIntensityDirty = true;
        }
    }

    inline void UploadRain(float density)
    {
        if (density != mRainDensity)
        {
            mRainDensity = density;
            mIsRainDensityDirty = true;
        }
    }

    void UploadLightningsStart(size_t lightningCount);

    inline void UploadBackgroundLightning(
        float ndcX,
        float progress,
        float renderProgress,
        float personalitySeed)
    {
        // Get NDC coordinates of world y=0 (i.e. sea level)
        float const ndcSeaLevel = mRenderParameters.View.WorldToNdc(vec2f::zero()).y;

        // Store vertices
        StoreLightningVertices(
            ndcX,
            ndcSeaLevel,
            progress,
            renderProgress,
            personalitySeed,
            mBackgroundLightningVertexCount);

        mBackgroundLightningVertexCount += 6;
    }

    inline void UploadForegroundLightning(
        vec2f tipWorldCoordinates,
        float progress,
        float renderProgress,
        float personalitySeed)
    {
        // Get NDC coordinates of tip point, a few metres down,
        // to make sure tip touches visually the point
        vec2f const ndcTip = mRenderParameters.View.WorldToNdc(
            tipWorldCoordinates
            + vec2f(0.0f, -3.0f));

        // Store vertices
        StoreLightningVertices(
            ndcTip.x,
            ndcTip.y,
            progress,
            renderProgress,
            personalitySeed,
            mLightningVertexBuffer.max_size() - (mForegroundLightningVertexCount + 6));

        mForegroundLightningVertexCount += 6;
    }

    void UploadLightningsEnd();

    void UploadCloudsStart(size_t cloudCount);

    inline void UploadCloud(
        uint32_t cloudId,
        float virtualX,     // [-1.5, +1.5]
        float virtualY,     // [-0.5, +0.5]
        float scale,
        float darkening)    // 0.0:dark, 1.0:light
    {
        //
        // We use Normalized Device Coordinates here
        //

        //
        // Map input slice [-0.5, +0.5], [-0.5, +0.5] into NDC [-1.0, +1.0], [-1.0, +1.0]
        //

        float const mappedX = virtualX * 2.0f;
        float const mappedY = virtualY * 2.0f;

        // TEST CODE: this code fits everything in the visible window
        ////float const mappedX = virtualX / 1.5f;
        ////float const mappedY = virtualY / 1.5f;
        ////scale /= 1.5f;


        //
        // Populate quad in buffer
        //

        float const aspectRatio = mRenderParameters.View.GetAspectRatio();

        size_t const cloudTextureIndex = static_cast<size_t>(cloudId) % mCloudTextureAtlasMetadata->GetFrameMetadata().size();

        auto cloudAtlasFrameMetadata = mCloudTextureAtlasMetadata->GetFrameMetadata(
            CloudTextureGroups::Cloud,
            static_cast<TextureFrameIndex>(cloudTextureIndex));

        float leftX = mappedX - scale * cloudAtlasFrameMetadata.FrameMetadata.AnchorWorldX;
        float rightX = mappedX + scale * (cloudAtlasFrameMetadata.FrameMetadata.WorldWidth - cloudAtlasFrameMetadata.FrameMetadata.AnchorWorldX);
        float topY = mappedY + scale * (cloudAtlasFrameMetadata.FrameMetadata.WorldHeight - cloudAtlasFrameMetadata.FrameMetadata.AnchorWorldY) * aspectRatio;
        float bottomY = mappedY - scale * cloudAtlasFrameMetadata.FrameMetadata.AnchorWorldY * aspectRatio;

        // top-left
        mCloudVertexBuffer.emplace_back(
            leftX,
            topY,
            cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.x,
            cloudAtlasFrameMetadata.TextureCoordinatesTopRight.y,
            darkening);

        // bottom-left
        mCloudVertexBuffer.emplace_back(
            leftX,
            bottomY,
            cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.x,
            cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.y,
            darkening);

        // top-right
        mCloudVertexBuffer.emplace_back(
            rightX,
            topY,
            cloudAtlasFrameMetadata.TextureCoordinatesTopRight.x,
            cloudAtlasFrameMetadata.TextureCoordinatesTopRight.y,
            darkening);

        // bottom-left
        mCloudVertexBuffer.emplace_back(
            leftX,
            bottomY,
            cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.x,
            cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.y,
            darkening);

        // top-right
        mCloudVertexBuffer.emplace_back(
            rightX,
            topY,
            cloudAtlasFrameMetadata.TextureCoordinatesTopRight.x,
            cloudAtlasFrameMetadata.TextureCoordinatesTopRight.y,
            darkening);

        // bottom-right
        mCloudVertexBuffer.emplace_back(
            rightX,
            bottomY,
            cloudAtlasFrameMetadata.TextureCoordinatesTopRight.x,
            cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.y,
            darkening);
    }

    void UploadCloudsEnd();

    void UploadLandStart(size_t slices);

    inline void UploadLand(
        float x,
        float yLand)
    {
        float const yVisibleWorldBottom = mRenderParameters.View.GetVisibleWorldBottomRight().y;

        //
        // Store Land element
        //

        LandSegment & landSegment = mLandSegmentBuffer.emplace_back();

        landSegment.x1 = x;
        landSegment.y1 = yLand;
        landSegment.depth1 = 0.0f;
        landSegment.x2 = x;
        // If land is invisible (below), then keep both points at same height, or else interpolated lines
        // will have a slope varying with the y of the visible world bottom
        float yBottom = yLand >= yVisibleWorldBottom ? yVisibleWorldBottom : yLand;
        landSegment.y2 = yBottom;
        landSegment.depth2 = -(yBottom - yLand); // Height of land
    }

    void UploadLandEnd();

    void UploadOceanStart(size_t slices);

    inline void UploadOcean(
        float x,
        float yOcean,
        float oceanDepth)
    {
        float const yVisibleWorldBottom = mRenderParameters.View.GetVisibleWorldBottomRight().y;

        //
        // Store ocean element
        //

        OceanSegment & oceanSegment = mOceanSegmentBuffer.emplace_back();

        oceanSegment.x1 = x;
        float const oceanSegmentY1 = yOcean;
        oceanSegment.y1 = oceanSegmentY1;

        oceanSegment.x2 = x;
        float const oceanSegmentY2 = yVisibleWorldBottom;
        oceanSegment.y2 = oceanSegmentY2;

        switch (mOceanRenderMode)
        {
            case OceanRenderModeType::Texture:
            {
                // Texture sample Y levels: anchor texture at top of wave,
                // and set bottom at total visible height (after all, ocean texture repeats)
                oceanSegment.value1 = 0.0f; // This is at yOcean
                oceanSegment.value2 = yOcean - yVisibleWorldBottom; // Negative if yOcean invisible, but then who cares

                break;
            }

            case OceanRenderModeType::Depth:
            {
                // Depth: top=0.0, bottom=height as fraction of ocean depth
                oceanSegment.value1 = 0.0f;
                oceanSegment.value2 = oceanDepth != 0.0f
                    ? abs(oceanSegmentY2 - oceanSegmentY1) / oceanDepth
                    : 0.0f;

                break;
            }

            case OceanRenderModeType::Flat:
            {
                // Nop, but be nice
                oceanSegment.value1 = 0.0f;
                oceanSegment.value2 = 0.0f;

                break;
            }
        }
    }

    void UploadOceanEnd();

    void UploadAMBombPreImplosion(
        vec2f const & centerPosition,
        float progress,
        float radius)
    {
        float const leftX = centerPosition.x - radius;
        float const rightX = centerPosition.x + radius;
        float const topY = centerPosition.y + radius;
        float const bottomY = centerPosition.y - radius;

        // Triangle 1

        // Left, bottom
        mAMBombPreImplosionVertexBuffer.emplace_back(
            vec2f(leftX, bottomY),
            centerPosition,
            progress,
            radius);

        // Left top
        mAMBombPreImplosionVertexBuffer.emplace_back(
            vec2f(leftX, topY),
            centerPosition,
            progress,
            radius);

        // Right bottom
        mAMBombPreImplosionVertexBuffer.emplace_back(
            vec2f(rightX, bottomY),
            centerPosition,
            progress,
            radius);

        // Triangle 2

        // Left top
        mAMBombPreImplosionVertexBuffer.emplace_back(
            vec2f(leftX, topY),
            centerPosition,
            progress,
            radius);

        // Right bottom
        mAMBombPreImplosionVertexBuffer.emplace_back(
            vec2f(rightX, bottomY),
            centerPosition,
            progress,
            radius);

        // Right, top
        mAMBombPreImplosionVertexBuffer.emplace_back(
            vec2f(rightX, topY),
            centerPosition,
            progress,
            radius);
    }

    void UploadCrossOfLight(
        vec2f const & centerPosition,
        float progress)
    {
        auto const & viewModel = mRenderParameters.View;

        // Triangle 1

        mCrossOfLightVertexBuffer.emplace_back(
            vec2f(viewModel.GetVisibleWorldTopLeft().x, viewModel.GetVisibleWorldBottomRight().y), // left, bottom
            centerPosition,
            progress);

        mCrossOfLightVertexBuffer.emplace_back(
            viewModel.GetVisibleWorldTopLeft(), // left, top
            centerPosition,
            progress);

        mCrossOfLightVertexBuffer.emplace_back(
            viewModel.GetVisibleWorldBottomRight(), // right, bottom
            centerPosition,
            progress);

        // Triangle 2

        mCrossOfLightVertexBuffer.emplace_back(
            viewModel.GetVisibleWorldTopLeft(), // left, top
            centerPosition,
            progress);

        mCrossOfLightVertexBuffer.emplace_back(
            viewModel.GetVisibleWorldBottomRight(), // right, bottom
            centerPosition,
            progress);

        mCrossOfLightVertexBuffer.emplace_back(
            vec2f(viewModel.GetVisibleWorldBottomRight().x, viewModel.GetVisibleWorldTopLeft().y),  // right, top
            centerPosition,
            progress);
    }

    void UploadHeatBlasterFlame(
        vec2f const & centerPosition,
        float radius,
        HeatBlasterActionType action)
    {
        //
        // Populate vertices
        //

        float const quadHalfSize = (radius * 1.5f) / 2.0f; // Add some slack for transparency
        float const left = centerPosition.x - quadHalfSize;
        float const right = centerPosition.x + quadHalfSize;
        float const top = centerPosition.y + quadHalfSize;
        float const bottom = centerPosition.y - quadHalfSize;

        // Triangle 1

        mHeatBlasterFlameVertexBuffer[0].vertexPosition = vec2f(left, bottom);
        mHeatBlasterFlameVertexBuffer[0].flameSpacePosition = vec2f(-0.5f, -0.5f);

        mHeatBlasterFlameVertexBuffer[1].vertexPosition = vec2f(left, top);
        mHeatBlasterFlameVertexBuffer[1].flameSpacePosition = vec2f(-0.5f, 0.5f);

        mHeatBlasterFlameVertexBuffer[2].vertexPosition = vec2f(right, bottom);
        mHeatBlasterFlameVertexBuffer[2].flameSpacePosition = vec2f(0.5f, -0.5f);

        // Triangle 2

        mHeatBlasterFlameVertexBuffer[3].vertexPosition = vec2f(left, top);
        mHeatBlasterFlameVertexBuffer[3].flameSpacePosition = vec2f(-0.5f, 0.5f);

        mHeatBlasterFlameVertexBuffer[4].vertexPosition = vec2f(right, bottom);
        mHeatBlasterFlameVertexBuffer[4].flameSpacePosition = vec2f(0.5f, -0.5f);

        mHeatBlasterFlameVertexBuffer[5].vertexPosition = vec2f(right, top);
        mHeatBlasterFlameVertexBuffer[5].flameSpacePosition = vec2f(0.5f, 0.5f);

        //
        // Store shader
        //

        switch (action)
        {
            case HeatBlasterActionType::Cool:
            {
                mHeatBlasterFlameShaderToRender = Render::ProgramType::HeatBlasterFlameCool;
                break;
            }

            case HeatBlasterActionType::Heat:
            {
                mHeatBlasterFlameShaderToRender = Render::ProgramType::HeatBlasterFlameHeat;
                break;
            }
        }
    }

    void UploadFireExtinguisherSpray(
        vec2f const & centerPosition,
        float radius)
    {
        //
        // Populate vertices
        //

        float const quadHalfSize = (radius * 3.5f) / 2.0f; // Add some slack to account for transparency
        float const left = centerPosition.x - quadHalfSize;
        float const right = centerPosition.x + quadHalfSize;
        float const top = centerPosition.y + quadHalfSize;
        float const bottom = centerPosition.y - quadHalfSize;

        // Triangle 1

        mFireExtinguisherSprayVertexBuffer[0].vertexPosition = vec2f(left, bottom);
        mFireExtinguisherSprayVertexBuffer[0].spraySpacePosition = vec2f(-0.5f, -0.5f);

        mFireExtinguisherSprayVertexBuffer[1].vertexPosition = vec2f(left, top);
        mFireExtinguisherSprayVertexBuffer[1].spraySpacePosition = vec2f(-0.5f, 0.5f);

        mFireExtinguisherSprayVertexBuffer[2].vertexPosition = vec2f(right, bottom);
        mFireExtinguisherSprayVertexBuffer[2].spraySpacePosition = vec2f(0.5f, -0.5f);

        // Triangle 2

        mFireExtinguisherSprayVertexBuffer[3].vertexPosition = vec2f(left, top);
        mFireExtinguisherSprayVertexBuffer[3].spraySpacePosition = vec2f(-0.5f, 0.5f);

        mFireExtinguisherSprayVertexBuffer[4].vertexPosition = vec2f(right, bottom);
        mFireExtinguisherSprayVertexBuffer[4].spraySpacePosition = vec2f(0.5f, -0.5f);

        mFireExtinguisherSprayVertexBuffer[5].vertexPosition = vec2f(right, top);
        mFireExtinguisherSprayVertexBuffer[5].spraySpacePosition = vec2f(0.5f, 0.5f);

        //
        // Store shader
        //

        mFireExtinguisherSprayShaderToRender = Render::ProgramType::FireExtinguisherSpray;
    }

    void UploadShipsStart()
    {
        // Nop
    }

    void UploadShipStart(
        ShipId shipId,
        PlaneId maxMaxPlaneId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadStart(maxMaxPlaneId);
    }

    void UploadShipPointImmutableAttributes(
        ShipId shipId,
        vec2f const * textureCoordinates)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointImmutableAttributes(textureCoordinates);
    }

    void UploadShipPointMutableAttributesStart(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointMutableAttributesStart();
    }

    void UploadShipPointMutableAttributes(
        ShipId shipId,
        vec2f const * position,
        float const * light,
        float const * water,
        size_t lightAndWaterCount)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointMutableAttributes(
            position,
            light,
            water,
            lightAndWaterCount);
    }

    void UploadShipPointMutableAttributesPlaneId(
        ShipId shipId,
        float const * planeId,
        size_t startDst,
        size_t count)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointMutableAttributesPlaneId(
            planeId,
            startDst,
            count);
    }

    void UploadShipPointMutableAttributesDecay(
        ShipId shipId,
        float const * decay,
        size_t startDst,
        size_t count)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointMutableAttributesDecay(
            decay,
            startDst,
            count);
    }

    void UploadShipPointMutableAttributesEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointMutableAttributesEnd();
    }

    // Upload is Asynchronous - buffer may not be used until the
    // next UpdateStart
    void UploadShipPointColors(
        ShipId shipId,
        vec4f const * color,
        size_t startDst,
        size_t count)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        // Run upload asynchronously
        mRenderThread.QueueTask(
            [=]()
            {
                mShips[shipId]->UploadPointColors(
                    color,
                    startDst,
                    count);
            });
    }

    // Upload is Asynchronous - buffer may not be used until the
    // next UpdateStart
    void UploadShipPointTemperature(
        ShipId shipId,
        float const * temperature,
        size_t startDst,
        size_t count)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        // Run upload asynchronously
        mRenderThread.QueueTask(
            [=]()
            {
                mShips[shipId]->UploadPointTemperature(
                    temperature,
                    startDst,
                    count);
            });
    }

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

    inline void UploadShipElementsEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementsEnd();
    }

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

    inline void UploadShipFlamesStart(
        ShipId shipId,
        size_t count,
        float windSpeedMagnitude)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadFlamesStart(count, windSpeedMagnitude);
    }

    inline void UploadShipFlame(
        ShipId shipId,
        PlaneId planeId,
        vec2f const & baseCenterPosition,
        vec2f const & flameVector,
        float scale,
        float flamePersonalitySeed,
        bool isOnChain)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadFlame(
            planeId,
            baseCenterPosition,
            flameVector,
            scale,
            flamePersonalitySeed,
            isOnChain,
            mRenderParameters);
    }

    void UploadShipFlamesEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadFlamesEnd();
    }

    inline void UploadShipExplosion(
        ShipId shipId,
        PlaneId planeId,
        vec2f const & centerPosition,
        float halfQuadSize,
        ExplosionType explosionType,
        float personalitySeed,
        float progress)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadExplosion(
            planeId,
            centerPosition,
            halfQuadSize,
            explosionType,
            personalitySeed,
            progress);
    }

    inline void UploadShipSparkle(
        ShipId shipId,
        PlaneId planeId,
        vec2f const & position,
        vec2f const & velocityVector,
        float progress)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadSparkle(
            planeId,
            position,
            velocityVector,
            progress);
    }

    inline void UploadShipAirBubble(
        ShipId shipId,
        PlaneId planeId,
        vec2f const & position,
        float scale,
        float alpha)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadAirBubble(
            planeId,
            position,
            scale,
            alpha);
    }

    inline void UploadShipGenericMipMappedTextureRenderSpecification(
        ShipId shipId,
        PlaneId planeId,
        TextureFrameId<GenericMipMappedTextureGroups> const & textureFrameId,
        vec2f const & position)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadGenericMipMappedTextureRenderSpecification(
            planeId,
            textureFrameId,
            position);
    }

    inline void UploadShipGenericMipMappedTextureRenderSpecification(
        ShipId shipId,
        PlaneId planeId,
        TextureFrameId<GenericMipMappedTextureGroups> const & textureFrameId,
        vec2f const & position,
        float scale,
        vec2f const & rotationBase,
        vec2f const & rotationOffset,
        float alpha)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadGenericMipMappedTextureRenderSpecification(
            planeId,
            textureFrameId,
            position,
            scale,
            rotationBase,
            rotationOffset,
            alpha);
    }

    inline void UploadShipGenericMipMappedTextureRenderSpecification(
        ShipId shipId,
        PlaneId planeId,
        TextureFrameId<GenericMipMappedTextureGroups> const & textureFrameId,
        vec2f const & position,
        float scale,
        float angle,
        float alpha)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadGenericMipMappedTextureRenderSpecification(
            planeId,
            textureFrameId,
            position,
            scale,
            angle,
            alpha);
    }

    inline void UploadShipGenericMipMappedTextureRenderSpecification(
        ShipId shipId,
        PlaneId planeId,
        float personalitySeed,
        GenericMipMappedTextureGroups textureGroup,
        vec2f const & position,
        float scale,
        float alpha)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadGenericMipMappedTextureRenderSpecification(
            planeId,
            personalitySeed,
            textureGroup,
            position,
            scale,
            alpha);
    }

    inline void UploadShipElementEphemeralPointsStart(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementEphemeralPointsStart();
    }

    inline void UploadShipElementEphemeralPoint(
        ShipId shipId,
        int pointIndex)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementEphemeralPoint(
            pointIndex);
    }

    void UploadShipElementEphemeralPointsEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementEphemeralPointsEnd();
    }

    inline void UploadShipHighlight(
        ShipId shipId,
        HighlightModeType highlightMode,
        PlaneId planeId,
        vec2f const & centerPosition,
        float halfQuadSize,
        rgbColor color,
        float progress)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadHighlight(
            highlightMode,
            planeId,
            centerPosition,
            halfQuadSize,
            color,
            progress);
    }

    void UploadShipVectors(
        ShipId shipId,
        size_t count,
        vec2f const * position,
        float const * planeId,
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

    void UploadShipEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadEnd();
    }

    void UploadShipsEnd()
    {
        // Nop
    }

    void UploadTextNotificationStart(FontType fontType)
    {
        mNotificationRenderContext->UploadTextNotificationStart(fontType);
    }

    void UploadTextNotificationLine(
        FontType font,
        std::string const & text,
        AnchorPositionType anchor,
        vec2f const & screenOffset,
        float alpha)
    {
        mNotificationRenderContext->UploadTextNotificationLine(
            font,
            text,
            anchor,
            screenOffset,
            alpha);
    }

    void UploadTextNotificationEnd(FontType fontType)
    {
        mNotificationRenderContext->UploadTextNotificationEnd(fontType);
    }

    void UploadTextureNotificationStart()
    {
        mNotificationRenderContext->UploadTextureNotificationStart();
    }

    void UploadTextureNotification(
        TextureFrameId<GenericLinearTextureGroups> const & textureFrameId,
        AnchorPositionType anchor,
        vec2f const & screenOffset, // In texture-size fraction (0.0 -> 1.0)
        float alpha)
    {
        mNotificationRenderContext->UploadTextureNotification(
            textureFrameId,
            anchor,
            screenOffset,
            alpha);
    }

    void UploadTextureNotificationEnd()
    {
        mNotificationRenderContext->UploadTextureNotificationEnd();
    }

    void UploadEnd();

    void Draw();

    void RenderEnd();

private:

    inline void StoreLightningVertices(
        float ndcX,
        float ndcBottomY,
        float progress,
        float renderProgress,
        float personalitySeed,
        size_t vertexBufferIndex)
    {
        if (ndcBottomY > 1.0)
            return; // Above top, discard

        float constexpr LightningQuadWidth = 0.5f;

        float const leftX = ndcX - LightningQuadWidth / 2.0f;
        float const rightX = ndcX + LightningQuadWidth / 2.0f;
        float const topY = 1.0f;
        float const bottomY = ndcBottomY;

        // Append vertices - two triangles

        // Triangle 1

        // Top-left
        mLightningVertexBuffer.emplace_at(
            vertexBufferIndex++,
            vec2f(leftX, topY),
            -1.0f,
            ndcBottomY,
            progress,
            renderProgress,
            personalitySeed);

        // Top-Right
        mLightningVertexBuffer.emplace_at(
            vertexBufferIndex++,
            vec2f(rightX, topY),
            1.0f,
            ndcBottomY,
            progress,
            renderProgress,
            personalitySeed);

        // Bottom-left
        mLightningVertexBuffer.emplace_at(
            vertexBufferIndex++,
            vec2f(leftX, bottomY),
            -1.0f,
            ndcBottomY,
            progress,
            renderProgress,
            personalitySeed);

        // Triangle 2

        // Top-Right
        mLightningVertexBuffer.emplace_at(
            vertexBufferIndex++,
            vec2f(rightX, topY),
            1.0f,
            ndcBottomY,
            progress,
            renderProgress,
            personalitySeed);

        // Bottom-left
        mLightningVertexBuffer.emplace_at(
            vertexBufferIndex++,
            vec2f(leftX, bottomY),
            -1.0f,
            ndcBottomY,
            progress,
            renderProgress,
            personalitySeed);

        // Bottom-right
        mLightningVertexBuffer.emplace_at(
            vertexBufferIndex++,
            vec2f(rightX, bottomY),
            1.0f,
            ndcBottomY,
            progress,
            renderProgress,
            personalitySeed);
    }

private:

    void InitializeBuffersAndVAOs();
    void InitializeCloudTextures(ResourceLocator const & resourceLocator);
    void InitializeWorldTextures(ResourceLocator const & resourceLocator);
    void InitializeGenericTextures(ResourceLocator const & resourceLocator);
    void InitializeExplosionTextures(ResourceLocator const & resourceLocator);

    void RenderStars(RenderParameters const & renderParameters);
    void PrepareRenderLightnings(RenderParameters const & renderParameters);
    void RenderCloudsAndBackgroundLightnings(RenderParameters const & renderParameters);
    void RenderOcean(bool opaquely, RenderParameters const & renderParameters);
    void RenderOceanFloor(RenderParameters const & renderParameters);
    void RenderAMBombPreImplosions(RenderParameters const & renderParameters);
    void RenderCrossesOfLight(RenderParameters const & renderParameters);
    void RenderHeatBlasterFlame(RenderParameters const & renderParameters);
    void RenderFireExtinguisherSpray(RenderParameters const & renderParameters);
    void RenderForegroundLightnings(RenderParameters const & renderParameters);
    void RenderRain(RenderParameters const & renderParameters);
    void RenderWorldBorder(RenderParameters const & renderParameters);

    void ProcessParameterChanges(RenderParameters const & renderParameters);
    void ApplyViewModelChanges(RenderParameters const & renderParameters);
    void ApplyCanvasSizeChanges(RenderParameters const & renderParameters);
    void ApplyEffectiveAmbientLightIntensityChanges(RenderParameters const & renderParameters);
    // TODOOLD
    void OnOceanTransparencyUpdated();
    void OnOceanDarkeningRateUpdated();
    void OnOceanRenderParametersUpdated();
    void OnOceanTextureIndexUpdated();
    void OnLandRenderParametersUpdated();
    void OnLandTextureIndexUpdated();
    // Ship
    void OnFlatLampLightColorUpdated();
    void OnDefaultWaterColorUpdated();
    void OnWaterContrastUpdated();
    void OnWaterLevelOfDetailUpdated();
    void OnShowStressedSpringsUpdated();
    void OnDrawHeatOverlayUpdated();
    void OnHeatOverlayTransparencyUpdated();

    void RecalculateWorldBorder(RenderParameters const & renderParameters);
    float CalculateEffectiveAmbientLightIntensity() const;
    vec4f CalculateLampLightColor() const;
    vec4f CalculateWaterColor() const;

private:

    //
    // Render Thread
    //

    // The thread running all of our OpenGL calls
    TaskThread mRenderThread;

    // The asynchronous rendering tasks from the previous iteration,
    // which we have to wait for before proceeding further
    TaskThread::TaskCompletionIndicator mLastRenderUploadEndCompletionIndicator;
    TaskThread::TaskCompletionIndicator mLastRenderDrawCompletionIndicator;

    //
    // Types
    //

#pragma pack(push, 1)

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

    struct LightningVertex
    {
        vec2f ndc;
        float spacePositionX;
        float ndcBottomY;
        float progress;
        float renderProgress;
        float personalitySeed;

        LightningVertex(
            vec2f _ndc,
            float _spacePositionX,
            float _ndcBottomY,
            float _progress,
            float _renderProgress,
            float _personalitySeed)
            : ndc(_ndc)
            , spacePositionX(_spacePositionX)
            , ndcBottomY(_ndcBottomY)
            , progress(_progress)
            , renderProgress(_renderProgress)
            , personalitySeed(_personalitySeed)
        {}
    };

    struct CloudVertex
    {
        float ndcX;
        float ndcY;
        float ndcTextureX;
        float ndcTextureY;
        float darkness;

        CloudVertex(
            float _ndcX,
            float _ndcY,
            float _ndcTextureX,
            float _ndcTextureY,
            float _darkness)
            : ndcX(_ndcX)
            , ndcY(_ndcY)
            , ndcTextureX(_ndcTextureX)
            , ndcTextureY(_ndcTextureY)
            , darkness(_darkness)
        {}
    };

    struct LandSegment
    {
        float x1;
        float y1;
        float depth1;
        float x2;
        float y2;
        float depth2;
    };

    struct OceanSegment
    {
        float x1;
        float y1;
        float value1;

        float x2;
        float y2;
        float value2;
    };

    struct AMBombPreImplosionVertex
    {
        vec2f vertex;
        vec2f centerPosition;
        float progress;
        float radius;

        AMBombPreImplosionVertex(
            vec2f _vertex,
            vec2f _centerPosition,
            float _progress,
            float _radius)
            : vertex(_vertex)
            , centerPosition(_centerPosition)
            , progress(_progress)
            , radius(_radius)
        {}
    };

    struct CrossOfLightVertex
    {
        vec2f vertex;
        vec2f centerPosition;
        float progress;

        CrossOfLightVertex(
            vec2f _vertex,
            vec2f _centerPosition,
            float _progress)
            : vertex(_vertex)
            , centerPosition(_centerPosition)
            , progress(_progress)
        {}
    };

    struct HeatBlasterFlameVertex
    {
        vec2f vertexPosition;
        vec2f flameSpacePosition;

        HeatBlasterFlameVertex()
        {}
    };

    struct FireExtinguisherSprayVertex
    {
        vec2f vertexPosition;
        vec2f spraySpacePosition;

        FireExtinguisherSprayVertex()
        {}
    };

    struct RainVertex
    {
        float ndcX;
        float ndcY;

        RainVertex(
            float _ndcX,
            float _ndcY)
            : ndcX(_ndcX)
            , ndcY(_ndcY)
        {}
    };

    struct WorldBorderVertex
    {
        float x;
        float y;
        float textureX;
        float textureY;

        WorldBorderVertex(
            float _x,
            float _y,
            float _textureX,
            float _textureY)
            : x(_x)
            , y(_y)
            , textureX(_textureX)
            , textureY(_textureY)
        {}
    };

#pragma pack(pop)

    //
    // VBOs and uploaded buffers and params
    //

    BoundedVector<StarVertex> mStarVertexBuffer;
    bool mIsStarVertexBufferDirty;
    GameOpenGLVBO mStarVBO;
    size_t mStarVBOAllocatedVertexSize;

    BoundedVector<LightningVertex> mLightningVertexBuffer;
    size_t mBackgroundLightningVertexCount;
    size_t mForegroundLightningVertexCount;
    GameOpenGLVBO mLightningVBO;
    size_t mLightningVBOAllocatedVertexSize;

    BoundedVector<CloudVertex> mCloudVertexBuffer;
    GameOpenGLVBO mCloudVBO;
    size_t mCloudVBOAllocatedVertexSize;

    BoundedVector<LandSegment> mLandSegmentBuffer;
    GameOpenGLVBO mLandSegmentVBO;
    size_t mLandSegmentVBOAllocatedVertexSize;

    BoundedVector<OceanSegment> mOceanSegmentBuffer;
    GameOpenGLVBO mOceanSegmentVBO;
    size_t mOceanSegmentVBOAllocatedVertexSize;

    std::vector<AMBombPreImplosionVertex> mAMBombPreImplosionVertexBuffer;
    GameOpenGLVBO mAMBombPreImplosionVBO;
    size_t mAMBombPreImplosionVBOAllocatedVertexSize;

    std::vector<CrossOfLightVertex> mCrossOfLightVertexBuffer;
    GameOpenGLVBO mCrossOfLightVBO;
    size_t mCrossOfLightVBOAllocatedVertexSize;

    std::array<HeatBlasterFlameVertex, 6> mHeatBlasterFlameVertexBuffer;
    GameOpenGLVBO mHeatBlasterFlameVBO;

    std::array<FireExtinguisherSprayVertex, 6> mFireExtinguisherSprayVertexBuffer;
    GameOpenGLVBO mFireExtinguisherSprayVBO;

    float mStormAmbientDarkening;

    GameOpenGLVBO mRainVBO;
    float mRainDensity;
    bool mIsRainDensityDirty;

    std::vector<WorldBorderVertex> mWorldBorderVertexBuffer;
    GameOpenGLVBO mWorldBorderVBO;

    //
    // VAOs
    //

    GameOpenGLVAO mStarVAO;
    GameOpenGLVAO mLightningVAO;
    GameOpenGLVAO mCloudVAO;
    GameOpenGLVAO mLandVAO;
    GameOpenGLVAO mOceanVAO;
    GameOpenGLVAO mAMBombPreImplosionVAO;
    GameOpenGLVAO mCrossOfLightVAO;
    GameOpenGLVAO mHeatBlasterFlameVAO;
    GameOpenGLVAO mFireExtinguisherSprayVAO;
    GameOpenGLVAO mRainVAO;
    GameOpenGLVAO mWorldBorderVAO;

    //
    // Textures
    //

    GameOpenGLTexture mCloudTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<CloudTextureGroups>> mCloudTextureAtlasMetadata;

    UploadedTextureManager<WorldTextureGroups> mUploadedWorldTextureManager;

    std::vector<TextureFrameSpecification<WorldTextureGroups>> mOceanTextureFrameSpecifications;
    GameOpenGLTexture mOceanTextureOpenGLHandle;
    size_t mLoadedOceanTextureIndex;

    std::vector<TextureFrameSpecification<WorldTextureGroups>> mLandTextureFrameSpecifications;
    GameOpenGLTexture mLandTextureOpenGLHandle;
    size_t mLoadedLandTextureIndex;

    GameOpenGLTexture mGenericLinearTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<GenericLinearTextureGroups>> mGenericLinearTextureAtlasMetadata;

    GameOpenGLTexture mGenericMipMappedTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<GenericMipMappedTextureGroups>> mGenericMipMappedTextureAtlasMetadata;

    GameOpenGLTexture mExplosionTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<ExplosionTextureGroups>> mExplosionTextureAtlasMetadata;

    UploadedTextureManager<NoiseTextureGroups> mUploadedNoiseTexturesManager;

    //
    // Ships
    //

    std::vector<std::unique_ptr<ShipRenderContext>> mShips;

    //
    // HeatBlaster
    //

    std::optional<Render::ProgramType> mHeatBlasterFlameShaderToRender;

    //
    // Fire extinguisher
    //

    std::optional<Render::ProgramType> mFireExtinguisherSprayShaderToRender;

    //
    // Externally-controlled parameters that only affect Upload (i.e. that do
    // not affect rendering directly) or that purely serve as input to calculated
    // render parameters
    //

    float mAmbientLightIntensity;
    float mShipFlameSizeAdjustment;

private:

    //
    // Rendering externals
    //

    std::function<void()> const mSwapRenderBuffersFunction;

    //
    // Managers
    //

    std::unique_ptr<ShaderManager<ShaderManagerTraits>> mShaderManager;
    std::unique_ptr<NotificationRenderContext> mNotificationRenderContext;

    //
    // Render parameters
    //

    RenderParameters mRenderParameters;

    // TODOOLD
    float mOceanTransparency;
    float mOceanDarkeningRate;
    OceanRenderModeType mOceanRenderMode;
    std::vector<std::pair<std::string, RgbaImageData>> mOceanAvailableThumbnails;
    size_t mSelectedOceanTextureIndex;
    rgbColor mDepthOceanColorStart;
    rgbColor mDepthOceanColorEnd;
    rgbColor mFlatOceanColor;
    LandRenderModeType mLandRenderMode;
    std::vector<std::pair<std::string, RgbaImageData>> mLandAvailableThumbnails;
    size_t mSelectedLandTextureIndex;
    rgbColor mFlatLandColor;
    // Ship
    rgbColor mFlatLampLightColor;
    rgbColor mDefaultWaterColor;
    bool mShowShipThroughOcean;
    float mWaterContrast;
    float mWaterLevelOfDetail;
    VectorFieldRenderModeType mVectorFieldRenderMode;
    float mVectorFieldLengthMultiplier;
    bool mShowStressedSprings;
    bool mDrawHeatOverlay;
    float mHeatOverlayTransparency;    

    //
    // Statistics
    //

    PerfStats & mPerfStats;
    std::atomic<RenderStatistics> mRenderStats;
};

}