#include <GameCore/Utils.h>

#include <unordered_map>

#include "gtest/gtest.h"

TEST(UtilsTests, MakeFilenameSafeString_Beginning)
{
    char str[] = "\xec\xf5\xe8\xf1\xf8WOOZBAR";

    std::string const safeFilename = Utils::MakeFilenameSafeString(std::string(str));
    EXPECT_EQ(safeFilename, "WOOZBAR");
}

TEST(UtilsTests, MakeFilenameSafeString_Middle)
{
    char str[] = "FOO\xec\xf5\xe8\xf1\xf8ZBAR";

    std::string const safeFilename = Utils::MakeFilenameSafeString(std::string(str));
    EXPECT_EQ(safeFilename, "FOOZBAR");
}

TEST(UtilsTests, MakeFilenameSafeString_End)
{
    char str[] = "FOOZBAR\xec\xf5\xe8\xf1\xf8";

    std::string const safeFilename = Utils::MakeFilenameSafeString(std::string(str));
    EXPECT_EQ(safeFilename, "FOOZBAR");
}

TEST(UtilsTests, MakeFilenameSafeString_FilenameChars)
{
    char str[] = "FOO\\BAR/Z:";

    std::string const safeFilename = Utils::MakeFilenameSafeString(std::string(str));
    EXPECT_EQ(safeFilename, "FOOBARZ");
}

TEST(UtilsTests, MakeFilenameSafeString_BecomesEmpty)
{
    char str[] = "\xec\xf5\xe8\xf1\xf8";

    std::string const safeFilename = Utils::MakeFilenameSafeString(std::string(str));
    EXPECT_EQ(safeFilename, "");
}

TEST(UtilsTests, MakeFilenameSafeString_AlreadySafe)
{
    char str[] = "Foo Bar Hello";

    std::string const safeFilename = Utils::MakeFilenameSafeString(std::string(str));
    EXPECT_EQ(safeFilename, std::string(str));
}