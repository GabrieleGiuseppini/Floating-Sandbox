/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FontSet.h"

#include "GameException.h"
#include "TextureAtlas.h"
#include "TextureDatabase.h"

#include <cstring>
#include <memory>

BffFont BffFont::Load(
    std::string const & fontSetName,
    std::string const & fontRelativePath,
    IAssetManager const & assetManager)
{
    static constexpr size_t HeaderSize = 20 + 256;
    std::uint8_t header[HeaderSize];

    //
    // Load font
    //

    auto const readStream = assetManager.LoadFont(fontSetName, fontRelativePath);

    //
    // Parse header
    //

    size_t szRead = readStream->Read(header, HeaderSize);
    if (szRead < HeaderSize)
    {
        throw GameException("Font \"" + fontSetName + "::" + fontRelativePath + "\" is not a valid font");
    }

    // Check
    if (header[0] != 0xBF
        || header[1] != 0xF2)
    {
        throw GameException("Font \"" + fontSetName + "::" + fontRelativePath + "\" is not a valid font");
    }

    // Make sure the BPP is as expected
    if (header[18] != 32)
    {
        throw GameException("Font \"" + fontSetName + "::" + fontRelativePath + "\" has an unsupported BPP");
    }

    // Read texture image size
    int width = *(reinterpret_cast<int const *>(&(header[2])));
    int height = *(reinterpret_cast<int const *>(&(header[6])));
    ImageSize textureSize(width, height);

    // Read cell size
    width = *(reinterpret_cast<int const *>(&(header[10])));
    height = *(reinterpret_cast<int const *>(&(header[14])));
    ImageSize cellSize(width, height);

    // Read base texture character
    char const baseTextureCharacter = *(reinterpret_cast<char const *>(&(header[19])));

    // Read glyph widths
    std::array<uint8_t, 256> glyphWidths;
    std::memcpy(&(glyphWidths[0]), &(header[20]), 256);

    // Read texture image
    size_t const textureByteSize = textureSize.GetLinearSize() * sizeof(rgbaColor);
    std::unique_ptr<rgbaColor[]> textureData = std::make_unique<rgbaColor[]>(textureByteSize);
    szRead = readStream->Read(reinterpret_cast<uint8_t *>(textureData.get()), textureByteSize);
    if (szRead != textureByteSize)
    {
        throw GameException("Font \"" + fontSetName + "::" + fontRelativePath + "\" is not a valid font");
    }
    RgbaImageData texture(textureSize, std::move(textureData));

    return BffFont(
        baseTextureCharacter,
        cellSize,
        glyphWidths,
        textureSize.width / cellSize.width,
        std::move(texture));
}

template<typename TFontSet>
FontSet<TFontSet> FontSet<TFontSet>::Load(
    IAssetManager const & assetManager,
    SimpleProgressCallback const & progressCallback)
{
    //
    // Get list of available fonts
    //

    std::vector<IAssetManager::AssetDescriptor> fontAssetDescriptors = assetManager.EnumerateFonts(TFontSet::FontSetName);

    //
    // Load fonts, in enum order
    //

    size_t constexpr FontCount = static_cast<size_t>(TFontSet::FontKindType::_Last) + 1;

    std::vector<BffFont> bffFonts;

    for (size_t fontKind = 0; fontKind < FontCount; ++fontKind)
    {
        // Find this kind in assets
        size_t iAsset;
        for (iAsset = 0; iAsset < fontAssetDescriptors.size(); ++iAsset)
        {
            if (TFontSet::FontNameToFontKind(fontAssetDescriptors[iAsset].Name) == static_cast<typename TFontSet::FontKindType>(fontKind))
            {
                break;
            }
        }

        if (iAsset == fontAssetDescriptors.size())
        {
            throw GameException("Font " + std::to_string(fontKind) + " could not be found");
        }

        bffFonts.emplace_back(
            BffFont::Load(
                TFontSet::FontSetName,
                fontAssetDescriptors[iAsset].RelativePath,
                assetManager));

        progressCallback(static_cast<float>(bffFonts.size()) / static_cast<float>(FontCount));
    }

    if (bffFonts.size() != FontCount)
    {
        throw GameException("The number of loaded fonts does not match the number of expected fonts");
    }

    return InternalLoad(std::move(bffFonts));
}

enum class DummyFontTextureGroups : uint16_t
{
    Font = 0,

    _Last = Font
};

struct DummyFontTextureDatabase
{
    static inline std::string DatabaseName = "Fonts";
    using TextureGroupsType = DummyFontTextureGroups;
};

