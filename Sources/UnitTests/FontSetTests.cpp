#include <Core/FontSet.h>

#include <Render/GameFontSets.h>

#include "gtest/gtest.h"

TEST(FontSetTests, Load)
{
	//
	// Prepare
	//

	std::array<std::uint8_t, 256> glyphWidths1;
	std::array<std::uint8_t, 256> glyphWidths2;
	std::array<std::uint8_t, 256> glyphWidths3;

	for (int i = 0; i < 256; ++i)
	{
		glyphWidths1[i] = 10 + i % 4;
		glyphWidths2[i] = 20 + i % 4;
		glyphWidths3[i] = 30 + i % 4;
	}

	RgbaImageData fontAtlas1 = RgbaImageData(ImageSize(32 * 8, 20 * 8));	// 256 * 160
	RgbaImageData fontAtlas2 = RgbaImageData(ImageSize(32 * 8, 20 * 8));
	RgbaImageData fontAtlas3 = RgbaImageData(ImageSize(32 * 8, 20 * 8));

	std::vector<BffFont> bffFonts;
	bffFonts.emplace_back(' ', ImageSize(32, 20), glyphWidths1, 8, std::move(fontAtlas1));
	bffFonts.emplace_back(' ', ImageSize(32, 20), glyphWidths2, 8, std::move(fontAtlas2));
	bffFonts.emplace_back(' ', ImageSize(32, 20), glyphWidths3, 8, std::move(fontAtlas3));

	//
	// Load
	//

	FontSet<GameFontSets::FontSet> fontSet = FontSet<GameFontSets::FontSet>::InternalLoad(std::move(bffFonts));

	//
	// Verify
	//

	ImageSize constexpr ExpectedAtlasSize(
		512, // ceil_power_of_two of 32 * 8 + 32 * 8
		512); // ceil_power_of_two of 20 * 8 + 20 * 8


	// Metadata

	ASSERT_EQ(fontSet.Metadata.size(), 3u);

	EXPECT_EQ(fontSet.Metadata[0].CellSize.width, 32);
	EXPECT_EQ(fontSet.Metadata[0].CellSize.height, 20);

	float const dx = 0.5f / ExpectedAtlasSize.width;

	EXPECT_EQ(fontSet.Metadata[0].GlyphTextureAtlasBottomLefts[' '].x, dx);
	EXPECT_EQ(fontSet.Metadata[0].GlyphTextureAtlasBottomLefts['!'].x, dx + 32.0f / ExpectedAtlasSize.width);

	EXPECT_EQ(fontSet.Metadata[1].GlyphTextureAtlasBottomLefts[' '].x, 0.5f + dx);
	EXPECT_EQ(fontSet.Metadata[1].GlyphTextureAtlasBottomLefts['!'].x, 0.5f + dx + 32.0f / ExpectedAtlasSize.width);

	EXPECT_EQ(fontSet.Metadata[2].GlyphTextureAtlasBottomLefts[' '].x, dx);
	EXPECT_EQ(fontSet.Metadata[2].GlyphTextureAtlasBottomLefts['!'].x, dx + 32.0f / ExpectedAtlasSize.width);

	// Image

	EXPECT_EQ(fontSet.Atlas.Size, ExpectedAtlasSize);
}
