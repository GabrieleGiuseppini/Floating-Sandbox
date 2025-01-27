/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL_Ext.h"

#include <Core/GameException.h>
#include <Core/ImageData.h>
#include <Core/Log.h>

#include <cassert>
#include <cstdio>
#include <string>

/////////////////////////////////////////////////////////////////////////////////////////
// Types
/////////////////////////////////////////////////////////////////////////////////////////

template<typename T, typename TDeleter>
class GameOpenGLObject
{
public:

    GameOpenGLObject()
        : mValue(T())
    {}

    // Takes ownership of object
    explicit GameOpenGLObject(T value)
        : mValue(value)
    {}

    ~GameOpenGLObject()
    {
        TDeleter::Delete(mValue);
    }

    GameOpenGLObject(GameOpenGLObject const & other) = delete;

    GameOpenGLObject(GameOpenGLObject && other)
    {
        mValue = other.mValue;
        other.mValue = T();
    }

    // Takes ownership of object
    GameOpenGLObject & operator=(T value)
    {
        mValue = value;

        return *this;
    }

    GameOpenGLObject & operator=(GameOpenGLObject const & other) = delete;

    GameOpenGLObject & operator=(GameOpenGLObject && other)
    {
        if (!!(mValue))
        {
            TDeleter::Delete(mValue);
        }

        mValue = other.mValue;
        other.mValue = T();

        return *this;
    }

    bool operator !() const
    {
        return mValue == T();
    }

    T operator*() const
    {
        return mValue;
    }

    void reset() noexcept
    {
        if (!!(mValue))
        {
            TDeleter::Delete(mValue);
            mValue = T();
        }
    }

    [[nodiscard]] T release() noexcept
    {
        auto value = mValue;
        mValue = T();
        return value;
    }

private:

    T mValue;
};

struct GameOpenGLProgramDeleter
{
    static void Delete(GLuint p)
    {
        static_assert(GLuint() == 0, "Default value is not zero, i.e. the OpenGL NULL");

        if (p != 0)
        {
            glDeleteProgram(p);
        }
    }
};

struct GameOpenGLVBODeleter
{
    static void Delete(GLuint p)
    {
        static_assert(GLuint() == 0, "Default value is not zero, i.e. the OpenGL NULL");

        if (p != 0)
        {
            glDeleteBuffers(1, &p);
        }
    }
};

struct GameOpenGLVAODeleter
{
    static void Delete(GLuint p)
    {
        static_assert(GLuint() == 0, "Default value is not zero, i.e. the OpenGL NULL");

        if (p != 0)
        {
            glDeleteVertexArrays(1, &p);
        }
    }
};

struct GameOpenGLTextureDeleter
{
    static void Delete(GLuint p)
    {
        static_assert(GLuint() == 0, "Default value is not zero, i.e. the OpenGL NULL");

        if (p != 0)
        {
            glDeleteTextures(1, &p);
        }
    }
};

struct GameOpenGLFramebufferDeleter
{
    static void Delete(GLuint p)
    {
        static_assert(GLuint() == 0, "Default value is not zero, i.e. the OpenGL NULL");

        if (p != 0)
        {
            glDeleteFramebuffers(1, &p);
        }
    }
};

struct GameOpenGLRenderbufferDeleter
{
    static void Delete(GLuint p)
    {
        static_assert(GLuint() == 0, "Default value is not zero, i.e. the OpenGL NULL");

        if (p != 0)
        {
            glDeleteRenderbuffers(1, &p);
        }
    }
};

using GameOpenGLShaderProgram = GameOpenGLObject<GLuint, GameOpenGLProgramDeleter>;
using GameOpenGLVBO = GameOpenGLObject<GLuint, GameOpenGLVBODeleter>;
using GameOpenGLVAO = GameOpenGLObject<GLuint, GameOpenGLVAODeleter>;
using GameOpenGLTexture = GameOpenGLObject<GLuint, GameOpenGLTextureDeleter>;
using GameOpenGLFramebuffer = GameOpenGLObject<GLuint, GameOpenGLFramebufferDeleter>;
using GameOpenGLRenderbuffer = GameOpenGLObject<GLuint, GameOpenGLRenderbufferDeleter>;

/////////////////////////////////////////////////////////////////////////////////////////
// GameOpenGL
/////////////////////////////////////////////////////////////////////////////////////////

class GameOpenGL
{
public:

    static constexpr int MinOpenGLVersionMaj = 2;
    static constexpr int MinOpenGLVersionMin = 0;

public:

    static int MaxVertexAttributes;
    static int MaxViewportWidth;
    static int MaxViewportHeight;
    static int MaxTextureSize;
    static int MaxTextureUnits;
    static int MaxRenderbufferSize;
    static int MaxSupportedOpenGLVersionMajor;
    static int MaxSupportedOpenGLVersionMinor;

    static bool AvoidGlFinish;

public:

    static void InitOpenGL();

    static void CompileShader(
        std::string const & shaderSource,
        GLenum shaderType,
        GameOpenGLShaderProgram const & shaderProgram,
        std::string const & programName);

    static void LinkShaderProgram(
        GameOpenGLShaderProgram const & shaderProgram,
        std::string const & programName);

    static GLint GetParameterLocation(
        GameOpenGLShaderProgram const & shaderProgram,
        std::string const & parameterName);

    static void BindAttributeLocation(
        GameOpenGLShaderProgram const & shaderProgram,
        GLuint attributeIndex,
        std::string const & attributeName);

    static void UploadTexture(RgbaImageData const & texture);

    static void UploadTextureRegion(
        rgbaColor const * textureData,
        int xOffset,
        int yOffset,
        int width,
        int height);

    static void UploadMipmappedTexture(
        RgbaImageData && baseTexture, // In-place
        GLint internalFormat = GL_RGBA);

    static void UploadMipmappedTexture(
        RgbaImageData const & baseTexture, // Non-modifying
        GLint internalFormat = GL_RGBA);

    // Assumes contains tiles aligned on a power-of-two grid
    static void UploadMipmappedAtlasTexture(
        RgbaImageData baseTexture,
        int maxDimension);

    static void Flush();
};

inline void _CheckOpenGLError(char const * file, int line)
{
    GLenum const errorCode = glGetError();
    if (errorCode != GL_NO_ERROR)
    {
        std::string errorCodeString;
        switch (errorCode)
        {
            case GL_INVALID_ENUM:
            {
                errorCodeString = "INVALID_ENUM";
                break;
            }

            case GL_INVALID_VALUE:
            {
                errorCodeString = "INVALID_VALUE";
                break;
            }

            case GL_INVALID_OPERATION:
            {
                errorCodeString = "INVALID_OPERATION";
                break;
            }

            case GL_OUT_OF_MEMORY:
            {
                errorCodeString = "OUT_OF_MEMORY";
                break;
            }

            default:
            {
                errorCodeString = "Other (" + std::to_string(errorCode) + ")";
                break;
            }
        }

        throw GameException("OpenGL Error \"" + errorCodeString + "\" at file " + std::string(file) + ", line " + std::to_string(line));
    }
}

#define CheckOpenGLError() _CheckOpenGLError(__FILE__, __LINE__)
