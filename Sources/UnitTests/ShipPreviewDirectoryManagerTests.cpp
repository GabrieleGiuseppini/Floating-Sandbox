#include <Game/ShipPreviewDirectoryManager.h>

#include "TestingUtils.h"

#include <iomanip>
#include <sstream>

#include "gtest/gtest.h"

namespace {

    static std::filesystem::path TestRootDirectory = "C:\\Foo";

    static std::filesystem::path TmpDatabaseFilePath = "C:\\Foo\\tmpdb.db";

    auto MakePreviewImage(int magicNumber)
    {
        ImageSize dimensions(magicNumber, magicNumber);

        std::unique_ptr<rgbaColor[]> buffer = std::make_unique<rgbaColor[]>(dimensions.GetLinearSize());

        return RgbaImageData(
            dimensions,
            std::move(buffer));
    }

    PersistedShipPreviewImageDatabase MakeOldDb(
        std::vector<std::string> previewImageFileNames,
        std::filesystem::path const & databaseFilePath,
        std::shared_ptr<TestFileSystem> testFileSystem)
    {
        auto newDb = NewShipPreviewImageDatabase(testFileSystem);
        for (size_t i = 0; i < previewImageFileNames.size(); ++i)
        {
            newDb.Add(
                previewImageFileNames[i],
                std::filesystem::file_time_type::min() + std::chrono::seconds(10 + i),
                std::make_unique<RgbaImageData>(MakePreviewImage(static_cast<int>(i) + 1)));
        }

        auto const oldDb = PersistedShipPreviewImageDatabase(testFileSystem);

        newDb.Commit(
            databaseFilePath,
            oldDb,
            true,
            0);

        return PersistedShipPreviewImageDatabase::Load(
            databaseFilePath,
            testFileSystem);
    }

    PersistedShipPreviewImageDatabase MakeOldDb(
        size_t numEntries,
        std::filesystem::path const & databaseFilePath,
        std::shared_ptr<TestFileSystem> testFileSystem)
    {
        std::vector<std::string> previewImageFileNames;
        for (size_t i = 0; i < numEntries; ++i)
        {
            std::stringstream ss;
            ss << std::setw(5) << std::setfill('0') << (i * 10);

            previewImageFileNames.push_back(ss.str() + "_preview_image");
        }

        return MakeOldDb(previewImageFileNames, databaseFilePath, testFileSystem);
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

TEST_F(ShipPreviewImageDatabaseTests, Commit_NewSmallerThanOld_CompleteVisit_Shrinks)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    //
    // Make old DB
    //

    auto oldDb = MakeOldDb(
        10,
        "foo1",
        testFileSystem);

    //
    // Make new DB
    //

    auto newDb = NewShipPreviewImageDatabase(testFileSystem);

    newDb.Add(
        "b_preview_image_1",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(1)));

    newDb.Add(
        "a_preview_image_2",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(2)));

    newDb.Add(
        "a_preview_image_3",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(3)));

    //
    // Commit
    //

    auto const newDbFilename = "bar";

    bool const isCreated = newDb.Commit(
        newDbFilename,
        oldDb,
        true,
        1);

    ASSERT_TRUE(isCreated);

    //
    // Verify new DB file created
    //

    PersistedShipPreviewImageDatabase verifyDb = PersistedShipPreviewImageDatabase::Load(
        newDbFilename,
        testFileSystem);

    EXPECT_EQ(3u, verifyDb.mIndex.size());
}

TEST_F(ShipPreviewImageDatabaseTests, Commit_NewSmallerThanOld_IncompleteVisit_DoesNotShrink)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    //
    // Make old DB
    //

    auto oldDb = MakeOldDb(
        10,
        "foo1",
        testFileSystem);

    //
    // Make new DB
    //

    auto newDb = NewShipPreviewImageDatabase(testFileSystem);

    newDb.Add(
        "b_preview_image_1",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(1)));

    newDb.Add(
        "a_preview_image_2",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(2)));

    newDb.Add(
        "a_preview_image_3",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(3)));

    //
    // Commit
    //

    auto const newDbFilename = "bar";

    bool const isCreated = newDb.Commit(
        newDbFilename,
        oldDb,
        false,
        1);

    ASSERT_FALSE(isCreated);

    //
    // Verify new DB file not created
    //

    PersistedShipPreviewImageDatabase verifyDb = PersistedShipPreviewImageDatabase::Load(
        newDbFilename,
        testFileSystem);

    EXPECT_EQ(0u, verifyDb.mIndex.size());
}

