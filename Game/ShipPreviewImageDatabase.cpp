/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewImageDatabase.h"

#include <GameCore/Log.h>

PersistedShipPreviewImageDatabase PersistedShipPreviewImageDatabase::Load(
    std::filesystem::path const & databaseFilePath,
    std::shared_ptr<IFileSystem> fileSystem)
{
    try
    {
        std::shared_ptr<std::istream> databaseFileStream;
        std::map<std::filesystem::path, PreviewImageInfo> index;

        // Check if database file exists
        if (fileSystem->Exists(databaseFilePath))
        {
            // Open file
            databaseFileStream = fileSystem->OpenInputStream(databaseFilePath);

            // Populate index
            // TODOHERE
        }
        else
        {
            LogMessage("PersistedShipPreviewImageDatabase: no ship database found at \"", databaseFilePath.string(), "\"");
        }

        return PersistedShipPreviewImageDatabase(
            std::move(databaseFileStream),
            std::move(index),
            std::move(fileSystem));
    }
    catch (std::exception const & exc)
    {
        LogMessage("PersistedShipPreviewImageDatabase: error loading ship database \"", databaseFilePath.string(), "\": ", exc.what());

        // Ignore and continue as empty database
        return PersistedShipPreviewImageDatabase(std::move(fileSystem));
    }
}

void PersistedShipPreviewImageDatabase::Close()
{
    // TODOHERE
}

void NewShipPreviewImageDatabase::Add(
    std::filesystem::path const & previewImageFilePath,
    std::filesystem::file_time_type previewImageFileLastModified,
    std::unique_ptr<RgbaImageData> previewImage)
{
    // Store in index
    auto [it, isInserted] = mIndex.try_emplace(
        previewImageFilePath,
        previewImageFileLastModified,
        std::move(previewImage));

    // Make sure not already in map
    if (!isInserted)
    {
        throw GameException("Preview for \"" + previewImageFilePath.string() + "\" is already present in daatabase");
    }
}

bool NewShipPreviewImageDatabase::Commit(
    std::filesystem::path const & databaseFilePath,
    PersistedShipPreviewImageDatabase const & oldDatabase,
    bool isVisitCompleted) const
{
    // TODO
    return false;
}