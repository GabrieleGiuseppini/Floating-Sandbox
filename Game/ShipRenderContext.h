/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GlobalRenderContext.h"
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
#include <GameCore/SysSpecifics.h>
#include <GameCore/Vectors.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Render {

class ShipRenderContext
{
private:

    // Base dimensions of flame quads
    static float constexpr BasisHalfFlameQuadWidth = 10.5f;
    static float constexpr BasisFlameQuadHeight = 7.5f;

public:

    ShipRenderContext(
        ShipId shipId,
        size_t pointCount,
        size_t shipCount,
        RgbaImageData shipTexture,
        ShaderManager<ShaderManagerTraits> & shaderManager,
        GlobalRenderContext const & globalRenderContext,
        RenderParameters const & renderParameters,
        float shipFlameSizeAdjustment,
        float vectorFieldLengthMultiplier);

    ~ShipRenderContext();

public:

    void SetShipCount(size_t shipCount)
    {
        mShipCount = shipCount;
        mIsViewModelDirty = true;
    }

    void SetShipFlameSizeAdjustment(float shipFlameSizeAdjustment)
    {
        // Recalculate quad dimensions
        mHalfFlameQuadWidth = BasisHalfFlameQuadWidth * shipFlameSizeAdjustment;
        mFlameQuadHeight = BasisFlameQuadHeight * shipFlameSizeAdjustment;
    }

    void SetVectorFieldLengthMultiplier(float vectorFieldLengthMultiplier)
    {
        mVectorFieldLengthMultiplier = vectorFieldLengthMultiplier;
    }

public:

    void UploadStart(PlaneId maxMaxPlaneId);

    inline void UploadWind(float smoothedWindSpeedMagnitude)
    {
        mFlameWindSpeedMagnitude = smoothedWindSpeedMagnitude;
        mIsFlameWindSpeedMagnitudeDirty = true;
    }

    //
    // Points
    //

    void UploadPointImmutableAttributes(vec2f const * textureCoordinates);

    void UploadPointMutableAttributesStart();

    void UploadPointMutableAttributes(
        vec2f const * position,
        float const * light,
        float const * water);

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

    void UploadPointFrontierColors(FrontierColor const * colors);

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
    // Frontiers
    //

    void UploadElementFrontierEdgesStart(size_t edgesCount);

    inline void UploadElementFrontierEdge(
        int pointIndex1,
        int pointIndex2)
    {
        mFrontierEdgeElementBuffer.emplace_back(
            pointIndex1,
            pointIndex2);
    }

    void UploadElementFrontierEdgesEnd();

    //
    // Flames
    //

    void UploadFlamesStart(size_t count);

    /*
     * Assumptions:
     *  - upload happens in depth order (for depth sorting)
     *  - all background flames are uploaded before all foreground flames
     */
    inline void UploadBackgroundFlame(
        PlaneId planeId,
        vec2f const & baseCenterPosition,
        vec2f const & flameVector,
        float scale,
        float flamePersonalitySeed)
    {
        assert(mFlameForegroundCount == 0);

        StoreFlameQuad(
            planeId,
            baseCenterPosition,
            flameVector,
            scale,
            flamePersonalitySeed);

        ++mFlameBackgroundCount;
    }

    /*
     * Assumptions:
     *  - upload happens in depth order (for depth sorting)
     *  - all background flames are uploaded before all foreground flames
     */
    inline void UploadForegroundFlame(
        PlaneId planeId,
        vec2f const & baseCenterPosition,
        vec2f const & flameVector,
        float scale,
        float flamePersonalitySeed)
    {
        StoreFlameQuad(
            planeId,
            baseCenterPosition,
            flameVector,
            scale,
            flamePersonalitySeed);

        ++mFlameForegroundCount;
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
            explosionIndex = std::min(2.0f, std::floor(personalitySeed * 3.0f));
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
            1.0f, // Scale
            0.0f, // Angle
            1.0f); // Alpha
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
            rotationOffset.angleCw(rotationBase),
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
            static_cast<TextureFrameIndex>(std::floor(frameIndexF)),
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

