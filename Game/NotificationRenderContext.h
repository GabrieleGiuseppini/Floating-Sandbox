/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Font.h"
#include "ResourceLocator.h"
#include "ShaderTypes.h"

#include <GameOpenGL/ShaderManager.h>

#include <GameCore/GameTypes.h>

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace Render
{

/*
 * This class implements the machinery for rendering UI notifications.
 *
 * The class is fully owned by the RenderContext class.
 */
class NotificationRenderContext
{
public:

	NotificationRenderContext(
        ResourceLocator const & resourceLocator,
        ShaderManager<ShaderManagerTraits> & shaderManager,
        int canvasWidth,
        int canvasHeight,
        float effectiveAmbientLightIntensity);

    void UpdateCanvasSize(int width, int height)
    {
        mScreenToNdcX = 2.0f / static_cast<float>(width);
        mScreenToNdcY = 2.0f / static_cast<float>(height);

        // Re-create vertices next time
		for (auto & fc : mFontRenderContexts)
		{
			fc.SetVertexBufferDirty(true);
		}
    }

    void UpdateEffectiveAmbientLightIntensity(float effectiveAmbientLightIntensity);

public:

	void UploadTextStart()
	{
		// Cleanup
		for (auto & fc : mFontRenderContexts)
		{
			fc.GetVertexBuffer().clear();
		}
	}

	void UploadTextLine(
		std::string const & text,
		TextPositionType anchor,
		vec2f const & screenOffset, // In font cell-size fraction (0.0 -> 1.0)
		float alpha,
		FontType font)
	{
		//
		// Create vertices for this line
		//

		FontRenderContext & fontRenderContext = mFontRenderContexts[static_cast<size_t>(font)];
		FontMetadata const & fontMetadata = fontRenderContext.GetFontMetadata();

		//
		// Calculate line position in NDC coordinates
		//

		float constexpr MarginScreen = 10.0f;
		float constexpr MarginTopScreen = MarginScreen + 25.0f; // Consider menu bar

		vec2f linePositionNdc( // Top-left of quads
			screenOffset.x * mScreenToNdcX * static_cast<float>(fontMetadata.GetCharScreenWidth()),
			-screenOffset.y * mScreenToNdcY * static_cast<float>(fontMetadata.GetLineScreenHeight()));

		switch (anchor)
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
					text.c_str(),
					text.length());

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
					text.c_str(),
					text.length());

				linePositionNdc += vec2f(
					1.f - (MarginScreen + static_cast<float>(lineExtent.Width)) * mScreenToNdcX,
					1.f - MarginTopScreen * mScreenToNdcY);

				break;
			}
		}


		//
		// Emit quads for this line
		//

		fontMetadata.EmitQuadVertices(
			text.c_str(),
			text.length(),
			linePositionNdc,
			alpha,
			mScreenToNdcX,
			mScreenToNdcY,
			fontRenderContext.GetVertexBuffer());


		//
		// Remember that this font's render context vertex buffers are dirty now
		//

		fontRenderContext.SetVertexBufferDirty(true);
	}

	void UploadTextEnd()
	{
		// Nop
	}

	void Draw();

private:

    ShaderManager<ShaderManagerTraits> & mShaderManager;

    float mScreenToNdcX;
    float mScreenToNdcY;

    float mEffectiveAmbientLightIntensity;

private:

    //
    // Text render machinery
    //

    // Render state, grouped by font.
	//
	// This is ultimately where all the render-level information is stored;
	// we have N VBO buffers, one for each font.
    class FontRenderContext
    {
    public:

		FontRenderContext(
            FontMetadata fontMetadata,
            GLuint fontTextureHandle,
            GLuint vertexBufferVBOHandle,
            GLuint vaoHandle)
            : mFontMetadata(std::move(fontMetadata))
            , mFontTextureHandle(fontTextureHandle)
            , mVertexBufferVBOHandle(vertexBufferVBOHandle)
            , mVAOHandle(vaoHandle)
			, mIsVertexBufferDirty(false)
        {}

		FontRenderContext(FontRenderContext && other) noexcept
            : mFontMetadata(std::move(other.mFontMetadata))
            , mFontTextureHandle(std::move(other.mFontTextureHandle))
            , mVertexBufferVBOHandle(std::move(other.mVertexBufferVBOHandle))
            , mVAOHandle(std::move(other.mVAOHandle))
			, mIsVertexBufferDirty(other.mIsVertexBufferDirty)
        {}

        inline FontMetadata const & GetFontMetadata() const
        {
            return mFontMetadata;
        }

        inline GLuint GetFontTextureHandle() const
        {
            return *mFontTextureHandle;
        }

        inline GLuint GetVerticesVBOHandle() const
        {
            return *mVertexBufferVBOHandle;
        }

        inline GLuint GetVAOHandle() const
        {
            return *mVAOHandle;
        }

        inline std::vector<TextQuadVertex> const & GetVertexBuffer() const
        {
            return mVertexBuffer;
        }

        inline std::vector<TextQuadVertex> & GetVertexBuffer()
        {
            return mVertexBuffer;
        }

		bool IsVertexBufferDirty() const
		{
			return mIsVertexBufferDirty;
		}

		void SetVertexBufferDirty(bool isDirty)
		{
			mIsVertexBufferDirty = isDirty;
		}

    private:
        FontMetadata mFontMetadata;
        GameOpenGLTexture mFontTextureHandle;
        GameOpenGLVBO mVertexBufferVBOHandle;
        GameOpenGLVAO mVAOHandle;

        std::vector<TextQuadVertex> mVertexBuffer;

		// Flag tracking whether or not this font's vertex
		// data is dirty; when it is, we'll re-upload the
		// vertex data
		bool mIsVertexBufferDirty;
    };

    std::vector<FontRenderContext> mFontRenderContexts;
};

}