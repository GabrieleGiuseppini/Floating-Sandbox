/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameOpenGL.h"

#include <GameCore/GameMath.h>

#include <algorithm>
#include <memory>
#include <numeric>

int GameOpenGL::MaxVertexAttributes = 0;
int GameOpenGL::MaxViewportWidth = 0;
int GameOpenGL::MaxViewportHeight = 0;
int GameOpenGL::MaxTextureSize = 0;
int GameOpenGL::MaxRenderbufferSize = 0;

void GameOpenGL::InitOpenGL()
{
    int status = gladLoadGL();
    if (!status)
    {
        throw GameException("We are sorry, but this game requires OpenGL and it seems your graphics drivers does not support it; the error is: failed to initialize GLAD");
    }

    //
    // Check OpenGL version
    //

    LogMessage("OpenGL version: ", GLVersion.major, ".", GLVersion.minor);

    if (GLVersion.major < MinOpenGLVersionMaj
        || (GLVersion.major == MinOpenGLVersionMaj && GLVersion.minor < MinOpenGLVersionMin))
    {
        throw GameException(
            std::string("We are sorry, but this game requires at least OpenGL ")
            + std::to_string(MinOpenGLVersionMaj) + "." + std::to_string(MinOpenGLVersionMin)
            + ", while the version currently supported by your graphics driver is "
            + std::to_string(GLVersion.major) + "." + std::to_string(GLVersion.minor));
    }


    //
    // Init our extensions
    //

    InitOpenGLExt();


    //
    // Get some constants
    //

    GLint tmpConstant;

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &tmpConstant);
    MaxVertexAttributes = tmpConstant;
    LogMessage("GL_MAX_VERTEX_ATTRIBS=", MaxVertexAttributes);

    GLint maxViewportDims[2];
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &(maxViewportDims[0]));
    MaxViewportWidth = maxViewportDims[0];
    MaxViewportHeight = maxViewportDims[1];
    LogMessage("GL_MAX_VIEWPORT_DIMS=", MaxViewportWidth, "x", MaxViewportHeight);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &tmpConstant);
    MaxTextureSize = tmpConstant;
    LogMessage("GL_MAX_TEXTURE_SIZE=", MaxTextureSize);

    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &tmpConstant);
    MaxRenderbufferSize = tmpConstant;
    LogMessage("GL_MAX_RENDERBUFFER_SIZE=", MaxRenderbufferSize);
}

void GameOpenGL::CompileShader(
    std::string const & shaderSource,
    GLenum shaderType,
    GameOpenGLShaderProgram const & shaderProgram,
    std::string const & programName)
{
    char const * shaderSourceCString = shaderSource.c_str();
    std::string const shaderTypeName = (shaderType == GL_VERTEX_SHADER) ? "vertex" : "fragment";

    // Set source
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSourceCString, NULL);
    GLenum glError = glGetError();
    if (GL_NO_ERROR != glError)
    {
        throw GameException("Error setting " + shaderTypeName + " shader source for program \"" + programName + "\"");
    }

    // Compile
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
        throw GameException("Error compiling " + shaderTypeName + " shader: " + std::string(infoLog));
    }

    // Attach to program
    glAttachShader(*shaderProgram, shader);
    glError = glGetError();
    if (GL_NO_ERROR != glError)
    {
        throw GameException("Error attaching compiled " + shaderTypeName + " shader to program \"" + programName + "\"");
    }

    // Delete shader
    glDeleteShader(shader);
}

void GameOpenGL::LinkShaderProgram(
    GameOpenGLShaderProgram const & shaderProgram,
    std::string const & programName)
{
    glLinkProgram(*shaderProgram);

    // Check
    int success;
    glGetProgramiv(*shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(*shaderProgram, sizeof(infoLog), NULL, infoLog);
        throw GameException("Error linking " + programName + " shader program: " + std::string(infoLog));
    }
}

GLint GameOpenGL::GetParameterLocation(
    GameOpenGLShaderProgram const & shaderProgram,
    std::string const & parameterName)
{
    GLint parameterLocation = glGetUniformLocation(*shaderProgram, parameterName.c_str());

    GLenum glError = glGetError();
    if (parameterLocation == -1
        || GL_NO_ERROR != glError)
    {
        throw GameException("Cannot retrieve location of parameter \"" + parameterName + "\"");
    }

    return parameterLocation;
}

void GameOpenGL::BindAttributeLocation(
    GameOpenGLShaderProgram const & shaderProgram,
    GLuint attributeIndex,
    std::string const & attributeName)
{
    glBindAttribLocation(
        *shaderProgram,
        attributeIndex,
        attributeName.c_str());

    GLenum glError = glGetError();
    if (GL_NO_ERROR != glError)
    {
        throw GameException("Error binding attribute location for attribute \"" + attributeName + "\"");
    }
}

void GameOpenGL::UploadTexture(RgbaImageData texture)
{
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.Size.Width, texture.Size.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture.Data.get());
    GLenum glError = glGetError();
    if (GL_NO_ERROR != glError)
    {
        throw GameException("Error uploading texture onto GPU: " + std::to_string(glError));
    }
}

