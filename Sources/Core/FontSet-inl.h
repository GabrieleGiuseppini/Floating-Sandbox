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
    IAssetManager & assetManager)
{
    static constexpr size_t HeaderSize = 20 + 256;

    //
    // Load font
    //

    auto const buffer = assetManager.LoadFont(fontSetName, fontRelativePath);

    // Check

    if (buffer.GetSize() < HeaderSize
        || buffer[0] != 0xBF
        || buffer[1] != 0xF2)
    {
        throw GameException("Font \"" + fontSetName + "::" + fontRelativePath + "\" is not a valid font");
    }

    // Make sure the BPP is as expected
    if (buffer[18] != 32)
    {
        throw GameException("Font \"" + fontSetName + "::" + fontRelativePath + "\" has an unsupported BPP");
    }

    //
    // Parse
    //

    // Read texture image size
    int width = *(reinterpret_cast<int const *>(&(buffer[2])));
    int height = *(reinterpret_cast<int const *>(&(buffer[6])));
    ImageSize textureSize(width, height);

    if (buffer.GetSize() != HeaderSize + textureSize.GetLinearSize() * sizeof(rgbaColor))
    {
        throw GameException("Font \"" + fontSetName + "::" + fontRelativePath + "\" is not a valid font");
    }

    // Read cell size
    width = *(reinterpret_cast<int const *>(&(buffer[10])));
    height = *(reinterpret_cast<int const *>(&(buffer[14])));
    ImageSize cellSize(width, height);

    // Read base texture character
    char const baseTextureCharacter = *(reinterpret_cast<char const *>(&(buffer[19])));

    // Read glyph widths
    std::array<uint8_t, 256> glyphWidths;
    std::memcpy(&(glyphWidths[0]), &(buffer[20]), 256);

    // Read texture image
    size_t const textureByteSize = textureSize.GetLinearSize() * sizeof(rgbaColor);
    std::unique_ptr<rgbaColor[]> textureData = std::make_unique<rgbaColor[]>(textureByteSize);
    std::memcpy(textureData.get(), &(buffer[HeaderSize]), textureByteSize);
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
    IAssetManager & assetManager,
    ProgressCallback const & progressCallback)
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
            if (TFontSet::FontNameToFontKind(fontAssetDescriptors[iAsset].Name) == iAsset)
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

        progressCallback(
            static_cast<float>(bffFonts.size()) / static_cast<float>(FontCount),
            ProgressMessageType::LoadingFonts);
    }

    if (bffFonts.size() != FontCount)
    {
        throw GameException("The number of loaded fonts does not match the number of expected fonts");
    }

    //
    // Build font texture atlas
    //

    enum class FontTextureGroups : uint16_t
    {
        Font = 0,

        _Last = Font
    };

    std::vector<TextureFrame<FontTextureGroups>> fontTextureFrames;

    for (size_t f = 0; f < bffFonts.size(); ++f)
    {
        TextureFrameMetadata<FontTextureGroups> fontTextureFrameMetadata = TextureFrameMetadata<FontTextureGroups>(
            bffFonts[f].FontTexture.Size,
            static_cast<float>(bffFonts[f].FontTexture.Size.width),
            static_cast<float>(bffFonts[f].FontTexture.Size.height),
            false,
            ImageCoordinates(0, 0),
            vec2f::zero(),
            TextureFrameId<FontTextureGroups>(
                FontTextureGroups::Font,
                static_cast<TextureFrameIndex>(f)),
            std::to_string(f),
            std::to_string(f));

        fontTextureFrames.emplace_back(
            fontTextureFrameMetadata,
            std::move(bffFonts[f].FontTexture));
    }

    auto fontTextureAtlas = TextureAtlasBuilder<FontTextureGroups>::BuildAtlas(
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
            TextureFrameId<FontTextureGroups>(
                FontTextureGroups::Font,
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
            float const glyphWidth = static_cast<int>(bffFonts[f].GlyphWidths[ch]);
            float const glyphRightAtlasTextureSpace =
                glyphLeftAtlasTextureSpace
                + static_cast<float>(glyphWidth - 1) / static_cast<float>(fontTextureAtlas.Metadata.GetSize().width); // Dragons?

            // Texture-space top y
            // Note: font texture is flipped vertically (top of character is at lower V coordinates)
            int const glyphTextureRow = cTexture / bffFonts[f].GlyphsPerTextureRow;
            float const glyphTopAtlasTextureSpace =
                fontTextureFrameMetadata.TextureCoordinatesBottomLeft.y // Includes dead-center dx already
                + static_cast<float>(glyphTextureRow) * fontCellHeightAtlasTextureSpace;

            // Texture-space bottom y
            float const glyphBottomAtlasTextureSpace =
                glyphTopAtlasTextureSpace
                + static_cast<float>(bffFonts[f].CellSize.height - 1) / static_cast<float>(fontTextureAtlas.Metadata.GetSize().height); // Dragons?

            GlyphTextureBottomLefts[ch] = vec2f(glyphLeftAtlasTextureSpace, glyphBottomAtlasTextureSpace);
            GlyphTextureTopRights[ch] = vec2f(glyphRightAtlasTextureSpace, glyphTopAtlasTextureSpace);
        }

        // Store
        fontMetadata.emplace_back(
            BffFont::BaseTextureCharacter,
            bffFonts[f].CellSize,
            bffFonts[f].GlyphWidths,
            bffFonts[f].GlyphsPerTextureRow,
            vec2f(fontCellWidthAtlasTextureSpace, fontCellHeightAtlasTextureSpace),
            GlyphTextureBottomLefts,
            GlyphTextureTopRights);
    }

    return FontSet<TFontSet>(
        std::move(fontMetadata),
        std::move(fontTextureAtlas));
}
