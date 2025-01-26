#include <Core/TextureAtlas.h>

#include "TestingUtils.h"

#include "gtest/gtest.h"

RgbaImageData DUMMY_IMAGE(5, 5, rgbaColor(0x01, 0x01, 0x01, 0x01));

struct MyTestTextureDatabase
{
    static inline std::string DatabaseName = "MyTest";

    enum class MyTextureGroups : uint16_t
    {
        MyTestGroup1 = 0,

        _Last = MyTestGroup1
    };

    using TextureGroupsType = MyTextureGroups;

    static MyTextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "MyTestGroup1"))
            return MyTextureGroups::MyTestGroup1;
        else
            throw GameException("Unrecognized Test texture group \"" + str + "\"");
    }
};

TEST(TextureAtlasTests, Specification_OneTexture)
{
    std::vector<TextureAtlasBuilder<MyTestTextureDatabase>::TextureInfo> textureInfos{
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 5}, {43, 12} }
    };

    auto atlasSpecification = TextureAtlasBuilder<MyTestTextureDatabase>::BuildAtlasSpecification(
        textureInfos,
        TextureAtlasOptions::None,
        [](auto const &) -> TextureFrame<MyTestTextureDatabase>
        {
            return TextureFrame<MyTestTextureDatabase>(
                {
                    DUMMY_IMAGE.Size,
                    1.0f, 1.0f,
                    false,
                    ImageCoordinates(0, 0),
                    vec2f::zero(),
                    TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 5),
                    "0", "0"
                },
                DUMMY_IMAGE.Clone());
        });

    EXPECT_EQ(64, atlasSpecification.AtlasSize.width);
    EXPECT_EQ(16, atlasSpecification.AtlasSize.height);

    ASSERT_EQ(1u, atlasSpecification.TextureLocationInfos.size());
    ASSERT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 5), atlasSpecification.TextureLocationInfos[0].FrameId);
    ASSERT_EQ(0, atlasSpecification.TextureLocationInfos[0].InAtlasBottomLeft.x);
    ASSERT_EQ(0, atlasSpecification.TextureLocationInfos[0].InAtlasBottomLeft.y);
    ASSERT_EQ(43, atlasSpecification.TextureLocationInfos[0].InAtlasSize.width);
    ASSERT_EQ(12, atlasSpecification.TextureLocationInfos[0].InAtlasSize.height);
}

