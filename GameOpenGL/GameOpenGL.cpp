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

void GameOpenGL::InitOpenGL()
{
    int status = gladLoadGL();
    if (!status)
    {
        throw std::runtime_error("Sorry, but this game requires OpenGL and it seems your computer does not support it; the error is: failed to initialize GLAD");
    }

    //
    // Check OpenGL version
    //

    if (GLVersion.major < MinOpenGLVersionMaj
        || (GLVersion.major == MinOpenGLVersionMaj && GLVersion.minor < MinOpenGLVersionMin))
    {
        throw GameException(
            std::string("Sorry, but this game requires at least OpenGL ")
            + std::to_string(MinOpenGLVersionMaj) + "." + std::to_string(MinOpenGLVersionMin)
            + ", while the version currently supported by your computer is "
            + std::to_string(GLVersion.major) + "." + std::to_string(GLVersion.minor));
    }

    //
    // Get some constants
    //

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &MaxVertexAttributes);


    //
    // Log capabilities
    //

    LogCapabilities();
}

void GameOpenGL::LogCapabilities()
{
    LogMessage("OpenGL version: ", GLVersion.major, ".", GLVersion.minor);
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

void GameOpenGL::UploadTexture(ImageData texture)
{
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.Size.Width, texture.Size.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture.Data.get());
    GLenum glError = glGetError();
    if (GL_NO_ERROR != glError)
    {
        throw GameException("Error uploading texture onto GPU: " + std::to_string(glError));
    }
}

void GameOpenGL::UploadMipmappedTexture(ImageData baseTexture)
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

    std::unique_ptr<unsigned char const []> readBuffer(std::move(baseTexture.Data));
    std::unique_ptr<unsigned char []> writeBuffer;

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
        writeBuffer.reset(new unsigned char[width * height * 4]);

        // Create new buffer
        unsigned char const * rp = readBuffer.get();
        unsigned char * wp = writeBuffer.get();
        for (int h = 0; h < height; ++h)
        {
            for (int w = 0; w < width; ++w)
            {
                //
                // Apply box filter
                //

                int wIndex = ((h * width) + w) * 4;

                int rIndex = (((h * 2) * readImageSize.Width) + (w * 2)) * 4;
                int rIndexNextLine = ((((h * 2) + 1) * readImageSize.Width) + (w * 2)) * 4;

                for (int comp = 0; comp < 4; ++comp)
                {
                    int sum = 0;
                    int count = 0;

                    sum += static_cast<int>(rp[rIndex + comp]);
                    ++count;

                    if (readImageSize.Width > 1)
                    {
                        sum += static_cast<int>(rp[rIndex + 4 + comp]);
                        ++count;
                    }

                    if (readImageSize.Height > 1)
                    {
                        sum += static_cast<int>(rp[rIndexNextLine + comp]);
                        ++count;

                        if (readImageSize.Width > 1)
                        {
                            sum += static_cast<int>(rp[rIndexNextLine + 4 + comp]);
                            ++count;
                        }
                    }


                    wp[wIndex + comp] = static_cast<unsigned char>(sum / count);
                }
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
    ImageData baseTexture,
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

    std::unique_ptr<unsigned char const[]> readBuffer(std::move(baseTexture.Data));
    std::unique_ptr<unsigned char[]> writeBuffer;

    GLint lastUploadedTextureLevel = 0;
    for (int divisor = 2; maxDimension / divisor >= 1; divisor *= 2)
    {
        // Calculate dimensions of new write buffer
        int newWidth = std::max(1, baseTexture.Size.Width / divisor);
        int newHeight = std::max(1, baseTexture.Size.Height / divisor);

        // Allocate new write buffer
        writeBuffer.reset(new unsigned char[newWidth * newHeight * 4]);

        // Populate write buffer
        unsigned char const * rp = readBuffer.get();
        unsigned char * wp = writeBuffer.get();
        for (int h = 0; h < newHeight; ++h)
        {
            size_t frameIndexInReadBuffer = (h * 2) * (newWidth * 2) * 4;
            size_t frameIndexInWriteBuffer = (h) * (newWidth) * 4;

            for (int w = 0; w < newWidth; ++w)
            {
                //
                // Calculate and store average of the four neighboring pixels whose bottom-left corner is at (w*2, h*2)
                //

                float components[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

                for (int c = 0; c < 4; ++c)
                {
                    components[c] += static_cast<float>(rp[frameIndexInReadBuffer + w * 2 * 4 + c]);
                }

                for (int c = 0; c < 4; ++c)
                {
                    components[c] += static_cast<float>(rp[frameIndexInReadBuffer + w * 2 * 4 + c + 4]);
                }

                for (int c = 0; c < 4; ++c)
                {
                    components[c] += static_cast<float>(rp[frameIndexInReadBuffer + ((newWidth * 2) * 4) + w * 2 * 4 + c]);
                }

                for (int c = 0; c < 4; ++c)
                {
                    components[c] += static_cast<float>(rp[frameIndexInReadBuffer + ((newWidth * 2) * 4) + w * 2 * 4 + c + 4]);
                }

                // Store
                for (int c = 0; c < 4; ++c)
                {
                    wp[frameIndexInWriteBuffer + w * 4 + c] =
                        static_cast<unsigned char>(components[c] / 4.0f);
                }
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