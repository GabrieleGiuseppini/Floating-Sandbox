/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewDirectoryManager.h"

#include "ShipDefinitionFile.h"

#include <GameCore/Log.h>

#include <algorithm>

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
            return ShipDefinitionFile::IsShipDefinitionFile(filePath);
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
    // Get last-modified of preview image file
    auto const previewImageFileLastModified = mFileSystem->GetLastModifiedTime(shipPreview.PreviewImageFilePath);

    // See if this preview file may be served by old database
    // TODOHERE
    return shipPreview.LoadPreviewImage(maxImageSize);
}

void ShipPreviewDirectoryManager::Commit(bool isVisitCompleted)
{
    LogMessage("ShipPreviewDirectoryManager::Commit(", isVisitCompleted ? "true" : "false", "): started...");

    auto const newDatabaseTemporaryFilePath = (mDirectoryPath / DatabaseFileName).replace_extension("tmp");

    // 1) Commit new database
    bool hasFileBeenCreated = mNewDatabase.Commit(
        newDatabaseTemporaryFilePath,
        mOldDatabase,
        isVisitCompleted);

    // 2) Close old database
    mOldDatabase.Close();

    // 3) Swap temp file
    if (hasFileBeenCreated)
    {
        try
        {
            auto const newDatabaseFilePath = mDirectoryPath / DatabaseFileName;

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

    LogMessage("ShipPreviewDirectoryManager::Commit(): ...completed.");
}