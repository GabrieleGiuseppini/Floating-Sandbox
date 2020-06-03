/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/FileSystem.h>
#include <GameCore/ImageData.h>
#include <GameCore/Version.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#pragma pack(push)

class ShipPreviewImageDatabase
{
protected:

    using StringSizeType = std::uint16_t;

    struct DatabaseStructure
    {
        struct FileHeader
        {
            static std::array<char, 32> constexpr StockTitle{ 'F', 'L', 'O', 'A', 'T', 'I', 'N', 'G', ' ', 'S', 'A', 'N', 'D', 'B', 'O', 'X', ' ', 'S', 'H', 'I', 'P', ' ', 'P', 'R', 'E', 'V', 'I', 'E', 'W', ' ', 'D', 'B' };

            std::array<char, 32> Title;
            Version GameVersion;

            FileHeader(Version gameVersion)
                : GameVersion(gameVersion)
            {
                std::memcpy(Title.data(), StockTitle.data(), Title.size());
            }
        };

        struct IndexEntry
        {
            std::filesystem::file_time_type LastModified;
            std::istream::pos_type Position;
            size_t Size;
            StringSizeType FilenameLength;
            // Filename chars follow
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
    };

    using ByteBuffer = std::vector<char>;

    static void SerializeIndexEntry(
        ByteBuffer & buffer,
        std::filesystem::path const & filename,
        std::filesystem::file_time_type lastModified,
        std::istream::pos_type position,
        size_t size);

    static void DeserializeIndexEntry(
        ByteBuffer const & buffer,
        std::filesystem::path & filename,
        std::filesystem::file_time_type & lastModified,
        std::istream::pos_type & position,
        size_t & size);

    // Greedy buffer for copying data
    mutable std::vector<char> mDataBuffer;
};

#pragma pack(pop)

class PersistedShipPreviewImageDatabase final : ShipPreviewImageDatabase
{
public:

    static PersistedShipPreviewImageDatabase Load(
        std::filesystem::path const & databaseFilePath,
        std::shared_ptr<IFileSystem> fileSystem);

    std::optional<RgbaImageData> TryGetPreviewImage(
        std::filesystem::path const & previewImageFilename,
        std::filesystem::file_time_type lastModifiedTime);

    void Close();

private:

    struct PreviewImageInfo;

    // Makes for an empty DB
    PersistedShipPreviewImageDatabase(std::shared_ptr<IFileSystem> && mFileSystem)
        : mFileSystem(std::move(mFileSystem))
        , mDatabaseFileStream()
        , mIndex()
    {}

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

        PreviewImageInfo(
            std::filesystem::file_time_type lastModified,
            std::istream::pos_type position,
            size_t size)
            : LastModified(lastModified)
            , Position(position)
            , Size(size)
        {}
    };

    // Key is filename
    std::map<std::filesystem::path, PreviewImageInfo> mIndex;
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
        bool isVisitCompleted) const;

private:

    void AppendFromOldDatabase(
        std::ostream & newDatabaseFile,
        std::istream & oldDatabaseFile,
        std::istream::pos_type startOffset,
        size_t size) const;

    void AppendFromData(
        std::ostream & newDatabaseFile,
        char const * data,
        size_t size) const;

private:

    static size_t constexpr MinShipsForDatabase = 5;

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