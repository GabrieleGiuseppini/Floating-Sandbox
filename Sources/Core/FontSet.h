/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IAssetManager.h"
#include "ImageData.h"
#include "ProgressCallback.h"
#include "Vectors.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

/*
 * Representation of a font serialized according to the BFF specifications
 * (https://github.com/CodeheadUK/CBFG)
 *
 * For internal use by FontSet.
 */
struct BffFont final
{
    char const BaseTextureCharacter;
    ImageSize const CellSize; // Screen coordinates, i.e. pixels
    std::array<std::uint8_t, 256> const GlyphWidths;
    int const GlyphsPerTextureRow;
    RgbaImageData FontTexture;

    static BffFont Load(
        std::string const & fontSetName,
        std::string const & fontRelativePath,
        IAssetManager & assetManager);

    BffFont(
        char baseTextureCharacter,
        ImageSize const & cellSize,
        std::array<std::uint8_t, 256> const & glyphWidths,
        int glyphsPerTextureRow,
        RgbaImageData && fontTexture)
        : BaseTextureCharacter(baseTextureCharacter)
        , CellSize(cellSize)
        , GlyphWidths(glyphWidths)
        , GlyphsPerTextureRow(glyphsPerTextureRow)
        , FontTexture(std::move(fontTexture))
    {
    }
};

/*
 * Provides geometry metadata for a single font.
 */
struct FontMetadata final
{
    // TODO: see if all still needed
    char const BaseTextureCharacter;
    ImageSize const CellSize; // Screen coordinates, i.e. pixels
    std::array<std::uint8_t, 256> const GlyphWidths; // For each possible ASCII character, not only the ones in texture
    int const GlyphsPerTextureRow;

    FloatSize CellTextureAtlasSize; // Size of one cell of the font, in texture atlas space coordinates
    std::array<vec2f, 256> GlyphTextureAtlasBottomLefts; // Bottom-left of each glyph, in texture atlas space coordinates
    std::array<vec2f, 256> GlyphTextureAtlasTopRights; // Top-right of each glyph, in texture atlas space coordinates

    FontMetadata(
        char baseTextureCharacter,
        ImageSize const & cellSize,
        std::array<std::uint8_t, 256> const & glyphWidths,
        int glyphsPerTextureRow,
        FloatSize const & cellTextureAtlasSize,
        std::array<vec2f, 256> const & glyphTextureAtlasBottomLefts,
        std::array<vec2f, 256> const & glyphTextureAtlasTopRights)
        : BaseTextureCharacter(baseTextureCharacter)
        , CellSize(cellSize)
        , GlyphWidths(glyphWidths)
        , GlyphsPerTextureRow(glyphsPerTextureRow)
        , CellTextureAtlasSize(cellTextureAtlasSize)
        , GlyphTextureAtlasBottomLefts(glyphTextureAtlasBottomLefts)
        , GlyphTextureAtlasTopRights(glyphTextureAtlasTopRights)
    {}

    ImageSize CalculateTextLineScreenExtent(
        char const * text,
        size_t length) const
    {
        uint32_t width = 0;

        for (size_t c = 0; c < length; ++c)
            width += GlyphWidths[static_cast<size_t>(text[c])];

        return ImageSize(width, CellSize.height);
    }
};

/*
 * Provides loading services for a set of fonts.
 */
template<typename TFontSet>
struct FontSet final
{
    std::vector<FontMetadata> Metadata;
    RgbaImageData Atlas;

    static FontSet<TFontSet> Load(
        IAssetManager & assetManager,
        ProgressCallback const & progressCallback);

    FontSet(
        std::vector<FontMetadata> && metadata,
        RgbaImageData && atlas)
        : Metadata(std::move(metadata))
        , Atlas(std::move(atlas))
    {}

private:

    static FontSet<TFontSet> InternalLoad(std::vector<BffFont> && bffFonts);

    friend class FontSetTests_Load_Test;
};

#include "FontSet-inl.h"