    void UploadVectorsStart(
        size_t maxCount,
        vec4f const & color);

    void UploadVector(
        vec2f const & position,
        float planeId,
        vec2f const & vector,
        float lengthAdjustment)
    {
        static float const CosAlphaLeftRight = std::cos(-2.f * Pi<float> / 8.f);
        static float const SinAlphaLeft = std::sin(-2.f * Pi<float> / 8.f);
        static float const SinAlphaRight = -SinAlphaLeft;

        static vec2f const XMatrixLeft = vec2f(CosAlphaLeftRight, SinAlphaLeft);
        static vec2f const YMatrixLeft = vec2f(-SinAlphaLeft, CosAlphaLeftRight);
        static vec2f const XMatrixRight = vec2f(CosAlphaLeftRight, SinAlphaRight);
        static vec2f const YMatrixRight = vec2f(-SinAlphaRight, CosAlphaLeftRight);

        float const effectiveVectorLength = lengthAdjustment * mVectorFieldLengthMultiplier;

        //
        // Store endpoint positions of each segment
        //

        // Stem
        vec2f stemEndpoint = position + vector * effectiveVectorLength;
        mVectorArrowVertexBuffer.emplace_back(position, planeId);
        mVectorArrowVertexBuffer.emplace_back(stemEndpoint, planeId);

        // Left
        vec2f leftDir = vec2f(-vector.dot(XMatrixLeft), -vector.dot(YMatrixLeft)).normalise();
        mVectorArrowVertexBuffer.emplace_back(stemEndpoint, planeId);
        mVectorArrowVertexBuffer.emplace_back(stemEndpoint + leftDir * 0.2f, planeId);

        // Right
        vec2f rightDir = vec2f(-vector.dot(XMatrixRight), -vector.dot(YMatrixRight)).normalise();
        mVectorArrowVertexBuffer.emplace_back(stemEndpoint, planeId);
        mVectorArrowVertexBuffer.emplace_back(stemEndpoint + rightDir * 0.2f, planeId);
    }

    void UploadVectorsEnd();

    //
    // Overlays
    //

    void UploadCentersStart(size_t count);

    inline void UploadCenter(
        PlaneId planeId,
        vec2f const & position,
        ViewModel const & viewModel)
    {
        float const fPlaneId = static_cast<float>(planeId);

        // Append vertices - two triangles

        float const halfQuadWorldSize = viewModel.PixelWidthToWorldWidth(18.0f); // We want the quad size to be independent from zoom
        float const leftX = position.x - halfQuadWorldSize;
        float const rightX = position.x + halfQuadWorldSize;
        float const topY = position.y - halfQuadWorldSize;
        float const bottomY = position.y + halfQuadWorldSize;

        // Triangle 1

        // Top-left
        mCenterVertexBuffer.emplace_back(
            vec2f(leftX, topY),
            vec2f(-1.0f, 1.0f),
            fPlaneId);

        // Top-Right
        mCenterVertexBuffer.emplace_back(
            vec2f(rightX, topY),
            vec2f(1.0f, 1.0f),
            fPlaneId);

        // Bottom-left
        mCenterVertexBuffer.emplace_back(
            vec2f(leftX, bottomY),
            vec2f(-1.0f, -1.0f),
            fPlaneId);

        // Triangle 2

        // Top-Right
        mCenterVertexBuffer.emplace_back(
            vec2f(rightX, topY),
            vec2f(1.0f, 1.0f),
            fPlaneId);

        // Bottom-left
        mCenterVertexBuffer.emplace_back(
            vec2f(leftX, bottomY),
            vec2f(-1.0f, -1.0f),
            fPlaneId);

        // Bottom-right
        mCenterVertexBuffer.emplace_back(
            vec2f(rightX, bottomY),
            vec2f(1.0f, -1.0f),
            fPlaneId);
    }

