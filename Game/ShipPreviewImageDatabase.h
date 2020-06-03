/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/FileSystem.h>
#include <GameCore/ImageData.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <memory>

class ShipPreviewImageDatabase
{
protected:

};

class PersistedShipPreviewImageDatabase final : ShipPreviewImageDatabase
{
public:

    static PersistedShipPreviewImageDatabase Load(
        std::filesystem::path const & databaseFilePath,
        std::shared_ptr<IFileSystem> fileSystem);

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
    {}

    /*
     * Invoked always, even for unchanged preview images.
     */
    void Add(
        std::filesystem::path const & previewImageFilePath,
        std::filesystem::file_time_type previewImageFileLastModified,
        std::unique_ptr<RgbaImageData> previewImage); // null if no change from old DB

    bool Commit(
        std::filesystem::path const & databaseFilePath,
        PersistedShipPreviewImageDatabase const & oldDatabase,
        bool isVisitCompleted) const;

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