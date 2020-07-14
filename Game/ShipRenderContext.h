/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "RenderParameters.h"
#include "RenderTypes.h"
#include "ShaderTypes.h"
#include "ShipDefinition.h"
#include "TextureAtlas.h"
#include "TextureTypes.h"

#include <GameOpenGL/GameOpenGL.h>
#include <GameOpenGL/GameOpenGLMappedBuffer.h>
#include <GameOpenGL/ShaderManager.h>

#include <GameCore/BoundedVector.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/RunningAverage.h>
#include <GameCore/SysSpecifics.h>
#include <GameCore/Vectors.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Render {

class ShipRenderContext
{
public:

    ShipRenderContext(
        ShipId shipId,
        size_t pointCount,
        RgbaImageData shipTexture,
        ShaderManager<ShaderManagerTraits> & shaderManager,
        TextureAtlasMetadata<ExplosionTextureGroups> const & explosionTextureAtlasMetadata,
        TextureAtlasMetadata<GenericLinearTextureGroups> const & genericLinearTextureAtlasMetadata,
        TextureAtlasMetadata<GenericMipMappedTextureGroups> const & genericMipMappedTextureAtlasMetadata,
        RenderParameters const & renderParameters,
        // TODOOLD
        vec4f const & lampLightColor,
        vec4f const & waterColor,
        float waterContrast,
        float waterLevelOfDetail,
        bool showStressedSprings,
        bool drawHeatOverlay,
        float heatOverlayTransparency,
        ShipFlameRenderModeType shipFlameRenderMode,
        float shipFlameSizeAdjustment);

    ~ShipRenderContext();

public:    

    // TODOHERE

    void SetLampLightColor(vec4f lampLightColor)
    {
        mLampLightColor = lampLightColor;

        // React
        OnLampLightColorUpdated();
    }

    void SetWaterColor(vec4f waterColor)
    {
        mWaterColor = waterColor;

        // React
        OnWaterColorUpdated();
    }

    void SetWaterContrast(float waterContrast)
    {
        mWaterContrast = waterContrast;

        // React
        OnWaterContrastUpdated();
    }

    void SetWaterLevelThreshold(float waterLevelOfDetail)
    {
        mWaterLevelOfDetail = waterLevelOfDetail;

        // React
        OnWaterLevelOfDetailUpdated();
    }

    void SetShowStressedSprings(bool showStressedSprings)
    {
        mShowStressedSprings = showStressedSprings;
    }

    void SetDrawHeatOverlay(bool drawHeatOverlay)
    {
        mDrawHeatOverlay = drawHeatOverlay;
    }

    void SetHeatOverlayTransparency(float transparency)
    {
        mHeatOverlayTransparency = transparency;

        // React
        OnHeatOverlayTransparencyUpdated();
    }

    void SetShipFlameRenderMode(ShipFlameRenderModeType shipFlameRenderMode)
    {
        mShipFlameRenderMode = shipFlameRenderMode;
    }

    void SetShipFlameSizeAdjustment(float shipFlameSizeAdjustment)
    {
        mShipFlameSizeAdjustment = shipFlameSizeAdjustment;

        // React
        OnShipFlameSizeAdjustmentUpdated();
    }    

public:

    void UploadStart(PlaneId maxMaxPlaneId);

    //
    // Points
    //

    void UploadPointImmutableAttributes(vec2f const * textureCoordinates);

    void UploadPointMutableAttributesStart();

    void UploadPointMutableAttributes(
        vec2f const * position,
        float const * light,
        float const * water,
        size_t lightAndWaterCount);

    void UploadPointMutableAttributesPlaneId(
        float const * planeId,
        size_t startDst,
        size_t count);

    void UploadPointMutableAttributesDecay(
        float const * decay,
        size_t startDst,
        size_t count);

    void UploadPointMutableAttributesEnd();

    void UploadPointColors(
        vec4f const * color,
        size_t startDst,
        size_t count);

    void UploadPointTemperature(
        float const * temperature,
        size_t startDst,
        size_t count);

    //
    // Elements
    //

    /*
     * Signals that all elements, except may be triangles, will be re-uploaded. If triangles have changed, they
     * will also be uploaded; if they are not re-uploaded, then the last uploaded set is to be used.
     */
    void UploadElementsStart();

    inline void UploadElementPoint(int pointIndex)
    {
        mPointElementBuffer.emplace_back(pointIndex);
    }