void GameOpenGL::UploadMipmappedTexture(RgbaImageData baseTexture)
{
    //
    // Upload base image
    //

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, baseTexture.Size.Width, baseTexture.Size.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, baseTexture.Data.get());
    GLenum glError = glGetError();
    if (GL_NO_ERROR != glError)
    {
        throw GameException("Error uploading texture onto GPU: " + std::to_string(glError));
    }


    //
    // Create minified textures
    //

    ImageSize readImageSize(baseTexture.Size);

    std::unique_ptr<rgbaColor const []> readBuffer(std::move(baseTexture.Data));
    std::unique_ptr<rgbaColor[]> writeBuffer;

    for (GLint textureLevel = 1; ; ++textureLevel)
    {
        if (readImageSize.Width == 1 && readImageSize.Height == 1)
        {
            // We're done!
            break;
        }

        // Calculate dimensions of new write buffer
        int width = std::max(1, readImageSize.Width / 2);
        int height = std::max(1, readImageSize.Height / 2);

        // Allocate new write buffer
        writeBuffer.reset(new rgbaColor[width * height]);

        // Create new buffer
        rgbaColor const * rp = readBuffer.get();
        rgbaColor * wp = writeBuffer.get();
        for (int h = 0; h < height; ++h)
        {
            for (int w = 0; w < width; ++w)
            {
                //
                // Apply box filter
                //

                int wIndex = ((h * width) + w);

                int rIndex = (((h * 2) * readImageSize.Width) + (w * 2));
                int rIndexNextLine = ((((h * 2) + 1) * readImageSize.Width) + (w * 2));

                rgbaColorAccumulation sum(rp[rIndex]);

                if (readImageSize.Width > 1)
                    sum += rp[rIndex + 1];

                if (readImageSize.Height > 1)
                {
                    sum += rp[rIndexNextLine];

                    if (readImageSize.Width > 1)
                        sum += rp[rIndexNextLine + 1];
                }

                wp[wIndex] = sum.toRgbaColor();
            }
        }

        // Upload write buffer
        glTexImage2D(GL_TEXTURE_2D, textureLevel, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, writeBuffer.get());
        glError = glGetError();
        if (GL_NO_ERROR != glError)
        {
            throw GameException("Error uploading minified texture onto GPU: " + std::to_string(glError));
        }

        // Swap buffer
        readImageSize = ImageSize(width, height);
        readBuffer = std::move(writeBuffer);
    }
}

void GameOpenGL::UploadMipmappedPowerOfTwoTexture(
    RgbaImageData baseTexture,
    int maxDimension)
{
    assert(baseTexture.Size.Width == CeilPowerOfTwo(baseTexture.Size.Width));
    assert(baseTexture.Size.Height == CeilPowerOfTwo(baseTexture.Size.Height));


    //
    // Upload base image
    //

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        baseTexture.Size.Width,
        baseTexture.Size.Height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        baseTexture.Data.get());

    CheckOpenGLError();


    //
    // Create minified textures
    //

    std::unique_ptr<rgbaColor const[]> readBuffer(std::move(baseTexture.Data));
    std::unique_ptr<rgbaColor[]> writeBuffer;

    GLint lastUploadedTextureLevel = 0;
    for (int divisor = 2; maxDimension / divisor >= 1; divisor *= 2)
    {
        // Calculate dimensions of new write buffer
        int newWidth = std::max(1, baseTexture.Size.Width / divisor);
        int newHeight = std::max(1, baseTexture.Size.Height / divisor);

        // Allocate new write buffer
        writeBuffer.reset(new rgbaColor[newWidth * newHeight]);

        // Populate write buffer
        rgbaColor const * rp = readBuffer.get();
        rgbaColor * wp = writeBuffer.get();
        for (int h = 0; h < newHeight; ++h)
        {
            size_t frameIndexInReadBuffer = (h * 2) * (newWidth * 2);
            size_t frameIndexInWriteBuffer = (h) * (newWidth);

            for (int w = 0; w < newWidth; ++w)
            {
                //
                // Calculate and store average of the four neighboring pixels whose bottom-left corner is at (w*2, h*2)
                //

                rgbaColorAccumulation sum;

                sum += rp[frameIndexInReadBuffer + w * 2];
                sum += rp[frameIndexInReadBuffer + w * 2 + 1];
                sum += rp[frameIndexInReadBuffer + newWidth * 2 + w * 2];
                sum += rp[frameIndexInReadBuffer + newWidth * 2 + w * 2 + 1];

                wp[frameIndexInWriteBuffer + w] = sum.toRgbaColor();
            }
        }

        // Upload write buffer
        ++lastUploadedTextureLevel;
        glTexImage2D(GL_TEXTURE_2D, lastUploadedTextureLevel, GL_RGBA, newWidth, newHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, writeBuffer.get());
        CheckOpenGLError();

        // Swap buffer
        readBuffer = std::move(writeBuffer);
    }

    // Set max mipmap level
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, lastUploadedTextureLevel);
    CheckOpenGLError();
}

void GameOpenGL::Flush()
{
    // We do it here to have this call in the stack, helping
    // performance profiling
    glFlush();
}