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
template <typename TextureGroups>
class UploadedTextureManager
{
public:

    // Assumption: all previous frames have been uploaded already
    void UploadNextFrame(
        TextureGroup<TextureGroups> const & group,
        TextureFrameIndex const & frameIndex,
        GLint minFilter);

    void UploadGroup(
        TextureGroup<TextureGroups> const & group,
        GLint minFilter,
        ProgressCallback const & progressCallback);

    void UploadMipmappedGroup(
        TextureGroup<TextureGroups> const & group,
        GLint minFilter,
        ProgressCallback const & progressCallback);

    inline TextureFrameMetadata<TextureGroups> const & GetFrameMetadata(TextureFrameId<TextureGroups> const & frameId) const
    {
        return GetFrameMetadata(
            frameId.Group,
            frameId.FrameIndex);
    }

    inline TextureFrameMetadata<TextureGroups> const & GetFrameMetadata(
        TextureGroups group,
        TextureFrameIndex frameIndex) const
    {
        assert(static_cast<size_t>(group) < mFrameData.size());
        assert(frameIndex < mFrameData[static_cast<size_t>(group)].size());

        return mFrameData[static_cast<size_t>(group)][frameIndex].Metadata;
    }

    inline void BindTexture(TextureFrameId<TextureGroups> const & frameId) const
    {
        glBindTexture(
            GL_TEXTURE_2D,
            GetOpenGLHandle(
                frameId.Group,
                frameId.FrameIndex));
    }

    inline GLuint GetOpenGLHandle(TextureFrameId<TextureGroups> const & frameId) const
    {
        return GetOpenGLHandle(
            frameId.Group,
            frameId.FrameIndex);
    }

    inline GLuint GetOpenGLHandle(
        TextureGroups group,
        TextureFrameIndex frameIndex) const
    {
        assert(static_cast<size_t>(group) < mFrameData.size());
        assert(frameIndex < mFrameData[static_cast<size_t>(group)].size());

        return *(mFrameData[static_cast<size_t>(group)][frameIndex].OpenGLHandle);
    }

private:

    struct FrameData
    {
        TextureFrameMetadata<TextureGroups> Metadata;
        GameOpenGLTexture OpenGLHandle;

        FrameData(
            TextureFrameMetadata<TextureGroups> const & metadata,
            GLuint openGLHandle)
            : Metadata(metadata)
            , OpenGLHandle(openGLHandle)
        {}
    };

    std::vector<std::vector<FrameData>> mFrameData;
};

}