    inline void UploadElementSpring(
        int pointIndex1,
        int pointIndex2)
    {
        mSpringElementBuffer.emplace_back(
            pointIndex1,
            pointIndex2);
    }

    inline void UploadElementRope(
        int pointIndex1,
        int pointIndex2)
    {
        mRopeElementBuffer.emplace_back(
            pointIndex1,
            pointIndex2);
    }

    void UploadElementTrianglesStart(size_t trianglesCount);

    inline void UploadElementTriangle(
        size_t triangleIndex,
        int pointIndex1,
        int pointIndex2,
        int pointIndex3)
    {
        assert(triangleIndex < mTriangleElementBuffer.size());

        TriangleElement & triangleElement = mTriangleElementBuffer[triangleIndex];

        triangleElement.pointIndex1 = pointIndex1;
        triangleElement.pointIndex2 = pointIndex2;
        triangleElement.pointIndex3 = pointIndex3;
    }

    void UploadElementTrianglesEnd();

    void UploadElementsEnd();

    //
    // Stressed springs
    //

    void UploadElementStressedSpringsStart();

    inline void UploadElementStressedSpring(
        int pointIndex1,
        int pointIndex2)
    {
        mStressedSpringElementBuffer.emplace_back(
            pointIndex1,
            pointIndex2);
    }

    void UploadElementStressedSpringsEnd();

    //
    // Flames
    //

    void UploadFlamesStart(
        size_t count,
        float windSpeedMagnitude);

    /*
     * Note: assumption is that upload happens in plane ID order (for depth sorting).
     */
    inline void UploadFlame(
        PlaneId planeId,
        vec2f const & baseCenterPosition,
        vec2f const & flameVector,
        float scale,
        float flamePersonalitySeed,
        bool isOnChain)
    {
        //
        // Calculate flame quad - encloses the flame vector
        //

        //
        // C-------D
        // |       |
        // |       |
        // |       |
        // |       |
        // |       |
        // |---P---|
        // |       |
        // A-------B
        //

        // Y offset to focus bottom of flame at specified position; depends mostly on shader
        float const yOffset = (mShipFlameRenderMode == ShipFlameRenderModeType::Mode1)
            ? 0.066666f
            : 0.013333f;

        // Qn = normalized flame vector
        // Qnp = perpendicular to Qn (i.e. Q's normal)
        float Ql = flameVector.length();
        vec2f const Qn = flameVector.normalise(Ql);
        vec2f const Qnp = Qn.to_perpendicular(); // rotated by PI/2, i.e. oriented to the left (wrt rest vector)

        // P' = point P lowered by yOffset
        vec2f const Pp = baseCenterPosition - Qn * yOffset * mFlameQuadHeight * scale;
        // P'' = opposite of P' on top
        vec2f const Ppp = Pp + flameVector * mFlameQuadHeight * scale;

        // Qhw = vector delineating one half of the quad width, the one to the left;
        // its length is not affected by velocity, only its direction
        vec2f const Qhw = Qnp * mHalfFlameQuadWidth * scale;

        // A, B = left-bottom, right-bottom
        vec2f const A = Pp + Qhw;
        vec2f const B = Pp - Qhw;
        // C, D = left-top, right-top
        vec2f const C = Ppp + Qhw;
        vec2f const D = Ppp - Qhw;

        //
        // Store quad vertices
        //

        size_t vertexIndex;
        if (isOnChain)
        {
            // Background flame
            vertexIndex = mFlameBackgroundCount * 6u;
            ++mFlameBackgroundCount;
        }
        else
        {
            // Foreground flame
            // - Note: this is against Z order
            ++mFlameForegroundCount;
            vertexIndex = mFlameVertexBuffer.size() - mFlameForegroundCount * 6u;
        }

        assert(vertexIndex < mFlameVertexBuffer.size());

        // Triangle 1

        // Top-left
        mFlameVertexBuffer.emplace_at(
            vertexIndex++,
            vec2f(C.x, C.y),
            static_cast<float>(planeId),
            flamePersonalitySeed,
            vec2f(-1.0f, 1.0f));

        // Top-right
        mFlameVertexBuffer.emplace_at(
            vertexIndex++,
            vec2f(D.x, D.y),
            static_cast<float>(planeId),
            flamePersonalitySeed,
            vec2f(1.0f, 1.0f));

        // Bottom-left
        mFlameVertexBuffer.emplace_at(
            vertexIndex++,
            vec2f(A.x, A.y),
            static_cast<float>(planeId),
            flamePersonalitySeed,
            vec2f(-1.0f, 0.0f));

        // Triangle 2

        // Top-Right
        mFlameVertexBuffer.emplace_at(
            vertexIndex++,
            vec2f(D.x, D.y),
            static_cast<float>(planeId),
            flamePersonalitySeed,
            vec2f(1.0f, 1.0f));

        // Bottom-left
        mFlameVertexBuffer.emplace_at(
            vertexIndex++,
            vec2f(A.x, A.y),
            static_cast<float>(planeId),
            flamePersonalitySeed,
            vec2f(-1.0f, 0.0f));

        // Bottom-right
        mFlameVertexBuffer.emplace_at(
            vertexIndex++,
            vec2f(B.x, B.y),
            static_cast<float>(planeId),
            flamePersonalitySeed,
            vec2f(1.0f, 0.0f));
    }

