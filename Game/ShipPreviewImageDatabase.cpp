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

    LogMessage("TODOHERE: sizeof(IndexEntry)=", sizeof(DatabaseStructure::IndexEntry),
        " file_time_type=", sizeof(std::filesystem::file_time_type),
        " pos_type=", sizeof(std::istream::pos_type),
        " size_t=", sizeof(size_t),
        " StringSizeType=", sizeof(StringSizeType));

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

size_t ShipPreviewImageDatabase::DeserializeIndexEntry(
    ByteBuffer const & buffer,
    size_t bufferIndex,
    std::filesystem::path & filename,
    std::filesystem::file_time_type & lastModified,
    std::istream::pos_type & position,
    size_t & size)
{
    DatabaseStructure::IndexEntry const * const indexEntry =
        reinterpret_cast<DatabaseStructure::IndexEntry const *>(&(buffer[bufferIndex]));

    lastModified = indexEntry->LastModified;
    position = indexEntry->Position;
    size = indexEntry->Size;

    std::string filenameString = std::string(
        &(buffer[bufferIndex + sizeof(DatabaseStructure::IndexEntry)]),
        indexEntry->FilenameLength);

    filename = std::filesystem::path(filenameString);

    return bufferIndex + sizeof(DatabaseStructure::IndexEntry) + indexEntry->FilenameLength;
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
            databaseFileStream->exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);

            // Load and check header
            {
                DatabaseStructure::FileHeader header(Version::Zero());
                databaseFileStream->read(reinterpret_cast<char *>(&header), sizeof(DatabaseStructure::FileHeader));

                if (!databaseFileStream->good()
                    || 0 != strncmp(header.Title.data(), DatabaseStructure::FileHeader::StockTitle.data(), header.Title.size()))
                {
                    throw std::exception("Database file is not recognized");
                }

                if (header.GameVersion > Version::CurrentVersion())
                {
                    throw std::exception("Database file was generated on a more recent version of the simulator");
                }

                if (header.SizeOfInt != sizeof(int))
                {
                    throw std::exception("Database file was generated on a different platform");
                }
            }

            // Read and populate index
            {
                // Move to beginning of tail
                databaseFileStream->seekg(-static_cast<std::streampos>(sizeof(DatabaseStructure::FileTrailer)), std::ios_base::end);

                // Save end index position
                auto const endIndexPosition = databaseFileStream->tellg();

                // Read tail
                DatabaseStructure::FileTrailer trailer(0);
                databaseFileStream->read(reinterpret_cast<char *>(&trailer), sizeof(DatabaseStructure::FileTrailer));

                // Save total file size
                auto const totalFileSize = databaseFileStream->tellg();

                // Check tail
                if (trailer.IndexOffset >= totalFileSize
                    || 0 != strncmp(trailer.Title.data(), DatabaseStructure::FileTrailer::StockTitle.data(), trailer.Title.size()))
                {
                    throw std::exception("Database file was not properly closed");
                }

                // Move to beginning of index
                databaseFileStream->seekg(trailer.IndexOffset, std::ios_base::beg);

                // Load index
                {
                    size_t const indexSize = endIndexPosition - trailer.IndexOffset;

                    // Alloc buffer
                    std::vector<char> indexBuffer(indexSize);

                    // Read whole index
                    databaseFileStream->read(indexBuffer.data(), indexBuffer.size());

                    // Deserialize entries
                    for (size_t indexOffset = 0; indexOffset != indexSize; /* incremented in loop */)
                    {
                        if (indexOffset > indexSize)
                        {
                            throw std::exception("Out-of-sync while deserializing index");
                        }

                        std::filesystem::path filename;
                        std::filesystem::file_time_type lastModified;
                        std::istream::pos_type position;
                        size_t size;

                        indexOffset = DeserializeIndexEntry(
                            indexBuffer,
                            indexOffset,
                            filename,
                            lastModified,
                            position,
                            size);

                        auto [it, isInserted] = index.try_emplace(
                            filename,
                            lastModified,
                            position,
                            size);

                        if (!isInserted)
                        {
                            throw std::exception("Index is inconsistent");
                        }
                    }
                }
            }
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
    static size_t constexpr MinShipsForDatabase = 10;

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
    // Prepare new index
    //

    // Prepare buffer
    size_t const newIndexEstimatedSize = EstimatedIndexEntrySize * mIndex.size();
    ByteBuffer newIndexBuffer;
    newIndexBuffer.reserve(newIndexEstimatedSize);

    //
    // Calculate longest possible streak of preview image data that
    // may be copied from old DB
    //
    // Here we optimize for a slowly-growing database that ultimately
    // covers an entire directory.
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
            SerializeIndexEntry(
                newIndexBuffer,
                oldDbIt->first,
                oldDbIt->second.LastModified,
                oldDbIt->second.Position,
                oldDbIt->second.Size);
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

    WriteFromData(
        *outputStream,
        reinterpret_cast<char *>(&header),
        sizeof(DatabaseStructure::FileHeader));

    //
    // 2) Copy preview images from old DB
    //

    if (copyEndOffset > copyStartOffset)
    {
        assert(!!oldDatabase.mDatabaseFileStream);

        WriteFromOldDatabase(
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

            WriteFromOldDatabase(
                *outputStream,
                *oldDatabase.mDatabaseFileStream,
                oldEntry->second.Position,
                oldEntry->second.Size);

            // Add entry to new index
            SerializeIndexEntry(
                newIndexBuffer,
                newDbIt->first,
                newDbIt->second.LastModified,
                currentOffset,
                oldEntry->second.Size);

            // Advance offset
            currentOffset += oldEntry->second.Size;
        }
        else
        {
            // There is new preview image data for this entry...
            // ...save it

            LogMessage("NewShipPreviewImageDatabase::Commit(): saving new preview image data for '", newDbIt->first.string(), "'...");

            auto const previewImageByteSize = newDbIt->second.PreviewImage->GetByteSize();

            WriteFromData(
                *outputStream,
                reinterpret_cast<char *>(newDbIt->second.PreviewImage->Data.get()),
                previewImageByteSize);

            // Add entry to new index
            SerializeIndexEntry(
                newIndexBuffer,
                newDbIt->first,
                newDbIt->second.LastModified,
                currentOffset,
                previewImageByteSize);

            // Advance offset
            currentOffset += previewImageByteSize;
        }
    }

    //
    // 4) Save index
    //

    // Save index start offset for later
    size_t const indexOffset = currentOffset;

    WriteFromData(
        *outputStream,
        newIndexBuffer.data(),
        newIndexBuffer.size());

    currentOffset += newIndexBuffer.size();

    //
    // 5) Append tail
    //

    DatabaseStructure::FileTrailer trailer(indexOffset);

    WriteFromData(
        *outputStream,
        reinterpret_cast<char *>(&trailer),
        sizeof(DatabaseStructure::FileTrailer));

    // Close output file
    outputStream.reset();

    return true;
}

void NewShipPreviewImageDatabase::WriteFromOldDatabase(
    std::ostream & newDatabaseFile,
    std::istream & oldDatabaseFile,
    std::istream::pos_type startOffset,
    size_t size) const
{
    size_t constexpr BlockSize = 4 * 1024 * 1024;

    std::vector<char> copyBuffer(BlockSize);

    oldDatabaseFile.seekg(startOffset);

    for (size_t copied = 0; copied < size; copied += BlockSize)
    {
        auto const toCopy = (size - copied) >= BlockSize ? BlockSize : (size - copied);
        oldDatabaseFile.read(copyBuffer.data(), toCopy);
        newDatabaseFile.write(copyBuffer.data(), toCopy);
    }
}

void NewShipPreviewImageDatabase::WriteFromData(
    std::ostream & newDatabaseFile,
    char const * data,
    size_t size) const
{
    newDatabaseFile.write(data, size);
}