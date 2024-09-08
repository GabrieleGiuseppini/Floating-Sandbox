/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-07-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GlobalRenderContext.h"
#include "RenderParameters.h"
#include "RenderTypes.h"
#include "ResourceLocator.h"
#include "ShaderTypes.h"
#include "TextureAtlas.h"
#include "TextureTypes.h"
#include "UploadedTextureManager.h"
#include "ViewModel.h"

#include <GameOpenGL/GameOpenGL.h>
#include <GameOpenGL/ShaderManager.h>

#include <GameCore/AABB.h>
#include <GameCore/BoundedVector.h>
#include <GameCore/Buffer2D.h>
#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/Vectors.h>

#include <array>
#include <cassert>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Render {

class WorldRenderContext
{
public:

    WorldRenderContext(
        ShaderManager<ShaderManagerTraits> & shaderManager,
        GlobalRenderContext const & globalRenderContext);

    ~WorldRenderContext();

    void InitializeCloudTextures(ResourceLocator const & resourceLocator);

    void InitializeWorldTextures(ResourceLocator const & resourceLocator);

    void InitializeFishTextures(ResourceLocator const & resourceLocator);

    void OnReset(RenderParameters const & renderParameters);

    inline std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureOceanAvailableThumbnails() const
    {
        return mOceanAvailableThumbnails;
    }

    inline std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureLandAvailableThumbnails() const
    {
        return mLandAvailableThumbnails;
    }

    inline float GetStormAmbientDarkening() const
    {
        return mStormAmbientDarkening;
    }

    inline float GetRainDensity() const
    {
        return mRainDensity;
    }

public:

    void SetSunRaysInclination(float value)
    {
        mSunRaysInclination = value;
        mIsSunRaysInclinationDirty = true;
    }

    void UploadStart();

    void UploadStarsStart(
        size_t uploadCount,
        size_t totalCount);

    inline void UploadStar(
        size_t starIndex,
        vec2f const & positionNdc,
        float brightness)
    {
        assert(starIndex < mStarVertexBuffer.size());

        //
        // Populate vertex in buffer
        //

        mStarVertexBuffer[starIndex] = StarVertex(positionNdc, brightness);
    }

    void UploadStarsEnd();

    inline void UploadWind(float smoothedWindSpeedMagnitude)
    {
        mRainWindSpeedMagnitude = smoothedWindSpeedMagnitude;
        mIsRainWindSpeedMagnitudeDirty = true;
    }

    inline bool UploadStormAmbientDarkening(float darkening)
    {
        if (darkening != mStormAmbientDarkening) // Damp frequency of calls
        {
            mStormAmbientDarkening = darkening;
            // Just storage, nothing else to do

            return true;
        }
        else
        {
            return false;
        }
    }