    void UploadCentersEnd();

    void UploadPointToPointArrowsStart(size_t count);

    inline void UploadPointToPointArrow(
        PlaneId planeId,
        vec2f const & startPoint,
        vec2f const & endPoint,
        rgbColor const & color)
    {
        static float const CosAlphaLeftRight = std::cos(-2.f * Pi<float> / 8.f);
        static float const SinAlphaLeft = std::sin(-2.f * Pi<float> / 8.f);
        static float const SinAlphaRight = -SinAlphaLeft;

        static vec2f const XMatrixLeft = vec2f(CosAlphaLeftRight, SinAlphaLeft);
        static vec2f const YMatrixLeft = vec2f(-SinAlphaLeft, CosAlphaLeftRight);
        static vec2f const XMatrixRight = vec2f(CosAlphaLeftRight, SinAlphaRight);
        static vec2f const YMatrixRight = vec2f(-SinAlphaRight, CosAlphaLeftRight);

        vec2f const stemVector = endPoint - startPoint;
        float const fPlaneId = static_cast<float>(planeId);
        vec3f const fColor = color.toVec3f();

        // Stem
        mPointToPointArrowVertexBuffer.emplace_back(startPoint, fPlaneId, fColor);
        mPointToPointArrowVertexBuffer.emplace_back(endPoint, fPlaneId, fColor);

        // Left
        vec2f leftDir = vec2f(-stemVector.dot(XMatrixLeft), -stemVector.dot(YMatrixLeft)).normalise();
        mPointToPointArrowVertexBuffer.emplace_back(endPoint, fPlaneId, fColor);
        mPointToPointArrowVertexBuffer.emplace_back(endPoint + leftDir * 0.2f, fPlaneId, fColor);

        // Right
        vec2f rightDir = vec2f(-stemVector.dot(XMatrixRight), -stemVector.dot(YMatrixRight)).normalise();
        mPointToPointArrowVertexBuffer.emplace_back(endPoint, fPlaneId, fColor);
        mPointToPointArrowVertexBuffer.emplace_back(endPoint + rightDir * 0.2f, fPlaneId, fColor);
    }

    void UploadPointToPointArrowsEnd();

    /////////////////////////////////////////

    void UploadEnd();

    void ProcessParameterChanges(RenderParameters const & renderParameters);

    void RenderPrepare(RenderParameters const & renderParameters);

    void RenderDraw(
        RenderParameters const & renderParameters,
        RenderStatistics & renderStats);

private:

    inline void StoreFlameQuad(
        PlaneId planeId,
        vec2f const & baseCenterPosition,
        vec2f const & flameVector,
        float scale,
        float flamePersonalitySeed)
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
        float constexpr YOffset = 0.066666f;

        // Qn = normalized flame vector
        // Qnp = perpendicular to Qn (i.e. Q's normal)
        float Ql = flameVector.length();
        vec2f const Qn = flameVector.normalise(Ql);
        vec2f const Qnp = Qn.to_perpendicular(); // rotated by PI/2, i.e. oriented to the left (wrt rest vector)

        // P' = point P lowered by yOffset
        vec2f const Pp = baseCenterPosition - Qn * YOffset * mFlameQuadHeight * scale;
        // P'' = opposite of P' on top
        vec2f const Ppp = Pp + flameVector * mFlameQuadHeight * scale;

        // Qhw = vector delineating one half of the quad width, the one to the left;
        // its length is not affected by velocity, only its direction
        vec2f const Qhw = Qnp * mHalfFlameQuadWidth * scale * 1.5f;

        // A, B = left-bottom, right-bottom
        vec2f const A = Pp + Qhw;
        vec2f const B = Pp - Qhw;
        // C, D = left-top, right-top
        vec2f const C = Ppp + Qhw;
        vec2f const D = Ppp - Qhw;