    void UploadFlamesEnd();

    //
    // Explosions
    //
    // Explosions don't have a start/end as there are multiple
    // physical sources of explosions.
    //

    inline void UploadExplosion(
        PlaneId planeId,
        vec2f const & centerPosition,
        float halfQuadSize,
        ExplosionType explosionType,
        float personalitySeed,
        float progress)
    {
        size_t const planeIndex = static_cast<size_t>(planeId);

        // Pre-sized
        assert(planeIndex < mExplosionPlaneVertexBuffers.size());

        // Get this plane's vertex buffer
        auto & vertexBuffer = mExplosionPlaneVertexBuffers[planeIndex].vertexBuffer;

        //
        // Populate the texture quad
        //

        // Resolution of atlas, for dead center calculations
        float const dTextureX = 1.0f / (2.0f * static_cast<float>(mExplosionTextureAtlasMetadata.GetSize().Width));
        float const dTextureY = 1.0f / (2.0f * static_cast<float>(mExplosionTextureAtlasMetadata.GetSize().Height));

        // Calculate render half quad size - magic offset to account for
        // empty outskirts of frames
        float const renderHalfQuadSize = halfQuadSize + 13.0f;

        // Calculate explosion index based off explosion type
        float explosionIndex;
        if (ExplosionType::Deflagration == explosionType)
        {
            // 0..2, randomly
            explosionIndex = std::min(2.0f, floor(personalitySeed * 3.0f));
        }
        else
        {
            assert(ExplosionType::Combustion == explosionType);

            explosionIndex = 3.0f;
        }

        // Calculate rotation based off personality seed
        float const angleCcw = personalitySeed * 2.0f * Pi<float>;

        // Append vertices - two triangles

        // Triangle 1

        // Top-left
        vertexBuffer.emplace_back(
            centerPosition,
            vec2f(-renderHalfQuadSize, renderHalfQuadSize),
            vec2f(0.0f + dTextureX, 1.0f - dTextureY),
            static_cast<float>(planeId),
            angleCcw,
            explosionIndex,
            progress);

        // Top-Right
        vertexBuffer.emplace_back(
            centerPosition,
            vec2f(renderHalfQuadSize, renderHalfQuadSize),
            vec2f(1.0f - dTextureX, 1.0f - dTextureY),
            static_cast<float>(planeId),
            angleCcw,
            explosionIndex,
            progress);

        // Bottom-left
        vertexBuffer.emplace_back(
            centerPosition,
            vec2f(-renderHalfQuadSize, -renderHalfQuadSize),
            vec2f(0.0f + dTextureX, 0.0f + dTextureY),
            static_cast<float>(planeId),
            angleCcw,
            explosionIndex,
            progress);

        // Triangle 2

        // Top-Right
        vertexBuffer.emplace_back(
            centerPosition,
            vec2f(renderHalfQuadSize, renderHalfQuadSize),
            vec2f(1.0f - dTextureX, 1.0f - dTextureY),
            static_cast<float>(planeId),
            angleCcw,
            explosionIndex,
            progress);

        // Bottom-left
        vertexBuffer.emplace_back(
            centerPosition,
            vec2f(-renderHalfQuadSize, -renderHalfQuadSize),
            vec2f(0.0f + dTextureX, 0.0f + dTextureY),
            static_cast<float>(planeId),
            angleCcw,
            explosionIndex,
            progress);

        // Bottom-right
        vertexBuffer.emplace_back(
            centerPosition,
            vec2f(renderHalfQuadSize, -renderHalfQuadSize),
            vec2f(1.0f - dTextureX, 0.0f + dTextureY),
            static_cast<float>(planeId),
            angleCcw,
            explosionIndex,
            progress);
    }

