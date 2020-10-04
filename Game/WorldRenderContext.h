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

#include <GameCore/BoundedVector.h>
#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/ImageSize.h>
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
        float growthProgress,   // [0.0, +1.0]
        RenderParameters const & renderParameters)
    {
        //
        // We use Normalized Device Coordinates here
        //

        // Calculate NDC x: map input slice [-0.5, +0.5] into NDC [-1.0, +1.0]
        float const ndcX = virtualX * 2.0f;

        // Calculate NDC y: apply perspective transform
        float constexpr zMin = GameParameters::HalfMaxWorldHeight;
        float constexpr zMax = 20.0f * zMin; // Magic number: at this (furthest) Z, clouds at virtualY=1.0 appear slightly above the horizon
        float const ndcY = (virtualY * GameParameters::HalfMaxWorldHeight - renderParameters.View.GetCameraWorldPosition().y) / (zMin + virtualZ * (zMax - zMin));

        //
        // Populate quad in buffer
        //

        float const aspectRatio = renderParameters.View.GetAspectRatio();

        size_t const cloudTextureIndex = static_cast<size_t>(cloudId) % mCloudTextureAtlasMetadata->GetFrameMetadata().size();

        auto const & cloudAtlasFrameMetadata = mCloudTextureAtlasMetadata->GetFrameMetadata(
            CloudTextureGroups::Cloud,
            static_cast<TextureFrameIndex>(cloudTextureIndex));

        float leftX = ndcX - scale * cloudAtlasFrameMetadata.FrameMetadata.AnchorCenterWorld.x;
        float rightX = ndcX + scale * (cloudAtlasFrameMetadata.FrameMetadata.WorldWidth - cloudAtlasFrameMetadata.FrameMetadata.AnchorCenterWorld.x);
        float topY = ndcY + scale * (cloudAtlasFrameMetadata.FrameMetadata.WorldHeight - cloudAtlasFrameMetadata.FrameMetadata.AnchorCenterWorld.y) * aspectRatio;
        float bottomY = ndcY - scale * cloudAtlasFrameMetadata.FrameMetadata.AnchorCenterWorld.y * aspectRatio;

        // TODOTEST
        /*
        vec2f const textureCoordinatesAlphaMaskBottomLeft = vec2f(
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter.x - growthProgress * cloudAtlasFrameMetadata.TextureSpaceWidth / 2.0f,
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter.y - growthProgress * cloudAtlasFrameMetadata.TextureSpaceHeight / 2.0f);
        vec2f const textureCoordinatesAlphaMaskTopRight = vec2f(
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter.x + growthProgress * cloudAtlasFrameMetadata.TextureSpaceWidth / 2.0f,
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter.y + growthProgress * cloudAtlasFrameMetadata.TextureSpaceHeight / 2.0f);
        */

        // TODO: make vectors args

        // top-left
        mCloudVertexBuffer.emplace_back(
            leftX,
            topY,
            cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.x,
            cloudAtlasFrameMetadata.TextureCoordinatesTopRight.y,
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter,
            darkening,
            growthProgress);

        // bottom-left
        mCloudVertexBuffer.emplace_back(
            leftX,
            bottomY,
            cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.x,
            cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.y,
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter,
            darkening,
            growthProgress);

        // top-right
        mCloudVertexBuffer.emplace_back(
            rightX,
            topY,
            cloudAtlasFrameMetadata.TextureCoordinatesTopRight.x,
            cloudAtlasFrameMetadata.TextureCoordinatesTopRight.y,
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter,
            darkening,
            growthProgress);

        // bottom-left
        mCloudVertexBuffer.emplace_back(
            leftX,
            bottomY,
            cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.x,
            cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.y,
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter,
            darkening,
            growthProgress);

        // top-right
        mCloudVertexBuffer.emplace_back(
            rightX,
            topY,
            cloudAtlasFrameMetadata.TextureCoordinatesTopRight.x,
            cloudAtlasFrameMetadata.TextureCoordinatesTopRight.y,
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter,
            darkening,
            growthProgress);

        // bottom-right
        mCloudVertexBuffer.emplace_back(
            rightX,
            bottomY,
            cloudAtlasFrameMetadata.TextureCoordinatesTopRight.x,
            cloudAtlasFrameMetadata.TextureCoordinatesBottomLeft.y,
            cloudAtlasFrameMetadata.TextureCoordinatesAnchorCenter,
            darkening,
            growthProgress);
    }

    void UploadCloudsEnd();

    void UploadLandStart(size_t slices);

    inline void UploadLand(
        float x,
        float yLand,
        RenderParameters const & renderParameters)
    {
        float const yVisibleWorldBottom = renderParameters.View.GetVisibleWorldBottomRight().y;

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
        float oceanDepth,
        RenderParameters const & renderParameters)
    {
        float const yVisibleWorldBottom = renderParameters.View.GetVisibleWorldBottomRight().y;

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

        switch (renderParameters.OceanRenderMode)
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
                    ? std::fabs(oceanSegmentY2 - oceanSegmentY1) / oceanDepth
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

    void UploadEnd();

    void ProcessParameterChanges(RenderParameters const & renderParameters);

    void RenderPrepareStars(RenderParameters const & renderParameters);
    void RenderDrawStars(RenderParameters const & renderParameters);

    void RenderPrepareLightnings(RenderParameters const & renderParameters);
    void RenderPrepareClouds(RenderParameters const & renderParameters);
    void RenderDrawCloudsAndBackgroundLightnings(RenderParameters const & renderParameters);

    void RenderPrepareOcean(RenderParameters const & renderParameters);
    void RenderDrawOcean(bool opaquely, RenderParameters const & renderParameters);

    void RenderPrepareOceanFloor(RenderParameters const & renderParameters);
    void RenderDrawOceanFloor(RenderParameters const & renderParameters);

    void RenderPrepareAMBombPreImplosions(RenderParameters const & renderParameters);
    void RenderDrawAMBombPreImplosions(RenderParameters const & renderParameters);

    void RenderPrepareCrossesOfLight(RenderParameters const & renderParameters);
    void RenderDrawCrossesOfLight(RenderParameters const & renderParameters);

    void RenderDrawForegroundLightnings(RenderParameters const & renderParameters);

    void RenderPrepareRain(RenderParameters const & renderParameters);
    void RenderDrawRain(RenderParameters const & renderParameters);

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
    void ApplyOceanDarkeningRateChanges(RenderParameters const & renderParameters);
    void ApplyOceanRenderParametersChanges(RenderParameters const & renderParameters);
    void ApplyOceanTextureIndexChanges(RenderParameters const & renderParameters);
    void ApplyLandRenderParametersChanges(RenderParameters const & renderParameters);
    void ApplyLandTextureIndexChanges(RenderParameters const & renderParameters);

    void RecalculateWorldBorder(RenderParameters const & renderParameters);

private:

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
        float textureX;
        float textureY;
        vec2f textureCenter;
        float darkness;
        float growthProgress;

        CloudVertex(
            float _ndcX,
            float _ndcY,
            float _textureX,
            float _textureY,
            vec2f _textureCenter,
            float _darkness,
            float _growthProgress)
            : ndcX(_ndcX)
            , ndcY(_ndcY)
            , textureX(_textureX)
            , textureY(_textureY)
            , textureCenter(_textureCenter)
            , darkness(_darkness)
            , growthProgress(_growthProgress)
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
    GameOpenGLVAO mRainVAO;
    GameOpenGLVAO mWorldBorderVAO;

    //
    // Textures
    //

    std::unique_ptr<TextureAtlasMetadata<CloudTextureGroups>> mCloudTextureAtlasMetadata;
    GameOpenGLTexture mCloudTextureAtlasOpenGLHandle;

    UploadedTextureManager<WorldTextureGroups> mUploadedWorldTextureManager;

    std::vector<TextureFrameSpecification<WorldTextureGroups>> mOceanTextureFrameSpecifications;
    GameOpenGLTexture mOceanTextureOpenGLHandle;

    std::vector<TextureFrameSpecification<WorldTextureGroups>> mLandTextureFrameSpecifications;
    GameOpenGLTexture mLandTextureOpenGLHandle;

    TextureAtlasMetadata<GenericLinearTextureGroups> const & mGenericLinearTextureAtlasMetadata;

private:

    // Shader manager
    ShaderManager<ShaderManagerTraits> & mShaderManager;

    // Thumbnails
    std::vector<std::pair<std::string, RgbaImageData>> mOceanAvailableThumbnails;
    std::vector<std::pair<std::string, RgbaImageData>> mLandAvailableThumbnails;
};

}