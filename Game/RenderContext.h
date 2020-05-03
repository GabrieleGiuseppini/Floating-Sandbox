/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventDispatcher.h"
#include "RenderTypes.h"
#include "ResourceLocator.h"
#include "ShaderTypes.h"
#include "ShipDefinition.h"
#include "ShipRenderContext.h"
#include "TextRenderContext.h"
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
#include <GameCore/ProgressCallback.h>
#include <GameCore/SysSpecifics.h>
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
        ResourceLocator const & resourceLocator,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        ProgressCallback const & progressCallback);

    ~RenderContext();

public:

	//
	// Components
	//

	std::shared_ptr<TextRenderContext> GetTextRenderContext() const
	{
		return mTextRenderContext;
	}


    //
    // World and view properties
    //

    float const & GetZoom() const
    {
        return mViewModel.GetZoom();
    }

    float ClampZoom(float zoom) const
    {
        return mViewModel.ClampZoom(zoom);
    }

    float const & SetZoom(float zoom)
    {
        float const & newZoom = mViewModel.SetZoom(zoom);

        OnViewModelUpdated();

        return newZoom;
    }

    vec2f const & GetCameraWorldPosition() const
    {
        return mViewModel.GetCameraWorldPosition();
    }

    vec2f ClampCameraWorldPosition(vec2f const & pos) const
    {
        return mViewModel.ClampCameraWorldPosition(pos);
    }

    vec2f const & SetCameraWorldPosition(vec2f const & pos)
    {
        vec2f const & newCameraWorldPosition = mViewModel.SetCameraWorldPosition(pos);

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

    void SetPixelOffset(float x, float y)
    {
        mViewModel.SetPixelOffset(x, y);

        OnViewModelUpdated();
    }

    void ResetPixelOffset()
    {
        mViewModel.ResetPixelOffset();

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

    float GetVisibleWorldLeft() const
    {
        return mViewModel.GetVisibleWorldTopLeft().x;
    }

    float GetVisibleWorldRight() const
    {
        return mViewModel.GetVisibleWorldBottomRight().x;
    }

    float GetVisibleWorldTop() const
    {
        return mViewModel.GetVisibleWorldTopLeft().y;
    }

    float GetVisibleWorldBottom() const
    {
        return mViewModel.GetVisibleWorldBottomRight().y;
    }

    float CalculateZoomForWorldWidth(float worldWidth) const
    {
        return mViewModel.CalculateZoomForWorldWidth(worldWidth);
    }

    float CalculateZoomForWorldHeight(float worldHeight) const
    {
        return mViewModel.CalculateZoomForWorldHeight(worldHeight);
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

        OnEffectiveAmbientLightIntensityUpdated();
    }

    float GetEffectiveAmbientLightIntensity() const
    {
        return mEffectiveAmbientLightIntensity;
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

    OceanRenderMode GetOceanRenderMode() const
    {
        return mOceanRenderMode;
    }

    void SetOceanRenderMode(OceanRenderMode oceanRenderMode)
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

    LandRenderMode GetLandRenderMode() const
    {
        return mLandRenderMode;
    }

    void SetLandRenderMode(LandRenderMode landRenderMode)
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

    ShipFlameRenderMode GetShipFlameRenderMode() const
    {
        return mShipFlameRenderMode;
    }

    void SetShipFlameRenderMode(ShipFlameRenderMode shipFlameRenderMode)
    {
        mShipFlameRenderMode = shipFlameRenderMode;

        OnShipFlameRenderModeUpdated();
    }

    float const & GetShipFlameSizeAdjustment() const
    {
        return mShipFlameSizeAdjustment;
    }

    void SetShipFlameSizeAdjustment(float shipFlameSizeAdjustment)
    {
        mShipFlameSizeAdjustment = shipFlameSizeAdjustment;

        OnShipFlameSizeAdjustmentUpdated();
    }

    static constexpr float MinShipFlameSizeAdjustment = 0.1f;
    static constexpr float MaxShipFlameSizeAdjustment = 20.0f;


    //
    // Screen <-> World transformations
    //

    inline vec2f ScreenToWorld(vec2f const & screenCoordinates) const
    {
        return mViewModel.ScreenToWorld(screenCoordinates);
    }

    inline vec2f ScreenOffsetToWorldOffset(vec2f const & screenOffset) const
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
        RgbaImageData texture);

    RgbImageData TakeScreenshot();

public:

	//////////////////////////////////////////////////////////////////
	// Rendering
	//////////////////////////////////////////////////////////////////

    void RenderStart();

    //
    // Stars
    //

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

    void RenderStars();

    //
    // Clouds
    //

    void RenderCloudsStart();


	inline void UploadStormAmbientDarkening(float darkening)
	{
		if (darkening != mCurrentStormAmbientDarkening)
		{
			mCurrentStormAmbientDarkening = darkening;
			OnEffectiveAmbientLightIntensityUpdated();
		}
	}

	inline void UploadRain(float density)
	{
		if (density != mCurrentRainDensity)
		{
			mCurrentRainDensity = density;
			OnRainDensityUpdated();
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
		float const ndcSeaLevel = mViewModel.WorldToNdc(vec2f::zero()).y;

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
		vec2f const ndcTip = mViewModel.WorldToNdc(
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

        size_t const cloudTextureIndex = static_cast<size_t>(cloudId) % mCloudTextureAtlasMetadata->GetFrameMetadata().size();

        auto cloudAtlasFrameMetadata = mCloudTextureAtlasMetadata->GetFrameMetadata(
            CloudTextureGroups::Cloud,
            static_cast<TextureFrameIndex>(cloudTextureIndex));

        float leftX = mappedX - scale * cloudAtlasFrameMetadata.FrameMetadata.AnchorWorldX;
        float rightX = mappedX + scale * (cloudAtlasFrameMetadata.FrameMetadata.WorldWidth - cloudAtlasFrameMetadata.FrameMetadata.AnchorWorldX);
        float topY = mappedY + scale * (cloudAtlasFrameMetadata.FrameMetadata.WorldHeight - cloudAtlasFrameMetadata.FrameMetadata.AnchorWorldY) * mViewModel.GetAspectRatio();
        float bottomY = mappedY - scale * cloudAtlasFrameMetadata.FrameMetadata.AnchorWorldY * mViewModel.GetAspectRatio();

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


    void RenderCloudsEnd();


    //
    // Land
    //

    void UploadLandStart(size_t slices);

    inline void UploadLand(
        float x,
        float yLand)
    {
        float const yVisibleWorldBottom = mViewModel.GetVisibleWorldBottomRight().y;

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

    void RenderLand();


    //
    // Ocean
    //

    void UploadOceanStart(size_t slices);

    inline void UploadOcean(
        float x,
        float yOcean,
        float oceanDepth)
    {
        float const yVisibleWorldBottom = mViewModel.GetVisibleWorldBottomRight().y;

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
            case OceanRenderMode::Texture:
            {
                // Texture sample Y levels: anchor texture at top of wave,
                // and set bottom at total visible height (after all, ocean texture repeats)
                oceanSegment.value1 = 0.0f; // This is at yOcean
                oceanSegment.value2 = yOcean - yVisibleWorldBottom; // Negative if yOcean invisible, but then who cares

                break;
            }

            case OceanRenderMode::Depth:
            {
                // Depth: top=0.0, bottom=height as fraction of ocean depth
                oceanSegment.value1 = 0.0f;
                oceanSegment.value2 = oceanDepth != 0.0f
                    ? abs(oceanSegmentY2 - oceanSegmentY1) / oceanDepth
                    : 0.0f;

                break;
            }

            case OceanRenderMode::Flat:
            {
                // Nop, but be nice
                oceanSegment.value1 = 0.0f;
                oceanSegment.value2 = 0.0f;

                break;
            }
        }
    }

    void UploadOceanEnd();

    void RenderOceanOpaquely()
    {
        RenderOcean(true);
    }

    void RenderOceanTransparently()
    {
        RenderOcean(false);
    }


    //
    // Crosses of light
    //

    void UploadCrossOfLight(
        vec2f const & centerPosition,
        float progress)
    {
        // Triangle 1

        mCrossOfLightVertexBuffer.emplace_back(
            vec2f(mViewModel.GetVisibleWorldTopLeft().x, mViewModel.GetVisibleWorldBottomRight().y), // left, bottom
            centerPosition,
            progress);

        mCrossOfLightVertexBuffer.emplace_back(
            mViewModel.GetVisibleWorldTopLeft(), // left, top
            centerPosition,
            progress);

        mCrossOfLightVertexBuffer.emplace_back(
            mViewModel.GetVisibleWorldBottomRight(), // right, bottom
            centerPosition,
            progress);

        // Triangle 2

        mCrossOfLightVertexBuffer.emplace_back(
            mViewModel.GetVisibleWorldTopLeft(), // left, top
            centerPosition,
            progress);

        mCrossOfLightVertexBuffer.emplace_back(
            mViewModel.GetVisibleWorldBottomRight(), // right, bottom
            centerPosition,
            progress);

        mCrossOfLightVertexBuffer.emplace_back(
            vec2f(mViewModel.GetVisibleWorldBottomRight().x, mViewModel.GetVisibleWorldTopLeft().y),  // right, top
            centerPosition,
            progress);
    }

    //
    // HeatBlaster flame
    //

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

    //
    // Fire extinguisher spray
    //

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

    /////////////////////////////////////////////////////////////////////////
    // Ships
    /////////////////////////////////////////////////////////////////////////

    void RenderShipsStart();


    void RenderShipStart(
        ShipId shipId,
        PlaneId maxMaxPlaneId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->RenderStart(maxMaxPlaneId);
    }

    //
    // Ship Points
    //

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

    void UploadShipPointColors(
        ShipId shipId,
        vec4f const * color,
        size_t startDst,
        size_t count)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointColors(
            color,
            startDst,
            count);
    }

    void UploadShipPointTemperature(
        ShipId shipId,
        float const * temperature,
        size_t startDst,
        size_t count)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointTemperature(
            temperature,
            startDst,
            count);
    }

    //
    // Ship elements
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

    inline void UploadShipElementsEnd(
        ShipId shipId,
        bool doFinalizeEphemeralPoints)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementsEnd(doFinalizeEphemeralPoints);
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
    // Flames
    //

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
            isOnChain);
    }

    void UploadShipFlamesEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadFlamesEnd();
    }

    //
    // Explosions
    //

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

    //
    // Sparkles
    //

    inline void UploadShipSparklesStart(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadSparklesStart();
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

    inline void UploadShipSparklesEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadSparklesEnd();
    }

    //
    // Air bubbles and generic textures
    //

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

    //
    // Ephemeral points
    //

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

    //
    // Highlights
    //

    inline void UploadShipHighlight(
        ShipId shipId,
        HighlightMode highlightMode,
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

    //
    // Vectors
    //

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



    void RenderShipEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->RenderEnd();
    }

    void RenderShipsEnd();


    //
    // Final
    //

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

    void RenderOcean(bool opaquely);

    void RenderCrossesOfLight();
    void RenderHeatBlasterFlame();
    void RenderFireExtinguisherSpray();
	void RenderForegroundLightnings();
	void RenderRain();
    void RenderWorldBorder();

    void OnViewModelUpdated();
    void OnEffectiveAmbientLightIntensityUpdated();
	void OnRainDensityUpdated();
    void OnOceanTransparencyUpdated();
    void OnOceanDarkeningRateUpdated();
    void OnOceanRenderParametersUpdated();
    void OnOceanTextureIndexUpdated();
    void OnLandRenderParametersUpdated();
    void OnLandTextureIndexUpdated();

    void OnFlatLampLightColorUpdated();
    void OnDefaultWaterColorUpdated();
    void OnWaterContrastUpdated();
    void OnWaterLevelOfDetailUpdated();
    void OnShipRenderModeUpdated();
    void OnDebugShipRenderModeUpdated();
    void OnVectorFieldRenderModeUpdated();
    void OnShowStressedSpringsUpdated();
    void OnDrawHeatOverlayUpdated();
    void OnHeatOverlayTransparencyUpdated();
    void OnShipFlameRenderModeUpdated();
    void OnShipFlameSizeAdjustmentUpdated();

    void UpdateWorldBorder();
    vec4f CalculateLampLightColor() const;
    vec4f CalculateWaterColor() const;