TEST(TextureAtlasTests, Specification_MultipleTextures)
{
    // Original guess: 256x256
    std::vector<TextureAtlasBuilder<MyTestTextureDatabase>::TextureInfo> textureInfos{
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0}, {128, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1}, {128, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 2}, {128, 128} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 3}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 4}, {256, 256} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 5}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 6}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 7}, {64, 64} }
    };

    auto atlasSpecification = TextureAtlasBuilder<MyTestTextureDatabase>::BuildAtlasSpecification(
        textureInfos,
        TextureAtlasOptions::None,
        [](auto const &) -> TextureFrame<MyTestTextureDatabase>
        {
            return TextureFrame<MyTestTextureDatabase>(
                {
                    DUMMY_IMAGE.Size,
                    1.0f, 1.0f,
                    false,
                    ImageCoordinates(0, 0),
                    vec2f::zero(),
                    TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 15),
                    "0", "0"
                },
                DUMMY_IMAGE.Clone());
        });

    EXPECT_EQ(512, atlasSpecification.AtlasSize.width);
    EXPECT_EQ(256, atlasSpecification.AtlasSize.height);

    ASSERT_EQ(8u, atlasSpecification.TextureLocationInfos.size());

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 4), atlasSpecification.TextureLocationInfos[0].FrameId);
    EXPECT_EQ(0, atlasSpecification.TextureLocationInfos[0].InAtlasBottomLeft.x);
    EXPECT_EQ(0, atlasSpecification.TextureLocationInfos[0].InAtlasBottomLeft.y);
    EXPECT_EQ(256, atlasSpecification.TextureLocationInfos[0].InAtlasSize.width);
    EXPECT_EQ(256, atlasSpecification.TextureLocationInfos[0].InAtlasSize.height);

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 2), atlasSpecification.TextureLocationInfos[1].FrameId);
    EXPECT_EQ(256, atlasSpecification.TextureLocationInfos[1].InAtlasBottomLeft.x);
    EXPECT_EQ(0, atlasSpecification.TextureLocationInfos[1].InAtlasBottomLeft.y);
    EXPECT_EQ(128, atlasSpecification.TextureLocationInfos[1].InAtlasSize.width);
    EXPECT_EQ(128, atlasSpecification.TextureLocationInfos[1].InAtlasSize.height);

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0), atlasSpecification.TextureLocationInfos[2].FrameId);
    EXPECT_EQ(256, atlasSpecification.TextureLocationInfos[2].InAtlasBottomLeft.x);
    EXPECT_EQ(128, atlasSpecification.TextureLocationInfos[2].InAtlasBottomLeft.y);
    EXPECT_EQ(128, atlasSpecification.TextureLocationInfos[2].InAtlasSize.width);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[2].InAtlasSize.height);

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1), atlasSpecification.TextureLocationInfos[3].FrameId);
    EXPECT_EQ(256, atlasSpecification.TextureLocationInfos[3].InAtlasBottomLeft.x);
    EXPECT_EQ(128 + 64, atlasSpecification.TextureLocationInfos[3].InAtlasBottomLeft.y);
    EXPECT_EQ(128, atlasSpecification.TextureLocationInfos[3].InAtlasSize.width);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[3].InAtlasSize.height);

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 3), atlasSpecification.TextureLocationInfos[4].FrameId);
    EXPECT_EQ(256 + 128, atlasSpecification.TextureLocationInfos[4].InAtlasBottomLeft.x);
    EXPECT_EQ(0, atlasSpecification.TextureLocationInfos[4].InAtlasBottomLeft.y);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[4].InAtlasSize.width);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[4].InAtlasSize.height);

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 5), atlasSpecification.TextureLocationInfos[5].FrameId);
    EXPECT_EQ(256 + 128, atlasSpecification.TextureLocationInfos[5].InAtlasBottomLeft.x);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[5].InAtlasBottomLeft.y);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[5].InAtlasSize.width);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[5].InAtlasSize.height);

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 6), atlasSpecification.TextureLocationInfos[6].FrameId);
    EXPECT_EQ(256 + 128, atlasSpecification.TextureLocationInfos[6].InAtlasBottomLeft.x);
    EXPECT_EQ(64 + 64, atlasSpecification.TextureLocationInfos[6].InAtlasBottomLeft.y);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[6].InAtlasSize.width);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[6].InAtlasSize.height);

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 7), atlasSpecification.TextureLocationInfos[7].FrameId);
    EXPECT_EQ(256 + 128, atlasSpecification.TextureLocationInfos[7].InAtlasBottomLeft.x);
    EXPECT_EQ(64 + 64 + 64, atlasSpecification.TextureLocationInfos[7].InAtlasBottomLeft.y);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[7].InAtlasSize.width);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[7].InAtlasSize.height);
}

TEST(TextureAtlasTests, Specification_RegularAtlas)
{
    std::vector<TextureAtlasBuilder<MyTestTextureDatabase>::TextureInfo> textureInfos{
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 2}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 3}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 4}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 5}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 6}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 7}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 8}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 9}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 10}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 11}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 12}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 13}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 14}, {64, 64} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 15}, {64, 64} }
    };

    auto atlasSpecification = TextureAtlasBuilder<MyTestTextureDatabase>::BuildRegularAtlasSpecification(textureInfos);

    EXPECT_EQ(256, atlasSpecification.AtlasSize.width);
    EXPECT_EQ(256, atlasSpecification.AtlasSize.height);

    ASSERT_EQ(16u, atlasSpecification.TextureLocationInfos.size());

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0), atlasSpecification.TextureLocationInfos[0].FrameId);
    EXPECT_EQ(0, atlasSpecification.TextureLocationInfos[0].InAtlasBottomLeft.x);
    EXPECT_EQ(0, atlasSpecification.TextureLocationInfos[0].InAtlasBottomLeft.y);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[0].InAtlasSize.width);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[0].InAtlasSize.height);

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1), atlasSpecification.TextureLocationInfos[1].FrameId);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[1].InAtlasBottomLeft.x);
    EXPECT_EQ(0, atlasSpecification.TextureLocationInfos[1].InAtlasBottomLeft.y);

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 2), atlasSpecification.TextureLocationInfos[2].FrameId);
    EXPECT_EQ(128, atlasSpecification.TextureLocationInfos[2].InAtlasBottomLeft.x);
    EXPECT_EQ(0, atlasSpecification.TextureLocationInfos[2].InAtlasBottomLeft.y);

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 4), atlasSpecification.TextureLocationInfos[4].FrameId);
    EXPECT_EQ(0, atlasSpecification.TextureLocationInfos[4].InAtlasBottomLeft.x);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[4].InAtlasBottomLeft.y);

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 5), atlasSpecification.TextureLocationInfos[5].FrameId);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[5].InAtlasBottomLeft.x);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[5].InAtlasBottomLeft.y);

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 6), atlasSpecification.TextureLocationInfos[6].FrameId);
    EXPECT_EQ(128, atlasSpecification.TextureLocationInfos[6].InAtlasBottomLeft.x);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[6].InAtlasBottomLeft.y);

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 7), atlasSpecification.TextureLocationInfos[7].FrameId);
    EXPECT_EQ(128 + 64, atlasSpecification.TextureLocationInfos[7].InAtlasBottomLeft.x);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[7].InAtlasBottomLeft.y);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[7].InAtlasSize.width);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[7].InAtlasSize.height);

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 14), atlasSpecification.TextureLocationInfos[14].FrameId);
    EXPECT_EQ(128, atlasSpecification.TextureLocationInfos[14].InAtlasBottomLeft.x);
    EXPECT_EQ(128 + 64, atlasSpecification.TextureLocationInfos[14].InAtlasBottomLeft.y);

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 15), atlasSpecification.TextureLocationInfos[15].FrameId);
    EXPECT_EQ(128 + 64, atlasSpecification.TextureLocationInfos[15].InAtlasBottomLeft.x);
    EXPECT_EQ(128 + 64, atlasSpecification.TextureLocationInfos[15].InAtlasBottomLeft.y);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[15].InAtlasSize.width);
    EXPECT_EQ(64, atlasSpecification.TextureLocationInfos[15].InAtlasSize.height);
}

