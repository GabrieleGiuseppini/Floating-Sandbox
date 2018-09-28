/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL.h"
#include "ProgressCallback.h"
#include "TextureDatabase.h"

#include <cassert>
#include <map>

class TextureRenderManager
{
public:

    void UploadGroup(
        TextureGroup const & group,
        ProgressCallback const & progressCallback);

    void UploadMipmappedGroup(
        TextureGroup const & group,
        ProgressCallback const & progressCallback);

    inline TextureFrameMetadata const & GetFrameMetadata(TextureFrameId const & frameId) const
    {
        assert(0 != mFrameData.count(frameId));

        return mFrameData.at(frameId).Metadata;
    }

    inline GLuint GetOpenGLHandle(TextureFrameId const & frameId) const
    {
        assert(0 != mFrameData.count(frameId));

        return mFrameData.at(frameId).OpenGLHandle;
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

    std::map<TextureFrameId, FrameData> mFrameData;
};
