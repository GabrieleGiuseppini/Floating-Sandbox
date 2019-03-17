/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "RenderCore.h"
#include "ShipDefinition.h"
#include "TextureAtlas.h"
#include "ViewModel.h"

#include <GameOpenGL/GameOpenGL.h>
#include <GameOpenGL/ShaderManager.h>

#include <GameCore/BoundedVector.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/SysSpecifics.h>
#include <GameCore/Vectors.h>

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
        size_t shipCount,
        size_t pointCount,
        RgbaImageData shipTexture,
        ShipDefinition::TextureOriginType textureOrigin,
        ShaderManager<ShaderManagerTraits> & shaderManager,
        GameOpenGLTexture & genericTextureAtlasOpenGLHandle,
        TextureAtlasMetadata const & genericTextureAtlasMetadata,
        RenderStatistics & renderStatistics,
        ViewModel const & viewModel,
        float ambientLightIntensity,
        float waterContrast,
        float waterLevelOfDetail,
        ShipRenderMode shipRenderMode,
        DebugShipRenderMode debugShipRenderMode,
        VectorFieldRenderMode vectorFieldRenderMode,
        bool showStressedSprings);

    ~ShipRenderContext();

public:

    void OnViewModelUpdated()
    {
        // Recalculate ortho matrices
        UpdateOrthoMatrices();
    }

    void SetShipCount(size_t shipCount)
    {
        mShipCount = shipCount;

        // Recalculate ortho matrices
        UpdateOrthoMatrices();
    }

    void SetAmbientLightIntensity(float ambientLightIntensity)
    {
        mAmbientLightIntensity = ambientLightIntensity;

        // Set parameters
        OnAmbientLightIntensityUpdated();
    }

    void SetWaterContrast(float waterContrast)
    {
        mWaterContrast = waterContrast;

        // Set parameters
        OnWaterContrastUpdated();
    }

    void SetWaterLevelThreshold(float waterLevelOfDetail)
    {
        mWaterLevelOfDetail = waterLevelOfDetail;

        // Set parameters
        OnWaterLevelOfDetailUpdated();
    }

    void SetShipRenderMode(ShipRenderMode shipRenderMode)
    {
        mShipRenderMode = shipRenderMode;
    }

    void SetDebugShipRenderMode(DebugShipRenderMode debugShipRenderMode)
    {
        mDebugShipRenderMode = debugShipRenderMode;
    }

    void SetVectorFieldRenderMode(VectorFieldRenderMode vectorFieldRenderMode)
    {
        mVectorFieldRenderMode = vectorFieldRenderMode;
    }

    void SetShowStressedSprings(bool showStressedSprings)
    {
        mShowStressedSprings = showStressedSprings;
    }

