/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameException.h"
#include "ImageData.h"
#include "TextureDatabase.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <glad/glad.h>

#include <cstdio>
#include <stdexcept>
#include <string>

namespace Render {

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

using GameOpenGLShaderProgram = GameOpenGLObject<GLuint, GameOpenGLProgramDeleter>;
using GameOpenGLVBO = GameOpenGLObject<GLuint, GameOpenGLVBODeleter>;
using GameOpenGLTexture = GameOpenGLObject<GLuint, GameOpenGLTextureDeleter>;

/////////////////////////////////////////////////////////////////////////////////////////
// GameOpenGL
/////////////////////////////////////////////////////////////////////////////////////////

class GameOpenGL
{
public:

    static void InitOpenGL()
    {
        int status = gladLoadGL();
        if (!status)
        {
            throw std::runtime_error("Failed to initialize GLAD");
        }

        //
        // Check OpenGL version
        //

        int versionMaj = 0;
        int versionMin = 0;
        char const * glVersion = (char*)glGetString(GL_VERSION);
        if (nullptr == glVersion)
        {
            throw std::runtime_error("OpenGL completely not supported");
        }

        sscanf(glVersion, "%d.%d", &versionMaj, &versionMin);
        if (versionMaj < 2)
        {
            throw std::runtime_error("This game requires at least OpenGL 2.0 support; the version currently supported by your computer is " + std::string(glVersion));
        }

        //
        // Get some constants
        //

        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &MaxVertexAttributes);
    }

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

    static void UploadMipmappedTexture(TextureFrame baseTexture);

    static void UploadMipmappedTexture(ImageData baseTexture);

private:

    static int MaxVertexAttributes;
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

}