#include <Game/FileSystem.h>

#include <Core/SysSpecifics.h>

#include "gtest/gtest.h"

TEST(FileSystemTests, MakeFilenameSafeString_Beginning)
{
    char str[] = "\xec\xf5\xe8\xf1\xf8WOOZBAR";

    std::string const safeFilename = FileSystem::MakeFilenameSafeString(std::string(str));
#if FS_IS_OS_WINDOWS()
    EXPECT_EQ(safeFilename, "WOOZBAR");
#else
    EXPECT_EQ(safeFilename, "\xEC\xF5\xE8\xF1\xF8WOOZBAR");
#endif
}

TEST(FileSystemTests, MakeFilenameSafeString_Middle)
{
    char str[] = "FOO\xec\xf5\xe8\xf1\xf8ZBAR";

    std::string const safeFilename = FileSystem::MakeFilenameSafeString(std::string(str));
#if FS_IS_OS_WINDOWS()
    EXPECT_EQ(safeFilename, "FOOZBAR");
#else
    EXPECT_EQ(safeFilename, "FOO\xEC\xF5\xE8\xF1\xF8ZBAR");
#endif
}

TEST(FileSystemTests, MakeFilenameSafeString_End)
{
    char str[] = "FOOZBAR\xec\xf5\xe8\xf1\xf8";

    std::string const safeFilename = FileSystem::MakeFilenameSafeString(std::string(str));
#if FS_IS_OS_WINDOWS()
    EXPECT_EQ(safeFilename, "FOOZBAR");
#else
    EXPECT_EQ(safeFilename, "FOOZBAR\xEC\xF5\xE8\xF1\xF8");
#endif
}

TEST(FileSystemTests, MakeFilenameSafeString_FilenameChars)
{
    char str[] = "FOO\\BAR/Z:";

    std::string const safeFilename = FileSystem::MakeFilenameSafeString(std::string(str));
    EXPECT_EQ(safeFilename, "FOOBARZ");
}

TEST(FileSystemTests, MakeFilenameSafeString_BecomesEmpty)
{
    char str[] = "\xec\xf5\xe8\xf1\xf8";

    std::string const safeFilename = FileSystem::MakeFilenameSafeString(std::string(str));
#if FS_IS_OS_WINDOWS()
    EXPECT_EQ(safeFilename, "");
#else
    EXPECT_EQ(safeFilename, "\xEC\xF5\xE8\xF1\xF8");
#endif
}

TEST(FileSystemTests, MakeFilenameSafeString_AlreadySafe)
{
    char str[] = "Foo Bar Hello";

    std::string const safeFilename = FileSystem::MakeFilenameSafeString(std::string(str));
    EXPECT_EQ(safeFilename, std::string(str));
}

#if FS_IS_OS_WINDOWS()
class IsFileUnderDirectoryTest_WindowsOnly : public testing::TestWithParam<std::tuple<std::string, std::string, bool>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    IsFileUnderDirectoryTests,
    IsFileUnderDirectoryTest_WindowsOnly,
    ::testing::Values( // Dir, File, Result
        std::make_tuple("C:\\", "C:\\foo\\zorro\\blah", true),
        std::make_tuple("C:\\foo", "C:\\foo\\zorro\\blah", true),
        std::make_tuple("C:\\foo\\zorro", "C:\\foo\\zorro\\blah", true),
        std::make_tuple("C:\\foo\\zorro\\blah", "C:\\foo\\zorro\\blah", true),

        std::make_tuple("C:\\foo\\zorro\\blah\\krok", "C:\\", false),
        std::make_tuple("C:\\foo\\zorro\\blah\\krok", "C:\\foo", false),
        std::make_tuple("C:\\foo\\zorro\\blah\\krok", "C:\\foo\\zorro", false),
        std::make_tuple("C:\\foo\\zorro\\blah\\krok", "C:\\foo\\zorro\\blah", false)
    ));

TEST_P(IsFileUnderDirectoryTest_WindowsOnly, BasicCases)
{
    auto const directoryPath = std::filesystem::path(std::get<0>(GetParam()));
    auto const filePath = std::filesystem::path(std::get<1>(GetParam()));

    bool const result = FileSystem::IsFileUnderDirectory(filePath, directoryPath);
    EXPECT_EQ(result, std::get<2>(GetParam()));
}
#endif

class IsFileUnderDirectoryTest : public testing::TestWithParam<std::tuple<std::string, std::string, bool>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    IsFileUnderDirectoryTests,
    IsFileUnderDirectoryTest,
    ::testing::Values( // Dir, File, Result
        std::make_tuple("/", "/foo/zorro/blah", true),
        std::make_tuple("/foo", "/foo/zorro/blah", true),
        std::make_tuple("/foo/zorro", "/foo/zorro/blah", true),
        std::make_tuple("/foo/zorro/blah", "/foo/zorro/blah", true),

        std::make_tuple("/foo/zorro/blah/krok", "/", false),
        std::make_tuple("/foo/zorro/blah/krok", "/foo", false),
        std::make_tuple("/foo/zorro/blah/krok", "/foo/zorro", false),
        std::make_tuple("/foo/zorro/blah/krok", "/foo/zorro/blah", false)
    ));

TEST_P(IsFileUnderDirectoryTest, BasicCases)
{
    auto const directoryPath = std::filesystem::path(std::get<0>(GetParam()));
    auto const filePath = std::filesystem::path(std::get<1>(GetParam()));

    bool const result = FileSystem::IsFileUnderDirectory(filePath, directoryPath);
    EXPECT_EQ(result, std::get<2>(GetParam()));
}
