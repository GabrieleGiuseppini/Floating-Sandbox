/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameOpenGL/GameOpenGL.h>

#include <vector>

/*
 * This class maintains a number of textures uploaded to the GPU.
 */
template<typename TFrameEnum>
class UploadedTextureManager
{
public:

    template<typename TContentBuffer>
    void UploadFrame(
        TFrameEnum frameIndex,
        TContentBuffer const & frameImage,
        GLint internalFormat,
        GLenum format,
        GLenum type,
        GLint minFilter)
    {
        size_t f = static_cast<size_t>(frameIndex);

        // Make sure we have room for this group
        if (mOpenGLHandles.size() < f + 1)
            mOpenGLHandles.resize(f + 1);

        if (!mOpenGLHandles[f])
        {
            // Create OpenGL handle
            GLuint openGLHandle;
            glGenTextures(1, &openGLHandle);
            mOpenGLHandles[f] = openGLHandle;
        }

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, *mOpenGLHandles[f]);
        CheckOpenGLError();

        // Set repeat mode
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload texture data
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, frameImage.Size.width, frameImage.Size.height, 0, format, type, frameImage.Data.get());
        if (GL_NO_ERROR != glGetError())
        {
            throw GameException("Error uploading texture onto GPU");
        }

        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    inline void BindTexture(TFrameEnum frameIndex) const
    {
        glBindTexture(
            GL_TEXTURE_2D,
            GetOpenGLHandle(frameIndex));
    }

    inline GLuint GetOpenGLHandle(TFrameEnum frameIndex) const
    {
        return *mOpenGLHandles[static_cast<size_t>(frameIndex)];
    }

private:

    std::vector<GameOpenGLTexture> mOpenGLHandles; // Indexed by enum cast to integral
};
