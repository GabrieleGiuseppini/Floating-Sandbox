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

class TextRenderContext
{
public:

    TextRenderContext(
        ResourceLoader & resourceLoader,
        ShaderManager<ShaderManagerTraits> & shaderManager,
        int canvasWidth,
        int canvasHeight,
        float ambientLightIntensity,
        ProgressCallback const & progressCallback);

    void UpdateCanvasSize(int width, int height)
    {
        mScreenToNdcX = 2.0f / static_cast<float>(width);
        mScreenToNdcY = 2.0f / static_cast<float>(height);

        // Re-render text next time
        mAreTextSlotsDirty = true;
    }

    void UpdateAmbientLightIntensity(float ambientLightIntensity);

    void RenderStart();

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

    void ClearText(RenderedTextHandle textHandle)
    {
        assert(textHandle < mTextSlots.size());

        mTextSlots[textHandle].Generation = 0;

        // Remember we're dirty now
        mAreTextSlotsDirty = true;
    }

    void RenderEnd();

private:

    ShaderManager<ShaderManagerTraits> & mShaderManager;

    float mScreenToNdcX;
    float mScreenToNdcY;

    float mAmbientLightIntensity;


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
    };

    std::array<TextSlot, 8> mTextSlots;
    uint64_t mCurrentTextSlotGeneration;
    bool mAreTextSlotsDirty;


    //
    // Text render machinery
    //

    // Render information, grouped by font
    class FontRenderInfo
    {
    public:

        FontRenderInfo(
            FontMetadata fontMetadata,
            GLuint fontTextureHandle,
            GLuint vertexBufferVBOHandle)
            : mFontMetadata(std::move(fontMetadata))
            , mFontTextureHandle(fontTextureHandle)
            , mVertexBufferVBOHandle(vertexBufferVBOHandle)
        {}

        FontRenderInfo(FontRenderInfo && other)
            : mFontMetadata(std::move(other.mFontMetadata))
            , mFontTextureHandle(std::move(other.mFontTextureHandle))
            , mVertexBufferVBOHandle(std::move(other.mVertexBufferVBOHandle))
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

        inline std::vector<TextQuadVertex> const & GetVertexBuffer() const
        {
            return mVertexBuffer;
        }

        inline std::vector<TextQuadVertex> & GetVertexBuffer()
        {
            return mVertexBuffer;
        }

    private:
        FontMetadata mFontMetadata;
        GameOpenGLTexture mFontTextureHandle;
        GameOpenGLVBO mVertexBufferVBOHandle;

        std::vector<TextQuadVertex> mVertexBuffer;
    };

    std::vector<FontRenderInfo> mFontRenderInfos;
};

}