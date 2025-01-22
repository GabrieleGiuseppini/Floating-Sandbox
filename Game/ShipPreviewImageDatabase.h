/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "FileSystem.h"
#include "Version.h"

#include <GameCore/ImageData.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <vector>

class ShipPreviewImageDatabase
{
protected:

    using StringSizeType = std::uint16_t;

    struct DatabaseStructure
    {
#pragma pack(push, 1)
        struct FileHeader
        {
            static std::array<char, 32> constexpr StockTitle{ 'F', 'L', 'O', 'A', 'T', 'I', 'N', 'G', ' ', 'S', 'A', 'N', 'D', 'B', 'O', 'X', ' ', 'S', 'H', 'I', 'P', ' ', 'P', 'R', 'E', 'V', 'I', 'E', 'W', ' ', 'D', 'B' };

            std::array<char, 32> Title;
            Version GameVersion;
            size_t SizeOfSizeT;

            FileHeader(Version gameVersion)
                : GameVersion(gameVersion)
                , SizeOfSizeT(sizeof(size_t))
            {
                std::memcpy(Title.data(), StockTitle.data(), Title.size());
            }
        };

        struct IndexEntry
        {
            std::filesystem::file_time_type LastModified;
            std::istream::pos_type Position;
            size_t Size;
            ImageSize Dimensions;
            StringSizeType FilenameLength;
            // Filename chars follow

            IndexEntry()
                : Dimensions(0, 0)
            {}
        };

        struct FileTrailer
        {
            static std::array<char, 32> constexpr StockTitle{ 'T', 'A', 'I', 'L', 'T', 'A', 'I', 'L', 'T', 'A', 'I', 'L', 'T', 'A', 'I', 'L', 'T', 'A', 'I', 'L', 'T', 'A', 'I', 'L', 'T', 'A', 'I', 'L', 'T', 'A', 'I', 'L' };

            std::istream::pos_type IndexOffset;
            std::array<char, 32> Title;

            FileTrailer(std::istream::pos_type indexOffset)
                : IndexOffset(indexOffset)
            {
                std::memcpy(Title.data(), StockTitle.data(), Title.size());
            }
        };
#pragma pack(pop)

        static size_t constexpr PreviewImageStartOffset = sizeof(DatabaseStructure::FileHeader);
    };

    static constexpr size_t EstimatedIndexEntrySize = sizeof(DatabaseStructure) + 40;

    using ByteBuffer = std::vector<char>;

    static void SerializeIndexEntry(
        ByteBuffer & buffer,
        std::filesystem::path const & filename,
        std::filesystem::file_time_type lastModified,
        std::istream::pos_type position,
        size_t size,
        ImageSize dimensions);

    static size_t DeserializeIndexEntry(
        ByteBuffer const & buffer,
        size_t bufferIndex,
        std::filesystem::path & filename,
        std::filesystem::file_time_type & lastModified,
        std::istream::pos_type & position,
        size_t & size,
        ImageSize & dimensions);

    static size_t SerializePreviewImage(
        std::ostream & outputFile,
        RgbaImageData const & previewImage);

    static RgbaImageData DeserializePreviewImage(
        std::istream & inputFile,
        size_t size,
        ImageSize dimensions);
};

class PersistedShipPreviewImageDatabase final : ShipPreviewImageDatabase
{
public:

    static PersistedShipPreviewImageDatabase Load(
        std::filesystem::path const & databaseFilePath,
        std::shared_ptr<IFileSystem> fileSystem);

    // Makes for an empty DB
    PersistedShipPreviewImageDatabase(std::shared_ptr<IFileSystem> && mFileSystem)
        : mFileSystem(std::move(mFileSystem))
        , mDatabaseFileStream()
        , mIndex()
    {}

    std::optional<RgbaImageData> TryGetPreviewImage(
        std::filesystem::path const & previewImageFilename,
        std::filesystem::file_time_type lastModifiedTime);

    void Close();

private:

    struct PreviewImageInfo;

