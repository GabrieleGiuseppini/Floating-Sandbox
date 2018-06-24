/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ImageData.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <glad/glad.h>

#include <cstdio>
#include <stdexcept>
#include <string>

/////////////////////////////////////////////////////////////////////////////////////////
// Types
/////////////////////////////////////////////////////////////////////////////////////////

template<typename T, typename TDeleter>
class GameOpenGLObject
{
public:

    GameOpenGLObject()
        : mValue(0)
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
        other.mValue = 0;
    }

    GameOpenGLObject & operator=(GameOpenGLObject const & other) = delete;

    GameOpenGLObject & operator=(GameOpenGLObject && other)
    {
        TDeleter::Delete(mValue);
        mValue = other.mValue;
        other.mValue = 0;

        return *this;
    }

    bool operator !() const
    {
        return mValue == 0;
    }

    T operator*() const
    {
        return mValue;
    }

private:
    T mValue;
};

struct GameOpenGLProgramDeleter
{
    static void Delete(GLuint p)
    {
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
    }

    static void CompileShader(
        char const * shaderSource,
        GLenum shaderType,
        GameOpenGLShaderProgram const & shaderProgram);

    static void LinkShaderProgram(
        GameOpenGLShaderProgram const & shaderProgram,
        std::string const & programName);

    static GLint GetParameterLocation(
        GameOpenGLShaderProgram const & shaderProgram,
        std::string const & parameterName);

    static void UploadMipmappedTexture(ImageData baseTexture);
};
