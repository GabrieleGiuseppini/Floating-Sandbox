/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Font.h"

#include "GameException.h"

#include <cassert>
#include <cstring>
#include <fstream>
#include <memory>

namespace Render {

FontMetadata::FontMetadata(
    ImageSize cellSize,
    std::array<uint8_t, 256> glyphWidths,
    int glyphsPerTextureRow,
    float glyphTextureWidth,
    float glyphTextureHeight)
    : mCellSize(cellSize)
    , mGlyphWidths(glyphWidths)
    , mGlyphsPerTextureRow(glyphsPerTextureRow)
    , mGlyphTextureWidth(glyphTextureWidth)
    , mGlyphTextureHeight(glyphTextureHeight)
{
    // Pre-calculate texture origins (bottom-left)
    for (int c = FontBaseCharacter; c < 256; ++c)
    {
        int const glyphTextureRow = (c - FontBaseCharacter) / mGlyphsPerTextureRow;
        int const glyphTextureCol = (c - FontBaseCharacter) % mGlyphsPerTextureRow;

        // Calculate texture coordinates of box
        // Note: font texture is flipped vertically (top of character is at lower V coordinates)
        float const textureULeft = mGlyphTextureWidth * glyphTextureCol;
        float const textureVBottom = mGlyphTextureHeight * (glyphTextureRow + 1);
        mGlyphTextureOrigins[c] = vec2f(textureULeft, textureVBottom);
    }
}

Font Font::Load(std::filesystem::path const & filepath)
{
    //
    // Read file
    //

    std::error_code ec;
    size_t const fileSize = std::filesystem::file_size(filepath, ec);
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

    std::unique_ptr<uint8_t[]> header = std::make_unique<uint8_t[]>(HeaderSize);
    file.read(reinterpret_cast<char*>(header.get()), HeaderSize);

    std::unique_ptr<unsigned char[]> textureData = std::make_unique<unsigned char[]>(fileSize - HeaderSize);
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
    if (header[19] != FontBaseCharacter)
    {
        throw GameException("File \"" + filepath.string() + "\" is not a supported BFF font file: unexpected base character");
    }

    // Read image size
    int width = *(reinterpret_cast<int *>(header.get() + 2));
    int height = *(reinterpret_cast<int *>(header.get() + 6));
    ImageSize textureSize(width, height);

    assert(fileSize - HeaderSize == 4 * textureSize.Width * textureSize.Height);

    // Read cell size
    width = *(reinterpret_cast<int *>(header.get() + 10));
    height = *(reinterpret_cast<int *>(header.get() + 14));
    ImageSize cellSize(width, height);

    // Read glyph widths
    std::array<uint8_t, 256> glyphWidths;
    std::memcpy(&(glyphWidths[0]), &(header[20]), 256);

    // Return font
    return Font(
        FontMetadata(
            cellSize,
            glyphWidths,
            textureSize.Width / cellSize.Width,
            static_cast<float>(cellSize.Width) / static_cast<float>(textureSize.Width),
            static_cast<float>(cellSize.Height) / static_cast<float>(textureSize.Height)),
        ImageData(
            textureSize,
            std::move(textureData)));
}

}