    //
    // Sparkles
    //

    inline void UploadSparkle(
        PlaneId planeId,
        vec2f const & position,
        vec2f const & velocityVector,
        float progress)
    {
        //
        // Calculate sparkle quad
        //

        float const halfQuadSide =
            1.5f
            * (1.0f - SmoothStep(0.5f, 0.75f, progress)); // Shrinks as time goes

        // Calculate quad coordinates
        float const leftX = position.x - halfQuadSide;
        float const rightX = position.x + halfQuadSide;
        float const topY = position.y - halfQuadSide;
        float const bottomY = position.y + halfQuadSide;


        //
        // Store vertices
        //

        // Triangle 1

        // Top-left
        mSparkleVertexBuffer.emplace_back(
            vec2f(leftX, topY),
            static_cast<float>(planeId),
            progress,
            velocityVector,
            vec2f(-1.0f, -1.0f));

        // Top-right
        mSparkleVertexBuffer.emplace_back(
            vec2f(rightX, topY),
            static_cast<float>(planeId),
            progress,
            velocityVector,
            vec2f(1.0f, -1.0f));

        // Bottom-left
        mSparkleVertexBuffer.emplace_back(
            vec2f(leftX, bottomY),
            static_cast<float>(planeId),
            progress,
            velocityVector,
            vec2f(-1.0f, 1.0f));

        // Triangle 2

        // Top-right
        mSparkleVertexBuffer.emplace_back(
            vec2f(rightX, topY),
            static_cast<float>(planeId),
            progress,
            velocityVector,
            vec2f(1.0f, -1.0f));

        // Bottom-left
        mSparkleVertexBuffer.emplace_back(
            vec2f(leftX, bottomY),
            static_cast<float>(planeId),
            progress,
            velocityVector,
            vec2f(-1.0f, 1.0f));

        // Bottom-right
        mSparkleVertexBuffer.emplace_back(
            vec2f(rightX, bottomY),
            static_cast<float>(planeId),
            progress,
            velocityVector,
            vec2f(1.0f, 1.0f));
    }

    //
    // Air bubbles and generic textures
    //
    // Generic textures don't have a start/end as there are multiple
    // physical sources of generic textures.
    //

    inline void UploadAirBubble(
        PlaneId planeId,
        vec2f const & position,
        float scale,
        float alpha)
    {
        StoreGenericMipMappedTextureRenderSpecification(
            planeId,
            TextureFrameId<GenericMipMappedTextureGroups>(GenericMipMappedTextureGroups::AirBubble, 0),
            position,
            scale,
            0.0f, // angle
            alpha,
            mGenericMipMappedTextureAirBubbleVertexBuffer);
    }

    inline void UploadGenericMipMappedTextureRenderSpecification(
        PlaneId planeId,
        TextureFrameId<GenericMipMappedTextureGroups> const & textureFrameId,
        vec2f const & position)
    {
        UploadGenericMipMappedTextureRenderSpecification(
            planeId,
            textureFrameId,
            position,
            1.0f,
            0.0f,
            1.0f);
    }

    inline void UploadGenericMipMappedTextureRenderSpecification(
        PlaneId planeId,
        TextureFrameId<GenericMipMappedTextureGroups> const & textureFrameId,
        vec2f const & position,
        float scale,
        vec2f const & rotationBase,
        vec2f const & rotationOffset,
        float alpha)
    {
        UploadGenericMipMappedTextureRenderSpecification(
            planeId,
            textureFrameId,
            position,
            scale,
            rotationBase.angleCw(rotationOffset),
            alpha);
    }

