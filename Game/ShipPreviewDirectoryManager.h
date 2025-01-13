/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "FileSystem.h"
#include "ShipDefinition.h"
#include "ShipPreviewData.h"
#include "ShipPreviewImageDatabase.h"

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

    RgbaImageData LoadPreviewImage(
        ShipPreviewData const & shipPreview,
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