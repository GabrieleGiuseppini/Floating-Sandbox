#include <GameLib/TextureAtlas.h>

#include "gtest/gtest.h"

namespace Render {

TEST(TextureAtlasTests, OneTexture)
{
    std::vector<TextureAtlasBuilder::TextureInfo> textureInfos{
        { {TextureGroupType::Cloud, 5}, {512, 256} }
    };

    auto atlasSpecification = TextureAtlasBuilder::BuildAtlasSpecification(textureInfos);

    EXPECT_EQ(512, atlasSpecification.AtlasSize.Width);
    EXPECT_EQ(256, atlasSpecification.AtlasSize.Height);

    ASSERT_EQ(1, atlasSpecification.TexturePositions.size());
    ASSERT_EQ(TextureFrameId(TextureGroupType::Cloud, 5), atlasSpecification.TexturePositions[0].FrameId);
    ASSERT_EQ(0, atlasSpecification.TexturePositions[0].FrameLeftX);
    ASSERT_EQ(512, atlasSpecification.TexturePositions[0].FrameTopY);
}

TEST(TextureAtlasTests, OptimalPlacement)
{
    // TODOHERE
    std::vector<TextureAtlasBuilder::TextureInfo> textureInfos{
        { {TextureGroupType::Cloud, 5}, {512, 256} }
    };

    auto atlasSpecification = TextureAtlasBuilder::BuildAtlasSpecification(textureInfos);

    EXPECT_EQ(512, atlasSpecification.AtlasSize.Width);
    EXPECT_EQ(256, atlasSpecification.AtlasSize.Height);

    ASSERT_EQ(1, atlasSpecification.TexturePositions.size());
    ASSERT_EQ(TextureFrameId(TextureGroupType::Cloud, 5), atlasSpecification.TexturePositions[0].FrameId);
    ASSERT_EQ(0, atlasSpecification.TexturePositions[0].FrameLeftX);
    ASSERT_EQ(512, atlasSpecification.TexturePositions[0].FrameTopY);
}

}