private:

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
    // Buffers
    //

    BoundedVector<StarVertex> mStarVertexBuffer;
    GameOpenGLVBO mStarVBO;

	GameOpenGLMappedBuffer<LightningVertex, GL_ARRAY_BUFFER> mLightningVertexBuffer;
	size_t mBackgroundLightningVertexCount;
	size_t mForegroundLightningVertexCount;
	GameOpenGLVBO mLightningVBO;

    GameOpenGLMappedBuffer<CloudVertex, GL_ARRAY_BUFFER> mCloudVertexBuffer;
    GameOpenGLVBO mCloudVBO;

    GameOpenGLMappedBuffer<LandSegment, GL_ARRAY_BUFFER> mLandSegmentBuffer;
    size_t mLandSegmentBufferAllocatedSize;
    GameOpenGLVBO mLandVBO;

    GameOpenGLMappedBuffer<OceanSegment, GL_ARRAY_BUFFER> mOceanSegmentBuffer;
    size_t mOceanSegmentBufferAllocatedSize;
    GameOpenGLVBO mOceanVBO;

    std::vector<CrossOfLightVertex> mCrossOfLightVertexBuffer;
    GameOpenGLVBO mCrossOfLightVBO;

    std::array<HeatBlasterFlameVertex, 6> mHeatBlasterFlameVertexBuffer;
    GameOpenGLVBO mHeatBlasterFlameVBO;

    std::array<FireExtinguisherSprayVertex, 6> mFireExtinguisherSprayVertexBuffer;
    GameOpenGLVBO mFireExtinguisherSprayVBO;

	GameOpenGLVBO mRainVBO;

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

    bool mIsWorldBorderVisible;

    GameOpenGLTexture mGenericLinearTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<GenericLinearTextureGroups>> mGenericLinearTextureAtlasMetadata;

    GameOpenGLTexture mGenericMipMappedTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<GenericMipMappedTextureGroups>> mGenericMipMappedTextureAtlasMetadata;

    GameOpenGLTexture mExplosionTextureAtlasOpenGLHandle;
    std::unique_ptr<TextureAtlasMetadata<ExplosionTextureGroups>> mExplosionTextureAtlasMetadata;

    UploadedTextureManager<NoiseTextureGroups> mUploadedNoiseTexturesManager;

    //
    // Misc Parameters
    //

    float mCurrentStormAmbientDarkening;
	float mCurrentRainDensity;
    float mEffectiveAmbientLightIntensity;

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

