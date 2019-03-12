/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextRenderContext.h"

namespace Render {

TextRenderContext::TextRenderContext(
    ResourceLoader & resourceLoader,
    ShaderManager<ShaderManagerTraits> & shaderManager,
    int canvasWidth,
    int canvasHeight,
    float ambientLightIntensity,
    ProgressCallback const & progressCallback)
    : mShaderManager(shaderManager)
    , mScreenToNdcX(2.0f / static_cast<float>(canvasWidth))
    , mScreenToNdcY(2.0f / static_cast<float>(canvasHeight))
    , mAmbientLightIntensity(ambientLightIntensity)
    , mTextSlots()
    , mCurrentTextSlotGeneration(0)
    , mAreTextSlotsDirty(false)
    , mFontRenderInfos()
{
    //
    // Load fonts
    //

    progressCallback(0.0f, "Loading fonts...");

    std::vector<Font> fonts = Font::LoadAll(
        resourceLoader,
        [&progressCallback](float progress, std::string const & /*message*/)
        {
            progressCallback(progress, "Loading fonts...");
        });

    progressCallback(1.0f, "Loading fonts...");


    //
    // Initialize render machinery
    //

    mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();

    for (Font & font : fonts)
    {
        // Create OpenGL handle for the texture
        GLuint textureOpenGLHandle;
        glGenTextures(1, &textureOpenGLHandle);

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, textureOpenGLHandle);
        CheckOpenGLError();

        // Set repeat mode
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        CheckOpenGLError();

        // Set filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        CheckOpenGLError();

        // Upload texture data
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, font.Texture.Size.Width, font.Texture.Size.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, font.Texture.Data.get());
        CheckOpenGLError();

        // Create vertices VBO
        GLuint vertexBufferVBOHandle;
        glGenBuffers(1, &vertexBufferVBOHandle);

        // Store font info
        mFontRenderInfos.emplace_back(
            font.Metadata,
            textureOpenGLHandle,
            vertexBufferVBOHandle);
    }


    //
    // Update parameters
    //

    UpdateAmbientLightIntensity(mAmbientLightIntensity);
}

void TextRenderContext::UpdateAmbientLightIntensity(float ambientLightIntensity)
{
    mAmbientLightIntensity = ambientLightIntensity;

    // Set parameter
    mShaderManager.ActivateProgram<ProgramType::TextNDC>();
    mShaderManager.SetProgramParameter<ProgramType::TextNDC, ProgramParameterType::AmbientLightIntensity>(
        ambientLightIntensity);
}

void TextRenderContext::RenderStart()
{
}

