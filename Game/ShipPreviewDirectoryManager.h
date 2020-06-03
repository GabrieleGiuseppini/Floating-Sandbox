/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipDefinition.h"
#include "ShipPreview.h"
#include "ShipPreviewImageDatabase.h"

#include <GameCore/FileSystem.h>
#include <GameCore/ImageData.h>

#include <filesystem>
#include <map>
#include <memory>
#include <vector>

class ShipPreviewDirectoryManager final
{
public:

    static std::unique_ptr<ShipPreviewDirectoryManager> Create(
        std::filesystem::path const & directoryPath);

    static std::unique_ptr<ShipPreviewDirectoryManager> Create(
        std::filesystem::path const & directoryPath,
        std::shared_ptr<IFileSystem> fileSystem);

    /*
     * Gets a list of all files in this directory that are ships. The files
     * are sorted by filename.
     */
    std::vector<std::filesystem::path> EnumerateShipFilePaths() const;

    RgbaImageData LoadPreviewImage(
        ShipPreview const & shipPreview,
        ImageSize const & maxImageSize);

    void Commit(bool isVisitCompleted);

private:

    ShipPreviewDirectoryManager(
        std::filesystem::path const & directoryPath,
        std::shared_ptr<IFileSystem> fileSystem,
        PersistedShipPreviewImageDatabase && oldDatabase)
        : mDirectoryPath(directoryPath)
        , mFileSystem(fileSystem)
        , mOldDatabase(std::move(oldDatabase))
        , mNewDatabase(fileSystem)
    {}

private:

    std::filesystem::path const mDirectoryPath;

    std::shared_ptr<IFileSystem> mFileSystem;

    PersistedShipPreviewImageDatabase mOldDatabase;
    NewShipPreviewImageDatabase mNewDatabase;
};