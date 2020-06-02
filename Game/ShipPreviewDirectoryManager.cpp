/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewDirectoryManager.h"

std::unique_ptr<ShipPreviewDirectoryManager> ShipPreviewDirectoryManager::Create(std::filesystem::path const & directoryPath)
{
    // TODO
    return nullptr;
}

void ShipPreviewDirectoryManager::Commit()
{
    mNewDatabase.Commit(
        mDirectoryPath,
        mOldDatabase);
}