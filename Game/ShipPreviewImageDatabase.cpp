/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-06-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipPreviewImageDatabase.h"

#include <GameCore/GameException.h>
#include <GameCore/Log.h>

#include <limits>
#include <utility>

void ShipPreviewImageDatabase::SerializeIndexEntry(
    ByteBuffer & buffer,
    std::filesystem::path const & filename,
    std::filesystem::file_time_type lastModified,
    std::istream::pos_type position,
    size_t size)
{
    auto const filenameString = filename.string();
    if (filenameString.size() > std::numeric_limits<StringSizeType>::max())
    {
        throw GameException("Filename is too long");
    }

    DatabaseStructure::IndexEntry indexEntry;

    indexEntry.LastModified = lastModified;
    indexEntry.Position = position;
    indexEntry.Size = size;
    indexEntry.FilenameLength = static_cast<StringSizeType>(filenameString.size());

    size_t const totalIndexEntrySize = sizeof(DatabaseStructure::IndexEntry) + filenameString.size();

    size_t const initialBufferSize = buffer.size();
    buffer.resize(initialBufferSize + totalIndexEntrySize);

    // Index entry
    std::memcpy(
        buffer.data() + initialBufferSize,
        reinterpret_cast<char const *>(&indexEntry),
        sizeof(DatabaseStructure::IndexEntry));

    // Filename characters
    std::memcpy(
        buffer.data() + initialBufferSize + sizeof(DatabaseStructure::IndexEntry),
        filenameString.data(),
        filenameString.size());
}

void ShipPreviewImageDatabase::DeserializeIndexEntry(
    ByteBuffer const & buffer,
    std::filesystem::path & filename,
    std::filesystem::file_time_type & lastModified,
    std::istream::pos_type & position,
    size_t & size)
{

}

//////////////////////////////////////////////////////////////////////////////////////////////////

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

std::optional<RgbaImageData> PersistedShipPreviewImageDatabase::TryGetPreviewImage(
    std::filesystem::path const & previewImageFilename,
    std::filesystem::file_time_type lastModifiedTime)
{
    // See if may serve from cache
    auto const it = mIndex.find(previewImageFilename);
    if (it != mIndex.end()
        && lastModifiedTime <= it->second.LastModified)
    {
        // TODOHERE
        return std::nullopt;
    }

    // No luck
    return std::nullopt;
}

