/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Font.h"
#include "ResourceLocator.h"
#include "ShaderTypes.h"
#include "TextureAtlas.h"
#include "TextureTypes.h"

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
		TextureAtlasMetadata<GenericLinearTextureGroups> const & genericLinearTextureAtlasMetadata,
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

	void UploadTextNotificationStart(FontType fontType)
	{
		//
		// Text notifications are sticky: we upload them once in a while and
		// continue drawing the same buffer
		//

		// Cleanup line buffers for this font
		auto & fontRenderContext = mFontRenderContexts[static_cast<size_t>(fontType)];
		fontRenderContext.GetTextLines().clear();
		fontRenderContext.SetLineDataDirty(true);
	}

	void UploadTextNotificationLine(
		FontType font,
		std::string const & text,
		AnchorPositionType anchor,
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
	}

	void UploadTextNotificationEnd(FontType /*fontType*/)
	{
		// Nop
	}

	void UploadTextureNotificationStart()
	{
		//
		// Texture notifications are sticky: we upload them once in a while and
		// continue drawing the same buffer
		//

		// Cleanup buffer
		mTextureNotificationVertexBuffer.clear();
		mIsTextureNotificationVertexBufferDirty = true;
	}

    void UploadTextureNotification(
        TextureFrameId<GenericLinearTextureGroups> const & textureFrameId,
		AnchorPositionType anchor,
        vec2f const & screenOffset, // In texture-size fraction (0.0 -> 1.0)
        float alpha);

	void UploadTextureNotificationEnd()
	{
		// Nop
	}

	void Draw();

private:

	class FontRenderContext;

	void GenerateTextVertices(FontRenderContext & context);

	void RenderTextNotifications();

	void RenderTextureNotifications();

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
		AnchorPositionType Anchor;
		vec2f ScreenOffset; // In font cell-size fraction (0.0 -> 1.0)
		float Alpha;

		TextLine(
			std::string const & text,
			AnchorPositionType anchor,
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

	//
	// Texture machinery
	//

	TextureAtlasMetadata<GenericLinearTextureGroups> const & mGenericLinearTextureAtlasMetadata;

#pragma pack(push, 1)
	struct TextureNotificationVertex
	{
        vec2f vertexPositionNDC;
        vec2f textureCoordinate;
        float alpha;

		TextureNotificationVertex(
            vec2f const & _vertexPositionNDC,
            vec2f const & _textureCoordinate,
            float _alpha)
			: vertexPositionNDC(_vertexPositionNDC)
			, textureCoordinate(_textureCoordinate)
			, alpha(_alpha)
		{}
	};
#pragma pack(pop)

	GameOpenGLVAO mTextureNotificationVAO;
	std::vector<TextureNotificationVertex> mTextureNotificationVertexBuffer;
	bool mIsTextureNotificationVertexBufferDirty;
	GameOpenGLVBO mTextureNotificationVBO;
};

}