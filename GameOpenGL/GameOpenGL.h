/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL_Ext.h"

#include <GameCore/GameException.h>
#include <GameCore/ImageData.h>
#include <GameCore/Log.h>

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

    GameOpenGLObject(T value)
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

    GameOpenGLObject & operator=(GameOpenGLObject const & other) = delete;

    GameOpenGLObject & operator=(GameOpenGLObject && other)
    {
        TDeleter::Delete(mValue);
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

    T release() noexcept
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

template <GLenum TTarget>
struct GameOpenGLMappedBufferDeleter
{
    static void Delete(void * p)
    {
        using ptr = void *;
        static_assert(ptr() == nullptr, "Default value is not nullptr, i.e. the OpenGL NULL");

        if (p != nullptr)
        {
            glUnmapBuffer(TTarget);
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
using GameOpenGLTexture = GameOpenGLObject<GLuint, GameOpenGLTextureDeleter>;
template <GLenum TTarget>
using GameOpenGLMappedBuffer = GameOpenGLObject<void *, GameOpenGLMappedBufferDeleter<TTarget>>;
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
    static int MaxRenderbufferSize;

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

    static void UploadTexture(ImageData texture);

    static void UploadMipmappedTexture(ImageData baseTexture);

    static void UploadMipmappedPowerOfTwoTexture(
        ImageData baseTexture,
        int maxDimension);

    template <GLenum TTarget>
    static GameOpenGLMappedBuffer<TTarget> MapBuffer(GLenum access)
    {
        void * pointer = glMapBuffer(TTarget, access);
        if (pointer == nullptr)
        {
            throw GameException("Cannot map buffer");
        }

        return GameOpenGLMappedBuffer<TTarget>(pointer);
    }

    template <GLenum TTarget>
    static void UnmapBuffer(GameOpenGLMappedBuffer<TTarget> buffer)
    {
        assert(!!buffer);

        auto result = glUnmapBuffer(TTarget);
        if (result == GL_FALSE)
        {
            throw GameException("Cannot unmap buffer");
        }

        buffer.release();
    }

    static void Flush();
};

inline void _CheckOpenGLError(char const * file, int line)
{
    GLenum errorCode = glGetError();
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
