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

    using TextureGroupsEnumType = MyTextureGroups;

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
    TestAssetManager testAssetManager(
        {
            TestTextureDatabase{
                MyTestTextureDatabase::DatabaseName,
                {},
                "TODO"
            }
        }
    );

    // Load texture database
    auto db = TextureDatabase<MyTestTextureDatabase>::Load(testAssetManager);

    // TODOHERE
}

// TODO: not all frames matched
// TODO: missing frame
