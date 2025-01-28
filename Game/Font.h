/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "RenderTypes.h"
#include "ResourceLocator.h"

#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/ProgressCallback.h>
#include <GameCore/Vectors.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace Render
{

/*
 * This class encapsulates all the information about a font, and exposes the
 * font's rendering primitives.
 */
class FontMetadata
{
public:

    static constexpr char BaseCharacter = ' ';

public:

    /*
     * Returns the width - in screen coordinates, i.e. pixels - of this character.
     */
    template<typename TChar>
    inline int GetGlyphScreenWidth(TChar ch) const
    {
        return static_cast<int>(mGlyphWidths[static_cast<size_t>(ch)]);
    }

	/*
	 * Returns the height - in screen coordinates, i.e. pixels - of this character.
	 */
    template<typename TChar>
	inline int GetGlyphScreenHeight(TChar /*ch*/) const
	{
		return mCellSize.height;
	}

    /*
     * Returns the number of glyphs on each row if this font's texture.
     */
    inline int GetGlyphsPerTextureRow() const
    {
        return mGlyphsPerTextureRow;
    }

    /*
     * Returns the width - in screen coordinates, i.e. pixels - of one cell in this font's texture.
     */
    inline int GetCellScreenWidth() const
    {
        return mCellSize.width;
    }

    /*
     * Returns the height - in screen coordinates, i.e. pixels - of one cell in this font's texture.
     */
    inline int GetCellScreenHeight() const
    {
        return mCellSize.width;
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
            width += mGlyphWidths[static_cast<size_t>(text[c])];

        return ImageSize(width, mCellSize.height);
    }

private:

    friend struct Font;

    FontMetadata(
        ImageSize cellSize,
        std::array<uint8_t, 256> glyphWidths,
        int glyphsPerTextureRow);

    ImageSize const mCellSize; // Screen coordinates, i.e. pixels
    std::array<uint8_t, 256> const mGlyphWidths;
    int const mGlyphsPerTextureRow;
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
