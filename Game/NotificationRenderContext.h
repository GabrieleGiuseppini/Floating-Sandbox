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

private:

	enum class TextNotificationType
	{
		StatusText = 0,
		NotificationText = 1,

		_Last = NotificationText
	};

public:

	NotificationRenderContext(
        ResourceLocator const & resourceLocator,
        ShaderManager<ShaderManagerTraits> & shaderManager,
		GlobalRenderContext const & globalRenderContext);

public:

	void UploadStart();

	inline void UploadStatusTextStart()
	{
		UploadTextStart(TextNotificationType::StatusText);
	}

	inline void UploadStatusTextLine(
		std::string const & text,
		AnchorPositionType anchor,
		vec2f const & screenOffset, // In font cell-size fraction (0.0 -> 1.0)
		float alpha)
	{
		UploadTextLine(
			TextNotificationType::StatusText,
			text,
			anchor,
			screenOffset,
			alpha);
	}

	inline void UploadStatusTextEnd()
	{
		UploadTextEnd(TextNotificationType::StatusText);
	}

	inline void UploadNotificationTextStart()
	{
		UploadTextStart(TextNotificationType::NotificationText);
	}

	inline void UploadNotificationTextLine(
		std::string const & text,
		AnchorPositionType anchor,
		vec2f const & screenOffset, // In font cell-size fraction (0.0 -> 1.0)
		float alpha)
	{
		UploadTextLine(
			TextNotificationType::NotificationText,
			text,
			anchor,
			screenOffset,
			alpha);
	}

	inline void UploadNotificationTextEnd()
	{
		UploadTextEnd(TextNotificationType::NotificationText);
	}

	inline void UploadTextureNotificationStart()
	{
		//
		// Texture notifications are sticky: we upload them once in a while and
		// continue drawing the same buffer
		//

		// Cleanup buffers
		mTextureNotifications.clear();
		mIsTextureNotificationDataDirty = true;
	}

	void UploadTextureNotification(
		TextureFrameId<GenericLinearTextureGroups> const & textureFrameId,
		AnchorPositionType anchor,
		vec2f const & screenOffset, // In texture-size fraction (0.0 -> 1.0)
		float alpha)
	{
		//
		// Store notification data
		//

		mTextureNotifications.emplace_back(
			textureFrameId,
			anchor,
			screenOffset,
			alpha);
	}

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

	void RenderPrepare();

	void RenderDraw();

private:

	void ApplyViewModelChanges(RenderParameters const & renderParameters);
	void ApplyCanvasSizeChanges(RenderParameters const & renderParameters);
	void ApplyEffectiveAmbientLightIntensityChanges(RenderParameters const & renderParameters);

	void RenderPrepareTextNotifications();
	void RenderDrawTextNotifications();

	void RenderPrepareTextureNotifications();
	void RenderDrawTextureNotifications();

	void RenderPrepareHeatBlasterFlame();
	void RenderDrawHeatBlasterFlame();

	void RenderPrepareFireExtinguisherSpray();
	void RenderDrawFireExtinguisherSpray();

