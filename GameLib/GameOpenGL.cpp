/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameOpenGL.h"

#include "GameException.h"

#include <algorithm>
#include <memory>

void GameOpenGL::CompileShader(
    char const * shaderSource,
    GLenum shaderType,
    GameOpenGLShaderProgram const & shaderProgram)
{
    // Compile
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);

    // Check
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
        throw GameException("Error Compiling vertex shader: " + std::string(infoLog));
    }

    // Attach to program
    glAttachShader(*shaderProgram, shader);

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
    if (parameterLocation == -1)
    {
        throw GameException("ERROR retrieving location of parameter \"" + parameterName + "\"");
    }

    return parameterLocation;
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
        throw GameException("Error uploading ship texture onto GPU: " + std::to_string(glError));
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
            throw GameException("Error uploading minified ship texture onto GPU: " + std::to_string(glError));
        }

        // Swap buffer
        readImageSize = ImageSize(width, height);
        readBuffer = std::move(writeBuffer);
    }
}