    inline void UploadGenericMipMappedTextureRenderSpecification(
        PlaneId planeId,
        float personalitySeed,
        GenericMipMappedTextureGroups textureGroup,
        vec2f const & position,
        float scale,
        float alpha)
    {
        // Choose frame
        size_t const frameCount = mGenericMipMappedTextureAtlasMetadata.GetFrameCount(textureGroup);
        float frameIndexF = personalitySeed * frameCount;
        TextureFrameIndex const frameIndex = std::min(
            static_cast<TextureFrameIndex>(floor(frameIndexF)),
            static_cast<TextureFrameIndex>(frameCount - 1));

        // Choose angle
        float const angleCw = (frameIndexF - static_cast<float>(frameIndex)) * 2.0f * Pi<float>;

        UploadGenericMipMappedTextureRenderSpecification(
            planeId,
            TextureFrameId<GenericMipMappedTextureGroups>(textureGroup, frameIndex),
            position,
            scale,
            angleCw,
            alpha);
    }

    inline void UploadGenericMipMappedTextureRenderSpecification(
        PlaneId planeId,
        TextureFrameId<GenericMipMappedTextureGroups> const & textureFrameId,
        vec2f const & position,
        float scale,
        float angleCw,
        float alpha)
    {
        size_t const planeIndex = static_cast<size_t>(planeId);

        // Pre-sized
        assert(planeIndex < mGenericMipMappedTexturePlaneVertexBuffers.size());

        // Get this plane's vertex buffer
        auto & vertexBuffer = mGenericMipMappedTexturePlaneVertexBuffers[planeIndex].vertexBuffer;

        // Populate the texture quad
        StoreGenericMipMappedTextureRenderSpecification(
            planeId,
            textureFrameId,
            position,
            scale,
            angleCw,
            alpha,
            vertexBuffer);
    }

    //
    // Ephemeral point elements
    //

    void UploadElementEphemeralPointsStart();

    inline void UploadElementEphemeralPoint(
        int pointIndex)
    {
        mEphemeralPointElementBuffer.emplace_back(pointIndex);
    }

    void UploadElementEphemeralPointsEnd();

    //
    // Highlights
    //
    // Highlights don't have a start/end as there are multiple
    // physical sources of highlights.
    //

    inline void UploadHighlight(
        HighlightModeType highlightMode,
        PlaneId planeId,
        vec2f const & centerPosition,
        float halfQuadSize,
        rgbColor color,
        float progress)
    {
        vec3f const vColor = color.toVec3f();
        float const fPlaneId = static_cast<float>(planeId);

        // Append vertices - two triangles

        float const leftX = centerPosition.x - halfQuadSize;
        float const rightX = centerPosition.x + halfQuadSize;
        float const topY = centerPosition.y - halfQuadSize;
        float const bottomY = centerPosition.y + halfQuadSize;

        auto & highlightVertexBuffer = mHighlightVertexBuffers[static_cast<size_t>(highlightMode)];

        // Triangle 1

        // Top-left
        highlightVertexBuffer.emplace_back(
            vec2f(leftX, topY),
            vec2f(-1.0f, 1.0f),
            vColor,
            progress,
            fPlaneId);

        // Top-Right
        highlightVertexBuffer.emplace_back(
            vec2f(rightX, topY),
            vec2f(1.0f, 1.0f),
            vColor,
            progress,
            fPlaneId);

        // Bottom-left
        highlightVertexBuffer.emplace_back(
            vec2f(leftX, bottomY),
            vec2f(-1.0f, -1.0f),
            vColor,
            progress,
            fPlaneId);

        // Triangle 2

        // Top-Right
        highlightVertexBuffer.emplace_back(
            vec2f(rightX, topY),
            vec2f(1.0f, 1.0f),
            vColor,
            progress,
            fPlaneId);

        // Bottom-left
        highlightVertexBuffer.emplace_back(
            vec2f(leftX, bottomY),
            vec2f(-1.0f, -1.0f),
            vColor,
            progress,
            fPlaneId);

        // Bottom-right
        highlightVertexBuffer.emplace_back(
            vec2f(rightX, bottomY),
            vec2f(1.0f, -1.0f),
            vColor,
            progress,
            fPlaneId);
    }

    //
    // Vectors
    //

    void UploadVectors(
        size_t count,
        vec2f const * position,
        float const * planeId,
        vec2f const * vector,
        float lengthAdjustment,
        vec4f const & color);

    void UploadEnd();

    void Draw(
        RenderParameters const & renderParameters,
        RenderStatistics & renderStats);

private:

