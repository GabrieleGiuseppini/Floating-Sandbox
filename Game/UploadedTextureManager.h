/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "RenderCore.h"
#include "TextureDatabase.h"

#include <GameOpenGL/GameOpenGL.h>

#include <GameCore/GameTypes.h>
#include <GameCore/ProgressCallback.h>
#include <GameCore/Vectors.h>

#include <cassert>
#include <optional>
#include <vector>

namespace Render {

/*
 * This class maintains metadata about a number of textures uploaded to the GPU.
 */
class UploadedTextureManager
{
public:

    void UploadGroup(
        TextureGroup const & group,
        ProgressCallback const & progressCallback);

    void UploadMipmappedGroup(
        TextureGroup const & group,
        GLint minFilter,
        ProgressCallback const & progressCallback);

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

        return *(mFrameData[static_cast<size_t>(group)][frameIndex].OpenGLHandle);
    }

private:

    struct FrameData
    {
        TextureFrameMetadata Metadata;
        GameOpenGLTexture OpenGLHandle;

        FrameData(
            TextureFrameMetadata const & metadata,
            GLuint openGLHandle)
            : Metadata(metadata)
            , OpenGLHandle(openGLHandle)
        {}
    };

    std::vector<std::vector<FrameData>> mFrameData;
};

}