TEST(TextureAtlasTests, Specification_RoundsAtlasSize)
{
    std::vector<TextureAtlasBuilder<MyTestTextureDatabase>::TextureInfo> textureInfos{
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 4}, {256, 256} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 5}, {32, 64} }
    };

    auto atlasSpecification = TextureAtlasBuilder<MyTestTextureDatabase>::BuildAtlasSpecification(
        textureInfos,
        TextureAtlasOptions::None,
        [](auto const &) -> TextureFrame<MyTestTextureDatabase>
        {
            return TextureFrame<MyTestTextureDatabase>(
                {
                    DUMMY_IMAGE.Size,
                    1.0f, 1.0f,
                    false,
                    ImageCoordinates(0, 0),
                    vec2f::zero(),
                    TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 15),
                    "0", "0"
                },
                DUMMY_IMAGE.Clone());
        });

    EXPECT_EQ(256, atlasSpecification.AtlasSize.width);
    EXPECT_EQ(512, atlasSpecification.AtlasSize.height);
}

TEST(TextureAtlasTests, Specification_DuplicateSuppression)
{
    RgbaImageData image1(4, 4, rgbaColor(0x01, 0x01, 0x01, 0x01));
    RgbaImageData image2a(5, 5, rgbaColor(0x01, 0x01, 0x01, 0x01));
    RgbaImageData image2b(5, 5, rgbaColor(0x01, 0x01, 0x01, 0x01));
    RgbaImageData image3(5, 5, rgbaColor(0x01, 0x02, 0x01, 0x01));

    TextureFrameMetadata<MyTestTextureDatabase> stockFrameMetadata(
        DUMMY_IMAGE.Size,
        1.0f, 1.0f,
        false,
        ImageCoordinates(0, 0),
        vec2f::zero(),
        TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 15),
        "0", "0");

    // Original guess: 256x256
    std::vector<TextureAtlasBuilder<MyTestTextureDatabase>::TextureInfo> textureInfos{
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0}, {4, 4} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1}, {5, 5} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 2}, {5, 5} },
        { {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 3}, {5, 5} }
    };

    auto atlasSpecification = TextureAtlasBuilder<MyTestTextureDatabase>::BuildAtlasSpecification(
        textureInfos,
        TextureAtlasOptions::SuppressDuplicates,
        [&](TextureFrameId<MyTestTextureDatabase::MyTextureGroups> const & frameId) -> TextureFrame<MyTestTextureDatabase>
        {
            RgbaImageData img(4, 4, rgbaColor(0x01, 0x01, 0x01, 0x01));
            if (frameId.FrameIndex == 0)
                img = image1.Clone();
            else if (frameId.FrameIndex == 1)
                img = image2a.Clone();
            else if (frameId.FrameIndex == 2)
                img = image2b.Clone();
            else
            {
                assert(frameId.FrameIndex == 3);
                img = image3.Clone();
            }

            auto tf = TextureFrame<MyTestTextureDatabase>(
                stockFrameMetadata,
                std::move(img));
            tf.Metadata.FrameId = frameId;

            return tf;
        });

    EXPECT_EQ(16, atlasSpecification.AtlasSize.width);
    EXPECT_EQ(8, atlasSpecification.AtlasSize.height);

    ASSERT_EQ(3u, atlasSpecification.TextureLocationInfos.size());

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1), atlasSpecification.TextureLocationInfos[0].FrameId);
    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 3), atlasSpecification.TextureLocationInfos[1].FrameId);
    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0), atlasSpecification.TextureLocationInfos[2].FrameId);

    ASSERT_EQ(1u, atlasSpecification.DuplicateTextureInfos.size());

    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 2), atlasSpecification.DuplicateTextureInfos[0].DuplicateFrameMetadata.FrameId);
    EXPECT_EQ(stockFrameMetadata.Size, atlasSpecification.DuplicateTextureInfos[0].DuplicateFrameMetadata.Size);
    EXPECT_EQ(TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1), atlasSpecification.DuplicateTextureInfos[0].OriginalFrameId);
}

