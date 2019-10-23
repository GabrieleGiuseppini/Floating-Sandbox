/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Font.h"
#include "RenderCore.h"
#include "ResourceLoader.h"

#include <GameOpenGL/ShaderManager.h>

#include <GameCore/GameTypes.h>
#include <GameCore/ProgressCallback.h>

#include <array>
#include <string>
#include <vector>

namespace Render
{

/*
 * This class implements the state of text rendering, 
 * and provides primitives to manipulate the state.
 *
 * The class reasons in screen coordinates. We stick to screen coordinates
 * (one font pixel is one screen pixel) as the font doesn't look nice when 
 * scaled up or down and using a cheap texture filtering. 
 */
class TextRenderContext
{
public:

    TextRenderContext(
        ResourceLoader & resourceLoader,
        ShaderManager<ShaderManagerTraits> & shaderManager,
        int canvasWidth,
        int canvasHeight,
        float effectiveAmbientLightIntensity,
        ProgressCallback const & progressCallback);

    void UpdateCanvasSize(int width, int height)
    {
        mScreenToNdcX = 2.0f / static_cast<float>(width);
        mScreenToNdcY = 2.0f / static_cast<float>(height);

        // Re-create vertices next time
        mAreTextSlotsDirty = true;
    }

    void UpdateEffectiveAmbientLightIntensity(float effectiveAmbientLightIntensity);

	//
	// Text management
	//

	inline int GetLineScreenHeight(FontType font) const
	{
		auto & fontRenderContext = mFontRenderContexts[static_cast<size_t>(font)];
		return fontRenderContext.GetFontMetadata().GetLineScreenHeight();
	}

	// TODOOLD

	RenderedTextHandle AddText(
		std::vector<std::string> const & textLines,
		TextPositionType position,
		float alpha,
		FontType font)
	{
		// Find oldest slot
		size_t oldestSlotIndex = 0;
		uint64_t oldestSlotGeneration = std::numeric_limits<uint64_t>::max();
		for (size_t slotIndex = 0; slotIndex < mTextSlots.size(); ++slotIndex)
		{
			if (mTextSlots[slotIndex].Generation < oldestSlotGeneration)
			{
				oldestSlotIndex = slotIndex;
				oldestSlotGeneration = mTextSlots[slotIndex].Generation;
			}
		}

		// Store info
		mTextSlots[oldestSlotIndex].TextLines = textLines;
		mTextSlots[oldestSlotIndex].Position = position;
		mTextSlots[oldestSlotIndex].Alpha = alpha;
		mTextSlots[oldestSlotIndex].Font = font;
		mTextSlots[oldestSlotIndex].Generation = ++mCurrentTextSlotGeneration;
		mTextSlots[oldestSlotIndex].VertexBufferIndexStart = 0;
		mTextSlots[oldestSlotIndex].VertexBufferCount = 0;

		// Remember we're dirty now
		mAreTextSlotsDirty = true;

		return static_cast<RenderedTextHandle>(oldestSlotIndex);
	}

	void UpdateText(
		RenderedTextHandle textHandle,
		std::vector<std::string> const & textLines,
		float alpha)
	{
		assert(textHandle < mTextSlots.size());

		mTextSlots[textHandle].TextLines = textLines;
		mTextSlots[textHandle].Alpha = alpha;

		// Remember we're dirty now
		mAreTextSlotsDirty = true;
	}

	void UpdateText(
		RenderedTextHandle textHandle,
		float alpha)
	{
		assert(textHandle < mTextSlots.size());

		mTextSlots[textHandle].Alpha = alpha;

		// Optimization: update alpha's in-place, but only if so far we don't
		// need to re-generate all vertex buffers
		if (!mAreTextSlotsDirty)
		{
			// Update all alpha's in this text's vertex buffer
			auto & fontRenderContext = mFontRenderContexts[static_cast<size_t>(mTextSlots[textHandle].Font)];
			TextQuadVertex * vertexBuffer = &(fontRenderContext.GetVertexBuffer()[mTextSlots[textHandle].VertexBufferIndexStart]);
			for (size_t v = 0; v < mTextSlots[textHandle].VertexBufferCount; ++v)
			{
				vertexBuffer[v].alpha = alpha;
			}

			// Remember this font's vertex buffers are dirty now
			fontRenderContext.SetVertexBufferDirty(true);
		}
	}

	void ClearText(RenderedTextHandle textHandle)
	{
		assert(textHandle < mTextSlots.size());

		mTextSlots[textHandle].Generation = 0;

		// Remember we're dirty now
		mAreTextSlotsDirty = true;
	}

	//
	// Rendering
	//

    void Render();

private:

    ShaderManager<ShaderManagerTraits> & mShaderManager;

    float mScreenToNdcX;
    float mScreenToNdcY;

    float mEffectiveAmbientLightIntensity;


	//
	// A single line of text being rendered
	//

	struct TextLine
	{
		std::string Text;
		vec2f NdcCoordinates;
		float Alpha;
		FontType Font;

		// Position and number of vertices for this line in the font's vertex buffer
		size_t FontVertexBufferIndexStart;
		size_t FontVertexBufferCount;
	};

	// Map associating handle to TextLine
	// TODOHERE


	// TODOOLD: nuke

    //
    // Text state slots
    //

    struct TextSlot
    {
        uint64_t Generation;

        std::vector<std::string> TextLines;
        TextPositionType Position;
        float Alpha;
        FontType Font;

        // Position and number of vertices for this slot in the font's vertex buffer
        size_t VertexBufferIndexStart;
        size_t VertexBufferCount;
    };

    std::array<TextSlot, 8> mTextSlots;
    uint64_t mCurrentTextSlotGeneration;
    bool mAreTextSlotsDirty;



    //
    // Text render machinery
    //

    // Render state, grouped by font.
	//
	// This is ultimately where all the render-level information is stored.
	// We have N "render states", one for each font, and these are the things that are
	// ultimately rendered.
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