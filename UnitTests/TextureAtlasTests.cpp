#include <Game/TextureAtlas.h>

#include <Game/TextureTypes.h>

#include "gtest/gtest.h"

namespace Render {

TEST(TextureAtlasTests, OneTexture_MipMappable)
{
    std::vector<TextureAtlasBuilder<CloudTextureGroups>::TextureInfo> textureInfos{
        { {CloudTextureGroups::Cloud, 5}, {512, 256} }
    };

    auto atlasSpecification = TextureAtlasBuilder<CloudTextureGroups>::BuildMipMappableAtlasSpecification(textureInfos);

    EXPECT_EQ(512, atlasSpecification.AtlasSize.Width);
    EXPECT_EQ(256, atlasSpecification.AtlasSize.Height);

    ASSERT_EQ(1, atlasSpecification.TexturePositions.size());
    ASSERT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 5), atlasSpecification.TexturePositions[0].FrameId);
    ASSERT_EQ(0, atlasSpecification.TexturePositions[0].FrameLeftX);
    ASSERT_EQ(0, atlasSpecification.TexturePositions[0].FrameBottomY);
}

TEST(TextureAtlasTests, Placement1_MipMappable)
{
    // Original guess: 256x256
    std::vector<TextureAtlasBuilder<CloudTextureGroups>::TextureInfo> textureInfos{
        { {CloudTextureGroups::Cloud, 0}, {128, 64} },
        { {CloudTextureGroups::Cloud, 1}, {128, 64} },
        { {CloudTextureGroups::Cloud, 2}, {128, 128} },
        { {CloudTextureGroups::Cloud, 3}, {64, 64} },
        { {CloudTextureGroups::Cloud, 4}, {256, 256} },
        { {CloudTextureGroups::Cloud, 5}, {64, 64} },
        { {CloudTextureGroups::Cloud, 6}, {64, 64} },
        { {CloudTextureGroups::Cloud, 7}, {64, 64} }
    };

    auto atlasSpecification = TextureAtlasBuilder<CloudTextureGroups>::BuildMipMappableAtlasSpecification(textureInfos);

    EXPECT_EQ(512, atlasSpecification.AtlasSize.Width);
    EXPECT_EQ(256, atlasSpecification.AtlasSize.Height);

    ASSERT_EQ(8, atlasSpecification.TexturePositions.size());

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 4), atlasSpecification.TexturePositions[0].FrameId);
    EXPECT_EQ(0, atlasSpecification.TexturePositions[0].FrameLeftX);
    EXPECT_EQ(0, atlasSpecification.TexturePositions[0].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 2), atlasSpecification.TexturePositions[1].FrameId);
    EXPECT_EQ(256, atlasSpecification.TexturePositions[1].FrameLeftX);
    EXPECT_EQ(0, atlasSpecification.TexturePositions[1].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 0), atlasSpecification.TexturePositions[2].FrameId);
    EXPECT_EQ(256 + 128, atlasSpecification.TexturePositions[2].FrameLeftX);
    EXPECT_EQ(0, atlasSpecification.TexturePositions[2].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 1), atlasSpecification.TexturePositions[3].FrameId);
    EXPECT_EQ(256 + 128, atlasSpecification.TexturePositions[3].FrameLeftX);
    EXPECT_EQ(64, atlasSpecification.TexturePositions[3].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 3), atlasSpecification.TexturePositions[4].FrameId);
    EXPECT_EQ(256, atlasSpecification.TexturePositions[4].FrameLeftX);
    EXPECT_EQ(128, atlasSpecification.TexturePositions[4].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 5), atlasSpecification.TexturePositions[5].FrameId);
    EXPECT_EQ(256 + 64, atlasSpecification.TexturePositions[5].FrameLeftX);
    EXPECT_EQ(128, atlasSpecification.TexturePositions[5].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 6), atlasSpecification.TexturePositions[6].FrameId);
    EXPECT_EQ(256 + 64 + 64, atlasSpecification.TexturePositions[6].FrameLeftX);
    EXPECT_EQ(128, atlasSpecification.TexturePositions[6].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 7), atlasSpecification.TexturePositions[7].FrameId);
    EXPECT_EQ(256 + 64 + 64 + 64, atlasSpecification.TexturePositions[7].FrameLeftX);
    EXPECT_EQ(128, atlasSpecification.TexturePositions[7].FrameBottomY);
}