TEST(TextureAtlasTests, Placement_InAtlasSizeMatchingFrameSize)
{
    RgbaImageData frame0Image(8, 8, rgbaColor(0x01, 0x01, 0x01, 0x01));
    RgbaImageData frame1Image(4, 4, rgbaColor(0x04, 0x04, 0x04, 0x04));

    auto const specification = TextureAtlasBuilder<MyTestTextureDatabase>::AtlasSpecification(
        {
            TextureAtlasBuilder<MyTestTextureDatabase>::AtlasSpecification::TextureLocationInfo(
                {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1},
                vec2i(0, 0), // Position
                frame1Image.Size), // In-atlas size
            TextureAtlasBuilder<MyTestTextureDatabase>::AtlasSpecification::TextureLocationInfo(
                {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0},
                vec2i(4, 0), // Position
                frame0Image.Size), // In-atlas size
        },
        {},
        ImageSize(12, 8));

    auto const atlas = TextureAtlasBuilder<MyTestTextureDatabase>::InternalBuildAtlas(
        specification,
        TextureAtlasOptions::None,
        [&](TextureFrameId<MyTestTextureDatabase::MyTextureGroups> const & frameId)
        {
            if (frameId.FrameIndex == 0)
            {
                return TextureFrame<MyTestTextureDatabase>(
                    {
                        frame0Image.Size,
                        1.0f, 1.0f,
                        false,
                        ImageCoordinates(0, 0),
                        vec2f::zero(),
                        frameId,
                        "0", "0"
                    },
                    frame0Image.Clone());
            }
            else
            {
                EXPECT_EQ(frameId.FrameIndex, 1);

                return TextureFrame<MyTestTextureDatabase>(
                    {
                        frame1Image.Size,
                        1.0f, 1.0f,
                        false,
                        ImageCoordinates(1, 2),
                        vec2f::zero(),
                        frameId,
                        "1", "1"
                    },
                    frame1Image.Clone());
            }
        },
        [](float, ProgressMessageType) {});

    EXPECT_EQ(atlas.Metadata.GetSize().width, 12);
    EXPECT_EQ(atlas.Metadata.GetSize().height, 8);

    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 12; ++x)
        {
            auto const c = atlas.AtlasData[{x, y}];

            if (x < 4)
            {
                if (y < 4)
                {
                    EXPECT_EQ(c, rgbaColor(0x04, 0x04, 0x04, 0x04));
                }
                else
                {
                    EXPECT_EQ(c, rgbaColor(0x00, 0x00, 0x00, 0x00));
                }
            }
            else
            {
                EXPECT_EQ(c, rgbaColor(0x01, 0x01, 0x01, 0x01));
            }
        }
    }

    float const dx = 0.5f / 12.0f;
    float const dy = 0.5f / 8.0f;

    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0 }).TextureCoordinatesBottomLeft.x, dx + 4.0f / 12.0f, 0.0001f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0 }).TextureCoordinatesBottomLeft.y, dy + 0.0f / 8.0f, 0.0001f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0 }).TextureCoordinatesAnchorCenter.x, dx + 4.0f / 12.0f + 0.0f / 12.0f, 0.0001f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0 }).TextureCoordinatesAnchorCenter.y, dy + 0.0f / 8.0f + 0.0f / 8.0f, 0.0001f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0 }).TextureCoordinatesTopRight.x, 4.0f / 12.0f + 8.0f / 12.0f - dx, 0.0001f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0 }).TextureCoordinatesTopRight.y, 0.0f / 8.0f + 8.0f / 8.0f - dy, 0.0001f));

    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1 }).TextureCoordinatesBottomLeft.x, dx, 0.0001f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1 }).TextureCoordinatesBottomLeft.y, dy, 0.0001f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1 }).TextureCoordinatesAnchorCenter.x, dx + 1.0f / 12.0f, 0.0001f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1 }).TextureCoordinatesAnchorCenter.y, dy + 2.0f / 8.0f, 0.0001f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1 }).TextureCoordinatesTopRight.x, 4.0f / 12.0f - dx, 0.0001f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1 }).TextureCoordinatesTopRight.y, 4.0f / 8.0f - dy, 0.0001f));
}