    inline void UploadRain(float density)
    {
        if (density != mRainDensity) // Damp frequency of calls
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
        float personalitySeed,
        RenderParameters const & renderParameters)
    {
        // Get NDC coordinates of world y=0 (i.e. sea level)
        float const ndcSeaLevel = renderParameters.View.WorldToNdc(vec2f::zero()).y;

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
        float personalitySeed,
        RenderParameters const & renderParameters)
    {
        // Get NDC coordinates of tip point, a few metres down,
        // to make sure tip touches visually the point
        vec2f const ndcTip = renderParameters.View.WorldToNdc(
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
        float virtualX,         // [-1.5, +1.5]
        float virtualY,         // [0.0, +1.0]
        float virtualZ,         // [0.0, +1.0]
        float scale,
        float darkening,        // 0.0:dark, 1.0:light
        float totalDistanceTraveled,
        RenderParameters const & renderParameters)
    {
        //
        // We use Normalized Device Coordinates here
        //

        // Calculate NDC x: map input slice [-0.5, +0.5] into NDC [-1.0, +1.0]
        float const ndcX = virtualX * 2.0f;

        // Calculate NDC y: apply perspective transform
        float constexpr ZMin = 1.0f;
        float constexpr ZMax = 20.0f * ZMin; // Magic number: so that at this (furthest) Z, denominator is so large that clouds at virtualY=1.0 appear slightly above the horizon
        float const ndcY = (virtualY - mCloudNormalizedViewCamY) / (ZMin + virtualZ * (ZMax - ZMin));

        //
        // Populate quad in buffer
        //

        size_t const cloudTextureIndex = static_cast<size_t>(cloudId) % mCloudTextureAtlasMetadata->GetAllFramesMetadata().size();

        auto const & cloudAtlasFrameMetadata = mCloudTextureAtlasMetadata->GetFrameMetadata(
            CloudTextureGroups::Cloud,
            static_cast<TextureFrameIndex>(cloudTextureIndex));

        float const aspectRatio = renderParameters.View.GetAspectRatio();

        float const ndcWidth = scale * cloudAtlasFrameMetadata.FrameMetadata.WorldWidth;
        float const ndcHeight = scale * cloudAtlasFrameMetadata.FrameMetadata.WorldHeight * aspectRatio;

        float const leftX = ndcX - scale * cloudAtlasFrameMetadata.FrameMetadata.AnchorCenterWorld.x;
        float const rightX = leftX + ndcWidth;
        float const bottomY = ndcY - scale * cloudAtlasFrameMetadata.FrameMetadata.AnchorCenterWorld.y * aspectRatio;
        float const topY = bottomY + ndcHeight;

        // Calculate virtual texture coords: ensure unity circle is always covered
        // TODO: redo better
        float minVirtualTexX, maxVirtualTexX;
        float minVirtualTexY, maxVirtualTexY;
        if (ndcWidth >= ndcHeight)
        {
            minVirtualTexX = 0.5f - ndcWidth / ndcHeight * 0.5f;
            maxVirtualTexX = 0.5f + ndcWidth / ndcHeight * 0.5f;
            minVirtualTexY = 0.0f;
            maxVirtualTexY = 1.0f;
        }
        else
        {
            minVirtualTexX = 0.0f;
            maxVirtualTexX = 1.0f;
            minVirtualTexY = 0.5f - ndcHeight / ndcWidth * 0.5f;
            maxVirtualTexY = 0.5f + ndcHeight / ndcWidth * 0.5f;
        }

        // top-left
        mCloudVertexBuffer.emplace_back(
            vec2f(leftX, topY),
            vec2f(cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.x, cloudAtlasFrameMetadata.TextureCoordinatesTopRight.y),
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter,
            vec2f(minVirtualTexX, maxVirtualTexY),
            darkening,
            totalDistanceTraveled);

        // bottom-left
        mCloudVertexBuffer.emplace_back(
            vec2f(leftX, bottomY),
            vec2f(cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.x, cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.y),
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter,
            vec2f(minVirtualTexX, minVirtualTexY),
            darkening,
            totalDistanceTraveled);

        // top-right
        mCloudVertexBuffer.emplace_back(
            vec2f(rightX, topY),
            vec2f(cloudAtlasFrameMetadata.TextureCoordinatesTopRight.x, cloudAtlasFrameMetadata.TextureCoordinatesTopRight.y),
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter,
            vec2f(maxVirtualTexX, maxVirtualTexY),
            darkening,
            totalDistanceTraveled);

        // bottom-left
        mCloudVertexBuffer.emplace_back(
            vec2f(leftX, bottomY),
            vec2f(cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.x, cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.y),
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter,
            vec2f(minVirtualTexX, minVirtualTexY),
            darkening,
            totalDistanceTraveled);

        // top-right
        mCloudVertexBuffer.emplace_back(
            vec2f(rightX, topY),
            vec2f(cloudAtlasFrameMetadata.TextureCoordinatesTopRight.x, cloudAtlasFrameMetadata.TextureCoordinatesTopRight.y),
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter,
            vec2f(maxVirtualTexX, maxVirtualTexY),
            darkening,
            totalDistanceTraveled);

        // bottom-right
        mCloudVertexBuffer.emplace_back(
            vec2f(rightX, bottomY),
            vec2f(cloudAtlasFrameMetadata.TextureCoordinatesTopRight.x, cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.y),
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter,
            vec2f(maxVirtualTexX, minVirtualTexY),
            darkening,
            totalDistanceTraveled);
    }

    void UploadCloudsEnd();

    bool IsCloudShadowsRenderingEnabled(RenderParameters const & renderParameters) const
    {
        return renderParameters.OceanRenderDetail == OceanRenderDetailType::Detailed;
    }

    void UploadCloudShadows(
        float const * shadowBuffer,
        size_t shadowSampleCount);

    void UploadLandStart(size_t slices);

    inline void UploadLand(
        float x,
        float yLand,
        RenderParameters const & renderParameters)
    {
        float const yVisibleWorldBottom = renderParameters.View.GetVisibleWorld().BottomRight.y;

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

    void UploadOceanBasicStart(size_t slices);

    inline void UploadOceanBasic(
        float x,
        float yOcean,
        RenderParameters const & renderParameters)
    {
        float const yVisibleWorldBottom = renderParameters.View.GetVisibleWorld().BottomRight.y;

        //
        // Store ocean element
        //

        OceanBasicSegment & oceanSegment = mOceanBasicSegmentBuffer.emplace_back();

        oceanSegment.x1 = x;
        oceanSegment.y1 = yOcean;

        oceanSegment.x2 = x;
        oceanSegment.y2 = yVisibleWorldBottom;

        switch (renderParameters.OceanRenderMode)
        {
            case OceanRenderModeType::Texture:
            {
                // Texture sample Y levels: anchor texture at top of wave,
                // and set bottom at total visible height (after all, ocean texture repeats)
                oceanSegment.yWater1 = 0.0f; // This is at yOcean
                oceanSegment.yWater2 = yOcean - yVisibleWorldBottom; // Negative if yOcean invisible, but then who cares

                break;
            }

            case OceanRenderModeType::Depth:
            {
                // Nop, but be nice
                oceanSegment.yWater1 = 0.0f;
                oceanSegment.yWater2 = 0.0f;

                break;
            }

            case OceanRenderModeType::Flat:
            {
                // Nop, but be nice
                oceanSegment.yWater1 = 0.0f;
                oceanSegment.yWater2 = 0.0f;

                break;
            }
        }
    }

    void UploadOceanBasicEnd();

    void UploadOceanDetailedStart(size_t slices);

    inline void UploadOceanDetailed(
        float x,
        float yBack,
        float yMid,
        float yFront,
        float d2YFront,
        RenderParameters const & renderParameters)
    {
        float const yTop = std::max(yBack, std::max(yMid, yFront)) + 10.0f; // Magic offset to allow shader to anti-alias close to the boundary
        float const yVisibleWorldBottom = renderParameters.View.GetVisibleWorld().BottomRight.y;

        //
        // Store ocean element
        //

        OceanDetailedSegment & oceanSegment = mOceanDetailedSegmentBuffer.emplace_back();

        oceanSegment.x1 = x;
        oceanSegment.y1 = yTop;
        oceanSegment.yBack1 = yBack;
        oceanSegment.yMid1 = yMid;
        oceanSegment.yFront1 = yFront;
        oceanSegment.d2YFront1 = d2YFront;

        oceanSegment.x2 = x;
        oceanSegment.y2 = yVisibleWorldBottom;
        oceanSegment.yBack2 = yBack;
        oceanSegment.yMid2 = yMid;
        oceanSegment.yFront2 = yFront;
        oceanSegment.d2YFront2 = d2YFront;

        switch (renderParameters.OceanRenderMode)
        {
            case OceanRenderModeType::Texture:
            {
                // We squash the top a little towards the rest position, to give a slight ondulation
                oceanSegment.yTexture1 = yTop * 0.75f;
                oceanSegment.yTexture2 = yVisibleWorldBottom;

                break;
            }

            case OceanRenderModeType::Depth:
            {
                // Nop, but be nice
                oceanSegment.yTexture1 = 0.0f;
                oceanSegment.yTexture2 = 0.0f;

                break;
            }

            case OceanRenderModeType::Flat:
            {
                // Nop, but be nice
                oceanSegment.yTexture1 = 0.0f;
                oceanSegment.yTexture2 = 0.0f;

                break;
            }
        }
    }

    void UploadOceanDetailedEnd();

    void UploadFishesStart(size_t fishCount);

    inline void UploadFish(
        TextureFrameId<FishTextureGroups> const & textureFrameId,
        vec2f const & position, // position of center
        vec2f const & worldSize,
        float angleCw,
        float horizontalScale,
        float tailX,
        float tailSwing,
        float tailProgress)
    {
        auto const & frame = mFishTextureAtlasMetadata->GetFrameMetadata(textureFrameId);

        // Calculate bounding box, assuming textures are anchored in the center
        float const offsetX = worldSize.x / 2.0f * horizontalScale;
        float const offsetY = worldSize.y / 2.0f;

        // top-left
        mFishVertexBuffer.emplace_back(
            position,
            vec2f(-offsetX, offsetY),
            frame.TextureCoordinatesBottomLeft,
            frame.TextureCoordinatesTopRight,
            vec2f(frame.TextureCoordinatesBottomLeft.x, frame.TextureCoordinatesTopRight.y),
            angleCw,
            tailX,
            tailSwing,
            tailProgress);

        // bottom-left
        mFishVertexBuffer.emplace_back(
            position,
            vec2f(-offsetX, -offsetY),
            frame.TextureCoordinatesBottomLeft,
            frame.TextureCoordinatesTopRight,
            vec2f(frame.TextureCoordinatesBottomLeft.x, frame.TextureCoordinatesBottomLeft.y),
            angleCw,
            tailX,
            tailSwing,
            tailProgress);

        // top-right
        mFishVertexBuffer.emplace_back(
            position,
            vec2f(offsetX, offsetY),
            frame.TextureCoordinatesBottomLeft,
            frame.TextureCoordinatesTopRight,
            vec2f(frame.TextureCoordinatesTopRight.x, frame.TextureCoordinatesTopRight.y),
            angleCw,
            tailX,
            tailSwing,
            tailProgress);

        // bottom-left
        mFishVertexBuffer.emplace_back(
            position,
            vec2f(-offsetX, -offsetY),
            frame.TextureCoordinatesBottomLeft,
            frame.TextureCoordinatesTopRight,
            vec2f(frame.TextureCoordinatesBottomLeft.x, frame.TextureCoordinatesBottomLeft.y),
            angleCw,
            tailX,
            tailSwing,
            tailProgress);

        // top-right
        mFishVertexBuffer.emplace_back(
            position,
            vec2f(offsetX, offsetY),
            frame.TextureCoordinatesBottomLeft,
            frame.TextureCoordinatesTopRight,
            vec2f(frame.TextureCoordinatesTopRight.x, frame.TextureCoordinatesTopRight.y),
            angleCw,
            tailX,
            tailSwing,
            tailProgress);

        // bottom-right
        mFishVertexBuffer.emplace_back(
            position,
            vec2f(offsetX, -offsetY),
            frame.TextureCoordinatesBottomLeft,
            frame.TextureCoordinatesTopRight,
            vec2f(frame.TextureCoordinatesTopRight.x, frame.TextureCoordinatesBottomLeft.y),
            angleCw,
            tailX,
            tailSwing,
            tailProgress);
    }

    void UploadFishesEnd();

    inline void UploadAMBombPreImplosion(
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

    inline void UploadCrossOfLight(
        vec2f const & centerPosition,
        float progress,
        RenderParameters const & renderParameters)
    {
        auto const & viewModel = renderParameters.View;

        // Triangle 1

        mCrossOfLightVertexBuffer.emplace_back(
            vec2f(viewModel.GetVisibleWorld().TopLeft.x, viewModel.GetVisibleWorld().BottomRight.y), // left, bottom
            centerPosition,
            progress);

        mCrossOfLightVertexBuffer.emplace_back(
            viewModel.GetVisibleWorld().TopLeft, // left, top
            centerPosition,
            progress);

        mCrossOfLightVertexBuffer.emplace_back(
            viewModel.GetVisibleWorld().BottomRight, // right, bottom
            centerPosition,
            progress);

        // Triangle 2

        mCrossOfLightVertexBuffer.emplace_back(
            viewModel.GetVisibleWorld().TopLeft, // left, top
            centerPosition,
            progress);

        mCrossOfLightVertexBuffer.emplace_back(
            viewModel.GetVisibleWorld().BottomRight, // right, bottom
            centerPosition,
            progress);

        mCrossOfLightVertexBuffer.emplace_back(
            vec2f(viewModel.GetVisibleWorld().BottomRight.x, viewModel.GetVisibleWorld().TopLeft.y),  // right, top
            centerPosition,
            progress);
    }

    void UploadAABBsStart(size_t aabbCount);

    inline void UploadAABB(
        Geometry::AABB const & aabb,
        vec4f const & color)
    {
        // TopLeft -> TopRight
        mAABBVertexBuffer.emplace_back(
            color,
            aabb.BottomLeft.x, aabb.TopRight.y);
        mAABBVertexBuffer.emplace_back(
            color,
            aabb.TopRight.x, aabb.TopRight.y);

        // TopRight -> BottomRight
        mAABBVertexBuffer.emplace_back(
            color,
            aabb.TopRight.x, aabb.TopRight.y);
        mAABBVertexBuffer.emplace_back(
            color,
            aabb.TopRight.x, aabb.BottomLeft.y);

        // BottomRight -> BottomLeft
        mAABBVertexBuffer.emplace_back(
            color,
            aabb.TopRight.x, aabb.BottomLeft.y);
        mAABBVertexBuffer.emplace_back(
            color,
            aabb.BottomLeft.x, aabb.BottomLeft.y);

        // BottomLeft -> TopLeft
        mAABBVertexBuffer.emplace_back(
            color,
            aabb.BottomLeft.x, aabb.BottomLeft.y);
        mAABBVertexBuffer.emplace_back(
            color,
            aabb.BottomLeft.x, aabb.TopRight.y);
    }

    void UploadAABBsEnd();

    void UploadEnd();

    void ProcessParameterChanges(RenderParameters const & renderParameters);

    void RenderDrawSky(RenderParameters const & renderParameters);

    void RenderPrepareStars(RenderParameters const & renderParameters);
    void RenderDrawStars(RenderParameters const & renderParameters);

    void RenderPrepareLightnings(RenderParameters const & renderParameters);
    void RenderPrepareClouds(RenderParameters const & renderParameters);
    void RenderDrawCloudsAndBackgroundLightnings(RenderParameters const & renderParameters);

    void RenderPrepareOcean(RenderParameters const & renderParameters);
    void RenderDrawOcean(bool opaquely, RenderParameters const & renderParameters);

    void RenderPrepareOceanFloor(RenderParameters const & renderParameters);
    void RenderDrawOceanFloor(RenderParameters const & renderParameters);

    void RenderPrepareFishes(RenderParameters const & renderParameters);
    void RenderDrawFishes(RenderParameters const & renderParameters);

    void RenderPrepareAMBombPreImplosions(RenderParameters const & renderParameters);
    void RenderDrawAMBombPreImplosions(RenderParameters const & renderParameters);

    void RenderPrepareCrossesOfLight(RenderParameters const & renderParameters);
    void RenderDrawCrossesOfLight(RenderParameters const & renderParameters);

    void RenderDrawForegroundLightnings(RenderParameters const & renderParameters);

    void RenderPrepareRain(RenderParameters const & renderParameters);
    void RenderDrawRain(RenderParameters const & renderParameters);

    void RenderPrepareAABBs(RenderParameters const & renderParameters);
    void RenderDrawAABBs(RenderParameters const & renderParameters);

    void RenderDrawWorldBorder(RenderParameters const & renderParameters);

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

    void ApplyViewModelChanges(RenderParameters const & renderParameters);
    void ApplyCanvasSizeChanges(RenderParameters const & renderParameters);
    void ApplyEffectiveAmbientLightIntensityChanges(RenderParameters const & renderParameters);
    void ApplySkyChanges(RenderParameters const & renderParameters);
    void ApplyOceanDarkeningRateChanges(RenderParameters const & renderParameters);
    void ApplyOceanRenderParametersChanges(RenderParameters const & renderParameters);
    void ApplyOceanTextureIndexChanges(RenderParameters const & renderParameters);
    void ApplyLandRenderParametersChanges(RenderParameters const & renderParameters);
    void ApplyLandTextureIndexChanges(RenderParameters const & renderParameters);
    void ApplyLandNoiseChanges(RenderParameters const & renderParameters);

    void RecalculateClearCanvasColor(RenderParameters const & renderParameters);
    void RecalculateWorldBorder(RenderParameters const & renderParameters);
    static std::unique_ptr<Buffer2D<float, struct IntegralTag>> MakeLandNoise(RenderParameters const & renderParameters);

private:

    GlobalRenderContext const & mGlobalRenderContext;

    ShaderManager<ShaderManagerTraits> & mShaderManager;

private:

    //
    // Types
    //

#pragma pack(push, 1)

    struct SkyVertex
    {
        float ndcX;
        float ndcY;

        SkyVertex(
            float _ndcX,
            float _ndcY)
            : ndcX(_ndcX)
            , ndcY(_ndcY)
        {}
    };

    struct StarVertex
    {
        vec2f positionNdc;
        float brightness;

        StarVertex(
            vec2f const & _positionNdc,
            float _brightness)
            : positionNdc(_positionNdc)
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
        vec2f ndcPosition;
        vec2f atlasTexturePos;
        vec2f atlasTextureCenter;
        vec2f virtualTexturePos;
        float darkness;
        float totalDistanceTraveled;

        CloudVertex(
            vec2f _ndcPosition,
            vec2f _atlasTexturePos,
            vec2f _atlasTextureCenter,
            vec2f _virtualTexturePos,
            float _darkness,
            float _totalDistanceTraveled)
            : ndcPosition(_ndcPosition)
            , atlasTexturePos(_atlasTexturePos)
            , atlasTextureCenter(_atlasTextureCenter)
            , virtualTexturePos(_virtualTexturePos)
            , darkness(_darkness)
            , totalDistanceTraveled(_totalDistanceTraveled)
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

    struct OceanBasicSegment
    {
        float x1;
        float y1;
        float yWater1;

        float x2;
        float y2;
        float yWater2;
    };

    struct OceanDetailedSegment
    {
        float x1;
        float y1;
        float yTexture1;
        float yBack1;
        float yMid1;
        float yFront1;
        float d2YFront1;

        float x2;
        float y2;
        float yTexture2;
        float yBack2;
        float yMid2;
        float yFront2;
        float d2YFront2;
    };

    struct FishVertex
    {
        vec2f centerPosition;
        vec2f vertexOffset;
        vec2f textureSpaceLefBottom;
        vec2f textureSpaceRightTop;
        vec2f textureCoordinate;
        float angleCw;
        float tailX;
        float tailSwing;
        float tailProgress;

        FishVertex(
            vec2f _centerPosition,
            vec2f _vertexOffset,
            vec2f _textureSpaceLefBottom,
            vec2f _textureSpaceRightTop,
            vec2f _textureCoordinate,
            float _angleCw,
            float _tailX,
            float _tailSwing,
            float _tailProgress)
            : centerPosition(_centerPosition)
            , vertexOffset(_vertexOffset)
            , textureSpaceLefBottom(_textureSpaceLefBottom)
            , textureSpaceRightTop(_textureSpaceRightTop)
            , textureCoordinate(_textureCoordinate)
            , angleCw(_angleCw)
            , tailX(_tailX)
            , tailSwing(_tailSwing)
            , tailProgress(_tailProgress)
        {}
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

    struct AABBVertex
    {
        vec4f color;
        float x;
        float y;

        AABBVertex(
            vec4f const & _color,
            float _x,
            float _y)
            : color(_color)
            , x(_x)
            , y(_y)
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

    GameOpenGLVBO mSkyVBO;

    BoundedVector<StarVertex> mStarVertexBuffer;
    size_t mDirtyStarsCount;
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
    float mCloudNormalizedViewCamY;

    BoundedVector<LandSegment> mLandSegmentBuffer;
    GameOpenGLVBO mLandSegmentVBO;
    size_t mLandSegmentVBOAllocatedVertexSize;

    BoundedVector<OceanBasicSegment> mOceanBasicSegmentBuffer;
    GameOpenGLVBO mOceanBasicSegmentVBO;
    size_t mOceanBasicSegmentVBOAllocatedVertexSize;

    BoundedVector<OceanDetailedSegment> mOceanDetailedSegmentBuffer;
    GameOpenGLVBO mOceanDetailedSegmentVBO;
    size_t mOceanDetailedSegmentVBOAllocatedVertexSize;

    BoundedVector<FishVertex> mFishVertexBuffer;
    GameOpenGLVBO mFishVBO;
    size_t mFishVBOAllocatedVertexSize;

    std::vector<AMBombPreImplosionVertex> mAMBombPreImplosionVertexBuffer;
    GameOpenGLVBO mAMBombPreImplosionVBO;
    size_t mAMBombPreImplosionVBOAllocatedVertexSize;

    std::vector<CrossOfLightVertex> mCrossOfLightVertexBuffer;
    GameOpenGLVBO mCrossOfLightVBO;
    size_t mCrossOfLightVBOAllocatedVertexSize;

    BoundedVector<AABBVertex> mAABBVertexBuffer;
    GameOpenGLVBO mAABBVBO;
    size_t mAABBVBOAllocatedVertexSize;

    float mStormAmbientDarkening;

    GameOpenGLVBO mRainVBO;
    float mRainDensity;
    bool mIsRainDensityDirty;
    float mRainWindSpeedMagnitude;
    bool mIsRainWindSpeedMagnitudeDirty;

    std::vector<WorldBorderVertex> mWorldBorderVertexBuffer;
    GameOpenGLVBO mWorldBorderVBO;

    //
    // VAOs
    //

    GameOpenGLVAO mSkyVAO;
    GameOpenGLVAO mStarVAO;
    GameOpenGLVAO mLightningVAO;
    GameOpenGLVAO mCloudVAO;
    GameOpenGLVAO mLandVAO;
    GameOpenGLVAO mOceanBasicVAO;
    GameOpenGLVAO mOceanDetailedVAO;
    GameOpenGLVAO mFishVAO;
    GameOpenGLVAO mAMBombPreImplosionVAO;
    GameOpenGLVAO mCrossOfLightVAO;
    GameOpenGLVAO mAABBVAO;
    GameOpenGLVAO mRainVAO;
    GameOpenGLVAO mWorldBorderVAO;

    //
    // Textures
    //

    std::unique_ptr<TextureAtlasMetadata<CloudTextureGroups>> mCloudTextureAtlasMetadata;
    GameOpenGLTexture mCloudTextureAtlasOpenGLHandle;

    GameOpenGLTexture mCloudShadowsTextureOpenGLHandle;
    size_t mCloudShadowsTextureSize;
    bool mHasCloudShadowsTextureBeenAllocated;

    UploadedTextureManager<WorldTextureGroups> mUploadedWorldTextureManager;

    std::vector<TextureFrameSpecification<WorldTextureGroups>> mOceanTextureFrameSpecifications;
    GameOpenGLTexture mOceanTextureOpenGLHandle;

    std::vector<TextureFrameSpecification<WorldTextureGroups>> mLandTextureFrameSpecifications;
    GameOpenGLTexture mLandTextureOpenGLHandle;
    GameOpenGLTexture mLandNoiseTextureOpenGLHandle;
    std::unique_ptr<Buffer2D<float, struct IntegralTag>> mLandNoiseToUpload;

    std::unique_ptr<TextureAtlasMetadata<FishTextureGroups>> mFishTextureAtlasMetadata;
    GameOpenGLTexture mFishTextureAtlasOpenGLHandle;

    TextureAtlasMetadata<GenericLinearTextureGroups> const & mGenericLinearTextureAtlasMetadata;

    // Thumbnails
    std::vector<std::pair<std::string, RgbaImageData>> mOceanAvailableThumbnails;
    std::vector<std::pair<std::string, RgbaImageData>> mLandAvailableThumbnails;

    //
    // Parameters (storage here)
    //

    float mSunRaysInclination;
    bool mIsSunRaysInclinationDirty;
};

}