TEST_F(ShipPreviewImageDatabaseTests, Commit_OverwritesAll)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    //
    // Make old DB
    //

    auto oldDb = MakeOldDb(
        {
            "preview_d",
            "preview_m",
            "preview_s"
        },
        "foo1",
        testFileSystem);

    //
    // Make new DB
    //

    auto newDb = NewShipPreviewImageDatabase(testFileSystem);

    newDb.Add(
        "preview_d",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(20)));

    newDb.Add(
        "preview_m",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(21)));

    newDb.Add(
        "preview_s",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(22)));

    //
    // Commit
    //

    auto const newDbFilename = "bar";

    bool const isCreated = newDb.Commit(
        newDbFilename,
        oldDb,
        true,
        1);

    ASSERT_TRUE(isCreated);

    //
    // Verify new DB file created
    //

    PersistedShipPreviewImageDatabase verifyDb = PersistedShipPreviewImageDatabase::Load(
        newDbFilename,
        testFileSystem);

    EXPECT_EQ(3u, verifyDb.mIndex.size());

    auto verifyIndexIt = verifyDb.mIndex.cbegin();
    EXPECT_EQ("preview_d", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(20, 20), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_m", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(21, 21), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_s", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(22, 22), verifyIndexIt->second.Dimensions);
}

TEST_F(ShipPreviewImageDatabaseTests, Commit_NewAdds1_AtBeginning)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    //
    // Make old DB
    //

    auto oldDb = MakeOldDb(
        {
            "preview_d",
            "preview_m",
            "preview_s"
        },
        "foo1",
        testFileSystem);

    //
    // Make new DB
    //

    auto newDb = NewShipPreviewImageDatabase(testFileSystem);

    newDb.Add(
        "preview_d",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_m",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_s",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_a",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(4)));

    //
    // Commit
    //

    auto const newDbFilename = "bar";

    bool const isCreated = newDb.Commit(
        newDbFilename,
        oldDb,
        true,
        1);

    ASSERT_TRUE(isCreated);

    //
    // Verify new DB file created
    //

    PersistedShipPreviewImageDatabase verifyDb = PersistedShipPreviewImageDatabase::Load(
        newDbFilename,
        testFileSystem);

    EXPECT_EQ(4u, verifyDb.mIndex.size());

    auto verifyIndexIt = verifyDb.mIndex.cbegin();
    EXPECT_EQ("preview_a", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(4, 4), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_d", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(1, 1), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_m", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(2, 2), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_s", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(3, 3), verifyIndexIt->second.Dimensions);
}

TEST_F(ShipPreviewImageDatabaseTests, Commit_NewAdds2_AtBeginning)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    //
    // Make old DB
    //

    auto oldDb = MakeOldDb(
        {
            "preview_d",
            "preview_m",
            "preview_s"
        },
        "foo1",
        testFileSystem);

    //
    // Make new DB
    //

    auto newDb = NewShipPreviewImageDatabase(testFileSystem);

    newDb.Add(
        "preview_d",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_b",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(5)));

    newDb.Add(
        "preview_m",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_s",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_a",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(4)));

    //
    // Commit
    //

    auto const newDbFilename = "bar";

    bool const isCreated = newDb.Commit(
        newDbFilename,
        oldDb,
        true,
        1);

    ASSERT_TRUE(isCreated);

    //
    // Verify new DB file created
    //

    PersistedShipPreviewImageDatabase verifyDb = PersistedShipPreviewImageDatabase::Load(
        newDbFilename,
        testFileSystem);

    EXPECT_EQ(5u, verifyDb.mIndex.size());

    auto verifyIndexIt = verifyDb.mIndex.cbegin();
    EXPECT_EQ("preview_a", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(4, 4), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_b", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(5, 5), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_d", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(1, 1), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_m", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(2, 2), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_s", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(3, 3), verifyIndexIt->second.Dimensions);
}

TEST_F(ShipPreviewImageDatabaseTests, Commit_NewAdds1_InMiddle)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    //
    // Make old DB
    //

    auto oldDb = MakeOldDb(
        {
            "preview_d",
            "preview_m",
            "preview_s"
        },
        "foo1",
        testFileSystem);

    //
    // Make new DB
    //

    auto newDb = NewShipPreviewImageDatabase(testFileSystem);

    newDb.Add(
        "preview_d",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_m",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_s",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_f",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(4)));

    //
    // Commit
    //

    auto const newDbFilename = "bar";

    bool const isCreated = newDb.Commit(
        newDbFilename,
        oldDb,
        true,
        1);

    ASSERT_TRUE(isCreated);

    //
    // Verify new DB file created
    //

    PersistedShipPreviewImageDatabase verifyDb = PersistedShipPreviewImageDatabase::Load(
        newDbFilename,
        testFileSystem);

    EXPECT_EQ(4u, verifyDb.mIndex.size());

    auto verifyIndexIt = verifyDb.mIndex.cbegin();
    EXPECT_EQ("preview_d", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(1, 1), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_f", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(4, 4), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_m", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(2, 2), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_s", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(3, 3), verifyIndexIt->second.Dimensions);
}

TEST_F(ShipPreviewImageDatabaseTests, Commit_NewAdds2_InMiddle)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    //
    // Make old DB
    //

    auto oldDb = MakeOldDb(
        {
            "preview_d",
            "preview_m",
            "preview_s"
        },
        "foo1",
        testFileSystem);

    //
    // Make new DB
    //

    auto newDb = NewShipPreviewImageDatabase(testFileSystem);

    newDb.Add(
        "preview_d",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_m",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_s",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_f",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(4)));

    newDb.Add(
        "preview_g",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(5)));

    //
    // Commit
    //

    auto const newDbFilename = "bar";

    bool const isCreated = newDb.Commit(
        newDbFilename,
        oldDb,
        true,
        1);

    ASSERT_TRUE(isCreated);

    //
    // Verify new DB file created
    //

    PersistedShipPreviewImageDatabase verifyDb = PersistedShipPreviewImageDatabase::Load(
        newDbFilename,
        testFileSystem);

    EXPECT_EQ(5u, verifyDb.mIndex.size());

    auto verifyIndexIt = verifyDb.mIndex.cbegin();
    EXPECT_EQ("preview_d", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(1, 1), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_f", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(4, 4), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_g", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(5, 5), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_m", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(2, 2), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_s", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(3, 3), verifyIndexIt->second.Dimensions);
}

