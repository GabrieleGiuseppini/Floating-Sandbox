/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewDirectoryManager.h"

#include "ShipDeSerializer.h"

#include <GameCore/Log.h>

#include <algorithm>
#include <chrono>

static std::filesystem::path const DatabaseFileName = ".floatingsandbox_shipdb";

std::unique_ptr<ShipPreviewDirectoryManager> ShipPreviewDirectoryManager::Create(std::filesystem::path const & directoryPath)
{
    return Create(
        directoryPath,
        std::make_shared<FileSystem>());
}

std::unique_ptr<ShipPreviewDirectoryManager> ShipPreviewDirectoryManager::Create(
    std::filesystem::path const & directoryPath,
    std::shared_ptr<IFileSystem> fileSystem)
{
    return std::unique_ptr<ShipPreviewDirectoryManager>(
        new ShipPreviewDirectoryManager(
            directoryPath,
            fileSystem,
            PersistedShipPreviewImageDatabase::Load(directoryPath / DatabaseFileName, fileSystem)));
}

std::vector<std::filesystem::path> ShipPreviewDirectoryManager::EnumerateShipFilePaths() const
{
    std::vector<std::filesystem::path> shipFilePaths;

    //
    // Enumerate files
    //

    LogMessage("ShipPreviewDirectoryManager::EnumerateShipFilePaths(): start");

    auto allFilePaths = mFileSystem->ListFiles(mDirectoryPath);

    std::copy_if(
        allFilePaths.cbegin(),
        allFilePaths.cend(),
        std::back_inserter(shipFilePaths),
        [](auto const & filePath)
        {
            return ShipDeSerializer::IsAnyShipDefinitionFile(filePath);
        });

    //
    // Sort by filename
    //

    std::sort(
        shipFilePaths.begin(),
        shipFilePaths.end(),
        [](auto const & a, auto const & b) -> bool
        {
            return a.filename().compare(b.filename()) < 0;
        });

    LogMessage("ShipPreviewDirectoryManager::EnumerateShipFilePaths(): end (", shipFilePaths.size(), " files)");

    return shipFilePaths;
}

RgbaImageData ShipPreviewDirectoryManager::LoadPreviewImage(
    ShipPreview const & shipPreview,
    ImageSize const & maxImageSize)
{
    auto const previewImageFilename = shipPreview.PreviewImageFilePath.filename();

    // Get last-modified of preview image file
    // (will throw if the file does not exist)
    auto const previewImageFileLastModified = mFileSystem->GetLastModifiedTime(shipPreview.PreviewImageFilePath);

    // See if this preview file may be served by old database
    auto oldDbPreviewImage = mOldDatabase.TryGetPreviewImage(previewImageFilename, previewImageFileLastModified);
    if (oldDbPreviewImage.has_value())
    {
        //
        // Served by DB
        //

        // Tell new DB that this preview comes from old DB
        mNewDatabase.Add(
            previewImageFilename,
            previewImageFileLastModified,
            nullptr);

        return std::move(*oldDbPreviewImage);
    }
    else
    {
        //
        // Not served by DB
        //

        // Needs to be loaded from scratch
        LogMessage("ShipPreviewDirectoryManager::LoadPreviewImage(): can't serve '", previewImageFilename.string(), "' from persisted DB; loading...");

        // Load preview image
        RgbaImageData previewImage = shipPreview.LoadPreviewImage(maxImageSize);

        // Add to new DB
        mNewDatabase.Add(
            previewImageFilename,
            previewImageFileLastModified,
            previewImage.MakeCopy());

        return previewImage;
    }
}

void ShipPreviewDirectoryManager::Commit(bool isVisitCompleted)
{
    LogMessage("ShipPreviewDirectoryManager::Commit(", isVisitCompleted ? "true" : "false", "): started...");

    auto const startTime = std::chrono::steady_clock::now();

    auto const newDatabaseFilePath = mDirectoryPath / DatabaseFileName;
    auto const newDatabaseTemporaryFilePath = std::filesystem::path(newDatabaseFilePath).replace_extension("tmp");

    // Commit new database
    bool const hasFileBeenCreated = mNewDatabase.Commit(
        newDatabaseTemporaryFilePath,
        mOldDatabase,
        isVisitCompleted);

    // Close old database
    mOldDatabase.Close();

    if (hasFileBeenCreated)
    {
        // Swap temp file

        try
        {
            // Delete old database file
            mFileSystem->DeleteFile(newDatabaseFilePath);

            // Rename temp DB as new DB
            mFileSystem->RenameFile(newDatabaseTemporaryFilePath, newDatabaseFilePath);
        }
        catch (std::exception const & exc)
        {
            LogMessage("ShipPreviewDirectoryManager::Commit(): error: ", exc.what());
        }
    }
    else
    {
        // Delete database if there's nothing in this folder for it
        if (mNewDatabase.IsEmpty()
            && isVisitCompleted)
        {
            mFileSystem->DeleteFile(newDatabaseFilePath);
        }
    }

    auto const endTime = std::chrono::steady_clock::now();

    LogMessage("ShipPreviewDirectoryManager::Commit(): ...completed (",
        std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count(), "us)");
}