template<typename TFontSet>
FontSet<TFontSet> FontSet<TFontSet>::InternalLoad(std::vector<BffFont> && bffFonts)
{
    //
    // Build font texture atlas
    //

    std::vector<TextureFrame<DummyFontTextureDatabase>> fontTextureFrames;

    for (size_t f = 0; f < bffFonts.size(); ++f)
    {
        TextureFrameMetadata<DummyFontTextureDatabase> fontTextureFrameMetadata = TextureFrameMetadata<DummyFontTextureDatabase>(
            bffFonts[f].FontTexture.Size,
            static_cast<float>(bffFonts[f].FontTexture.Size.width),
            static_cast<float>(bffFonts[f].FontTexture.Size.height),
            false,
            ImageCoordinates(0, 0), // Anchor
            vec2f::zero(), // Anchor
            vec2f::zero(), // Anchor
            TextureFrameId<DummyFontTextureGroups>(
                DummyFontTextureGroups::Font,
                static_cast<TextureFrameIndex>(f)),
            std::to_string(f),
            std::to_string(f));

        fontTextureFrames.emplace_back(
            fontTextureFrameMetadata,
            std::move(bffFonts[f].FontTexture));
    }

    auto fontTextureAtlas = TextureAtlasBuilder<DummyFontTextureDatabase>::BuildAtlas(
        std::move(fontTextureFrames),
        TextureAtlasOptions::None);

    //
    // Calculate font metadata
    //

    std::vector<FontMetadata> fontMetadata;

    // Initialize font texture atlas metadata
    for (size_t f = 0; f < bffFonts.size(); ++f)
    {
        auto const & fontTextureFrameMetadata = fontTextureAtlas.Metadata.GetFrameMetadata(
            TextureFrameId<DummyFontTextureGroups>(
                DummyFontTextureGroups::Font,
                static_cast<TextureFrameIndex>(f)));

        // Dimensions of a cell of this font, in the atlas' texture space coordinates
        float const fontCellWidthAtlasTextureSpace = static_cast<float>(bffFonts[f].CellSize.width) / static_cast<float>(fontTextureAtlas.Metadata.GetSize().width);
        float const fontCellHeightAtlasTextureSpace = static_cast<float>(bffFonts[f].CellSize.height) / static_cast<float>(fontTextureAtlas.Metadata.GetSize().height);

        // Coordinates for each character
        std::array<vec2f, 256> GlyphTextureBottomLefts;
        std::array<vec2f, 256> GlyphTextureTopRights;
        for (int ch = 0; ch < 256; ++ch)
        {
            assert('?' >= bffFonts[f].BaseTextureCharacter);

            int const cTexture = (ch < bffFonts[f].BaseTextureCharacter)
                ? static_cast<int>('?' - bffFonts[f].BaseTextureCharacter)
                : static_cast<int>(ch - bffFonts[f].BaseTextureCharacter);

            // Texture-space left x
            int const glyphTextureCol = cTexture % bffFonts[f].GlyphsPerTextureRow;
            float const glyphLeftAtlasTextureSpace =
                fontTextureFrameMetadata.TextureCoordinatesBottomLeft.x // Includes dead-center dx already
                + static_cast<float>(glyphTextureCol) * fontCellWidthAtlasTextureSpace;

            // Texture-space right x
            int const glyphWidth = static_cast<int>(bffFonts[f].GlyphWidths[ch]);
            float const glyphRightAtlasTextureSpace =
                glyphLeftAtlasTextureSpace
                + static_cast<float>(glyphWidth - 1) / static_cast<float>(fontTextureAtlas.Metadata.GetSize().width); // -1 because 2*dx

            // Texture-space top y
            // Note: font texture is flipped vertically (top of character is at lower V coordinates)
            int const glyphTextureRow = cTexture / bffFonts[f].GlyphsPerTextureRow;
            float const glyphTopAtlasTextureSpace =
                fontTextureFrameMetadata.TextureCoordinatesBottomLeft.y // Includes dead-center dx already
                + static_cast<float>(glyphTextureRow) * fontCellHeightAtlasTextureSpace;

            // Texture-space bottom y
            int const glyphHeight = bffFonts[f].CellSize.height;
            float const glyphBottomAtlasTextureSpace =
                glyphTopAtlasTextureSpace
                + static_cast<float>(glyphHeight - 1) / static_cast<float>(fontTextureAtlas.Metadata.GetSize().height); // -1 because 2*dx

            GlyphTextureBottomLefts[ch] = vec2f(glyphLeftAtlasTextureSpace, glyphBottomAtlasTextureSpace);
            GlyphTextureTopRights[ch] = vec2f(glyphRightAtlasTextureSpace, glyphTopAtlasTextureSpace);
        }

        // Store
        fontMetadata.emplace_back(
            bffFonts[f].CellSize,
            bffFonts[f].GlyphWidths,
            GlyphTextureBottomLefts,
            GlyphTextureTopRights);
    }

    return FontSet<TFontSet>(
        std::move(fontMetadata),
        std::move(fontTextureAtlas.Image));
}