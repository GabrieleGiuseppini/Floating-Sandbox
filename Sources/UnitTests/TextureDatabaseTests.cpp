#include <Core/TextureDatabase.h>

#include "TestingUtils.h"

#include "gtest/gtest.h"

struct MyTestTextureDatabase
{
    static inline std::string DatabaseName = "MyTest";

    enum class MyTextureGroups : uint16_t
    {
        MyTestGroup1 = 0,
        MyTestGroup2 = 1,

        _Last = MyTestGroup2
    };

    using TextureGroupsType = MyTextureGroups;

    static MyTextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "MyTestGroup1"))
            return MyTextureGroups::MyTestGroup1;
        else if (Utils::CaseInsensitiveEquals(str, "MyTestGroup2"))
            return MyTextureGroups::MyTestGroup2;
        else
            throw GameException("Unrecognized Test texture group \"" + str + "\"");
    }
};

TEST(TextureDatabaseTests, Loading)
{
    // Prepare test DB
    TestAssetManager testAssetManager;
    testAssetManager.TestTextureDatabases =
    {
        TestTextureDatabase{
            MyTestTextureDatabase::DatabaseName,
            {
                TestTextureDatabase::DatabaseFrameInfo{{"George_0", "George_0.png", "George_0.png"}, ImageSize(1, 2)},
                TestTextureDatabase::DatabaseFrameInfo{{"George_2", "George_2.png", "George_2.png"}, ImageSize(222, 223)},
                TestTextureDatabase::DatabaseFrameInfo{{"John_1", "John_1.png", "Hello/John_1.png"}, ImageSize(111, 112)},
                TestTextureDatabase::DatabaseFrameInfo{{"Ringo_0", "Ringo_0.png", "Ringo_0.png"}, ImageSize(2022, 2023)},
                TestTextureDatabase::DatabaseFrameInfo{{"Ringo_1", "Ringo_1.png", "Ringo_1.png"}, ImageSize(2122, 2123)},
            },
            R"xxx(
[
    {
	    "groupName": "MyTestGroup1",
	    "hasOwnAmbientLight": false,
	    "frames":[
		    {
			    "worldWidth": 10.0,
			    "worldHeight": 20.0,
			    "frameNamePattern": "George_\\d+"
		    },
		    {
			    "worldWidth": 100.0,
			    "worldHeight": 200.0,
			    "frameNamePattern": "John_\\d+"
		    }
	    ]
    },
    {
	    "groupName": "MyTestGroup2",
	    "hasOwnAmbientLight": true,
        "autoAssignFrameIndices": true,
	    "frames":[
		    {
			    "worldWidth": 10000.0,
			    "worldHeight": 2000.0,
			    "frameNamePattern": "Ringo_\\d+"
		    }
	    ]
    }
]
                )xxx"
            }
    };

    // Load texture database
    auto db = TextureDatabase<MyTestTextureDatabase>::Load(testAssetManager);

    auto const & groups = db.GetGroups();
    ASSERT_EQ(groups.size(), 2);

    // Group 1
    {
        ASSERT_EQ(groups[0].Group, MyTestTextureDatabase::MyTextureGroups::MyTestGroup1);
        ASSERT_EQ(groups[0].GetFrameCount(), 3);
        // FS1
        auto const & fs1 = groups[0].GetFrameSpecification(0);
        EXPECT_EQ(fs1.RelativePath, "George_0.png");
        ASSERT_EQ(fs1.Metadata.FrameId, TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 0));
        EXPECT_EQ(fs1.Metadata.Size, ImageSize(1, 2));
        EXPECT_EQ(fs1.Metadata.WorldWidth, 10.0f);
        // FS2
        auto const & fs2 = groups[0].GetFrameSpecification(1);
        EXPECT_EQ(fs2.RelativePath, "Hello/John_1.png");
        ASSERT_EQ(fs2.Metadata.FrameId, TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 1));
        EXPECT_EQ(fs2.Metadata.Size, ImageSize(111, 112));
        EXPECT_EQ(fs2.Metadata.WorldWidth, 100.0f);
        // FS3
        auto const & fs3 = groups[0].GetFrameSpecification(2);
        EXPECT_EQ(fs3.RelativePath, "George_2.png");
        ASSERT_EQ(fs3.Metadata.FrameId, TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup1, 2));
        EXPECT_EQ(fs3.Metadata.Size, ImageSize(222, 223));
        EXPECT_EQ(fs3.Metadata.WorldWidth, 10.0f);
    }

    // Group 2
    {
        ASSERT_EQ(groups[1].Group, MyTestTextureDatabase::MyTextureGroups::MyTestGroup2);
        ASSERT_EQ(groups[1].GetFrameCount(), 2);
        // FS1
        auto const & fs1 = groups[1].GetFrameSpecification(0);
        EXPECT_EQ(fs1.RelativePath, "Ringo_0.png");
        ASSERT_EQ(fs1.Metadata.FrameId, TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup2, 0));
        EXPECT_EQ(fs1.Metadata.Size, ImageSize(2022, 2023));
        EXPECT_EQ(fs1.Metadata.WorldWidth, 10000.0f);
        // FS2
        auto const & fs2 = groups[1].GetFrameSpecification(1);
        EXPECT_EQ(fs2.RelativePath, "Ringo_1.png");
        ASSERT_EQ(fs2.Metadata.FrameId, TextureFrameId<MyTestTextureDatabase::MyTextureGroups>(MyTestTextureDatabase::MyTextureGroups::MyTestGroup2, 1));
        EXPECT_EQ(fs2.Metadata.Size, ImageSize(2122, 2123));
        EXPECT_EQ(fs2.Metadata.WorldWidth, 10000.0f);
    }
}