    template<typename TVertexBuffer>
    inline void StoreGenericMipMappedTextureRenderSpecification(
        PlaneId planeId,
        TextureFrameId<GenericMipMappedTextureGroups> const & textureFrameId,
        vec2f const & position,
        float scale,
        float angleCw,
        float alpha,
        TVertexBuffer & vertexBuffer)
    {
        //
        // Populate the texture quad
        //

        TextureAtlasFrameMetadata<GenericMipMappedTextureGroups> const & frame =
            mGenericMipMappedTextureAtlasMetadata.GetFrameMetadata(textureFrameId);

        float const leftX = -frame.FrameMetadata.AnchorWorldX;
        float const rightX = frame.FrameMetadata.WorldWidth - frame.FrameMetadata.AnchorWorldX;
        float const topY = frame.FrameMetadata.WorldHeight - frame.FrameMetadata.AnchorWorldY;
        float const bottomY = -frame.FrameMetadata.AnchorWorldY;

        float const lightSensitivity =
            frame.FrameMetadata.HasOwnAmbientLight ? 0.0f : 1.0f;

        // Append vertices - two triangles

        // Triangle 1

        // Top-left
        vertexBuffer.emplace_back(
            position,
            vec2f(leftX, topY),
            vec2f(frame.TextureCoordinatesBottomLeft.x, frame.TextureCoordinatesTopRight.y),
            static_cast<float>(planeId),
            scale,
            -angleCw,
            alpha,
            lightSensitivity);

        // Top-Right
        vertexBuffer.emplace_back(
            position,
            vec2f(rightX, topY),
            frame.TextureCoordinatesTopRight,
            static_cast<float>(planeId),
            scale,
            -angleCw,
            alpha,
            lightSensitivity);

        // Bottom-left
        vertexBuffer.emplace_back(
            position,
            vec2f(leftX, bottomY),
            frame.TextureCoordinatesBottomLeft,
            static_cast<float>(planeId),
            scale,
            -angleCw,
            alpha,
            lightSensitivity);

        // Triangle 2

        // Top-Right
        vertexBuffer.emplace_back(
            position,
            vec2f(rightX, topY),
            frame.TextureCoordinatesTopRight,
            static_cast<float>(planeId),
            scale,
            -angleCw,
            alpha,
            lightSensitivity);

        // Bottom-left
        vertexBuffer.emplace_back(
            position,
            vec2f(leftX, bottomY),
            frame.TextureCoordinatesBottomLeft,
            static_cast<float>(planeId),
            scale,
            -angleCw,
            alpha,
            lightSensitivity);

        // Bottom-right
        vertexBuffer.emplace_back(
            position,
            vec2f(rightX, bottomY),
            vec2f(frame.TextureCoordinatesTopRight.x, frame.TextureCoordinatesBottomLeft.y),
            static_cast<float>(planeId),
            scale,
            -angleCw,
            alpha,
            lightSensitivity);
    }

private:

    template<ProgramType ShaderProgram>
    void RenderFlames(
        size_t startFlameIndex,
        size_t flameCount,
        RenderParameters const & renderParameters,
        RenderStatistics & renderStats);

    void RenderSparkles(RenderParameters const & renderParameters);
    void RenderGenericMipMappedTextures(RenderParameters const & renderParameters, RenderStatistics & renderStats);
    void RenderExplosions(RenderParameters const & renderParameters);
    void RenderHighlights(RenderParameters const & renderParameters);
    void RenderVectorArrows(RenderParameters const & renderParameters);

    void ProcessParameterChanges(RenderParameters const & renderParameters);
    void ApplyViewModelChanges(RenderParameters const & renderParameters);
    void ApplyEffectiveAmbientLightIntensityChanges(RenderParameters const & renderParameters);
    // TODOOLD
    void OnLampLightColorUpdated();
    void OnWaterColorUpdated();
    void OnWaterContrastUpdated();
    void OnWaterLevelOfDetailUpdated();
    void OnHeatOverlayTransparencyUpdated();
    void OnShipFlameSizeAdjustmentUpdated();

private:

    ShipId const mShipId;
    size_t const mPointCount;

    PlaneId mMaxMaxPlaneId; // Make plane ID ever
    bool mIsMaxMaxPlaneIdDirty;

    //
    // Types
    //

#pragma pack(push, 1)

    struct PointElement
    {
        int pointIndex;

        PointElement(int _pointIndex)
            : pointIndex(_pointIndex)
        {}
    };

    struct LineElement
    {
        int pointIndex1;
        int pointIndex2;