TEST(TextureAtlasTests, Placement_InAtlasSizeLargerThanFrameSize)
{
    RgbaImageData frame0Image(5, 5, rgbaColor(0x01, 0x01, 0x01, 0x01));
    RgbaImageData frame1Image(3, 2, rgbaColor(0x04, 0x04, 0x04, 0x04));

    auto const specification = TextureAtlasBuilder<MyTestTextureDatabase>::AtlasSpecification(
        {
            TextureAtlasBuilder<MyTestTextureDatabase>::AtlasSpecification::TextureLocationInfo(
                {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1},
                vec2i(0, 0), // Position
                ImageSize(4, 4)), // In-atlas size
            TextureAtlasBuilder<MyTestTextureDatabase>::AtlasSpecification::TextureLocationInfo(
                {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0},
                vec2i(4, 0), // Position
                ImageSize(8, 8)), // In-atlas size
        },
        {},
        ImageSize(12, 8));

    auto const atlas = TextureAtlasBuilder<MyTestTextureDatabase>::InternalBuildAtlas(
        specification,
        TextureAtlasOptions::None,
        [&](TextureFrameId<MyTestTextureDatabase::MyTextureGroups> const & frameId)
        {
            if (frameId.FrameIndex == 0)
            {
                return TextureFrame<MyTestTextureDatabase>(
                    {
                        frame0Image.Size,
                        1.0f, 1.0f,
                        false,
                        ImageCoordinates(0, 0),
                        vec2f::zero(),
                        frameId,
                        "0", "0"
                    },
                    frame0Image.Clone());
            }
            else
            {
                EXPECT_EQ(frameId.FrameIndex, 1);

                return TextureFrame<MyTestTextureDatabase>(
                    {
                        frame1Image.Size,
                        1.0f, 1.0f,
                        false,
                        ImageCoordinates(2, 3),
                        vec2f::zero(),
                        frameId,
                        "1", "1"
                    },
                    frame1Image.Clone());
            }
        },
        [](float, ProgressMessageType) {});

    EXPECT_EQ(atlas.Metadata.GetSize().width, 12);
    EXPECT_EQ(atlas.Metadata.GetSize().height, 8);

    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 12; ++x)
        {
            auto const c = atlas.AtlasData[{x, y}];

            if (x < 4)
            {
                // Frame 1

                if (x >= 0 && x < 3 && y >= 1 && y < 3)
                {
                        EXPECT_EQ(c, rgbaColor(0x04, 0x04, 0x04, 0x04));
                }
                else
                {
                    EXPECT_EQ(c, rgbaColor(0x00, 0x00, 0x00, 0x00));
                }
            }
            else
            {
                // Frame 0
                if (x >= (4 + 1) && x < (4 + 1 + 5) && y >= 1 && y < 6)
                {
                    EXPECT_EQ(c, rgbaColor(0x01, 0x01, 0x01, 0x01));
                }
                else
                {
                    EXPECT_EQ(c, rgbaColor(0x00, 0x00, 0x00, 0x00));
                }
            }
        }
    }

    float const dx = 0.5f / 12.0f;
    float const dy = 0.5f / 8.0f;

    // Frame 0: @ (4 + 1, 0 + 1) x (5, 5)
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0 }).TextureCoordinatesBottomLeft.x, dx + 5.0f / 12.0f, 0.01f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0 }).TextureCoordinatesBottomLeft.y, dy + 1.0f / 8.0f, 0.01f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0 }).TextureCoordinatesAnchorCenter.x, dx + 5.0f / 12.0f + 0.0f / 12.0f, 0.01f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0 }).TextureCoordinatesAnchorCenter.y, dy + 1.0f / 8.0f + 0.0f / 8.0f, 0.01f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0 }).TextureCoordinatesTopRight.x, 5.0f / 12.0f + 5.0f / 12.0f - dx, 0.01f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0 }).TextureCoordinatesTopRight.y, 1.0f / 8.0f + 5.0f / 8.0f - dy, 0.01f));

    // Frame 1: @ (0 + 0, 0 + 1) x (3, 2)
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1 }).TextureCoordinatesBottomLeft.x, dx, 0.01f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1 }).TextureCoordinatesBottomLeft.y, dy + 1.0f / 8.0f, 0.01f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1 }).TextureCoordinatesAnchorCenter.x, dx + 2.0f / 12.0f, 0.01f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1 }).TextureCoordinatesAnchorCenter.y, dy + 1.0f / 8.0f + 3.0f / 8.0f, 0.01f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1 }).TextureCoordinatesTopRight.x, 0.0f / 12.0f + 3.0f / 12.0f - dx, 0.01f));
    EXPECT_TRUE(ApproxEquals(atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1 }).TextureCoordinatesTopRight.y, 1.0f / 8.0f + 2.0f / 8.0f - dy, 0.01f));
}

