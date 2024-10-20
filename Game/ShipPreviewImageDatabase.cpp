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
    size_t size,
    ImageSize dimensions)
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
    indexEntry.Dimensions = dimensions;
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

size_t ShipPreviewImageDatabase::DeserializeIndexEntry(
    ByteBuffer const & buffer,
    size_t bufferIndex,
    std::filesystem::path & filename,
    std::filesystem::file_time_type & lastModified,
    std::istream::pos_type & position,
    size_t & size,
    ImageSize & dimensions)
{
    DatabaseStructure::IndexEntry indexEntry;

    std::memcpy(
        reinterpret_cast<char *>(&indexEntry),
        &(buffer[bufferIndex]),
        sizeof(DatabaseStructure::IndexEntry));

    lastModified = indexEntry.LastModified;
    position = indexEntry.Position;
    size = indexEntry.Size;
    dimensions = indexEntry.Dimensions;

    std::string filenameString = std::string(
        &(buffer[bufferIndex + sizeof(DatabaseStructure::IndexEntry)]),
        indexEntry.FilenameLength);

    filename = std::filesystem::path(filenameString);

    return bufferIndex + sizeof(DatabaseStructure::IndexEntry) + indexEntry.FilenameLength;
}

size_t ShipPreviewImageDatabase::SerializePreviewImage(
    std::ostream & outputFile,
    RgbaImageData const & previewImage)
{
    auto const previewImageByteSize = previewImage.GetByteSize();

    outputFile.write(
        reinterpret_cast<char *>(previewImage.Data.get()),
        previewImageByteSize);

    return previewImageByteSize;
}

