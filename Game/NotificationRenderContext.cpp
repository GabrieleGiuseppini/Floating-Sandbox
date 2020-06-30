/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "NotificationRenderContext.h"

namespace Render {

NotificationRenderContext::NotificationRenderContext(
    ResourceLocator const & resourceLocator,
    ShaderManager<ShaderManagerTraits> & shaderManager,
    int canvasWidth,
    int canvasHeight,
    float effectiveAmbientLightIntensity)
    : mShaderManager(shaderManager)
    , mScreenToNdcX(2.0f / static_cast<float>(canvasWidth))
    , mScreenToNdcY(2.0f / static_cast<float>(canvasHeight))
    , mEffectiveAmbientLightIntensity(effectiveAmbientLightIntensity)
	//
    , mFontRenderContexts()
{
    //
    // Load fonts
    //

    std::vector<Font> fonts = Font::LoadAll(
        resourceLocator,
        [](float, std::string const &) {});


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

void NotificationRenderContext::UpdateEffectiveAmbientLightIntensity(float intensity)
{
    mEffectiveAmbientLightIntensity = intensity;

    float const lighteningStrength = Step(0.5f, 1.0f - mEffectiveAmbientLightIntensity);

    // Set parameter
    mShaderManager.ActivateProgram<ProgramType::TextNDC>();
    mShaderManager.SetProgramParameter<ProgramType::TextNDC, ProgramParameterType::TextLighteningStrength>(
        lighteningStrength);
}

void NotificationRenderContext::Draw()
{
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