    PersistedShipPreviewImageDatabase(
        std::shared_ptr<std::istream> && databaseFileStream,
        std::map<std::filesystem::path, PreviewImageInfo> && index,
        std::shared_ptr<IFileSystem> && mFileSystem)
        : mFileSystem(std::move(mFileSystem))
        , mDatabaseFileStream(std::move(databaseFileStream))
        , mIndex(std::move(index))
    {}

private:

    friend class NewShipPreviewImageDatabase;

    std::shared_ptr<IFileSystem> mFileSystem;

    std::shared_ptr<std::istream> mDatabaseFileStream;

    struct PreviewImageInfo
    {
        std::filesystem::file_time_type LastModified;
        std::istream::pos_type Position;
        size_t Size;
        ImageSize Dimensions;

        PreviewImageInfo(
            std::filesystem::file_time_type lastModified,
            std::istream::pos_type position,
            size_t size,
            ImageSize dimensions)
            : LastModified(lastModified)
            , Position(position)
            , Size(size)
            , Dimensions(dimensions)
        {}
    };

    // Key is filename
    std::map<std::filesystem::path, PreviewImageInfo> mIndex;

private:

    friend class ShipPreviewImageDatabaseTests_Commit_CompleteVisit_NoOldDatabase_Test;
    friend class ShipPreviewImageDatabaseTests_Commit_CompleteVisit_NoOldDatabase_NoDbIfLessThanMinimumShips_Test;
    friend class ShipPreviewImageDatabaseTests_Commit_NewSmallerThanOld_CompleteVisit_Shrinks_Test;
    friend class ShipPreviewImageDatabaseTests_Commit_NewSmallerThanOld_IncompleteVisit_DoesNotShrink_Test;
    friend class ShipPreviewImageDatabaseTests_Commit_OverwritesAll_Test;
    friend class ShipPreviewImageDatabaseTests_Commit_NewAdds1_AtBeginning_Test;
    friend class ShipPreviewImageDatabaseTests_Commit_NewAdds2_AtBeginning_Test;
    friend class ShipPreviewImageDatabaseTests_Commit_NewAdds1_InMiddle_Test;
    friend class ShipPreviewImageDatabaseTests_Commit_NewAdds2_InMiddle_Test;
    friend class ShipPreviewImageDatabaseTests_Commit_NewAdds1_AtEnd_Test;
    friend class ShipPreviewImageDatabaseTests_Commit_NewAdds2_AtEnd_Test;
    friend class ShipPreviewImageDatabaseTests_Commit_NewOverwrites1_Test;
};

class NewShipPreviewImageDatabase final : ShipPreviewImageDatabase
{
public:

    NewShipPreviewImageDatabase(std::shared_ptr<IFileSystem> fileSystem)
        : mFileSystem(std::move(fileSystem))
        , mIndex()
    {
    }

    bool IsEmpty() const
    {
        return mIndex.empty();
    }

    /*
     * Invoked always, even for unchanged preview images.
     */
    void Add(
        std::filesystem::path const & previewImageFilename,
        std::filesystem::file_time_type previewImageFileLastModified,
        std::unique_ptr<RgbaImageData> previewImage); // null if no change from old DB

    bool Commit(
        std::filesystem::path const & databaseFilePath,
        PersistedShipPreviewImageDatabase const & oldDatabase,
        bool isVisitCompleted,
        size_t minShipsForDatabase = 10) const;

private:

    void WriteFromOldDatabase(
        std::ostream & newDatabaseFile,
        std::istream & oldDatabaseFile,
        std::istream::pos_type startOffset,
        size_t size) const;

    void WriteFromData(
        std::ostream & newDatabaseFile,
        char const * data,
        size_t size) const;

private:

    std::shared_ptr<IFileSystem> mFileSystem;

    struct PreviewImageInfo
    {
        std::filesystem::file_time_type LastModified;
        std::unique_ptr<RgbaImageData> PreviewImage; // null if no change from old DB

        PreviewImageInfo(
            std::filesystem::file_time_type lastModified,
            std::unique_ptr<RgbaImageData> && previewImage)
            : LastModified(lastModified)
            , PreviewImage(std::move(previewImage))
        {}
    };

    // Key is filename
    std::map<std::filesystem::path, PreviewImageInfo> mIndex;
};