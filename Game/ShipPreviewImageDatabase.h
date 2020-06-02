/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/ImageData.h>

#include <filesystem>
#include <iostream>
#include <map>
#include <memory>

class ShipPreviewImageDatabase
{
};

class PersistedShipPreviewImageDatabase final : ShipPreviewImageDatabase
{
public:

private:

    friend class NewShipPreviewImageDatabase;

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

    NewShipPreviewImageDatabase()
        : mIndex()
        , mIsDeprecated(false)
    {}

    void Add(
        std::filesystem::path const & previewImageFilePath,
        std::filesystem::file_time_type previewImageFileLastModified,
        std::unique_ptr<RgbaImageData> previewImage); // null if no change from old DB

    void Deprecate()
    {
        mIsDeprecated = true;
    }

    void Commit(
        std::filesystem::path const & directoryPath,
        PersistedShipPreviewImageDatabase const & oldDatabase) const;

private:

    struct PreviewImageInfo
    {
        std::filesystem::file_time_type LastModified;
        std::unique_ptr<RgbaImageData> PreviewImage; // null if no change from old DB
    };

    // Key is filename
    std::map<std::filesystem::path, PreviewImageInfo> mIndex;

    bool mIsDeprecated;
};