        LineElement(
            int _pointIndex1,
            int _pointIndex2)
            : pointIndex1(_pointIndex1)
            , pointIndex2(_pointIndex2)
        {}
    };

    struct TriangleElement
    {
        int pointIndex1;
        int pointIndex2;
        int pointIndex3;
    };

    struct FlameVertex
    {
        vec2f vertexPosition;
        float planeId;
        float flamePersonalitySeed;
        vec2f flameSpacePosition;

        FlameVertex(
            vec2f _vertexPosition,
            float _planeId,
            float _flamePersonalitySeed,
            vec2f _flameSpacePosition)
            : vertexPosition(_vertexPosition)
            , planeId(_planeId)
            , flamePersonalitySeed(_flamePersonalitySeed)
            , flameSpacePosition(_flameSpacePosition)
        {}
    };

    struct ExplosionVertex
    {
        vec2f centerPosition;
        vec2f vertexOffset;
        vec2f textureCoordinate;

        float planeId;

        float angle;
        float explosionIndex;
        float progress;

        ExplosionVertex(
            vec2f _centerPosition,
            vec2f _vertexOffset,
            vec2f _textureCoordinate,
            float _planeId,
            float _angle,
            float _explosionIndex,
            float _progress)
            : centerPosition(_centerPosition)
            , vertexOffset(_vertexOffset)
            , textureCoordinate(_textureCoordinate)
            , planeId(_planeId)
            , angle(_angle)
            , explosionIndex(_explosionIndex)
            , progress(_progress)
        {}
    };

    struct SparkleVertex
    {
        vec2f vertexPosition;
        float planeId;
        float progress;
        vec2f velocityVector;
        vec2f sparkleSpacePosition;

        SparkleVertex(
            vec2f _vertexPosition,
            float _planeId,
            float _progress,
            vec2f _velocityVector,
            vec2f _sparkleSpacePosition)
            : vertexPosition(_vertexPosition)
            , planeId(_planeId)
            , progress(_progress)
            , velocityVector(_velocityVector)
            , sparkleSpacePosition(_sparkleSpacePosition)
        {}
    };

    struct GenericTextureVertex
    {
        vec2f centerPosition;
        vec2f vertexOffset;
        vec2f textureCoordinate;

        float planeId;

        float scale;
        float angle;
        float alpha;
        float ambientLightSensitivity;

        GenericTextureVertex(
            vec2f _centerPosition,
            vec2f _vertexOffset,
            vec2f _textureCoordinate,
            float _planeId,
            float _scale,
            float _angle,
            float _alpha,
            float _ambientLightSensitivity)
            : centerPosition(_centerPosition)
            , vertexOffset(_vertexOffset)
            , textureCoordinate(_textureCoordinate)
            , planeId(_planeId)
            , scale(_scale)
            , angle(_angle)
            , alpha(_alpha)
            , ambientLightSensitivity(_ambientLightSensitivity)
        {}
    };

    struct HighlightVertex
    {
        vec2f vertexPosition;
        vec2f vertexSpacePosition;
        vec3f color;
        float progress;
        float planeId;

        HighlightVertex(
            vec2f _vertexPosition,
            vec2f _vertexSpacePosition,
            vec3f _color,
            float _progress,
            float _planeId)
            : vertexPosition(_vertexPosition)
            , vertexSpacePosition(_vertexSpacePosition)
            , color(_color)
            , progress(_progress)
            , planeId(_planeId)
        {}
    };

#pragma pack(pop)

    struct ExplosionPlaneData
    {
        std::vector<ExplosionVertex> vertexBuffer;
    };

    struct GenericTexturePlaneData
    {
        std::vector<GenericTextureVertex> vertexBuffer;
    };

    //
    // Buffers
    //

    std::unique_ptr<vec4f> mPointAttributeGroup1Buffer; // Position, TextureCoordinates
    GameOpenGLVBO mPointAttributeGroup1VBO;

    std::unique_ptr<vec4f> mPointAttributeGroup2Buffer; // Light, Water, PlaneId, Decay
    GameOpenGLVBO mPointAttributeGroup2VBO;

    GameOpenGLVBO mPointColorVBO;

    GameOpenGLVBO mPointTemperatureVBO;

    std::vector<LineElement> mStressedSpringElementBuffer;
    GameOpenGLVBO mStressedSpringElementVBO;

