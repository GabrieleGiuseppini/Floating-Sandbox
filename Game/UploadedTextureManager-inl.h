/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "UploadedTextureManager.h"

#include <GameCore/GameException.h>

namespace Render {

template <typename TextureGroups>
void UploadedTextureManager<TextureGroups>::UploadNextFrame(
    TextureGroup<TextureGroups> const & group,
    TextureFrameIndex const & frameIndex,
    GLint minFilter)
{
    // Make sure we have room for this group
    if (mFrameData.size() < static_cast<size_t>(group.Group) + 1)
        mFrameData.resize(static_cast<size_t>(group.Group) + 1);
    std::vector<FrameData> & frameDataGroup = mFrameData[static_cast<size_t>(group.Group)];

    // Get frame specification
    auto const & frameSpec = group.GetFrameSpecification(frameIndex);

    // Load frame
    auto frame = frameSpec.LoadFrame();

    // Create OpenGL handle
    GLuint openGLHandle;
    glGenTextures(1, &openGLHandle);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, openGLHandle);
    CheckOpenGLError();

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame.Metadata.Size.Width, frame.Metadata.Size.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame.TextureData.Data.get());
    if (GL_NO_ERROR != glGetError())
    {
        throw GameException("Error uploading texture onto GPU");
    }

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    // Store metadata
    assert(frameSpec.Metadata.FrameId.FrameIndex == frameDataGroup.size());
    frameDataGroup.emplace_back(
        frameSpec.Metadata,
        openGLHandle);
}

template <typename TextureGroups>
void UploadedTextureManager<TextureGroups>::UploadGroup(
    TextureGroup<TextureGroups> const & group,
    GLint minFilter,
    ProgressCallback const & progressCallback)
{
    float totalFramesCount = static_cast<float>(group.GetFrameCount());
    float currentFramesCount = 0;

    for (auto const & frameSpec : group.GetFrameSpecifications())
    {
        // Upload frame
        UploadNextFrame(
            group,
            frameSpec.Metadata.FrameId.FrameIndex,
            minFilter);

        // Notify progress
        currentFramesCount += 1.0f;
        progressCallback(currentFramesCount / totalFramesCount, "Loading texture group...");
    }
}

template <typename TextureGroups>
void UploadedTextureManager<TextureGroups>::UploadMipmappedGroup(
    TextureGroup<TextureGroups> const & group,
    GLint minFilter,
    ProgressCallback const & progressCallback)
{
    // Make sure we have room for this group
    if (mFrameData.size() < static_cast<size_t>(group.Group) + 1)
        mFrameData.resize(static_cast<size_t>(group.Group) + 1);
    std::vector<FrameData> & frameDataGroup = mFrameData[static_cast<size_t>(group.Group)];

    float totalFramesCount = static_cast<float>(group.GetFrameCount());
    float currentFramesCount = 0;

    for (auto const & frameSpec : group.GetFrameSpecifications())
    {
        // Load frame
        auto frame = frameSpec.LoadFrame();

        // Notify progress
        currentFramesCount += 1.0f;
        progressCallback(currentFramesCount / totalFramesCount, "Loading textures...");

        // Create OpenGL handle
        GLuint openGLHandle;
        glGenTextures(1, &openGLHandle);

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, openGLHandle);

        // Upload texture
        GameOpenGL::UploadMipmappedTexture(std::move(frame.TextureData));

        // Set repeat mode
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);

        // Store data
        assert(frameSpec.Metadata.FrameId.FrameIndex == frameDataGroup.size());
        frameDataGroup.emplace_back(
            frameSpec.Metadata,
            openGLHandle);
    }
}

}