TEST(TextureDatabaseTests, NotAllGroupsCovered)
{
    // Prepare test DB
    TestAssetManager testAssetManager;
    testAssetManager.TestTextureDatabases =
    {
        TestTextureDatabase{
            MyTestTextureDatabase::DatabaseName,
            {
                TestTextureDatabase::DatabaseFrameInfo{{"George_0", "George_0.png"}, ImageSize(1, 2)},
            },
            R"xxx(
[
    {
	    "groupName": "MyTestGroup1",
	    "hasOwnAmbientLight": false,
	    "frames":[
		    {
			    "worldWidth": 10.0,
			    "worldHeight": 20.0,
			    "frameNamePattern": "George_\\d+"
		    }
        ]
    }
]
            )xxx"
        }
    };

    EXPECT_THROW(
        TextureDatabase<MyTestTextureDatabase>::Load(testAssetManager),
        GameException);
}


TEST(TextureDatabaseTests, NotAllFramesCovered)
{
    // Prepare test DB
    TestAssetManager testAssetManager;
    testAssetManager.TestTextureDatabases =
    {
        TestTextureDatabase{
            MyTestTextureDatabase::DatabaseName,
            {
                TestTextureDatabase::DatabaseFrameInfo{{"George_0", "George_0.png", "George_0.png"}, ImageSize(1, 2)},
                TestTextureDatabase::DatabaseFrameInfo{{"John_1", "John_1.png", "John_1.png"}, ImageSize(111, 112)},
                TestTextureDatabase::DatabaseFrameInfo{{"Ringo_0", "Ringo_0.png", "Ringo_0.png"}, ImageSize(2022, 2023)},
                TestTextureDatabase::DatabaseFrameInfo{{"Ringo_1", "Ringo_1.png", "Ringo_1.png"}, ImageSize(2122, 2123)},
            },
            R"xxx(
[
    {
	    "groupName": "MyTestGroup1",
	    "hasOwnAmbientLight": false,
	    "frames":[
		    {
			    "worldWidth": 10.0,
			    "worldHeight": 20.0,
			    "frameNamePattern": "George_\\d+"
		    }
	    ]
    },
    {
	    "groupName": "MyTestGroup2",
	    "hasOwnAmbientLight": true,
        "autoAssignFrameIndices": true,
	    "frames":[
		    {
			    "worldWidth": 10000.0,
			    "worldHeight": 2000.0,
			    "frameNamePattern": "Ringo_\\d+"
		    }
	    ]
    }
]
                )xxx"
        }
    };

    EXPECT_THROW(
        TextureDatabase<MyTestTextureDatabase>::Load(testAssetManager),
        GameException);
}

TEST(TextureDatabaseTests, NotAllFramesFound)
{
    // Prepare test DB
    TestAssetManager testAssetManager;
    testAssetManager.TestTextureDatabases =
    {
        TestTextureDatabase{
            MyTestTextureDatabase::DatabaseName,
            {
                TestTextureDatabase::DatabaseFrameInfo{{"George_0", "George_0.png", "George_0.png"}, ImageSize(1, 2)},
                TestTextureDatabase::DatabaseFrameInfo{{"George_2", "George_2.png", "George_2.png"}, ImageSize(222, 223)},
                TestTextureDatabase::DatabaseFrameInfo{{"Ringo_0", "Ringo_0.png", "Ringo_0.png"}, ImageSize(2022, 2023)},
                TestTextureDatabase::DatabaseFrameInfo{{"Ringo_1", "Ringo_1.png", "Ringo_1.png"}, ImageSize(2122, 2123)},
            },
            R"xxx(
[
    {
	    "groupName": "MyTestGroup1",
	    "hasOwnAmbientLight": false,
	    "frames":[
		    {
			    "worldWidth": 10.0,
			    "worldHeight": 20.0,
			    "frameNamePattern": "George_\\d+"
		    },
		    {
			    "worldWidth": 100.0,
			    "worldHeight": 200.0,
			    "frameNamePattern": "John_\\d+"
		    }
	    ]
    },
    {
	    "groupName": "MyTestGroup2",
	    "hasOwnAmbientLight": true,
        "autoAssignFrameIndices": true,
	    "frames":[
		    {
			    "worldWidth": 10000.0,
			    "worldHeight": 2000.0,
			    "frameNamePattern": "Ringo_\\d+"
		    }
	    ]
    }
]
                )xxx"
        }
    };

    EXPECT_THROW(
        TextureDatabase<MyTestTextureDatabase>::Load(testAssetManager),
        GameException);
}
