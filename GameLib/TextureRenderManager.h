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

    void AddRenderPolygon(
        TextureFrameId const & textureFrameId,
        vec2f const & position,
        float scale,
        std::optional<std::pair<vec2f, vec2f>> const & orientation,
        std::vector<TextureRenderPolygonVertex> & renderPolygonVertexBuffer) const;

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