void PersistedShipPreviewImageDatabase::Close()
{
    mDatabaseFileStream.reset();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewShipPreviewImageDatabase::Add(
    std::filesystem::path const & previewImageFilename,
    std::filesystem::file_time_type previewImageFileLastModified,
    std::unique_ptr<RgbaImageData> previewImage)
{
    // Store in index
    auto [it, isInserted] = mIndex.try_emplace(
        previewImageFilename,
        previewImageFileLastModified,
        std::move(previewImage));

    // Make sure not already in map
    if (!isInserted)
    {
        throw GameException("Preview for '" + previewImageFilename.string() + "' is already present in database");
    }
}

bool NewShipPreviewImageDatabase::Commit(
    std::filesystem::path const & databaseFilePath,
    PersistedShipPreviewImageDatabase const & oldDatabase,
    bool isVisitCompleted) const
{
    // Do not create a database for just a few ships
    if (oldDatabase.mIndex.empty()
        && mIndex.size() < MinShipsForDatabase)
    {
        return false;
    }

    // If the visit is not completed, we want to continue
    // only when we have information about (potentially) more
    // previews than there are in the old database
    if (!isVisitCompleted
        && mIndex.size() <= oldDatabase.mIndex.size())
    {
        return false;
    }

    // If we are here, either the visit was completed, or
    // it was not completed but we have more entries than
    // in the old database
    assert(isVisitCompleted || mIndex.size() > oldDatabase.mIndex.size());

    //
    //
    //

    std::map<std::filesystem::path, PersistedShipPreviewImageDatabase::PreviewImageInfo> newPersistedIndex;

    //
    // Calculate longest possible streak of preview image data that
    // may be copied from old DB
    //

    size_t const copyStartOffset = sizeof(DatabaseStructure::FileHeader);
    size_t copyEndOffset = copyStartOffset;

    auto oldDbIt = oldDatabase.mIndex.cbegin();
    auto newDbIt = mIndex.cbegin();

    for (; oldDbIt != oldDatabase.mIndex.cend() && newDbIt != mIndex.cend(); ++oldDbIt, ++newDbIt)
    {
        if (oldDbIt->first == newDbIt->first    // Entry is for same preview file in both DBs
            && !(newDbIt->second.PreviewImage)) // New DB has no new info for this entry
        {
            LogMessage("NewShipPreviewImageDatabase::Commit(): copying old preview image data for '", oldDbIt->first.string(), "' (coalescing)...");

            // Extend copy
            copyEndOffset += oldDbIt->second.Size;

            // Add entry to new index
            newPersistedIndex.emplace(
                oldDbIt->first,
                oldDbIt->second);
        }
        else
        {
            // Stop with copying
            break;
        }
    }

    if (oldDbIt == oldDatabase.mIndex.cend() && newDbIt == mIndex.cend())
    {
        // New DB is exactly like old DB...
        // ...nothing to commit
        LogMessage("NewShipPreviewImageDatabase::Commit(): new DB matches old DB, nothing to commit");
        return false;
    }

    //
    // Save database
    //

    std::shared_ptr<std::ostream> outputStream = mFileSystem->OpenOutputStream(databaseFilePath);

    //
    // 1) Write header
    //

    DatabaseStructure::FileHeader header(Version::CurrentVersion());

    AppendFromData(
        *outputStream,
        reinterpret_cast<char *>(&header),
        sizeof(DatabaseStructure::FileHeader));

    //
    // 2) Copy preview images from old DB
    //

    if (copyEndOffset > copyStartOffset)
    {
        assert(!!oldDatabase.mDatabaseFileStream);

        AppendFromOldDatabase(
            *outputStream,
            *oldDatabase.mDatabaseFileStream,
            copyStartOffset,
            copyEndOffset - copyStartOffset);
    }

    //
    // 3) Copy/Write all remaining preview images
    //

    size_t currentOffset = copyEndOffset;

    for (; newDbIt != mIndex.cend(); ++newDbIt)
    {
        if (!newDbIt->second.PreviewImage)
        {
            // No new preview image data for this entry...
            // ...copy from old

            LogMessage("NewShipPreviewImageDatabase::Commit(): copying old preview image data for '", newDbIt->first.string(), "'...");

            auto const & oldEntry = oldDatabase.mIndex.find(newDbIt->first);
            assert(oldEntry != oldDatabase.mIndex.end());

            assert(!!oldDatabase.mDatabaseFileStream);

            AppendFromOldDatabase(
                *outputStream,
                *oldDatabase.mDatabaseFileStream,
                oldEntry->second.Position,
                oldEntry->second.Size);

            // Add entry to new index
            newPersistedIndex.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(newDbIt->first),
                std::forward_as_tuple(newDbIt->second.LastModified, currentOffset, oldEntry->second.Size));

            // Advance offset
            currentOffset += oldEntry->second.Size;
        }
        else
        {
            // There is new preview image data for this entry...
            // ...save it

            LogMessage("NewShipPreviewImageDatabase::Commit(): saving new preview image data for '", newDbIt->first.string(), "'...");

            auto const previewImageByteSize = newDbIt->second.PreviewImage->GetByteSize();

            AppendFromData(
                *outputStream,
                reinterpret_cast<char *>(newDbIt->second.PreviewImage->Data.get()),
                previewImageByteSize);

            // Add entry to new index
            newPersistedIndex.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(newDbIt->first),
                std::forward_as_tuple(newDbIt->second.LastModified, currentOffset, previewImageByteSize));

            // Advance offset
            currentOffset += previewImageByteSize;
        }
    }

    //
    // 4) Save index
    //

    // Save index offset for later
    size_t const indexOffset = currentOffset;

    // Prepare data buffer
    mDataBuffer.clear();

    // Serialize all entries to buffer
    for (auto const entry : newPersistedIndex)
    {
        SerializeIndexEntry(
            mDataBuffer,
            entry.first,
            entry.second.LastModified,
            entry.second.Position,
            entry.second.Size);
    }

    // Append buffer
    AppendFromData(
        *outputStream,
        mDataBuffer.data(),
        mDataBuffer.size());

    currentOffset += mDataBuffer.size();

    //
    // 5) Append tail
    //

    DatabaseStructure::FileTrailer trailer(indexOffset);

    AppendFromData(
        *outputStream,
        reinterpret_cast<char *>(&trailer),
        sizeof(DatabaseStructure::FileTrailer));

    // Close output file
    outputStream.reset();

    return true;
}

void NewShipPreviewImageDatabase::AppendFromOldDatabase(
    std::ostream & newDatabaseFile,
    std::istream & oldDatabaseFile,
    std::istream::pos_type startOffset,
    size_t size) const
{
    size_t constexpr BlockSize = 4 * 1024 * 1024;

    if (mDataBuffer.size() < BlockSize)
        mDataBuffer.resize(BlockSize);

    oldDatabaseFile.seekg(startOffset);

    for (size_t copied = 0; copied < size; copied += BlockSize)
    {
        auto const toCopy = (size - copied) >= BlockSize ? BlockSize : (size - copied);
        oldDatabaseFile.read(mDataBuffer.data(), toCopy);
        newDatabaseFile.write(mDataBuffer.data(), toCopy);
    }
}

void NewShipPreviewImageDatabase::AppendFromData(
    std::ostream & newDatabaseFile,
    char const * data,
    size_t size) const
{
    newDatabaseFile.write(data, size);
}