void TextRenderContext::RenderEnd()
{
    if (mAreTextSlotsDirty)
    {
        //
        // Rebuild all vertices
        //

        // Cleanup
        for (auto & textRenderInfo : mFontRenderInfos)
        {
            textRenderInfo.GetVertexBuffer().clear();
        }

        // Process all slots
        for (size_t slot = 0; slot < mTextSlots.size(); ++slot)
        {
            if (mTextSlots[slot].Generation > 0)
            {
                //
                // Render this text
                //

                FontRenderInfo & fontRenderInfo = mFontRenderInfos[static_cast<size_t>(mTextSlots[slot].Font)];
                FontMetadata const & fontMetadata = fontRenderInfo.GetFontMetadata();

                //
                // Calculate cursor position
                //

                float constexpr MarginScreen = 10.0f;
                float constexpr MarginTopScreen = MarginScreen + 25.0f; // Consider menu bar
                int constexpr MarginLine = 0; // Pixels

                ImageSize textExtent = ImageSize::Zero();
                int lineHeightIncrement = 0;
                for (auto const & line : mTextSlots[slot].TextLines)
                {
                    auto const lineExtent = fontMetadata.CalculateTextExtent(
                        line.c_str(),
                        line.length());

                    textExtent = ImageSize(
                        std::max(textExtent.Width, lineExtent.Width),
                        (textExtent.Height != 0 ? MarginLine : 0)
                        + textExtent.Height + lineExtent.Height);

                    lineHeightIncrement = std::max(lineHeightIncrement, lineExtent.Height + MarginLine);
                }

                float const lineHeightIncrementNdc = lineHeightIncrement * mScreenToNdcY;

                vec2f cursorPositionNdc; // Top-left
                switch (mTextSlots[slot].Position)
                {
                    case TextPositionType::BottomLeft:
                    {
                        cursorPositionNdc = vec2f(
                            -1.f + MarginScreen * mScreenToNdcX,
                            -1.f + (MarginScreen + static_cast<float>(textExtent.Height)) * mScreenToNdcY);

                        break;
                    }

                    case TextPositionType::BottomRight:
                    {
                        cursorPositionNdc = vec2f(
                            1.f - (MarginScreen + static_cast<float>(textExtent.Width)) * mScreenToNdcX,
                            -1.f + (MarginScreen + static_cast<float>(textExtent.Height)) * mScreenToNdcY);

                        break;
                    }

                    case TextPositionType::TopLeft:
                    {
                        cursorPositionNdc = vec2f(
                            -1.f + MarginScreen * mScreenToNdcX,
                            1.f - MarginTopScreen * mScreenToNdcY);

                        break;
                    }

                    case TextPositionType::TopRight:
                    {
                        cursorPositionNdc = vec2f(
                            1.f - (MarginScreen + static_cast<float>(textExtent.Width)) * mScreenToNdcX,
                            1.f - MarginTopScreen * mScreenToNdcY);

                        break;
                    }
                }

                float lineOffsetNdc = 0.0f;
                for (std::string const & line : mTextSlots[slot].TextLines)
                {
                    fontMetadata.EmitQuadVertices(
                        line.c_str(),
                        line.size(),
                        vec2f(
                            cursorPositionNdc.x,
                            cursorPositionNdc.y - lineOffsetNdc),
                        mTextSlots[slot].Alpha,
                        mScreenToNdcX,
                        mScreenToNdcY,
                        fontRenderInfo.GetVertexBuffer());

                    lineOffsetNdc += lineHeightIncrementNdc;
                }
            }
        }

        // Remember slots are not dirty anymore
        mAreTextSlotsDirty = false;
    }


    //
    // Render vertices
    //

    bool isProgramActivated = false;

    for (auto const & fontRenderInfo : mFontRenderInfos)
    {
        auto const & vertexBuffer = fontRenderInfo.GetVertexBuffer();
        if (!vertexBuffer.empty())
        {
            // Activate program (once for all fonts)
            if (!isProgramActivated)
            {
                mShaderManager.ActivateProgram<ProgramType::TextNDC>();
                isProgramActivated = true;
            }

            // Bind VBO
            glBindBuffer(GL_ARRAY_BUFFER, fontRenderInfo.GetVerticesVBOHandle());
            CheckOpenGLError();

            // Describe shared attribute indices
            glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::SharedAttribute0), 4, GL_FLOAT, GL_FALSE, (2 + 2 + 1) * sizeof(float), (void*)0);
            glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::SharedAttribute1));
            glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::SharedAttribute1), 1, GL_FLOAT, GL_FALSE, (2 + 2 + 1) * sizeof(float), (void*)((2 + 2) * sizeof(float)));
            CheckOpenGLError();

            // Enable vertex attribute 0
            glEnableVertexAttribArray(0);
            CheckOpenGLError();

            // Upload vertex buffer
            glBufferData(
                GL_ARRAY_BUFFER,
                vertexBuffer.size() * sizeof(TextQuadVertex),
                vertexBuffer.data(),
                GL_STATIC_DRAW);
            CheckOpenGLError();

            // Bind texture
            mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();
            glBindTexture(GL_TEXTURE_2D, fontRenderInfo.GetFontTextureHandle());
            CheckOpenGLError();

            // Draw vertices
            glDrawArrays(
                GL_TRIANGLES,
                0,
                static_cast<GLsizei>(vertexBuffer.size()));
        }
    }
}

}