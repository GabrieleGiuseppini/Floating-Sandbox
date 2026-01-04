/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-07-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameShaderSets.h"
#include "GameTextureDatabases.h"
#include "GlobalRenderContext.h"
#include "RenderParameters.h"
#include "ViewModel.h"

#include <OpenGLCore/GameOpenGL.h>
#include <OpenGLCore/ShaderManager.h>

#include <Core/AABB.h>
#include <Core/BoundedVector.h>
#include <Core/Buffer2D.h>
#include <Core/Colors.h>
#include <Core/GameTypes.h>
#include <Core/IAssetManager.h>
#include <Core/ImageData.h>
#include <Core/RunningAverage.h>
#include <Core/TextureAtlas.h>
#include <Core/Vectors.h>

#include <array>
#include <cassert>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

class WorldRenderContext
{
public:

    WorldRenderContext(
        IAssetManager const & assetManager,
        ShaderManager<GameShaderSets::ShaderSet> & shaderManager,
        GlobalRenderContext & globalRenderContext);

    ~WorldRenderContext() = default;

    void InitializeCloudTextures();

    void InitializeWorldTextures();

    void InitializeFishTextures();

    void OnReset(RenderParameters const & renderParameters);

    inline std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureOceanAvailableThumbnails() const
    {
        return mOceanAvailableThumbnails;
    }

    inline std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureLandAvailableThumbnails() const
    {
        return mLandAvailableThumbnails;
    }