TEST(TextureAtlasTests, Placement1_NonMipMappable)
{
    // Original guess: 256x256
    std::vector<TextureAtlasBuilder<CloudTextureGroups>::TextureInfo> textureInfos{
        { {CloudTextureGroups::Cloud, 0}, {128, 64} },
        { {CloudTextureGroups::Cloud, 1}, {128, 64} },
        { {CloudTextureGroups::Cloud, 2}, {128, 128} },
        { {CloudTextureGroups::Cloud, 3}, {64, 64} },
        { {CloudTextureGroups::Cloud, 4}, {256, 256} },
        { {CloudTextureGroups::Cloud, 5}, {64, 64} },
        { {CloudTextureGroups::Cloud, 6}, {64, 64} },
        { {CloudTextureGroups::Cloud, 7}, {20, 20} }
    };

    auto atlasSpecification = TextureAtlasBuilder<CloudTextureGroups>::BuildAtlasSpecification(textureInfos);

    EXPECT_EQ(512, atlasSpecification.AtlasSize.Width);
    EXPECT_EQ(256, atlasSpecification.AtlasSize.Height);

    ASSERT_EQ(8, atlasSpecification.TexturePositions.size());

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 4), atlasSpecification.TexturePositions[0].FrameId);
    EXPECT_EQ(0, atlasSpecification.TexturePositions[0].FrameLeftX);
    EXPECT_EQ(0, atlasSpecification.TexturePositions[0].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 2), atlasSpecification.TexturePositions[1].FrameId);
    EXPECT_EQ(256, atlasSpecification.TexturePositions[1].FrameLeftX);
    EXPECT_EQ(0, atlasSpecification.TexturePositions[1].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 0), atlasSpecification.TexturePositions[2].FrameId);
    EXPECT_EQ(256 + 128, atlasSpecification.TexturePositions[2].FrameLeftX);
    EXPECT_EQ(0, atlasSpecification.TexturePositions[2].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 1), atlasSpecification.TexturePositions[3].FrameId);
    EXPECT_EQ(256 + 128, atlasSpecification.TexturePositions[3].FrameLeftX);
    EXPECT_EQ(64, atlasSpecification.TexturePositions[3].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 3), atlasSpecification.TexturePositions[4].FrameId);
    EXPECT_EQ(256, atlasSpecification.TexturePositions[4].FrameLeftX);
    EXPECT_EQ(128, atlasSpecification.TexturePositions[4].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 5), atlasSpecification.TexturePositions[5].FrameId);
    EXPECT_EQ(256 + 64, atlasSpecification.TexturePositions[5].FrameLeftX);
    EXPECT_EQ(128, atlasSpecification.TexturePositions[5].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 6), atlasSpecification.TexturePositions[6].FrameId);
    EXPECT_EQ(256 + 64 + 64, atlasSpecification.TexturePositions[6].FrameLeftX);
    EXPECT_EQ(128, atlasSpecification.TexturePositions[6].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 7), atlasSpecification.TexturePositions[7].FrameId);
    EXPECT_EQ(256 + 64 + 64 + 64, atlasSpecification.TexturePositions[7].FrameLeftX);
    EXPECT_EQ(128, atlasSpecification.TexturePositions[7].FrameBottomY);
}

TEST(TextureAtlasTests, RoundsAtlasSize_MipMappable)
{
    std::vector<TextureAtlasBuilder<CloudTextureGroups>::TextureInfo> textureInfos{
        { {CloudTextureGroups::Cloud, 4}, {256, 256} },
        { {CloudTextureGroups::Cloud, 5}, {32, 64} }
    };

    auto atlasSpecification = TextureAtlasBuilder<CloudTextureGroups>::BuildMipMappableAtlasSpecification(textureInfos);

    EXPECT_EQ(512, atlasSpecification.AtlasSize.Width);
    EXPECT_EQ(256, atlasSpecification.AtlasSize.Height);
}