private:

    //
    // Managers
    //

    std::shared_ptr<GameEventDispatcher> mGameEventHandler;
    std::unique_ptr<ShaderManager<ShaderManagerTraits>> mShaderManager;
    std::shared_ptr<TextRenderContext> mTextRenderContext;

    //
    // The current render parameters
    //

    ViewModel mViewModel;

    rgbColor mFlatSkyColor;
    float mAmbientLightIntensity;
    float mOceanTransparency;
    float mOceanDarkeningRate;
    OceanRenderMode mOceanRenderMode;
    std::vector<std::pair<std::string, RgbaImageData>> mOceanAvailableThumbnails;
    size_t mSelectedOceanTextureIndex;
    rgbColor mDepthOceanColorStart;
    rgbColor mDepthOceanColorEnd;
    rgbColor mFlatOceanColor;
    LandRenderMode mLandRenderMode;
    std::vector<std::pair<std::string, RgbaImageData>> mLandAvailableThumbnails;
    size_t mSelectedLandTextureIndex;
    rgbColor mFlatLandColor;

    rgbColor mFlatLampLightColor;
    rgbColor mDefaultWaterColor;
    bool mShowShipThroughOcean;
    float mWaterContrast;
    float mWaterLevelOfDetail;
    ShipRenderMode mShipRenderMode;
    DebugShipRenderMode mDebugShipRenderMode;
    VectorFieldRenderMode mVectorFieldRenderMode;
    float mVectorFieldLengthMultiplier;
    bool mShowStressedSprings;
    bool mDrawHeatOverlay;
    float mHeatOverlayTransparency;
    ShipFlameRenderMode mShipFlameRenderMode;
    float mShipFlameSizeAdjustment;

    //
    // Statistics
    //

    RenderStatistics mRenderStatistics;
};

}