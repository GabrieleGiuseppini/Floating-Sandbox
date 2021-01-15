/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "RenderTypes.h"
#include "ResourceLocator.h"

#include <GameCore/ImageData.h>
#include <GameCore/ProgressCallback.h>
#include <GameCore/Vectors.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace Render
{

static constexpr char FontBaseCharacter = ' ';

/*
 * This class encapsulates all the information about a font, and exposes the
 * font's rendering primitives.
 */
class FontMetadata
{
public:

    /*
     * Returns the width - in screen coordinates, i.e. pixels - of this font.
     */
    inline int GetCharScreenWidth() const
    {
        return mCellSize.Width;
    }

	/*
	 * Returns the height - in screen coordinates, i.e. pixels - of this font.
	 */
	inline int GetLineScreenHeight() const
	{
		return mCellSize.Height;
	}

	/*
	 * Calculates the dimensions of the specified line in screen
	 * coordinates.
	 */
    inline ImageSize CalculateTextLineScreenExtent(
        char const * text,
        size_t length) const
    {
        uint32_t width = 0;
        for (size_t c = 0; c < length; ++c)
            width += mGlyphWidths[static_cast<unsigned char>(text[c])];

        return ImageSize(width, mCellSize.Height);
    }

	/*
	 * Populates quad vertices for all the characters in the string.
	 */
    inline size_t EmitQuadVertices(
        char const * text,
        size_t length,
        vec2f cursorPositionNdc,
        float alpha,
        float screenToNdcX,
        float screenToNdcY,
        std::vector<TextQuadVertex> & vertices) const
    {
        size_t totalVertices = 0;

        float const cellWidthNdc = screenToNdcX * mCellSize.Width;
        float const cellHeightNdc = screenToNdcY * mCellSize.Height;

        for (size_t c = 0; c < length; ++c)
        {
            unsigned char ch = static_cast<unsigned char>(text[c]);

            float const textureULeft = mGlyphTextureOrigins[ch].x;
            float const textureURight = textureULeft + mGlyphTextureWidth;
            float const textureVBottom = mGlyphTextureOrigins[ch].y;
            float const textureVTop = textureVBottom - mGlyphTextureHeight;

            // Top-left
            vertices.emplace_back(
                cursorPositionNdc.x,
                cursorPositionNdc.y + cellHeightNdc,
                textureULeft,
                textureVTop,
                alpha);

            // Bottom-left
            vertices.emplace_back(
                cursorPositionNdc.x,
                cursorPositionNdc.y,
                textureULeft,
                textureVBottom,
                alpha);

            // Top-right
            vertices.emplace_back(
                cursorPositionNdc.x + cellWidthNdc,
                cursorPositionNdc.y + cellHeightNdc,
                textureURight,
                textureVTop,
                alpha);

            // Bottom-left
            vertices.emplace_back(
                cursorPositionNdc.x,
                cursorPositionNdc.y,
                textureULeft,
                textureVBottom,
                alpha);

            // Top-right
            vertices.emplace_back(
                cursorPositionNdc.x + cellWidthNdc,
                cursorPositionNdc.y + cellHeightNdc,
                textureURight,
                textureVTop,
                alpha);

            // Bottom-right
            vertices.emplace_back(
                cursorPositionNdc.x + cellWidthNdc,
                cursorPositionNdc.y,
                textureURight,
                textureVBottom,
                alpha);

            cursorPositionNdc.x += screenToNdcX * mGlyphWidths[ch];

            totalVertices += 6;
        }

        return totalVertices;
    }

private:

    friend struct Font;

    FontMetadata(
        ImageSize cellSize,
        std::array<uint8_t, 256> glyphWidths,
        int glyphsPerTextureRow,
        float glyphTextureWidth,
        float glyphTextureHeight);

    ImageSize const mCellSize;
    std::array<uint8_t, 256> const mGlyphWidths;
    std::array<vec2f, 256> mGlyphTextureOrigins; // Bottom-left
    int const mGlyphsPerTextureRow;
    float const mGlyphTextureWidth;
    float const mGlyphTextureHeight;
};

struct Font
{
    FontType Type;

    FontMetadata Metadata;

    RgbaImageData Texture;

    static std::vector<Font> LoadAll(
        ResourceLocator const & resourceLocator,
        ProgressCallback const & progressCallback);

private:

    static Font Load(
        FontType type,
        std::filesystem::path const & filepath);

    Font(
        FontType type,
        FontMetadata metadata,
        RgbaImageData texture)
        : Type(type)
        , Metadata(std::move(metadata))
        , Texture(std::move(texture))
    {}
};

}