    GameOpenGLMappedBuffer<FlameVertex, GL_ARRAY_BUFFER> mFlameVertexBuffer;
    size_t mFlameVertexBufferAllocatedSize;
    size_t mFlameBackgroundCount;
    size_t mFlameForegroundCount;
    GameOpenGLVBO mFlameVertexVBO;
    RunningAverage<18> mWindSpeedMagnitudeRunningAverage;
    float mCurrentWindSpeedMagnitudeAverage;

    std::vector<ExplosionPlaneData> mExplosionPlaneVertexBuffers;
    GameOpenGLVBO mExplosionVBO;
    size_t mExplosionVBOAllocatedVertexSize;

    std::vector<SparkleVertex> mSparkleVertexBuffer;
    GameOpenGLVBO mSparkleVBO;
    size_t mSparkleVBOAllocatedVertexSize;

    std::vector<GenericTextureVertex> mGenericMipMappedTextureAirBubbleVertexBuffer; // Specifically for air bubbles; mixed planes
    std::vector<GenericTexturePlaneData> mGenericMipMappedTexturePlaneVertexBuffers; // For all other generic textures; separate buffers per-plane
    GameOpenGLVBO mGenericMipMappedTextureVBO;
    size_t mGenericMipMappedTextureVBOAllocatedVertexSize;

    std::array<std::vector<HighlightVertex>, static_cast<size_t>(HighlightModeType::_Last) + 1> mHighlightVertexBuffers;
    GameOpenGLVBO mHighlightVBO;
    size_t mHighlightVBOAllocatedVertexSize;

    std::vector<vec3f> mVectorArrowVertexBuffer;
    GameOpenGLVBO mVectorArrowVBO;
    size_t mVectorArrowVBOAllocatedVertexSize;
    vec4f mVectorArrowColor;
    bool mIsVectorArrowColorDirty;

    //
    // Element (index) buffers
    //
    // We use a single VBO for all element indices except stressed springs
    //

    std::vector<PointElement> mPointElementBuffer;
    std::vector<PointElement> mEphemeralPointElementBuffer;
    std::vector<LineElement> mSpringElementBuffer;
    std::vector<LineElement> mRopeElementBuffer;
    std::vector<TriangleElement> mTriangleElementBuffer;
    bool mAreElementBuffersDirty;
    GameOpenGLVBO mElementVBO;
    size_t mElementVBOAllocatedIndexSize;

    // Indices at which these elements begin in the VBO; populated
    // when we upload element indices to the VBO
    size_t mPointElementVBOStartIndex;
    size_t mEphemeralPointElementVBOStartIndex;
    size_t mSpringElementVBOStartIndex;
    size_t mRopeElementVBOStartIndex;
    size_t mTriangleElementVBOStartIndex;


    //
    // VAOs
    //

    GameOpenGLVAO mShipVAO;
    GameOpenGLVAO mFlameVAO;
    GameOpenGLVAO mExplosionVAO;
    GameOpenGLVAO mSparkleVAO;
    GameOpenGLVAO mGenericMipMappedTextureVAO;
    GameOpenGLVAO mHighlightVAO;
    GameOpenGLVAO mVectorArrowVAO;


    //
    // Textures
    //

    GameOpenGLTexture mShipTextureOpenGLHandle;
    GameOpenGLTexture mStressedSpringTextureOpenGLHandle;

    TextureAtlasMetadata<ExplosionTextureGroups> const & mExplosionTextureAtlasMetadata;
    [[maybe_unused]]
    TextureAtlasMetadata<GenericLinearTextureGroups> const & mGenericLinearTextureAtlasMetadata;
    TextureAtlasMetadata<GenericMipMappedTextureGroups> const & mGenericMipMappedTextureAtlasMetadata;

private:

    //
    // Managers
    //

    ShaderManager<ShaderManagerTraits> & mShaderManager;

    //
    // Parameters
    //

    // TODO: should nuke all of these
    vec4f mLampLightColor;
    vec4f mWaterColor;
    float mWaterContrast;
    float mWaterLevelOfDetail;
    bool mShowStressedSprings;
    bool mDrawHeatOverlay;
    float mHeatOverlayTransparency;
    ShipFlameRenderModeType mShipFlameRenderMode;
    float mShipFlameSizeAdjustment;
    float mHalfFlameQuadWidth;
    float mFlameQuadHeight;
};

}