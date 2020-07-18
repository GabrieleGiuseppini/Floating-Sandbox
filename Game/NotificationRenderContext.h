/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Font.h"
#include "GlobalRenderContext.h"
#include "RenderParameters.h"
#include "ResourceLocator.h"
#include "ShaderTypes.h"
#include "TextureAtlas.h"
#include "TextureTypes.h"

#include <GameOpenGL/ShaderManager.h>

#include <GameCore/GameTypes.h>

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
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
		GlobalRenderContext const & globalRenderContext);

public:

	void UploadStart();

	inline void UploadTextNotificationStart(FontType fontType)
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

	inline void UploadTextNotificationLine(
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

	inline void UploadTextNotificationEnd(FontType /*fontType*/)
	{
		// Nop
	}

	inline void UploadTextureNotificationStart()
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

	inline void UploadTextureNotificationEnd()
	{
		// Nop
	}

	inline void UploadHeatBlasterFlame(
		vec2f const & centerPosition,
		float radius,
		HeatBlasterActionType action)
	{
		//
		// Populate vertices
		//

		float const quadHalfSize = (radius * 1.5f) / 2.0f; // Add some slack for transparency
		float const left = centerPosition.x - quadHalfSize;
		float const right = centerPosition.x + quadHalfSize;
		float const top = centerPosition.y + quadHalfSize;
		float const bottom = centerPosition.y - quadHalfSize;

		// Triangle 1

		mHeatBlasterFlameVertexBuffer[0].vertexPosition = vec2f(left, bottom);
		mHeatBlasterFlameVertexBuffer[0].flameSpacePosition = vec2f(-0.5f, -0.5f);

		mHeatBlasterFlameVertexBuffer[1].vertexPosition = vec2f(left, top);
		mHeatBlasterFlameVertexBuffer[1].flameSpacePosition = vec2f(-0.5f, 0.5f);

		mHeatBlasterFlameVertexBuffer[2].vertexPosition = vec2f(right, bottom);
		mHeatBlasterFlameVertexBuffer[2].flameSpacePosition = vec2f(0.5f, -0.5f);

		// Triangle 2

		mHeatBlasterFlameVertexBuffer[3].vertexPosition = vec2f(left, top);
		mHeatBlasterFlameVertexBuffer[3].flameSpacePosition = vec2f(-0.5f, 0.5f);

		mHeatBlasterFlameVertexBuffer[4].vertexPosition = vec2f(right, bottom);
		mHeatBlasterFlameVertexBuffer[4].flameSpacePosition = vec2f(0.5f, -0.5f);

		mHeatBlasterFlameVertexBuffer[5].vertexPosition = vec2f(right, top);
		mHeatBlasterFlameVertexBuffer[5].flameSpacePosition = vec2f(0.5f, 0.5f);

		//
		// Store shader
		//

		switch (action)
		{
			case HeatBlasterActionType::Cool:
			{
				mHeatBlasterFlameShaderToRender = Render::ProgramType::HeatBlasterFlameCool;
				break;
			}

			case HeatBlasterActionType::Heat:
			{
				mHeatBlasterFlameShaderToRender = Render::ProgramType::HeatBlasterFlameHeat;
				break;
			}
		}
	}

	inline void UploadFireExtinguisherSpray(
		vec2f const & centerPosition,
		float radius)
	{
		//
		// Populate vertices
		//

		float const quadHalfSize = (radius * 3.5f) / 2.0f; // Add some slack to account for transparency
		float const left = centerPosition.x - quadHalfSize;
		float const right = centerPosition.x + quadHalfSize;
		float const top = centerPosition.y + quadHalfSize;
		float const bottom = centerPosition.y - quadHalfSize;

		// Triangle 1

		mFireExtinguisherSprayVertexBuffer[0].vertexPosition = vec2f(left, bottom);
		mFireExtinguisherSprayVertexBuffer[0].spraySpacePosition = vec2f(-0.5f, -0.5f);

		mFireExtinguisherSprayVertexBuffer[1].vertexPosition = vec2f(left, top);
		mFireExtinguisherSprayVertexBuffer[1].spraySpacePosition = vec2f(-0.5f, 0.5f);

		mFireExtinguisherSprayVertexBuffer[2].vertexPosition = vec2f(right, bottom);
		mFireExtinguisherSprayVertexBuffer[2].spraySpacePosition = vec2f(0.5f, -0.5f);

		// Triangle 2

		mFireExtinguisherSprayVertexBuffer[3].vertexPosition = vec2f(left, top);
		mFireExtinguisherSprayVertexBuffer[3].spraySpacePosition = vec2f(-0.5f, 0.5f);

		mFireExtinguisherSprayVertexBuffer[4].vertexPosition = vec2f(right, bottom);
		mFireExtinguisherSprayVertexBuffer[4].spraySpacePosition = vec2f(0.5f, -0.5f);

		mFireExtinguisherSprayVertexBuffer[5].vertexPosition = vec2f(right, top);
		mFireExtinguisherSprayVertexBuffer[5].spraySpacePosition = vec2f(0.5f, 0.5f);

		//
		// Store shader
		//

		mFireExtinguisherSprayShaderToRender = Render::ProgramType::FireExtinguisherSpray;
	}

	void UploadEnd();

	void ProcessParameterChanges(RenderParameters const & renderParameters);

	void Draw();

private:

	class FontRenderContext;

	void GenerateTextVertices(FontRenderContext & context);

	void ApplyViewModelChanges(RenderParameters const & renderParameters);
	void ApplyCanvasSizeChanges(RenderParameters const & renderParameters);	
	void ApplyEffectiveAmbientLightIntensityChanges(RenderParameters const & renderParameters);

	void RenderTextNotifications();
	void RenderTextureNotifications();
	void RenderHeatBlasterFlame();
	void RenderFireExtinguisherSpray();

private:

    ShaderManager<ShaderManagerTraits> & mShaderManager;

    float mScreenToNdcX;
    float mScreenToNdcY;

private:

	//
	// Types
	//

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

	struct HeatBlasterFlameVertex
	{
		vec2f vertexPosition;
		vec2f flameSpacePosition;

		HeatBlasterFlameVertex()
		{}
	};

	struct FireExtinguisherSprayVertex
	{
		vec2f vertexPosition;
		vec2f spraySpacePosition;

		FireExtinguisherSprayVertex()
		{}
	};

#pragma pack(pop)


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
	// Texture notifications
	//

	TextureAtlasMetadata<GenericLinearTextureGroups> const & mGenericLinearTextureAtlasMetadata;

	GameOpenGLVAO mTextureNotificationVAO;
	std::vector<TextureNotificationVertex> mTextureNotificationVertexBuffer;
	bool mIsTextureNotificationVertexBufferDirty;
	GameOpenGLVBO mTextureNotificationVBO;

	//
	// Tool notifications
	//

	GameOpenGLVAO mHeatBlasterFlameVAO;
	std::array<HeatBlasterFlameVertex, 6> mHeatBlasterFlameVertexBuffer;
	GameOpenGLVBO mHeatBlasterFlameVBO;
	std::optional<Render::ProgramType> mHeatBlasterFlameShaderToRender;

	GameOpenGLVAO mFireExtinguisherSprayVAO;
	std::array<FireExtinguisherSprayVertex, 6> mFireExtinguisherSprayVertexBuffer;
	GameOpenGLVBO mFireExtinguisherSprayVBO;
	std::optional<Render::ProgramType> mFireExtinguisherSprayShaderToRender;
};

}