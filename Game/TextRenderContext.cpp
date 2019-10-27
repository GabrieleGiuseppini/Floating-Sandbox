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
	//
	, mLines()
	, mLastRenderedTextHandle(0)
	, mAreLinesDirty(false)
    , mFontRenderContexts()
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

    // Initialize font render contexts
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

        mFontRenderContexts.emplace_back(
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

void TextRenderContext::Render()
{
	if (mAreLinesDirty)
	{
		//
		// Rebuild all vertex buffers
		//

		// Cleanup
		for (auto & context : mFontRenderContexts)
		{
			context.GetVertexBuffer().clear();
		}

		// Process all lines
		for (size_t l = 0; l < mLines.size(); ++l)
		{
			auto & line = *(mLines[l]);

			//
			// Create vertices for this line
			//

			FontRenderContext & fontRenderContext = mFontRenderContexts[static_cast<size_t>(line.Font)];
			FontMetadata const & fontMetadata = fontRenderContext.GetFontMetadata();

			//
			// Calculate line position in NDC coordinates
			//

			float constexpr MarginScreen = 10.0f;
			float constexpr MarginTopScreen = MarginScreen + 25.0f; // Consider menu bar

			vec2f linePositionNdc( // Top-left of quads
				line.ScreenOffset.x * mScreenToNdcX,
				-line.ScreenOffset.y * mScreenToNdcY);

			switch (line.Anchor)
			{
				case TextPositionType::BottomLeft:
				{
					linePositionNdc += vec2f(
						-1.f + MarginScreen * mScreenToNdcX,
						-1.f + (MarginScreen + static_cast<float>(fontMetadata.GetLineScreenHeight())) * mScreenToNdcY);

					break;
				}

				case TextPositionType::BottomRight:
				{
					auto const lineExtent = fontMetadata.CalculateTextLineScreenExtent(
						line.Text.c_str(),
						line.Text.length());

					linePositionNdc += vec2f(
						1.f - (MarginScreen + static_cast<float>(lineExtent.Width)) * mScreenToNdcX,
						-1.f + (MarginScreen + static_cast<float>(lineExtent.Height)) * mScreenToNdcY);

					break;
				}

				case TextPositionType::TopLeft:
				{
					linePositionNdc += vec2f(
						-1.f + MarginScreen * mScreenToNdcX,
						1.f - MarginTopScreen * mScreenToNdcY);

					break;
				}

				case TextPositionType::TopRight:
				{
					auto const lineExtent = fontMetadata.CalculateTextLineScreenExtent(
						line.Text.c_str(),
						line.Text.length());

					linePositionNdc += vec2f(
						1.f - (MarginScreen + static_cast<float>(lineExtent.Width)) * mScreenToNdcX,
						1.f - MarginTopScreen * mScreenToNdcY);

					break;
				}
			}


			//
			// Emit quads for this line
			//

			// Remember position of this line in the vertex buffer
			line.FontVertexBufferIndexStart = fontRenderContext.GetVertexBuffer().size();

			// Emit
			line.FontVertexBufferCount = fontMetadata.EmitQuadVertices(
				line.Text.c_str(),
				line.Text.length(),
				linePositionNdc,
				line.Alpha,
				mScreenToNdcX,
				mScreenToNdcY,
				fontRenderContext.GetVertexBuffer());


			//
			// Remember that this font's render context vertex buffers are dirty now
			//

			fontRenderContext.SetVertexBufferDirty(true);
		}

		// Remember lines are not dirty anymore
		mAreLinesDirty = false;
	}


    //
    // Render all fonts
    //

    bool isFirst = true;

    for (auto & context : mFontRenderContexts)
    {
		auto const & vertexBuffer = context.GetVertexBuffer();

		//
		// Re-upload vertex buffer if dirty
		//

		if (context.IsVertexBufferDirty())
		{
			glBindBuffer(GL_ARRAY_BUFFER, context.GetVerticesVBOHandle());			

			glBufferData(
				GL_ARRAY_BUFFER,
				vertexBuffer.size() * sizeof(TextQuadVertex),
				vertexBuffer.data(),
				GL_DYNAMIC_DRAW);
			CheckOpenGLError();

			glBindBuffer(GL_ARRAY_BUFFER, 0);

			context.SetVertexBufferDirty(false);
		}

		//
		// Render
		//

        if (!vertexBuffer.empty())
        {
            //
            // Render the vertices for this font
            //

            glBindVertexArray(context.GetVAOHandle());

            if (isFirst)
            {
                // Activate texture unit (once for all fonts)
                mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();

                // Activate program (once for all fonts)
                mShaderManager.ActivateProgram<ProgramType::TextNDC>();

                isFirst = false;
            }

            // Bind texture
            glBindTexture(GL_TEXTURE_2D, context.GetFontTextureHandle());
            CheckOpenGLError();

            // Draw vertices
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexBuffer.size()));

            glBindVertexArray(0);
        }
    }
}

}