        //
        // Store quad vertices
        //

        // Triangle 1

        // Top-left
        mFlameVertexBuffer.emplace_back(
            vec2f(C.x, C.y),
            static_cast<float>(planeId),
            flamePersonalitySeed,
            vec2f(-1.0f, 1.0f));

        // Top-right
        mFlameVertexBuffer.emplace_back(
            vec2f(D.x, D.y),
            static_cast<float>(planeId),
            flamePersonalitySeed,
            vec2f(1.0f, 1.0f));

        // Bottom-left
        mFlameVertexBuffer.emplace_back(
            vec2f(A.x, A.y),
            static_cast<float>(planeId),
            flamePersonalitySeed,
            vec2f(-1.0f, 0.0f));

        // Triangle 2

        // Top-Right
        mFlameVertexBuffer.emplace_back(
            vec2f(D.x, D.y),
            static_cast<float>(planeId),
            flamePersonalitySeed,
            vec2f(1.0f, 1.0f));

        // Bottom-left
        mFlameVertexBuffer.emplace_back(
            vec2f(A.x, A.y),
            static_cast<float>(planeId),
            flamePersonalitySeed,
            vec2f(-1.0f, 0.0f));

        // Bottom-right
        mFlameVertexBuffer.emplace_back(
            vec2f(B.x, B.y),
            static_cast<float>(planeId),
            flamePersonalitySeed,
            vec2f(1.0f, 0.0f));
    }

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

        float const leftX = -frame.FrameMetadata.AnchorCenterWorld.x;
        float const rightX = frame.FrameMetadata.WorldWidth - frame.FrameMetadata.AnchorCenterWorld.x;
        float const topY = frame.FrameMetadata.WorldHeight - frame.FrameMetadata.AnchorCenterWorld.y;
        float const bottomY = -frame.FrameMetadata.AnchorCenterWorld.y;

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

    void RenderPrepareFlames();

    template<ProgramType FlameShaderType>
    void RenderDrawFlames(
        size_t startFlameIndex,
        size_t flameCount,
        RenderStatistics & renderStats);

    void RenderPrepareSparkles(RenderParameters const & renderParameters);
    void RenderDrawSparkles(RenderParameters const & renderParameters);

    void RenderPrepareGenericMipMappedTextures(RenderParameters const & renderParameters);
    void RenderDrawGenericMipMappedTextures(RenderParameters const & renderParameters, RenderStatistics & renderStats);

    void RenderPrepareExplosions(RenderParameters const & renderParameters);
    void RenderDrawExplosions(RenderParameters const & renderParameters);

    void RenderPrepareHighlights(RenderParameters const & renderParameters);
    void RenderDrawHighlights(RenderParameters const & renderParameters);

    void RenderPrepareVectorArrows(RenderParameters const & renderParameters);
    void RenderDrawVectorArrows(RenderParameters const & renderParameters);

    void RenderPrepareCenters(RenderParameters const & renderParameters);
    void RenderDrawCenters(RenderParameters const & renderParameters);

    void RenderPreparePointToPointArrows(RenderParameters const & renderParameters);
    void RenderDrawPointToPointArrows(RenderParameters const & renderParameters);

    void ApplyShipStructureRenderModeChanges(RenderParameters const & renderParameters);
    void ApplyViewModelChanges(RenderParameters const & renderParameters);
    void ApplyEffectiveAmbientLightIntensityChanges(RenderParameters const & renderParameters);
    void ApplyFlatLampLightColorChanges(RenderParameters const & renderParameters);
    void ApplyWaterColorChanges(RenderParameters const & renderParameters);
    void ApplyWaterContrastChanges(RenderParameters const & renderParameters);
    void ApplyWaterLevelOfDetailChanges(RenderParameters const & renderParameters);
    void ApplyHeatSensitivityChanges(RenderParameters const & renderParameters);

    void SelectShipPrograms(RenderParameters const & renderParameters);

