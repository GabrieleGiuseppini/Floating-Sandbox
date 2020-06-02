/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewImageDatabase.h"

void NewShipPreviewImageDatabase::Add(
    std::filesystem::path const & previewImageFilePath,
    std::filesystem::file_time_type previewImageFileLastModified,
    std::unique_ptr<RgbaImageData> previewImage)
{
    // TODOHERE
}

void NewShipPreviewImageDatabase::Commit(
    std::filesystem::path const & directoryPath,
    PersistedShipPreviewImageDatabase const & oldDatabase) const
{
    // TODO
}