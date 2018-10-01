/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextureRenderManager.h"

#include "GameException.h"

void TextureRenderManager::UploadGroup(
    TextureGroup const & group,
    ProgressCallback const & progressCallback)
{
    // Make sure we have room for this group
    if (mFrameData.size() < static_cast<size_t>(group.Group) + 1)
        mFrameData.resize(static_cast<size_t>(group.Group) + 1);
    std::vector<FrameData> & frameDataGroup = mFrameData[static_cast<size_t>(group.Group)];

    float totalFramesCount = static_cast<float>(group.GetFrameCount());
    float currentFramesCount = 0;

    for (TextureFrameSpecification const & frameSpec : group.GetFrameSpecifications())
    {
        // Load frame
        TextureFrame frame = frameSpec.LoadFrame();

        // Notify progress
        currentFramesCount += 1.0f;
        progressCallback(currentFramesCount / totalFramesCount, "Loading texture group...");

        // Create OpenGL handle
        GLuint openGLHandle;
        glGenTextures(1, &openGLHandle);

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, openGLHandle);

        // Set repeat mode
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload texture data
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame.Metadata.Size.Width, frame.Metadata.Size.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame.Data.get());
        if (GL_NO_ERROR != glGetError())
        {
            throw GameException("Error uploading texture onto GPU");
        }

        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);

        // Store data
        assert(frameSpec.Metadata.FrameId.FrameIndex == frameDataGroup.size());
        frameDataGroup.emplace_back(
            frameSpec.Metadata,
            openGLHandle);
    }
}

void TextureRenderManager::UploadMipmappedGroup(
    TextureGroup const & group,
    ProgressCallback const & progressCallback)
{
    // Make sure we have room for this group
    if (mFrameData.size() < static_cast<size_t>(group.Group) + 1)
        mFrameData.resize(static_cast<size_t>(group.Group) + 1);
    std::vector<FrameData> & frameDataGroup = mFrameData[static_cast<size_t>(group.Group)];

    float totalFramesCount = static_cast<float>(group.GetFrameCount());
    float currentFramesCount = 0;

    for (TextureFrameSpecification const & frameSpec : group.GetFrameSpecifications())
    {
        // Load frame
        TextureFrame frame = frameSpec.LoadFrame();

        // Notify progress
        currentFramesCount += 1.0f;
        progressCallback(currentFramesCount / totalFramesCount, "Loading textures...");

        // Create OpenGL handle
        GLuint openGLHandle;
        glGenTextures(1, &openGLHandle);

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, openGLHandle);

        // Upload texture
        GameOpenGL::UploadMipmappedTexture(std::move(frame));

        // Set repeat mode
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
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

void TextureRenderManager::AddRenderPolygon(
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