TEST(TextureAtlasTests, Placement_Duplicates)
{
    RgbaImageData frame0Image(5, 5, rgbaColor(0x01, 0x01, 0x01, 0x01));
    RgbaImageData frame1aImage(3, 2, rgbaColor(0x04, 0x04, 0x04, 0x04)); // Original
    RgbaImageData frame1bImage(3, 2, rgbaColor(0x04, 0x04, 0x04, 0x04)); // Duplicate

    auto const specification = TextureAtlasBuilder<MyTestTextureDatabase>::AtlasSpecification(
        {
            TextureAtlasBuilder<MyTestTextureDatabase>::AtlasSpecification::TextureLocationInfo(
                {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1},
                vec2i(0, 0), // Position
                ImageSize(4, 4)), // In-atlas size
            TextureAtlasBuilder<MyTestTextureDatabase>::AtlasSpecification::TextureLocationInfo(
                {MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0},
                vec2i(4, 0), // Position
                ImageSize(8, 8)), // In-atlas size
        },
        {
            {
                TextureFrameMetadata<MyTestTextureDatabase>(
                    frame1bImage.Size,
                    1.0f, 1.0f,
                    false,
                    ImageCoordinates(0, 0),
                    vec2f::zero(),
                    TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 2),
                    "1b", "1b"),
                TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1)
            }
        },
        ImageSize(12, 8));

    auto const atlas = TextureAtlasBuilder<MyTestTextureDatabase>::InternalBuildAtlas(
        specification,
        TextureAtlasOptions::SuppressDuplicates,
        [&](TextureFrameId<MyTestTextureDatabase::MyTextureGroups> const & frameId)
        {
            if (frameId.FrameIndex == 0)
            {
                return TextureFrame<MyTestTextureDatabase>(
                    {
                        frame0Image.Size,
                        1.0f, 1.0f,
                        false,
                        ImageCoordinates(0, 0),
                        vec2f::zero(),
                        frameId,
                        "0", "0"
                    },
                    frame0Image.Clone());
            }
            else
            {
                EXPECT_EQ(frameId.FrameIndex, 1);

                return TextureFrame<MyTestTextureDatabase>(
                    {
                        frame1aImage.Size,
                        1.0f, 1.0f,
                        false,
                        ImageCoordinates(2, 3),
                        vec2f::zero(),
                        frameId,
                        "1a", "1a"
                    },
                    frame1aImage.Clone());
            }
        },
        [](float, ProgressMessageType) {});

    EXPECT_EQ(
        atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1 }).TextureCoordinatesBottomLeft,
        atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 2 }).TextureCoordinatesBottomLeft);
    EXPECT_EQ(
        atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1 }).TextureCoordinatesAnchorCenter,
        atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 2 }).TextureCoordinatesAnchorCenter);
    EXPECT_EQ(
        atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1 }).TextureCoordinatesTopRight,
        atlas.Metadata.GetFrameMetadata({ MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 2 }).TextureCoordinatesTopRight);
}
