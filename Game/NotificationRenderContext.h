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
 *
 * The geometry produced by this class is highly dependent on the screen
 * (canvas) size; for this reason, this context has to remember enough data
 * about the primitives it renders so to be able to re-calculate vertex
 * buffers when the screen size changes.
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
		// Recalculate screen -> NDC conversion factors
        mScreenToNdcX = 2.0f / static_cast<float>(width);
        mScreenToNdcY = 2.0f / static_cast<float>(height);

		// Make sure we re-calculate (and re-upload) all vertices
		// at the next iteration
		for (auto & fc : mFontRenderContexts)
		{
			fc.SetLineDataDirty(true);
		}
    }

    void UpdateEffectiveAmbientLightIntensity(float effectiveAmbientLightIntensity);

public:

	void UploadTextStart(FontType fontType)
	{
		// Cleanup line buffers for this font
		auto & fontRenderContext = mFontRenderContexts[static_cast<size_t>(fontType)];
		fontRenderContext.GetTextLines().clear();
		fontRenderContext.SetLineDataDirty(true);
	}

	void UploadTextLine(
		FontType font,
		std::string const & text,
		TextPositionType anchor,
		vec2f const & screenOffset, // In font cell-size fraction (0.0 -> 1.0)
		float alpha)
	{
		//
		// Store line data
		//

		auto & fontRenderContext = mFontRenderContexts[static_cast<size_t>(font)];

		fontRenderContext.GetTextLines().emplace_back(
			text,
			anchor,
			screenOffset,
			alpha);

		// Remember this font's vertices need to be re-generated and re-uploaded
		fontRenderContext.SetLineDataDirty(true);
	}

	void UploadTextEnd(FontType /*fontType*/)
	{
		// Nop
	}

	void Draw();

private:

	class FontRenderContext;

	void GenerateTextVertices(FontRenderContext & context);

	void RenderText();

private:

    ShaderManager<ShaderManagerTraits> & mShaderManager;

    float mScreenToNdcX;
    float mScreenToNdcY;

    float mEffectiveAmbientLightIntensity;

private:

    //
    // Text machinery
    //

	struct TextLine
	{
		std::string Text;
		TextPositionType Anchor;
		vec2f ScreenOffset; // In font cell-size fraction (0.0 -> 1.0)
		float Alpha;

		TextLine(
			std::string const & text,
			TextPositionType anchor,
			vec2f const & screenOffset,
			float alpha)
			: Text(text)
			, Anchor(anchor)
			, ScreenOffset(screenOffset)
			, Alpha(alpha)
		{}
	};

    // Render state, grouped by font.
	//
	// This is ultimately where all the primitive-level and render-level
	// information - grouped by font - is stored.
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
			, mTextLines()
			, mVertexBuffer()
			, mIsLineDataDirty(false)
        {}

		/* TODO: needed?
		FontRenderContext(FontRenderContext && other) noexcept
            : mFontMetadata(std::move(other.mFontMetadata))
            , mFontTextureHandle(std::move(other.mFontTextureHandle))
            , mVertexBufferVBOHandle(std::move(other.mVertexBufferVBOHandle))
            , mVAOHandle(std::move(other.mVAOHandle))
			, mIsVertexBufferDirty(other.mIsVertexBufferDirty)
        {}
		*/

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

		inline std::vector<TextLine> const & GetTextLines() const
		{
			return mTextLines;
		}

		inline std::vector<TextLine> & GetTextLines()
		{
			return mTextLines;
		}

        inline std::vector<TextQuadVertex> const & GetVertexBuffer() const
        {
            return mVertexBuffer;
        }

        inline std::vector<TextQuadVertex> & GetVertexBuffer()
        {
            return mVertexBuffer;
        }

		bool IsLineDataDirty() const
		{
			return mIsLineDataDirty;
		}

		void SetLineDataDirty(bool isDirty)
		{
			mIsLineDataDirty = isDirty;
		}

    private:

        FontMetadata mFontMetadata;
        GameOpenGLTexture mFontTextureHandle;
        GameOpenGLVBO mVertexBufferVBOHandle;
        GameOpenGLVAO mVAOHandle;

		std::vector<TextLine> mTextLines;
        std::vector<TextQuadVertex> mVertexBuffer;

		// Flag tracking whether or not this font's line
		// data is dirty; when it is, we'll re-build and
		// re-upload the vertex data
		bool mIsLineDataDirty;
    };

    std::vector<FontRenderContext> mFontRenderContexts;
};

}