TEST(TextureAtlasTests, RegularAtlas)
{
    std::vector<TextureAtlasBuilder<CloudTextureGroups>::TextureInfo> textureInfos{
        { {CloudTextureGroups::Cloud, 0}, {64, 64} },
        { {CloudTextureGroups::Cloud, 1}, {64, 64} },
        { {CloudTextureGroups::Cloud, 2}, {64, 64} },
        { {CloudTextureGroups::Cloud, 3}, {64, 64} },
        { {CloudTextureGroups::Cloud, 4}, {64, 64} },
        { {CloudTextureGroups::Cloud, 5}, {64, 64} },
        { {CloudTextureGroups::Cloud, 6}, {64, 64} },
        { {CloudTextureGroups::Cloud, 7}, {64, 64} },
        { {CloudTextureGroups::Cloud, 8}, {64, 64} },
        { {CloudTextureGroups::Cloud, 9}, {64, 64} },
        { {CloudTextureGroups::Cloud, 10}, {64, 64} },
        { {CloudTextureGroups::Cloud, 11}, {64, 64} },
        { {CloudTextureGroups::Cloud, 12}, {64, 64} },
        { {CloudTextureGroups::Cloud, 13}, {64, 64} },
        { {CloudTextureGroups::Cloud, 14}, {64, 64} },
        { {CloudTextureGroups::Cloud, 15}, {64, 64} }
    };

    auto atlasSpecification = TextureAtlasBuilder<CloudTextureGroups>::BuildRegularAtlasSpecification(textureInfos);

    EXPECT_EQ(256, atlasSpecification.AtlasSize.Width);
    EXPECT_EQ(256, atlasSpecification.AtlasSize.Height);

    ASSERT_EQ(16, atlasSpecification.TexturePositions.size());

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 0), atlasSpecification.TexturePositions[0].FrameId);
    EXPECT_EQ(0, atlasSpecification.TexturePositions[0].FrameLeftX);
    EXPECT_EQ(0, atlasSpecification.TexturePositions[0].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 1), atlasSpecification.TexturePositions[1].FrameId);
    EXPECT_EQ(64, atlasSpecification.TexturePositions[1].FrameLeftX);
    EXPECT_EQ(0, atlasSpecification.TexturePositions[1].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 2), atlasSpecification.TexturePositions[2].FrameId);
    EXPECT_EQ(128, atlasSpecification.TexturePositions[2].FrameLeftX);
    EXPECT_EQ(0, atlasSpecification.TexturePositions[2].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 4), atlasSpecification.TexturePositions[4].FrameId);
    EXPECT_EQ(0, atlasSpecification.TexturePositions[4].FrameLeftX);
    EXPECT_EQ(64, atlasSpecification.TexturePositions[4].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 5), atlasSpecification.TexturePositions[5].FrameId);
    EXPECT_EQ(64, atlasSpecification.TexturePositions[5].FrameLeftX);
    EXPECT_EQ(64, atlasSpecification.TexturePositions[5].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 6), atlasSpecification.TexturePositions[6].FrameId);
    EXPECT_EQ(128, atlasSpecification.TexturePositions[6].FrameLeftX);
    EXPECT_EQ(64, atlasSpecification.TexturePositions[6].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 7), atlasSpecification.TexturePositions[7].FrameId);
    EXPECT_EQ(128 + 64, atlasSpecification.TexturePositions[7].FrameLeftX);
    EXPECT_EQ(64, atlasSpecification.TexturePositions[7].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 14), atlasSpecification.TexturePositions[14].FrameId);
    EXPECT_EQ(128, atlasSpecification.TexturePositions[14].FrameLeftX);
    EXPECT_EQ(128 + 64, atlasSpecification.TexturePositions[14].FrameBottomY);

    EXPECT_EQ(TextureFrameId<CloudTextureGroups>(CloudTextureGroups::Cloud, 15), atlasSpecification.TexturePositions[15].FrameId);
    EXPECT_EQ(128 + 64, atlasSpecification.TexturePositions[15].FrameLeftX);
    EXPECT_EQ(128 + 64, atlasSpecification.TexturePositions[15].FrameBottomY);
}

}