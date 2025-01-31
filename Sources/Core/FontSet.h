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
    static constexpr char BaseCharacter = ' ';

    ImageSize const CellSize; // Screen coordinates, i.e. pixels
    std::array<std::uint8_t, 256> const GlyphWidths;
    int const GlyphsPerTextureRow;
    RgbaImageData FontTexture;

    static BffFont Load(
        std::string const & fontSetName,
        std::string const & fontRelativePath,
        IAssetManager & assetManager);

    BffFont(
        ImageSize const & cellSize,
        std::array<std::uint8_t, 256> const & glyphWidths,
        int glyphsPerTextureRow,
        RgbaImageData && fontTexture)
        : CellSize(cellSize)
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
    char const BaseCharacter;
    ImageSize const CellSize; // Screen coordinates, i.e. pixels
    std::array<std::uint8_t, 256> const GlyphWidths;
    int const GlyphsPerTextureRow;

    vec2f CellTextureAtlasSize; // Size of one cell of the font, in texture atlas space coordinates
    std::array<vec2f, 256> GlyphTextureAtlasBottomLefts; // Bottom-left of each glyph, in texture atlas space coordinates
    std::array<vec2f, 256> GlyphTextureAtlasTopRights; // Top-right of each glyph, in texture atlas space coordinates

    FontMetadata(
        char baseCharacter,
        ImageSize const & cellSize,
        std::array<std::uint8_t, 256> const & glyphWidths,
        int glyphsPerTextureRow,
        vec2f const & cellTextureAtlasSize,
        std::array<vec2f, 256> const & glyphTextureAtlasBottomLefts,
        std::array<vec2f, 256> const & glyphTextureAtlasTopRights)
        : BaseCharacter(baseCharacter)
        , CellSize(cellSize)
        , GlyphWidths(glyphWidths)
        , GlyphsPerTextureRow(glyphsPerTextureRow)
        , CellTextureAtlasSize(cellTextureAtlasSize)
        , GlyphTextureAtlasBottomLefts(glyphTextureAtlasBottomLefts)
        , GlyphTextureAtlasTopRights(glyphTextureAtlasTopRights)
    {}
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
};

#include "FontSet-inl.h"
