#include <Core/TextureDatabase.h>

#include "gtest/gtest.h"

struct TestTextureDatabase
{
    static inline std::string DatabaseName = "Test";

    enum class TextureGroups : uint16_t
    {
        MyTestGroup1 = 0,
        MyTestGroup2 = 1,

        _Last = MyTestGroup2
    };

    static TextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "MyTestGroup1"))
            return TextureGroups::MyTestGroup1;
        else if (Utils::CaseInsensitiveEquals(str, "MyTestGroup2"))
            return TextureGroups::MyTestGroup2;
        else
            throw GameException("Unrecognized Test texture group \"" + str + "\"");
    }
};

TEST(TextureDatabaseTests, Instantiation)
{
    // TODOHERE: provide a TestAssetManager, prepared with a database
    IAssetManager * foo = nullptr;
    IAssetManager testAssetManager = *foo;
    auto db = TextureDatabase<TestTextureDatabase>::Load(testAssetManager);
}
