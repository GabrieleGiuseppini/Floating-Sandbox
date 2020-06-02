/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipDefinition.h"
#include "ShipPreview.h"
#include "ShipPreviewImageDatabase.h"

#include <GameCore/ImageData.h>

#include <filesystem>
#include <map>
#include <memory>
#include <vector>

class ShipPreviewDirectoryManager final
{
public:

    static std::unique_ptr<ShipPreviewDirectoryManager> Create(std::filesystem::path const & directoryPath);

    std::vector<std::filesystem::path> const & GetShipFilePaths() const
    {
        return mShipFilePaths;
    }

    void Commit();

private:

    ShipPreviewDirectoryManager(
        std::filesystem::path const & directoryPath,
        std::vector<std::filesystem::path> && shipFilePaths,
        PersistedShipPreviewImageDatabase && oldDatabase)
        : mDirectoryPath(directoryPath)
        , mShipFilePaths(std::move(shipFilePaths))
        , mOldDatabase(std::move(oldDatabase))
        , mNewDatabase()
    {}

private:

    std::filesystem::path const mDirectoryPath;
    std::vector<std::filesystem::path> const mShipFilePaths; // Sorted by filename
    PersistedShipPreviewImageDatabase const mOldDatabase;
    NewShipPreviewImageDatabase mNewDatabase;
};