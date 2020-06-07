#include <Game/ShipPreviewDirectoryManager.h>

#include "Utils.h"

#include "gtest/gtest.h"

namespace {

    static std::filesystem::path TestRootDirectory = "C:\\Foo";

    static std::filesystem::path TmpDatabaseFilePath = "C:\\Foo\\tmpdb.db";

    auto MakePreviewImage(int magicNumber)
    {
        ImageSize dimensions(magicNumber, magicNumber);

        std::unique_ptr<rgbaColor[]> buffer = std::make_unique<rgbaColor[]>(dimensions.GetPixelCount());

        return RgbaImageData(
            dimensions,
            std::move(buffer));
    }

}

class ShipPreviewImageDatabaseTests : public testing::Test
{
};

TEST_F(ShipPreviewImageDatabaseTests, Commit_CompleteVisit_NoOldDatabase)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    auto oldDb = PersistedShipPreviewImageDatabase(testFileSystem);
    auto newDb = NewShipPreviewImageDatabase(testFileSystem);

    //
    // Populate new DB
    //

    newDb.Add(
        "b_preview_image_1",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(1)));

    newDb.Add(
        "a_preview_image_2",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(2)));

    //
    // Commit
    //

    bool const isCreated = newDb.Commit(
        TmpDatabaseFilePath,
        oldDb,
        true,
        1);

    //
    // Verify tmp DB file created
    //

    EXPECT_TRUE(isCreated);
    ASSERT_EQ(1u, testFileSystem->GetFileMap().size());
    EXPECT_EQ(1u, testFileSystem->GetFileMap().count(TmpDatabaseFilePath));

    //
    // Verify tmp DB file
    //

    PersistedShipPreviewImageDatabase verifyDb = PersistedShipPreviewImageDatabase::Load(
        TmpDatabaseFilePath,
        testFileSystem);

    ASSERT_EQ(2u, verifyDb.mIndex.size());

    auto verifyIndexIt = verifyDb.mIndex.cbegin();
    EXPECT_EQ("a_preview_image_2", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(2, 2), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("b_preview_image_1", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(1, 1), verifyIndexIt->second.Dimensions);
}

TEST_F(ShipPreviewImageDatabaseTests, Commit_CompleteVisit_NoOldDatabase_NoDbIfLessThanMinimumShips)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    auto oldDb = PersistedShipPreviewImageDatabase(testFileSystem);
    auto newDb = NewShipPreviewImageDatabase(testFileSystem);

    //
    // Populate new DB
    //

    newDb.Add(
        "b_preview_image_1",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(1)));

    newDb.Add(
        "a_preview_image_2",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(2)));

    //
    // Commit
    //

    bool const isCreated = newDb.Commit(
        TmpDatabaseFilePath,
        oldDb,
        true,
        5);

    //
    // Verify tmp DB file not created
    //

    EXPECT_FALSE(isCreated);
    ASSERT_EQ(0u, testFileSystem->GetFileMap().size());
}

// TODO: Commit_NewSmallerThanOld_CompleteVisit_Shrinks
// TODO: Commit_NewSmallerThanOld_IncompleteVisit_DoesNotShrink