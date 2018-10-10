/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL.h"
#include "ProgressCallback.h"
#include "TextureDatabase.h"
#include "TextureTypes.h"
#include "Vectors.h"

#include <cassert>
#include <optional>
#include <vector>

class TextureRenderManager
{
public:

    void UploadGroup(
        TextureGroup const & group,
        ProgressCallback const & progressCallback);

    void UploadMipmappedGroup(
        TextureGroup const & group,
        ProgressCallback const & progressCallback);

    inline void EmitRenderPolygon(
        TextureFrameId const & textureFrameId,
        vec2f const & position,
        float scale,
        std::optional<std::pair<vec2f, vec2f>> const & orientation,
        std::vector<TextureRenderPolygonVertex> & renderPolygonVertexBuffer) const
    {
        //
        // Calculate rotation matrix, based off cosine of the angle between rotation offset and rotation base
        //

        // First columns
        vec2f rotationMatrixX;
        vec2f rotationMatrixY;

        if (!!orientation)
        {
            float const alpha = orientation->first.angle(orientation->second);

            float const cosAlpha = cos(alpha);
            float const sinAlpha = sin(alpha);

            rotationMatrixX = vec2f(cosAlpha, sinAlpha);
            rotationMatrixY = vec2f(-sinAlpha, cosAlpha);
        }
        else
        {
            rotationMatrixX = vec2f(1.0f, 0.0f);
            rotationMatrixY = vec2f(0.0f, 1.0f);
        }


        //
        // Calculate rectangle vertices
        //

        TextureFrameMetadata const & frameMetadata = this->GetFrameMetadata(textureFrameId);

        // Relative to position
        float const relativeLeftX = -frameMetadata.AnchorWorldX * scale;
        float const relativeRightX = (frameMetadata.WorldWidth - frameMetadata.AnchorWorldX) * scale;
        float const relativeTopY = (frameMetadata.WorldHeight - frameMetadata.AnchorWorldY) * scale;
        float const relativeBottomY = -frameMetadata.AnchorWorldY * scale;

        vec2f const relativeTopLeft{ relativeLeftX, relativeTopY };
        vec2f const relativeTopRight{ relativeRightX, relativeTopY };
        vec2f const relativeBottomLeft{ relativeLeftX, relativeBottomY };
        vec2f const relativeBottomRight{ relativeRightX, relativeBottomY };

        //
        // Create vertices
        //

        renderPolygonVertexBuffer.emplace_back(
            vec2f(relativeTopLeft.dot(rotationMatrixX) + position.x, relativeTopLeft.dot(rotationMatrixY) + position.y),
            vec2f(0.0f, 1.0f),
            frameMetadata.HasOwnAmbientLight ? 0.0f : 1.0f);

        renderPolygonVertexBuffer.emplace_back(
            vec2f(relativeTopRight.dot(rotationMatrixX) + position.x, relativeTopRight.dot(rotationMatrixY) + position.y),
            vec2f(1.0f, 1.0f),
            frameMetadata.HasOwnAmbientLight ? 0.0f : 1.0f);

        renderPolygonVertexBuffer.emplace_back(
            vec2f(relativeBottomLeft.dot(rotationMatrixX) + position.x, relativeBottomLeft.dot(rotationMatrixY) + position.y),
            vec2f(0.0f, 0.0f),
            frameMetadata.HasOwnAmbientLight ? 0.0f : 1.0f);

        renderPolygonVertexBuffer.emplace_back(
            vec2f(relativeBottomRight.dot(rotationMatrixX) + position.x, relativeBottomRight.dot(rotationMatrixY) + position.y),
            vec2f(1.0f, 0.0f),
            frameMetadata.HasOwnAmbientLight ? 0.0f : 1.0f);
    }

    inline TextureFrameMetadata const & GetFrameMetadata(TextureFrameId const & frameId) const
    {
        return GetFrameMetadata(
            frameId.Group,
            frameId.FrameIndex);
    }

    inline TextureFrameMetadata const & GetFrameMetadata(
        TextureGroupType group,
        TextureFrameIndex frameIndex) const
    {
        assert(static_cast<size_t>(group) < mFrameData.size());
        assert(frameIndex < mFrameData[static_cast<size_t>(group)].size());

        return mFrameData[static_cast<size_t>(group)][frameIndex].Metadata;
    }

    inline void BindTexture(TextureFrameId const & frameId) const
    {
        glBindTexture(
            GL_TEXTURE_2D,
            GetOpenGLHandle(
                frameId.Group,
                frameId.FrameIndex));
    }

    inline GLuint GetOpenGLHandle(TextureFrameId const & frameId) const
    {
        return GetOpenGLHandle(
            frameId.Group,
            frameId.FrameIndex);
    }

    inline GLuint GetOpenGLHandle(
        TextureGroupType group,
        TextureFrameIndex frameIndex) const
    {
        assert(static_cast<size_t>(group) < mFrameData.size());
        assert(frameIndex < mFrameData[static_cast<size_t>(group)].size());

        return mFrameData[static_cast<size_t>(group)][frameIndex].OpenGLHandle;
    }

private:

    struct FrameData
    {
        TextureFrameMetadata Metadata;
        GLuint OpenGLHandle;

        FrameData(
            TextureFrameMetadata const & metadata,
            GLuint openGLHandle)
            : Metadata(metadata)
            , OpenGLHandle(openGLHandle)
        {}
    };

    std::vector<std::vector<FrameData>> mFrameData;
};
