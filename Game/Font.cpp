/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Font.h"

#include <GameCore/GameException.h>

#include <cassert>
#include <cstring>
#include <fstream>
#include <memory>

namespace Render {

FontMetadata::FontMetadata(
    ImageSize cellSize,
    std::array<uint8_t, 256> glyphWidths,
    int glyphsPerTextureRow)
    : mCellSize(cellSize)
    , mGlyphWidths(glyphWidths)
    , mGlyphsPerTextureRow(glyphsPerTextureRow)
{
}

std::vector<Render::Font> Font::LoadAll(
    ResourceLocator const & resourceLocator,
    ProgressCallback const & progressCallback)
{
    //
    // Get paths
    //

    std::vector<std::filesystem::path> filepaths = resourceLocator.GetFontPaths();


    //
    // Sort paths
    //

    std::sort(
        filepaths.begin(),
        filepaths.end(),
        [](auto const & fp1, auto const & fp2)
        {
            return fp1 < fp2;
        });


    //
    // Load fonts
    //

    std::vector<Font> fonts;

    for (size_t f = 0; f < filepaths.size(); ++f)
    {
        fonts.emplace_back(
            Font::Load(
                static_cast<FontType>(f),
                filepaths[f]));

        progressCallback(
            static_cast<float>(fonts.size()) / static_cast<float>(filepaths.size()),
            ProgressMessageType::LoadingFonts);
    }

    if (fonts.size() != static_cast<size_t>(FontType::_Last) + 1)
    {
        throw GameException("The number of loaded fonts does not match the number of expected fonts");
    }

    return fonts;

}

Font Font::Load(
    FontType type,
    std::filesystem::path const & filepath)
{
    //
    // Read file
    //

    std::error_code ec;
    size_t const fileSize = static_cast<size_t>(std::filesystem::file_size(filepath, ec));
    if (!!ec)
    {
        throw GameException("Cannot open file \"" + filepath.string() + "\"");
    }

    std::ifstream file(filepath.string(), std::ios::binary | std::ios::in);
    if (!file.is_open())
    {
        throw GameException("Cannot open file \"" + filepath.string() + "\"");
    }

    static constexpr size_t HeaderSize = 276;
    if (fileSize < HeaderSize)
    {
        throw GameException("Font file \"" + filepath.string() + "\" is not a BFF font file");
    }

    // Read header
    std::unique_ptr<uint8_t[]> header = std::make_unique<uint8_t[]>(HeaderSize);
    file.read(reinterpret_cast<char*>(header.get()), HeaderSize);

    // Read texture image
    assert(0 == ((fileSize - HeaderSize) % sizeof(rgbaColor)));
    std::unique_ptr<rgbaColor[]> textureData = std::make_unique<rgbaColor[]>((fileSize - HeaderSize) / sizeof(rgbaColor));
    file.read(reinterpret_cast<char*>(textureData.get()), fileSize - HeaderSize);


    //
    // Process font
    //

    // Make sure it's our file type
    if (header[0] != 0xBF || header[1] != 0xF2)
    {
        throw GameException("File \"" + filepath.string() + "\" is not a BFF font file");
    }

    // Make sure the BPP is as expected
    if (header[18] != 32)
    {
        throw GameException("File \"" + filepath.string() + "\" is not a supported BFF font file: BPP is not 32");
    }

    // Make sure base char is as expected
    if (header[19] != FontMetadata::BaseCharacter)
    {
        throw GameException("File \"" + filepath.string() + "\" is not a supported BFF font file: unexpected base character");
    }

    // Read texture image size
    int width = *(reinterpret_cast<int *>(header.get() + 2));
    int height = *(reinterpret_cast<int *>(header.get() + 6));
    ImageSize textureSize(width, height);

    assert(fileSize - HeaderSize == static_cast<size_t>(4 * textureSize.Width * textureSize.Height));

    // Read cell size
    width = *(reinterpret_cast<int *>(header.get() + 10));
    height = *(reinterpret_cast<int *>(header.get() + 14));
    ImageSize cellSize(width, height);

    // Read glyph widths
    std::array<uint8_t, 256> glyphWidths;
    std::memcpy(&(glyphWidths[0]), &(header[20]), 256);

    // Return font
    return Font(
        type,
        FontMetadata(
            cellSize,
            glyphWidths,
            textureSize.Width / cellSize.Width),
        RgbaImageData(
            textureSize,
            std::move(textureData)));
}

}