TEST_F(ShipPreviewImageDatabaseTests, Commit_NewAdds1_AtEnd)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    //
    // Make old DB
    //

    auto oldDb = MakeOldDb(
        {
            "preview_d",
            "preview_m",
            "preview_s"
        },
        "foo1",
        testFileSystem);

    //
    // Make new DB
    //

    auto newDb = NewShipPreviewImageDatabase(testFileSystem);

    newDb.Add(
        "preview_d",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_m",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_s",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_t",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(4)));

    //
    // Commit
    //

    auto const newDbFilename = "bar";

    bool const isCreated = newDb.Commit(
        newDbFilename,
        oldDb,
        true,
        1);

    ASSERT_TRUE(isCreated);

    //
    // Verify new DB file created
    //

    PersistedShipPreviewImageDatabase verifyDb = PersistedShipPreviewImageDatabase::Load(
        newDbFilename,
        testFileSystem);

    EXPECT_EQ(4u, verifyDb.mIndex.size());

    auto verifyIndexIt = verifyDb.mIndex.cbegin();
    EXPECT_EQ("preview_d", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(1, 1), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_m", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(2, 2), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_s", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(3, 3), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_t", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(4, 4), verifyIndexIt->second.Dimensions);
}

TEST_F(ShipPreviewImageDatabaseTests, Commit_NewAdds2_AtEnd)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    //
    // Make old DB
    //

    auto oldDb = MakeOldDb(
        {
            "preview_d",
            "preview_m",
            "preview_s"
        },
        "foo1",
        testFileSystem);

    //
    // Make new DB
    //

    auto newDb = NewShipPreviewImageDatabase(testFileSystem);

    newDb.Add(
        "preview_z",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(5)));

    newDb.Add(
        "preview_d",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_m",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_s",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_t",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(4)));

    //
    // Commit
    //

    auto const newDbFilename = "bar";

    bool const isCreated = newDb.Commit(
        newDbFilename,
        oldDb,
        true,
        1);

    ASSERT_TRUE(isCreated);

    //
    // Verify new DB file created
    //

    PersistedShipPreviewImageDatabase verifyDb = PersistedShipPreviewImageDatabase::Load(
        newDbFilename,
        testFileSystem);

    EXPECT_EQ(5u, verifyDb.mIndex.size());

    auto verifyIndexIt = verifyDb.mIndex.cbegin();
    EXPECT_EQ("preview_d", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(1, 1), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_m", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(2, 2), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_s", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(3, 3), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_t", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(4, 4), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_z", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(5, 5), verifyIndexIt->second.Dimensions);
}

TEST_F(ShipPreviewImageDatabaseTests, Commit_NewOverwrites1)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    //
    // Make old DB
    //

    auto oldDb = MakeOldDb(
        {
            "preview_d",
            "preview_m",
            "preview_s"
        },
        "foo1",
        testFileSystem);

    //
    // Make new DB
    //

    auto newDb = NewShipPreviewImageDatabase(testFileSystem);

    newDb.Add(
        "preview_d",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    newDb.Add(
        "preview_m",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        std::make_unique<RgbaImageData>(MakePreviewImage(5)));

    newDb.Add(
        "preview_s",
        std::filesystem::file_time_type::min() + std::chrono::seconds(10),
        nullptr);

    //
    // Commit
    //

    auto const newDbFilename = "bar";

    bool const isCreated = newDb.Commit(
        newDbFilename,
        oldDb,
        true,
        1);

    ASSERT_TRUE(isCreated);

    //
    // Verify new DB file created
    //

    PersistedShipPreviewImageDatabase verifyDb = PersistedShipPreviewImageDatabase::Load(
        newDbFilename,
        testFileSystem);

    EXPECT_EQ(3u, verifyDb.mIndex.size());

    auto verifyIndexIt = verifyDb.mIndex.cbegin();
    EXPECT_EQ("preview_d", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(1, 1), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_m", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(5, 5), verifyIndexIt->second.Dimensions);

    ++verifyIndexIt;
    EXPECT_EQ("preview_s", verifyIndexIt->first.string());
    EXPECT_EQ(ImageSize(3, 3), verifyIndexIt->second.Dimensions);
}