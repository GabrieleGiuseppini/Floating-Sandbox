/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-10-05
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <GameCore/Log.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

/*
 * Abstraction of file-system primitives to ease unit tests.
 */
struct IFileSystem
{
    virtual ~IFileSystem()
    {}

    /*
     * Checks whether a file or directory exists.
     */
    virtual bool Exists(std::filesystem::path const & path) = 0;

    /*
     * Gets the last-modified timestamp of a file.
     */
    virtual std::filesystem::file_time_type GetLastModifiedTime(std::filesystem::path const & path) = 0;

    /*
     * Creates a directory if it doesn't exist already.
     */
    virtual void EnsureDirectoryExists(std::filesystem::path const & directoryPath) = 0;

    /*
     * Opens a file for reading. Returns an empty pointer if the file does not exist.
     */
    virtual std::shared_ptr<std::istream> OpenInputStream(std::filesystem::path const & filePath) = 0;

    /*
     * Opens a file for writing. Overwrites the files if it exists already.
     *
     * The file is flushed and closed when the shared pointer goes out of scope.
     */
    virtual std::shared_ptr<std::ostream> OpenOutputStream(std::filesystem::path const & filePath) = 0;

    /*
     * Returns paths of all files in the specified directory.
     */
    virtual std::vector<std::filesystem::path> ListFiles(std::filesystem::path const & directoryPath) = 0;

    /*
     * Deletes a file.
     */
    virtual void DeleteFile(std::filesystem::path const & filePath) = 0;

    /*
     * Renames a file.
     */
    virtual void RenameFile(
        std::filesystem::path const & oldFilePath,
        std::filesystem::path const & newFilePath) = 0;
};

/*
 * IFileSystem concrete implementation working against the real file system.
 */
class FileSystem final : public IFileSystem
{
public:

    FileSystem()
    {}

    bool Exists(std::filesystem::path const & path) override
    {
        return std::filesystem::exists(path);
    }

    std::filesystem::file_time_type GetLastModifiedTime(std::filesystem::path const & path) override
    {
        return std::filesystem::last_write_time(path);
    }

    void EnsureDirectoryExists(std::filesystem::path const & directoryPath) override
    {
        std::filesystem::create_directories(directoryPath);
    }

    std::shared_ptr<std::istream> OpenInputStream(std::filesystem::path const & filePath) override
    {
        if (std::filesystem::exists(filePath)
            && std::filesystem::is_regular_file(filePath))
        {
            return std::shared_ptr<std::ifstream>(
                new std::ifstream(
                    filePath,
                    std::ios_base::in | std::ios_base::binary));
        }
        else
        {
            return std::unique_ptr<std::istream>();
        }
    }

    std::shared_ptr<std::ostream> OpenOutputStream(std::filesystem::path const & filePath) override
    {
        return std::shared_ptr<std::ostream>(
            new std::ofstream(
                filePath,
                std::ios_base::out | std::ios_base::binary | std::ios_base::trunc),
            [](std::ostream * os)
            {
                os->flush();
                delete os;
            });
    }

    virtual std::vector<std::filesystem::path> ListFiles(std::filesystem::path const & directoryPath) override
    {
        std::vector<std::filesystem::path> filePaths;

        // Be robust to users messing up
        if (std::filesystem::exists(directoryPath)
            && std::filesystem::is_directory(directoryPath))
        {
            auto directoryIterator = std::filesystem::directory_iterator(
                directoryPath,
                std::filesystem::directory_options::skip_permission_denied | std::filesystem::directory_options::follow_directory_symlink);

            for (auto const & entryIt : directoryIterator)
            {
                try
                {
                    auto const entryFilepath = entryIt.path();

                    if (std::filesystem::is_regular_file(entryFilepath))
                    {
                        // Make sure the filename may be converted to the local codepage
                        // (see https://developercommunity.visualstudio.com/content/problem/721120/stdfilesystempathgeneric-string-throws-an-exceptio.html)
                        std::string _ = entryFilepath.filename().string();
                        (void)_;

                        filePaths.push_back(entryFilepath);
                    }
                }
                catch (std::exception const & ex)
                {
                    LogMessage("Ignoring a file directory entry due to error: ", ex.what());

                    // Ignore this file
                }
            }
        }

        return filePaths;
    }

    virtual void DeleteFile(std::filesystem::path const & filePath) override
    {
        std::filesystem::remove(filePath);
    }

    virtual void RenameFile(
        std::filesystem::path const & oldFilePath,
        std::filesystem::path const & newFilePath) override
    {
        std::filesystem::rename(oldFilePath, newFilePath);
    }
};