    size_t GetUnderwaterPlantsSpeciesCount() const
    {
        return mGlobalRenderContext.GetGenericLinearTextureAtlasMetadata().GetFrameCount(GameTextureDatabases::GenericLinearTextureDatabase::TextureGroupsType::UnderwaterPlant);
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

    inline void UploadWind(vec2f const & instantSpeed, float basisWindMagnitude)
    {
        float const smoothedWindMagnitude = mWindSpeedMagnitudeRunningAverage.Update(instantSpeed.x);
        if (smoothedWindMagnitude != mCurrentSmoothedWindSpeedMagnitude) // Damp frequency of calls
        {
            mCurrentSmoothedWindSpeedMagnitude = smoothedWindMagnitude;
            mIsCurrentSmoothedWindSpeedMagnitudeDirty = true;
        }

        float const windDirection = (basisWindMagnitude >= 0.0f) ? 1.0f : -1.0f;
        if (windDirection != mCurrentWindDirection) // Damp frequency of calls
        {
            mCurrentWindDirection = windDirection;
            mIsCurrentWindDirectionDirty = true;
        }
    }

    inline void UploadUnderwaterCurrent(float spaceVelocity, float timeVelocity)
    {
        if (spaceVelocity != mCurrentUnderwaterCurrentSpaceVelocity)
        {
            mCurrentUnderwaterCurrentSpaceVelocity = spaceVelocity;
            mIsCurrentUnderwaterCurrentSpaceVelocityDirty = true;
        }

        if (timeVelocity != mCurrentUnderwaterCurrentTimeVelocity)
        {
            mCurrentUnderwaterCurrentTimeVelocity = timeVelocity;
            mIsCurrentUnderwaterCurrentTimeVelocityDirty = true;
        }
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
        float volumetricGrowthProgress, // 0.0 -> +INF
        RenderParameters const & renderParameters)
    {
        //
        // We use Normalized Device Coordinates here
        //

        // Apply perspective transformation
        vec2f const ndc = renderParameters.View.ApplyCloudPerspectiveTransformation(virtualX, virtualY, virtualZ);

        //
        // Populate quad in buffer
        //

        size_t const cloudTextureIndex = static_cast<size_t>(cloudId) % mCloudTextureAtlasMetadata->GetAllFramesMetadata().size();
        auto const & cloudAtlasFrameMetadata = mCloudTextureAtlasMetadata->GetFrameMetadata(
            GameTextureDatabases::CloudTextureGroups::Cloud,
            static_cast<TextureFrameIndex>(cloudTextureIndex));

        float const aspectRatio = renderParameters.View.GetAspectRatio();

        float const ndcWidth = scale * cloudAtlasFrameMetadata.FrameMetadata.WorldWidth;
        float const ndcHeight = scale * cloudAtlasFrameMetadata.FrameMetadata.WorldHeight * aspectRatio;
        float const leftX = ndc.x - ndcWidth / 2.0f;
        float const rightX = leftX + ndcWidth;

        // Cut short here if invisible
        if (leftX <= 1.0f && rightX >= -1.0f)
        {
            float const bottomY = ndc.y - ndcHeight / 2.0f;
            float const topY = bottomY + ndcHeight;

            float const textureWidthAdjust = std::max(cloudAtlasFrameMetadata.TextureSpaceWidth, cloudAtlasFrameMetadata.TextureSpaceHeight);

            // top-left
            mCloudVertexBuffer.emplace_back(
                vec2f(leftX, topY),
                vec2f(cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.x, cloudAtlasFrameMetadata.TextureCoordinatesTopRight.y),
                vec2f(-1.0f, 1.0f),
                darkening,
                volumetricGrowthProgress,
                textureWidthAdjust);

            // bottom-left
            mCloudVertexBuffer.emplace_back(
                vec2f(leftX, bottomY),
                cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft,
                vec2f(-1.0f, -1.0f),
                darkening,
                volumetricGrowthProgress,
                textureWidthAdjust);

            // top-right
            mCloudVertexBuffer.emplace_back(
                vec2f(rightX, topY),
                cloudAtlasFrameMetadata.TextureCoordinatesTopRight,
                vec2f(1.0f, 1.0f),
                darkening,
                volumetricGrowthProgress,
                textureWidthAdjust);

            // bottom-right
            mCloudVertexBuffer.emplace_back(
                vec2f(rightX, bottomY),
                vec2f(cloudAtlasFrameMetadata.TextureCoordinatesTopRight.x, cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.y),
                vec2f(1.0f, -1.0f),
                darkening,
                volumetricGrowthProgress,
                textureWidthAdjust);
        }
    }

    void UploadCloudsEnd();

    void UploadCloudShadows(
        float const * shadowBuffer,
        size_t shadowSampleCount);

    void UploadLandStart(size_t slices);

    inline void UploadLand(
        float x,
        float yLand,
        RenderParameters const & renderParameters)
    {
        float const yVisibleWorldBottom = renderParameters.View.GetVisibleWorldWithPixelOffset().BottomRight.y;

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
        float const yVisibleWorldBottom = renderParameters.View.GetVisibleWorldWithPixelOffset().BottomRight.y;

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
        float const yTop = std::max(yBack, std::max(yMid, yFront)) + mOceanDetailedUpperBandMagicOffset;
        float const yBottom = std::min(yBack, std::min(yMid, yFront)) - mOceanDetailedUpperBandMagicOffset;
        float const yVisibleWorldBottom = renderParameters.View.GetVisibleWorldWithPixelOffset().BottomRight.y;

        //
        // Store ocean element
        //

        OceanDetailedSegment & oceanSegment = mOceanDetailedSegmentBuffer.emplace_back();

        oceanSegment.x_upper_1 = x;
        oceanSegment.y_upper_1 = yTop;
        oceanSegment.yBack_upper_1 = yBack;
        oceanSegment.yMid_upper_1 = yMid;
        oceanSegment.yFront_upper_1 = yFront;
        oceanSegment.d2yFront_upper_1 = d2YFront;

        oceanSegment.x_upper_2 = x;
        oceanSegment.y_upper_2 = std::max(yVisibleWorldBottom, yBottom);
        oceanSegment.yBack_upper_2 = yBack;
        oceanSegment.yMid_upper_2 = yMid;
        oceanSegment.yFront_upper_2 = yFront;
        oceanSegment.d2yFront_upper_2 = d2YFront;

        oceanSegment.x_lower_1 = x;
        oceanSegment.y_lower_1 = oceanSegment.y_upper_2;

        oceanSegment.x_lower_2 = x;
        oceanSegment.y_lower_2 = yVisibleWorldBottom;

        switch (renderParameters.OceanRenderMode)
        {
            case OceanRenderModeType::Texture:
            {
                // We squash the top a little towards the rest position, to give a slight ondulation
                oceanSegment.yTexture_upper_1 = yTop * 0.75f; // Topmost pixel
                oceanSegment.yTexture_lower_2 = yVisibleWorldBottom; // Lowest pixel

                // The middle pixel's texture y is proportional to the upper (or lower) band's width
                // TODO: see if may be simplified
                oceanSegment.yTexture_lower_1 =
                    oceanSegment.yTexture_lower_2
                    + (oceanSegment.yTexture_upper_1 - oceanSegment.yTexture_lower_2)
                        * (oceanSegment.y_lower_1 - oceanSegment.y_lower_2) / (oceanSegment.y_upper_1 - oceanSegment.y_lower_2);
                oceanSegment.yTexture_upper_2 = oceanSegment.yTexture_lower_1;

                break;
            }

            case OceanRenderModeType::Depth:
            {
                // Nop, but be nice
                oceanSegment.yTexture_upper_1 = 0.0f;
                oceanSegment.yTexture_upper_2 = 0.0f;
                oceanSegment.yTexture_lower_1 = 0.0f;
                oceanSegment.yTexture_lower_2 = 0.0f;

                break;
            }

            case OceanRenderModeType::Flat:
            {
                // Nop, but be nice
                oceanSegment.yTexture_upper_1 = 0.0f;
                oceanSegment.yTexture_upper_2 = 0.0f;
                oceanSegment.yTexture_lower_1 = 0.0f;
                oceanSegment.yTexture_lower_2 = 0.0f;

                break;
            }
        }
    }

    void UploadOceanDetailedEnd();

    void UploadFishesStart(size_t fishCount);

    inline void UploadFish(
        TextureFrameId<GameTextureDatabases::FishTextureGroups> const & textureFrameId,
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

    void UploadUnderwaterPlantStaticVertexAttributesStart(size_t underwaterPlantCount);

    inline void UploadUnderwaterPlantStaticVertexAttributes(
        vec2f const & centerBottomPosition,
        size_t speciesIndex,
        float scale,
        float personalitySeed,
        bool isSpecular)
    {
        auto const & frame = mGlobalRenderContext.GetGenericLinearTextureAtlasMetadata().GetFrameMetadata(
            TextureFrameId(
                GameTextureDatabases::GenericLinearTextureDatabase::TextureGroupsType::UnderwaterPlant,
                static_cast<TextureFrameIndex>(speciesIndex)));

        float const worldWidth = frame.FrameMetadata.WorldWidth * scale;
        float const worldHeight = frame.FrameMetadata.WorldHeight * scale;

        // Calculate bounding box, as box contained in circle whose radius is the diagonal of the frame
        float const diagonalLength = std::sqrtf((worldWidth / 2.0f) * (worldWidth / 2.0f) + worldHeight * worldHeight);
        FloatSize const boundingBoxSize = FloatSize(diagonalLength * 2.0f, diagonalLength);
        float const leftX = centerBottomPosition.x - boundingBoxSize.width / 2.0f;
        float const rightX = centerBottomPosition.x + boundingBoxSize.width / 2.0f;

        // 1. Calculate plant space coordinates: all we need here are coordinates
        // that respect the euclidean space, so that rotations work fine.
        // We choose arbitrarily height=1.0 (at top of max quad), and adjust
        // width according to max quad width - that is, 2.0
        float const plantSpaceWidth = 2.0f;
        float const plantSpaceHeight = 1.0f;

        // 2. Calculate multiplicative factor for plan space coordinates, which
        // transform their portion corresponding to the texture image
        // into normalized texture coordinates (0..1, 0..1); parts outside of the 0..1
        // rect (which are outside of the texture image) will be clamped by the shader
        vec2f const textureInSpaceCoords = vec2f(
            boundingBoxSize.width / worldWidth / plantSpaceWidth,
            boundingBoxSize.height / worldHeight / plantSpaceHeight);

        // Tile index: take into account that we add h-flipped versions
        float const atlasTileIndex = static_cast<float>(speciesIndex * 2) + (isSpecular ? 1 : 0);

        // Top-left
        mUnderwaterPlantStaticVertexBuffer.emplace_back(
            vec2f(leftX, centerBottomPosition.y + boundingBoxSize.height),
            vec2f(-plantSpaceWidth / 2.0f, plantSpaceHeight),
            textureInSpaceCoords,
            personalitySeed,
            atlasTileIndex);

        // Bottom-left
        mUnderwaterPlantStaticVertexBuffer.emplace_back(
            vec2f(leftX, centerBottomPosition.y),
            vec2f(-plantSpaceWidth / 2.0f, 0.0f),
            textureInSpaceCoords,
            personalitySeed,
            atlasTileIndex);

        // Top-right
        mUnderwaterPlantStaticVertexBuffer.emplace_back(
            vec2f(rightX, centerBottomPosition.y + boundingBoxSize.height),
            vec2f(plantSpaceWidth / 2.0f, plantSpaceHeight),
            textureInSpaceCoords,
            personalitySeed,
            atlasTileIndex);

        // Bottom-right
        mUnderwaterPlantStaticVertexBuffer.emplace_back(
            vec2f(rightX, centerBottomPosition.y),
            vec2f(plantSpaceWidth / 2.0f, 0.0f),
            textureInSpaceCoords,
            personalitySeed,
            atlasTileIndex);
    }

    void UploadUnderwaterPlantStaticVertexAttributesEnd();

    inline void UploadUnderwaterPlantOceanDepths(std::vector<float> const & oceanDepths)
    {
        assert(oceanDepths.size() * 4 == mUnderwaterPlantDynamicVertexBuffer.size());

        //
        // Super-vectorized (4*4) on VS
        //

        static_assert(sizeof(UnderwaterPlantDynamicVertex) == sizeof(float));

        float * restrict tgt = reinterpret_cast<float *>(mUnderwaterPlantDynamicVertexBuffer.data());
        size_t const nSrc = oceanDepths.size();
        for (size_t i = 0; i < nSrc; ++i)
        {
            float const d = oceanDepths[i];
            *(tgt++) = d;
            *(tgt++) = d;
            *(tgt++) = d;
            *(tgt++) = d;
        }
    }

    inline void UploadUnderwaterPlantRotationAngle(float rotationAngle)
    {
        if (rotationAngle != mCurrentUnderwaterPlantsRotationAngle)
        {
            mCurrentUnderwaterPlantsRotationAngle = rotationAngle;
            mIsCurrentUnderwaterPlantsRotationAngleDirty = true;
        }
    }

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

    void UploadAntiGravityFieldsStart();

    void UploadAntiGravityField(
        vec2f const & startPos,
        vec2f const & endPos)
    {
        // TODO: where other constants live
        float constexpr AntiGravityFieldThicknessWorld = 10.0f;

        vec2f const centerPos = (startPos + endPos) / 2.0f;
        vec2f const displacementVector = (endPos - startPos);
        float const displacementLength = displacementVector.length();
        vec2f const direction = displacementVector.normalise(displacementLength);
        vec2f const vDisplacementHalf = direction.to_perpendicular() * AntiGravityFieldThicknessWorld / 2.0f;
        float const width = std::max(displacementLength, AntiGravityFieldThicknessWorld);

        // Along central H axis
        vec2f const p0 = centerPos - direction * width / 2.0f;
        vec2f const p1 = p0 + direction * AntiGravityFieldThicknessWorld / 2.0f;
        vec2f const p3 = centerPos + direction * width / 2.0f;
        vec2f const p2 = p3 - direction * AntiGravityFieldThicknessWorld / 2.0f;

        // Left quad
        // Left-top
        mAntiGravityFieldVertexBuffer.emplace_back(
            p0 - vDisplacementHalf,
            vec2f(0.0f, 1.0f),
            0.0f);
        // Left-bottom
        mAntiGravityFieldVertexBuffer.emplace_back(
            p0 + vDisplacementHalf,
            vec2f(0.0f, 0.0f),
            0.0f);
        // Right-top
        mAntiGravityFieldVertexBuffer.emplace_back(
            p1 - vDisplacementHalf,
            vec2f(0.5f, 1.0f),
            AntiGravityFieldThicknessWorld / 2.0f);
        // Right-bottom
        mAntiGravityFieldVertexBuffer.emplace_back(
            p1 + vDisplacementHalf,
            vec2f(0.5f, 0.0f),
            AntiGravityFieldThicknessWorld / 2.0f);

        // Mid quad
        // Left-top
        mAntiGravityFieldVertexBuffer.emplace_back(
            p1 - vDisplacementHalf,
            vec2f(0.5f, 1.0f),
            AntiGravityFieldThicknessWorld / 2.0f);
        // Left-bottom
        mAntiGravityFieldVertexBuffer.emplace_back(
            p1 + vDisplacementHalf,
            vec2f(0.5f, 0.0f),
            AntiGravityFieldThicknessWorld / 2.0f);
        // Right-top
        mAntiGravityFieldVertexBuffer.emplace_back(
            p2 - vDisplacementHalf,
            vec2f(0.5f, 1.0f),
            width - AntiGravityFieldThicknessWorld / 2.0f);
        // Right-bottom
        mAntiGravityFieldVertexBuffer.emplace_back(
            p2 + vDisplacementHalf,
            vec2f(0.5f, 0.0f),
            width - AntiGravityFieldThicknessWorld / 2.0f);

        // Right quad
        // Left-top
        mAntiGravityFieldVertexBuffer.emplace_back(
            p2 - vDisplacementHalf,
            vec2f(0.5f, 1.0f),
            width - AntiGravityFieldThicknessWorld / 2.0f);
        // Left-bottom
        mAntiGravityFieldVertexBuffer.emplace_back(
            p2 + vDisplacementHalf,
            vec2f(0.5f, 0.0f),
            width - AntiGravityFieldThicknessWorld / 2.0f);
        // Right-top
        mAntiGravityFieldVertexBuffer.emplace_back(
            p3 - vDisplacementHalf,
            vec2f(1.0f, 1.0f),
            width);
        // Right-bottom
        mAntiGravityFieldVertexBuffer.emplace_back(
            p3 + vDisplacementHalf,
            vec2f(1.0f, 0.0f),
            width);
    }

    void UploadAntiGravityFieldsEnd();

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

    void RenderPrepareUnderwaterPlants(float currentSimulationTime, RenderParameters const & renderParameters);
    void RenderDrawUnderwaterPlants(RenderParameters const & renderParameters);

    void RenderPrepareAntiGravityFields(float currentSimulationTime, RenderParameters const & renderParameters);
    void RenderDrawAntiGravityFields(RenderParameters const & renderParameters);

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

    void RenderPrepareEnd();

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
    void ApplyOceanDepthDarkeningRateChanges(RenderParameters const & renderParameters);
    void ApplyOceanRenderParametersChanges(RenderParameters const & renderParameters);
    void ApplyOceanTextureIndexChanges(RenderParameters const & renderParameters);
    void ApplyLandRenderParametersChanges(RenderParameters const & renderParameters);
    void ApplyLandTextureIndexChanges(RenderParameters const & renderParameters);

    void RecalculateClearCanvasColor(RenderParameters const & renderParameters);
    void RecalculateWorldBorder(RenderParameters const & renderParameters);

    RgbaImageData InternalMakeThumbnail(
        RgbaImageData const & imageData,
        float worldWidth,
        float worldHeight);

private:

    IAssetManager const & mAssetManager;
    ShaderManager<GameShaderSets::ShaderSet> & mShaderManager;
    GlobalRenderContext & mGlobalRenderContext;

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
        vec2f textureCoords;
        vec2f virtualTextureCoords;
        float darkness;
        float volumetricGrowthProgress;
        float textureWidthAdjust;

        CloudVertex(
            vec2f _ndcPosition,
            vec2f _textureCoords,
            vec2f _virtualTextureCoords,
            float _darkness,
            float _volumetricGrowthProgress,
            float _textureWidthAdjust)
            : ndcPosition(_ndcPosition)
            , textureCoords(_textureCoords)
            , virtualTextureCoords(_virtualTextureCoords)
            , darkness(_darkness)
            , volumetricGrowthProgress(_volumetricGrowthProgress)
            , textureWidthAdjust(_textureWidthAdjust)
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
        float x_upper_1;
        float y_upper_1;
        float yTexture_upper_1;
        float yBack_upper_1;
        float yMid_upper_1;
        float yFront_upper_1;
        float d2yFront_upper_1;

        float x_lower_1;
        float y_lower_1;
        float yTexture_lower_1;

        float x_upper_2;
        float y_upper_2;
        float yTexture_upper_2;
        float yBack_upper_2;
        float yMid_upper_2;
        float yFront_upper_2;
        float d2yFront_upper_2;

        float x_lower_2;
        float y_lower_2;
        float yTexture_lower_2;
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

    struct UnderwaterPlantStaticVertex
    {
        vec2f position;
        vec2f plantSpaceCoords;
        vec2f textureInSpaceCoords;
        float personalitySeed;
        float atlasTileIndex;

        UnderwaterPlantStaticVertex(
            vec2f _position,
            vec2f _plantSpaceCoords,
            vec2f _textureInSpaceCoords,
            float _personalitySeed,
            float _atlasTileIndex)
            : position(_position)
            , plantSpaceCoords(_plantSpaceCoords)
            , textureInSpaceCoords(_textureInSpaceCoords)
            , personalitySeed(_personalitySeed)
            , atlasTileIndex(_atlasTileIndex)
        {
        }
    };

    struct UnderwaterPlantDynamicVertex
    {
        float oceanHeight; // Unadulterated - i.e. positive up, 0 at 0

        UnderwaterPlantDynamicVertex(
            float _oceanHeight)
            : oceanHeight(_oceanHeight)
        {
        }
    };

    struct AntiGravityFieldVertex
    {
        vec2f position;
        vec2f fieldSpaceCoords;
        float worldXExtent;

        AntiGravityFieldVertex(
            vec2f _position,
            vec2f _fieldSpaceCoords,
            float _worldXExtent)
            : position(_position)
            , fieldSpaceCoords(_fieldSpaceCoords)
            , worldXExtent(_worldXExtent)
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

    BoundedVector<LandSegment> mLandSegmentBuffer;
    GameOpenGLVBO mLandSegmentVBO;
    size_t mLandSegmentVBOAllocatedVertexSize;

    BoundedVector<OceanBasicSegment> mOceanBasicSegmentBuffer;
    GameOpenGLVBO mOceanBasicSegmentVBO;
    size_t mOceanBasicSegmentVBOAllocatedVertexSize;

    BoundedVector<OceanDetailedSegment> mOceanDetailedSegmentBuffer;
    GameOpenGLVBO mOceanDetailedSegmentVBO;
    size_t mOceanDetailedSegmentVBOAllocatedVertexSize;
    float mOceanDetailedUpperBandMagicOffset; // Magic offset to allow shader to anti-alias close to the boundary

    BoundedVector<FishVertex> mFishVertexBuffer;
    GameOpenGLVBO mFishVBO;
    size_t mFishVBOAllocatedVertexSize;

    BoundedVector<UnderwaterPlantStaticVertex> mUnderwaterPlantStaticVertexBuffer;
    GameOpenGLVBO mUnderwaterPlantStaticVBO;
    size_t mUnderwaterPlantStaticVBOAllocatedVertexSize;
    bool mIsUnderwaterPlantStaticVertexBufferDirty;
    BoundedVector<UnderwaterPlantDynamicVertex> mUnderwaterPlantDynamicVertexBuffer;
    GameOpenGLVBO mUnderwaterPlantDynamicVBO;
    size_t mUnderwaterPlantDynamicVBOAllocatedVertexSize;

    std::vector<AntiGravityFieldVertex> mAntiGravityFieldVertexBuffer;
    GameOpenGLVBO mAntiGravityFieldVBO;
    bool mIsAntiGravityFieldVertexBufferDirty;

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
    GameOpenGLVAO mUnderwaterPlantVAO;
    GameOpenGLVAO mAntiGravityFieldVAO;
    GameOpenGLVAO mAMBombPreImplosionVAO;
    GameOpenGLVAO mCrossOfLightVAO;
    GameOpenGLVAO mAABBVAO;
    GameOpenGLVAO mRainVAO;
    GameOpenGLVAO mWorldBorderVAO;

    //
    // Textures
    //

    std::unique_ptr<TextureAtlasMetadata<GameTextureDatabases::CloudTextureDatabase>> mCloudTextureAtlasMetadata;
    GameOpenGLTexture mCloudTextureAtlasOpenGLHandle;

    GameOpenGLTexture mCloudShadowsTextureOpenGLHandle;
    bool mHasCloudShadowsTextureBeenAllocated;

    std::vector<TextureFrameSpecification<GameTextureDatabases::WorldTextureDatabase>> mOceanTextureFrameSpecifications;
    GameOpenGLTexture mOceanTextureOpenGLHandle;
    size_t mCurrentlyLoadedOceanTextureIndex;

    std::vector<TextureFrameSpecification<GameTextureDatabases::WorldTextureDatabase>> mLandTextureFrameSpecifications;
    GameOpenGLTexture mLandTextureOpenGLHandle;
    size_t mCurrentlyLoadedLandTextureIndex;

    std::unique_ptr<TextureAtlasMetadata<GameTextureDatabases::FishTextureDatabase>> mFishTextureAtlasMetadata;
    GameOpenGLTexture mFishTextureAtlasOpenGLHandle;

    TextureAtlasMetadata<GameTextureDatabases::GenericLinearTextureDatabase> const & mGenericLinearTextureAtlasMetadata;

    // Thumbnails
    std::vector<std::pair<std::string, RgbaImageData>> mOceanAvailableThumbnails;
    std::vector<std::pair<std::string, RgbaImageData>> mLandAvailableThumbnails;

    //
    // External scalars
    //

    // Wind
    RunningAverage<32> mWindSpeedMagnitudeRunningAverage;
    float mCurrentSmoothedWindSpeedMagnitude;
    bool mIsCurrentSmoothedWindSpeedMagnitudeDirty;
    float mCurrentWindDirection;
    bool mIsCurrentWindDirectionDirty;

    // Underwater currents
    float mCurrentUnderwaterCurrentSpaceVelocity;
    bool mIsCurrentUnderwaterCurrentSpaceVelocityDirty;
    float mCurrentUnderwaterCurrentTimeVelocity;
    bool mIsCurrentUnderwaterCurrentTimeVelocityDirty;

    // Underwater plants
    float mCurrentUnderwaterPlantsRotationAngle;
    bool mIsCurrentUnderwaterPlantsRotationAngleDirty;

    //
    // Parameters (storage here)
    //

    float mSunRaysInclination;
    bool mIsSunRaysInclinationDirty;
};