RgbaImageData ShipPreviewImageDatabase::DeserializePreviewImage(
    std::istream & inputFile,
    size_t size,
    ImageSize dimensions)
{
    // Alloc buffer
    std::unique_ptr<rgbaColor[]> buffer = std::make_unique<rgbaColor[]>(size);

    // Read
    inputFile.read(reinterpret_cast<char *>(buffer.get()), size);

    // Make image
    return RgbaImageData(
        dimensions,
        std::move(buffer));
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
                    throw std::runtime_error("Database file is not recognized");
                }

                if (header.GameVersion > Version::CurrentVersion())
                {
                    throw std::runtime_error("Database file was generated on a more recent version of the simulator");
                }

                if (header.SizeOfSizeT != sizeof(size_t))
                {
                    throw std::runtime_error("Database file was generated on a different platform");
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
                    throw std::runtime_error("Database file was not properly closed");
                }

                // Move to beginning of index
                databaseFileStream->seekg(trailer.IndexOffset, std::ios_base::beg);

                // Load index
                {
                    size_t const indexSize = static_cast<size_t>(endIndexPosition - trailer.IndexOffset);

                    // Alloc buffer
                    std::vector<char> indexBuffer(indexSize);

                    // Read whole index
                    databaseFileStream->read(indexBuffer.data(), indexBuffer.size());

                    // Deserialize entries
                    for (size_t indexOffset = 0; indexOffset != indexSize; /* incremented in loop */)
                    {
                        if (indexOffset >= indexSize)
                        {
                            throw std::runtime_error("Out-of-sync while deserializing index");
                        }

                        std::filesystem::path filename;
                        std::filesystem::file_time_type lastModified;
                        std::istream::pos_type position;
                        size_t size;
                        ImageSize dimensions(0, 0);

                        indexOffset = DeserializeIndexEntry(
                            indexBuffer,
                            indexOffset,
                            filename,
                            lastModified,
                            position,
                            size,
                            dimensions);

                        auto [_, isInserted] = index.try_emplace(
                            filename,
                            lastModified,
                            position,
                            size,
                            dimensions);

                        if (!isInserted)
                        {
                            throw std::runtime_error("Index is inconsistent");
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
    // See if may serve this file from the cache
    auto const cachedFileIt = mIndex.find(previewImageFilename);
    if (cachedFileIt != mIndex.end()
        && lastModifiedTime == cachedFileIt->second.LastModified)
    {
        //
        // Load preview from DB
        //

        // Position to the preview
        assert(!!mDatabaseFileStream);
        mDatabaseFileStream->seekg(cachedFileIt->second.Position);

        // Read preview
        return DeserializePreviewImage(
            *mDatabaseFileStream,
            cachedFileIt->second.Size,
            cachedFileIt->second.Dimensions);
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
        LogMessage("NewShipPreviewImageDatabase::Add: preview for '", previewImageFilename.string(), "' is already present in database");
    }
}

bool NewShipPreviewImageDatabase::Commit(
    std::filesystem::path const & databaseFilePath,
    PersistedShipPreviewImageDatabase const & oldDatabase,
    bool isVisitCompleted,
    size_t minShipsForDatabase) const
{
    // Do not create a database for just a few ships
    if (oldDatabase.mIndex.empty()
        && mIndex.size() < minShipsForDatabase)
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

    // Prepare buffer for new index
    size_t const newIndexEstimatedSize = EstimatedIndexEntrySize * mIndex.size();
    ByteBuffer newIndexBuffer;
    newIndexBuffer.reserve(newIndexEstimatedSize);

    //
    // Prepare output stream
    //

    std::shared_ptr<std::ostream> outputStream;

    auto const getOutputStream =
        [&]() -> std::ostream &
        {
            if (!outputStream)
            {
                // Open file

                outputStream = mFileSystem->OpenOutputStream(databaseFilePath);

                // Write header

                DatabaseStructure::FileHeader header(Version::CurrentVersion());

                WriteFromData(
                    *outputStream,
                    reinterpret_cast<char *>(&header),
                    sizeof(DatabaseStructure::FileHeader));
            }

            return *outputStream;
        };

    //
    // 1) Process new index elements vs old index elements
    //

    std::istream::pos_type currentNewDbPreviewImageOffset = DatabaseStructure::PreviewImageStartOffset;

    auto newDbIt = mIndex.cbegin();
    auto oldDbIt = oldDatabase.mIndex.cbegin();

    auto saveNewEntry =
        [&]() -> void
        {
            assert(newDbIt != mIndex.cend());
            assert(!!newDbIt->second.PreviewImage);

            LogMessage("NewShipPreviewImageDatabase::Commit(): saving new preview image data for '", newDbIt->first.string(), "'...");

            // Serialize preview image
            auto const previewImageByteSize = SerializePreviewImage(
                getOutputStream(),
                *(newDbIt->second.PreviewImage));

            // Add entry to new index
            SerializeIndexEntry(
                newIndexBuffer,
                newDbIt->first,
                newDbIt->second.LastModified,
                currentNewDbPreviewImageOffset,
                previewImageByteSize,
                newDbIt->second.PreviewImage->Size);

            // Advance preview image offset
            currentNewDbPreviewImageOffset += previewImageByteSize;
        };

    while (newDbIt != mIndex.cend() && oldDbIt != oldDatabase.mIndex.cend())
    {
        //
        // Catch-up old to new (i.e. skip old deleted files)
        //

        while (oldDbIt != oldDatabase.mIndex.cend() && oldDbIt->first < newDbIt->first)
        {
            ++oldDbIt;
        }

        if (oldDbIt == oldDatabase.mIndex.cend())
            break; // No more reason to continue here; may jump to streaming new

        //
        // Calc longest streak of old preview images matching new preview images
        //

        std::streampos copyOldDbStartOffset = oldDbIt->second.Position;
        std::streampos copyOldDbEndOffset = copyOldDbStartOffset;

        for (; oldDbIt != oldDatabase.mIndex.cend() && newDbIt != mIndex.cend(); ++oldDbIt, ++newDbIt)
        {
            if (oldDbIt->first == newDbIt->first    // Entry is for same preview file in both DBs
                && !(newDbIt->second.PreviewImage)) // New DB has no new info for this entry
            {
                // LogMessage("NewShipPreviewImageDatabase::Commit(): copying old preview image data for '", oldDbIt->first.string(), "' (coalescing)...");

                // Extend copy
                copyOldDbEndOffset += oldDbIt->second.Size;

                // Add entry to new index
                SerializeIndexEntry(
                    newIndexBuffer,
                    oldDbIt->first,
                    oldDbIt->second.LastModified,
                    currentNewDbPreviewImageOffset,
                    oldDbIt->second.Size,
                    oldDbIt->second.Dimensions);

                // Update next offset in preview image section of new db
                currentNewDbPreviewImageOffset += oldDbIt->second.Size;
            }
            else
            {
                // Stop with copying
                break;
            }
        }

        if (oldDbIt == oldDatabase.mIndex.cend() && newDbIt == mIndex.cend()
            && mIndex.size() == oldDatabase.mIndex.size()
            && copyOldDbStartOffset == static_cast<std::streampos>(DatabaseStructure::PreviewImageStartOffset))
        {
            // New DB is exactly like old DB...
            // ...nothing to commit
            LogMessage("NewShipPreviewImageDatabase::Commit(): new DB matches old DB, nothing to commit");
            return false;
        }

        //
        // Copy this streak of preview images from old DB
        //

        if (copyOldDbEndOffset > copyOldDbStartOffset)
        {
            assert(!!oldDatabase.mDatabaseFileStream);

            WriteFromOldDatabase(
                getOutputStream(),
                *oldDatabase.mDatabaseFileStream,
                copyOldDbStartOffset,
                static_cast<size_t>(copyOldDbEndOffset - copyOldDbStartOffset));

            // No need to advance preview image offset in new db,
            // we've been updating it all the time
        }

        //
        // At this moment, we have on of these options:
        //  - New DB is finished, or
        //  - Old DB is finished, or
        //  - New DB.Key > Old DB.Key [because of deleted files], or
        //  - New DB.Key < Old DB.Key [because of new files], or
        //  - New DB.Key == Old DB.Key [because new DB has newer image]
        //

        if (newDbIt == mIndex.cend()
            || oldDbIt == oldDatabase.mIndex.cend())
        {
            // No more reason to continue here; may jump to streaming
            // new and/or saving index
            break;
        }

        assert(newDbIt != mIndex.cend() && oldDbIt != oldDatabase.mIndex.cend());

        if (newDbIt->first <= oldDbIt->first)
        {
            //
            // Save this single new entry
            //

            saveNewEntry();

            // Advance new
            ++newDbIt;
        }
        else
        {
            assert(newDbIt->first > oldDbIt->first);

            // Will catch up old to new at next iteration
        }
    }

    //
    // 2) Serialize all remaining new entries to file
    //

    for (; newDbIt != mIndex.cend(); ++newDbIt)
    {
        assert(!!newDbIt->second.PreviewImage); // Or else we'd have still entries from the old DB

        saveNewEntry();
    }

    //
    // 3) Save index
    //

    // Save index start offset for later
    auto const newDbIndexStartOffset = currentNewDbPreviewImageOffset;

    WriteFromData(
        getOutputStream(),
        newIndexBuffer.data(),
        newIndexBuffer.size());

    //
    // 4) Append tail
    //

    DatabaseStructure::FileTrailer trailer(newDbIndexStartOffset);

    WriteFromData(
        getOutputStream(),
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