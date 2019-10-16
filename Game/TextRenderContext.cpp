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
    float effectiveAmbientLightIntensity,
    ProgressCallback const & progressCallback)
    : mShaderManager(shaderManager)
    , mScreenToNdcX(2.0f / static_cast<float>(canvasWidth))
    , mScreenToNdcY(2.0f / static_cast<float>(canvasHeight))
    , mEffectiveAmbientLightIntensity(effectiveAmbientLightIntensity)
    , mTextSlots()
    , mCurrentTextSlotGeneration(0)
    , mAreTextSlotsDirty(false)
    , mAreTextSlotVertexBuffersDirty(false)
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


    //
    // Initialize render machinery
    //

    mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();

    // Set hardcoded parameters
    mShaderManager.ActivateProgram<ProgramType::TextNDC>();
    mShaderManager.SetTextureParameters<ProgramType::TextNDC>();

    // Initialize fonts
    for (Font & font : fonts)
    {
        //
        // Initialize texture
        //

        GLuint textureOpenGLHandle;
        glGenTextures(1, &textureOpenGLHandle);

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

        glBindTexture(GL_TEXTURE_2D, 0);

        //
        // Initialize VBO
        //

        GLuint vertexBufferVBOHandle;
        glGenBuffers(1, &vertexBufferVBOHandle);

        //
        // Initialize VAO
        //

        GLuint vaoHandle;
        glGenVertexArrays(1, &vaoHandle);

        glBindVertexArray(vaoHandle);
        CheckOpenGLError();

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferVBOHandle);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Text1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Text1), 4, GL_FLOAT, GL_FALSE, (4 + 1) * sizeof(float), (void*)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Text2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Text2), 1, GL_FLOAT, GL_FALSE, (4 + 1) * sizeof(float), (void*)(4 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);

        //
        // Store font info
        //

        mFontRenderInfos.emplace_back(
            font.Metadata,
            textureOpenGLHandle,
            vertexBufferVBOHandle,
            vaoHandle);
    }


    //
    // Update parameters
    //

    UpdateEffectiveAmbientLightIntensity(mEffectiveAmbientLightIntensity);
}

void TextRenderContext::UpdateEffectiveAmbientLightIntensity(float intensity)
{
    mEffectiveAmbientLightIntensity = intensity;

    // Set parameter
    mShaderManager.ActivateProgram<ProgramType::TextNDC>();
    mShaderManager.SetProgramParameter<ProgramType::TextNDC, ProgramParameterType::EffectiveAmbientLightIntensity>(
        mEffectiveAmbientLightIntensity);
}

void TextRenderContext::RenderStart()
{
}

void TextRenderContext::RenderEnd()
{
    if (mAreTextSlotsDirty)
    {
        //
        // Rebuild all vertex buffers
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
                // Create vertices for this text
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

                mTextSlots[slot].VertexBufferIndexStart = fontRenderInfo.GetVertexBuffer().size();
                mTextSlots[slot].VertexBufferCount = 0;

                float lineOffsetNdc = 0.0f;
                for (std::string const & line : mTextSlots[slot].TextLines)
                {
                    auto vertexCount = fontMetadata.EmitQuadVertices(
                        line.c_str(),
                        line.size(),
                        vec2f(
                            cursorPositionNdc.x,
                            cursorPositionNdc.y - lineOffsetNdc),
                        mTextSlots[slot].Alpha,
                        mScreenToNdcX,
                        mScreenToNdcY,
                        fontRenderInfo.GetVertexBuffer());

                    mTextSlots[slot].VertexBufferCount += vertexCount;

                    lineOffsetNdc += lineHeightIncrementNdc;
                }
            }
        }

        // Remember vertex buffers are dirty now
        mAreTextSlotVertexBuffersDirty = true;

        // Remember slots are not dirty anymore
        mAreTextSlotsDirty = false;
    }

    if (mAreTextSlotVertexBuffersDirty)
    {
        //
        // Re-upload all vertex buffers for each font
        //

        for (auto const & fontRenderInfo : mFontRenderInfos)
        {
            auto const & vertexBuffer = fontRenderInfo.GetVertexBuffer();
            if (!vertexBuffer.empty())
            {
                glBindBuffer(GL_ARRAY_BUFFER, fontRenderInfo.GetVerticesVBOHandle());

                glBufferData(
                    GL_ARRAY_BUFFER,
                    vertexBuffer.size() * sizeof(TextQuadVertex),
                    vertexBuffer.data(),
                    GL_DYNAMIC_DRAW);

                glBindBuffer(GL_ARRAY_BUFFER, 0);

                CheckOpenGLError();
            }
        }

        // Remember vertex buffers are not dirty anymore
        mAreTextSlotVertexBuffersDirty = false;
    }


    //
    // Render vertices
    //

    bool isFirst = true;

    for (auto const & fontRenderInfo : mFontRenderInfos)
    {
        auto const & vertexBuffer = fontRenderInfo.GetVertexBuffer();
        if (!vertexBuffer.empty())
        {
            //
            // Render the vertices for this font
            //

            glBindVertexArray(fontRenderInfo.GetVAOHandle());

            if (isFirst)
            {
                // Activate texture unit (once for all fonts)
                mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();

                // Activate program (once for all fonts)
                mShaderManager.ActivateProgram<ProgramType::TextNDC>();

                isFirst = false;
            }

            // Bind texture
            glBindTexture(GL_TEXTURE_2D, fontRenderInfo.GetFontTextureHandle());
            CheckOpenGLError();

            // Draw vertices
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexBuffer.size()));

            glBindVertexArray(0);
        }
    }
}

}