public:

    void RenderStart(PlaneId maxMaxPlaneId);

    //
    // Points
    //

    void UploadPointImmutableAttributes(vec2f const * textureCoordinates);

    void UploadPointMutableAttributes(
        vec2f const * position,
        float const * light,
        float const * water);

    void UploadPointColors(
        vec4f const * color,
        size_t startDst,
        size_t count);

    void UploadPointPlaneIds(
        float const * planeId,
        size_t startDst,
        size_t count);

    //
    // Elements
    //

    /*
     * Signals that all elements, except triangles, will be re-uploaded. If triangles have changed, they
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
    // Generic textures
    //

    inline void UploadGenericTextureRenderSpecification(
        PlaneId planeId,
        TextureFrameId const & textureFrameId,
        vec2f const & position)
    {
        UploadGenericTextureRenderSpecification(
            planeId,
            textureFrameId,
            position,
            1.0f,
            0.0f,
            1.0f);
    }

    inline void UploadGenericTextureRenderSpecification(
        PlaneId planeId,
        TextureFrameId const & textureFrameId,
        vec2f const & position,
        float scale,
        vec2f const & rotationBase,
        vec2f const & rotationOffset,
        float alpha)
    {
        UploadGenericTextureRenderSpecification(
            planeId,
            textureFrameId,
            position,
            scale,
            rotationBase.angle(rotationOffset),
            alpha);
    }

    inline void UploadGenericTextureRenderSpecification(
        PlaneId planeId,
        TextureFrameId const & textureFrameId,
        vec2f const & position,
        float scale,
        float angle,
        float alpha)
    {
        size_t const planeIndex = static_cast<size_t>(planeId);

        assert(planeIndex < mGenericTexturePlaneVertexBuffers.size());

        // Get this plane's vertex buffer
        auto & vertexBuffer = mGenericTexturePlaneVertexBuffers[planeIndex].vertexBuffer;

        //
        // Populate the texture quad
        //

        TextureAtlasFrameMetadata const & frame = mGenericTextureAtlasMetadata.GetFrameMetadata(textureFrameId);

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
            angle,
            alpha,
            lightSensitivity);

        // Top-Right
        vertexBuffer.emplace_back(
            position,
            vec2f(rightX, topY),
            frame.TextureCoordinatesTopRight,
            static_cast<float>(planeId),
            scale,
            angle,
            alpha,
            lightSensitivity);

        // Bottom-left
        vertexBuffer.emplace_back(
            position,
            vec2f(leftX, bottomY),
            frame.TextureCoordinatesBottomLeft,
            static_cast<float>(planeId),
            scale,
            angle,
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
            angle,
            alpha,
            lightSensitivity);

        // Bottom-left
        vertexBuffer.emplace_back(
            position,
            vec2f(leftX, bottomY),
            frame.TextureCoordinatesBottomLeft,
            static_cast<float>(planeId),
            scale,
            angle,
            alpha,
            lightSensitivity);

        // Bottom-right
        vertexBuffer.emplace_back(
            position,
            vec2f(rightX, bottomY),
            vec2f(frame.TextureCoordinatesTopRight.x, frame.TextureCoordinatesBottomLeft.y),
            static_cast<float>(planeId),
            scale,
            angle,
            alpha,
            lightSensitivity);

        // Update max size among all planes
        mGenericTextureMaxPlaneVertexBufferSize = std::max(mGenericTextureMaxPlaneVertexBufferSize, vertexBuffer.size());
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
    // Vectors
    //

    void UploadVectors(
        size_t count,
        vec2f const * position,
        float const * planeId,
        vec2f const * vector,
        float lengthAdjustment,
        vec4f const & color);

    void RenderEnd();

private:

    void UpdateOrthoMatrices();
    void OnAmbientLightIntensityUpdated();
    void OnWaterContrastUpdated();
    void OnWaterLevelOfDetailUpdated();

    void RenderGenericTextures();
    void RenderVectorArrows();

private:

    ShipId const mShipId;
    size_t mShipCount;
    size_t const mPointCount;
    PlaneId mMaxMaxPlaneId;


    //
    // Types
    //

#pragma pack(push)

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

#pragma pack(pop)

    struct GenericTexturePlaneData
    {
        std::vector<GenericTextureVertex> vertexBuffer;
    };

    //
    // Buffers
    //

    GameOpenGLVBO mPointPositionVBO;
    GameOpenGLVBO mPointLightVBO;
    GameOpenGLVBO mPointWaterVBO;
    GameOpenGLVBO mPointColorVBO;
    GameOpenGLVBO mPointPlaneIdVBO;
    GameOpenGLVBO mPointTextureCoordinatesVBO;

    std::vector<LineElement> mStressedSpringElementBuffer;
    GameOpenGLVBO mStressedSpringElementVBO;

    std::vector<PointElement> mEphemeralPointElementBuffer;
    GameOpenGLVBO mEphemeralPointElementVBO;

    std::vector<GenericTexturePlaneData> mGenericTexturePlaneVertexBuffers;
    size_t mGenericTextureMaxPlaneVertexBufferSize;
    GameOpenGLVBO mGenericTextureVBO;
    size_t mGenericTextureVBOAllocatedSize;

    std::vector<vec3f> mVectorArrowVertexBuffer;
    GameOpenGLVBO mVectorArrowVBO;
    std::optional<vec4f> mVectorArrowColor;

    //
    // Element (index) buffers
    //
    // We use a single VBO for all element indices except stressed springs
    //

    std::vector<PointElement> mPointElementBuffer;
    std::vector<LineElement> mSpringElementBuffer;
    std::vector<LineElement> mRopeElementBuffer;
    std::vector<TriangleElement> mTriangleElementBuffer;

    GameOpenGLVBO mElementVBO;

    // Indices at which these elements begin in the VBO; populated
    // when we upload element indices to the VBO
    size_t mPointElementVBOStartIndex;
    size_t mSpringElementVBOStartIndex;
    size_t mRopeElementVBOStartIndex;
    size_t mTriangleElementVBOStartIndex;


    //
    // VAOs
    //

    GameOpenGLVAO mShipVAO;
    GameOpenGLVAO mGenericTextureVAO;
    GameOpenGLVAO mVectorArrowVAO;


    //
    // Textures
    //

    GameOpenGLTexture mShipTextureOpenGLHandle;
    GameOpenGLTexture mStressedSpringTextureOpenGLHandle;

    GameOpenGLTexture & mGenericTextureAtlasOpenGLHandle;
    TextureAtlasMetadata const & mGenericTextureAtlasMetadata;

private:

    //
    // Managers
    //

    ShaderManager<ShaderManagerTraits> & mShaderManager;

    //
    // Parameters
    //

    ViewModel const & mViewModel;

    float mAmbientLightIntensity;
    float mWaterContrast;
    float mWaterLevelOfDetail;
    ShipRenderMode mShipRenderMode;
    DebugShipRenderMode mDebugShipRenderMode;
    VectorFieldRenderMode mVectorFieldRenderMode;
    bool mShowStressedSprings;

    //
    // Statistics
    //

    RenderStatistics & mRenderStatistics;
};

}