private:

	struct FontTextureAtlasMetadata;
	struct TextNotificationTypeContext;

	inline void UploadTextStart(TextNotificationType textNotificationType)
	{
		//
		// Text notifications are sticky: we upload them once in a while and
		// continue drawing the same buffer
		//

		// Cleanup line buffers for this notification type
		auto & textContext = mTextNotificationTypeContexts[static_cast<size_t>(textNotificationType)];
		textContext.TextLines.clear();
		textContext.AreTextLinesDirty = true;
	}

	inline void UploadTextLine(
		TextNotificationType textNotificationType,
		std::string const & text,
		AnchorPositionType anchor,
		vec2f const & screenOffset, // In font cell-size fraction (0.0 -> 1.0)
		float alpha)
	{
		//
		// Store line into the context for this notification type
		//

		auto & textNotificationContext = mTextNotificationTypeContexts[static_cast<size_t>(textNotificationType)];

		textNotificationContext.TextLines.emplace_back(
			text,
			anchor,
			screenOffset,
			alpha);
	}

	inline void UploadTextEnd(TextNotificationType /*textNotificationType*/)
	{
		// Nop
	}

	void GenerateTextVertices(TextNotificationTypeContext & context) const;

	void GenerateTextureNotificationVertices();

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
    // Text notifications
    //

	/*
	 * Metadata for each font in the single font atlas.
	 */
	struct FontTextureAtlasMetadata
	{
		vec2f CellTextureAtlasSize; // Size of one cell of the font, in texture atlas space coordinates
		std::array<vec2f, 256> GlyphTextureAtlasOrigins; // Bottom-left of each glyph, in texture atlas space coordinates
		std::array<vec2f, 256> GlyphTextureAtlasSizes; // Dimensions of each glyph, in texture atlas space coordinates
		FontMetadata OriginalFontMetadata;

		FontTextureAtlasMetadata(
			vec2f cellTextureAtlasSize,
			std::array<vec2f, 256> glyphTextureAtlasOrigins,
			std::array<vec2f, 256> glyphTextureAtlasSizes,
			FontMetadata originalFontMetadata)
			: CellTextureAtlasSize(cellTextureAtlasSize)
			, GlyphTextureAtlasOrigins(std::move(glyphTextureAtlasOrigins))
			, GlyphTextureAtlasSizes(std::move(glyphTextureAtlasSizes))
			, OriginalFontMetadata(originalFontMetadata)
		{}

	};

	std::vector<FontTextureAtlasMetadata> mFontTextureAtlasMetadata; // This vector is storage, allowing for N:1 between contextes and fonts

	/*
	 * State per-line-of-text.
	 */
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

	/*
	 * State per-text-notification-type.
	 */
	struct TextNotificationTypeContext
	{
		FontTextureAtlasMetadata const & NotificationFontTextureAtlasMetadata; // The metadata of the font to be used for this notification type

		std::vector<TextLine> TextLines;
		bool AreTextLinesDirty; // When dirty, we'll re-build the quads for this notification type
		std::vector<TextQuadVertex> TextQuadVertexBuffer;

		explicit TextNotificationTypeContext(FontTextureAtlasMetadata const & notificationFontTextureAtlasMetadata)
			: NotificationFontTextureAtlasMetadata(notificationFontTextureAtlasMetadata)
			, TextLines()
			, AreTextLinesDirty(false)
			, TextQuadVertexBuffer()
		{}
	};

	std::vector<TextNotificationTypeContext> mTextNotificationTypeContexts;

	GameOpenGLVAO mTextVAO;
	size_t mCurrentTextQuadVertexBufferSize; // Number of elements (vertices)
	size_t mAllocatedTextQuadVertexBufferSize; // Number of elements (vertices)
	GameOpenGLVBO mTextVBO;

	// Fonts
	GameOpenGLTexture mFontAtlasTextureHandle;

	//
	// Texture notifications
	//

	TextureAtlasMetadata<GenericLinearTextureGroups> const & mGenericLinearTextureAtlasMetadata;

	struct TextureNotification
	{
		TextureFrameId<GenericLinearTextureGroups> FrameId;
		AnchorPositionType Anchor;
		vec2f ScreenOffset; // In texture-size fraction (0.0 -> 1.0)
		float Alpha;

		TextureNotification(
			TextureFrameId<GenericLinearTextureGroups> const & frameId,
			AnchorPositionType anchor,
			vec2f const & screenOffset,
			float alpha)
			: FrameId(frameId)
			, Anchor(anchor)
			, ScreenOffset(screenOffset)
			, Alpha(alpha)
		{}
	};

	std::vector<TextureNotification> mTextureNotifications;
	bool mIsTextureNotificationDataDirty; // When dirty, we'll re-build and re-upload the vertex data

	GameOpenGLVAO mTextureNotificationVAO;
	std::vector<TextureNotificationVertex> mTextureNotificationVertexBuffer;
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