private:

    ShipId const mShipId;
    size_t const mPointCount;

    size_t mShipCount;
    PlaneId mMaxMaxPlaneId; // Make plane ID ever
    bool mIsViewModelDirty;

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

    struct CenterVertex
    {
        vec2f vertexPosition;
        vec2f vertexSpacePosition;
        float planeId;

        CenterVertex(
            vec2f _vertexPosition,
            vec2f _vertexSpacePosition,
            float _planeId)
            : vertexPosition(_vertexPosition)
            , vertexSpacePosition(_vertexSpacePosition)
            , planeId(_planeId)
        {}
    };

    struct PointToPointArrowVertex
    {
        vec2f vertexPosition;
        float planeId;
        vec3f color;

        PointToPointArrowVertex(
            vec2f _vertexPosition,
            float _planeId,
            vec3f _color)
            : vertexPosition(_vertexPosition)
            , planeId(_planeId)
            , color(_color)
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

    GameOpenGLVBO mPointFrontierColorVBO;

    std::vector<LineElement> mStressedSpringElementBuffer;
    GameOpenGLVBO mStressedSpringElementVBO;
    size_t mStressedSpringElementVBOAllocatedElementSize;

    BoundedVector<LineElement> mFrontierEdgeElementBuffer;
    bool mIsFrontierEdgeElementBufferDirty;
    GameOpenGLVBO mFrontierEdgeElementVBO;
    size_t mFrontierEdgeElementVBOAllocatedElementSize;

    BoundedVector<FlameVertex> mFlameVertexBuffer;
    size_t mFlameBackgroundCount;
    size_t mFlameForegroundCount;
    GameOpenGLVBO mFlameVBO;
    size_t mFlameVBOAllocatedVertexSize;
    float mFlameWindSpeedMagnitude;
    bool mIsFlameWindSpeedMagnitudeDirty;

    std::vector<ExplosionPlaneData> mExplosionPlaneVertexBuffers;
    size_t mExplosionTotalVertexCount; // Calculated at RenderPrepare and cached for convenience
    GameOpenGLVBO mExplosionVBO;
    size_t mExplosionVBOAllocatedVertexSize;

    std::vector<SparkleVertex> mSparkleVertexBuffer;
    GameOpenGLVBO mSparkleVBO;
    size_t mSparkleVBOAllocatedVertexSize;

    std::vector<GenericTextureVertex> mGenericMipMappedTextureAirBubbleVertexBuffer; // Specifically for air bubbles; mixed planes
    std::vector<GenericTexturePlaneData> mGenericMipMappedTexturePlaneVertexBuffers; // For all other generic textures; separate buffers per-plane
    size_t mGenericMipMappedTextureTotalVertexCount; // Calculated at RenderPrepare and cached for convenience
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

    std::vector<CenterVertex> mCenterVertexBuffer;
    bool mIsCenterVertexBufferDirty;
    GameOpenGLVBO mCenterVBO;
    size_t mCenterVBOAllocatedVertexSize;

    std::vector<PointToPointArrowVertex> mPointToPointArrowVertexBuffer;
    bool mIsPointToPointArrowsVertexBufferDirty;
    GameOpenGLVBO mPointToPointArrowVBO;
    size_t mPointToPointArrowVBOAllocatedVertexSize;

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
    GameOpenGLVAO mCenterVAO;
    GameOpenGLVAO mPointToPointArrowVAO;

    //
    // The shaders to use for ship structures
    //

    ProgramType mShipPointsProgram;
    ProgramType mShipRopesProgram;
    ProgramType mShipSpringsProgram;
    ProgramType mShipTrianglesProgram;

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
    // Externally-controlled parameters that only affect Upload (i.e. that do
    // not affect rendering directly) or that purely serve as input to calculated
    // render parameters
    //

    float mHalfFlameQuadWidth;
    float mFlameQuadHeight;
    float mVectorFieldLengthMultiplier;
};

}