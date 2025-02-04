/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-10-05
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "FileStreams.h"

#include <Core/GameException.h>
#include <Core/Log.h>
#include <Core/Streams.h>
#include <Core/Utils.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <regex>
#include <vector>

 /*
  * File system utilities.
  */
class FileSystem final
{
public:

    static bool Exists(std::filesystem::path const & path)
    {
        return std::filesystem::exists(path);
    }

    static std::filesystem::file_time_type GetLastModifiedTime(std::filesystem::path const & path)
    {
        return std::filesystem::last_write_time(path);
    }

    static void EnsureDirectoryExists(std::filesystem::path const & directoryPath)
    {
        if (!std::filesystem::exists(directoryPath))
        {
            std::filesystem::create_directories(directoryPath);
        }
    }

    static std::vector<std::filesystem::path> ListFiles(std::filesystem::path const & directoryPath)
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

    static void DeleteFile(std::filesystem::path const & filePath)
    {
        std::filesystem::remove(filePath);
    }

    static void RenameFile(
        std::filesystem::path const & oldFilePath,
        std::filesystem::path const & newFilePath)
    {
        std::filesystem::rename(oldFilePath, newFilePath);
    }

    static std::regex MakeFilenameMatchRegex(std::string const & pattern)
    {
        std::string regexPattern = Utils::FindAndReplaceAll(pattern, ".", "\\.");
        regexPattern = Utils::FindAndReplaceAll(regexPattern, "*", ".*");

        return std::regex(regexPattern, std::regex_constants::icase);
    }

    static std::string MakeFilenameSafeString(std::string const & str)
    {
        // Make sure the filename may be converted to the local codepage
        // (see https://developercommunity.visualstudio.com/content/problem/721120/stdfilesystempathgeneric-string-throws-an-exceptio.html)

        // Go char by char and only add safe chars

        std::string result;

        for (size_t i = 0; i < str.length(); ++i)
        {
            if (str[i] != '\\' && str[i] != '/' && str[i] != ':' && str[i] != '\"' && str[i] != '*')
            {
                std::string const charString = str.substr(i, 1);
                std::string const strTest = result + charString;

                try
                {
                    std::string _ = std::filesystem::path(strTest).filename().string();
                    (void)_;

                    // Safe, keep it
                    result += charString;
                }
                catch (...)
                {
                    // Skip it
                }
            }
        }

        return result;
    }

    static bool IsFileUnderDirectory(
        std::filesystem::path const & filePath,
        std::filesystem::path const & directoryPath)
    {
        std::filesystem::path const normalizedFilePath = filePath.lexically_normal();
        std::filesystem::path const normalizedDirectoryPath = directoryPath.lexically_normal();
        auto const [dirEnd, _] = std::mismatch(normalizedDirectoryPath.begin(), normalizedDirectoryPath.end(), normalizedFilePath.begin(), normalizedFilePath.end());
        return dirEnd == normalizedDirectoryPath.end();
    }
};


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
     * Opens a binary file for reading. Throws if the file does not exist.
     */
    virtual std::unique_ptr<BinaryReadStream> OpenBinaryInputStream(std::filesystem::path const & filePath) = 0;

    /*
     * Opens a text file for reading. Throws if the file does not exist.
     */
    virtual std::unique_ptr<TextReadStream> OpenTextInputStream(std::filesystem::path const & filePath) = 0;

    /*
     * Opens a binary file for writing. Overwrites the files if it exists already.
     * Also ensures the directory exists.
     */
    virtual std::unique_ptr<BinaryWriteStream> OpenBinaryOutputStream(std::filesystem::path const & filePath) = 0;

    /*
     * Opens a text file for writing. Overwrites the files if it exists already.
     * Also ensures the directory exists.
     */
    virtual std::unique_ptr<TextWriteStream> OpenTextOutputStream(std::filesystem::path const & filePath) = 0;

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
class FileSystemImpl final : public IFileSystem
{
public:

    FileSystemImpl()
    {}

    bool Exists(std::filesystem::path const & path) override
    {
        return FileSystem::Exists(path);
    }

    std::filesystem::file_time_type GetLastModifiedTime(std::filesystem::path const & path) override
    {
        return FileSystem::GetLastModifiedTime(path);
    }

    void EnsureDirectoryExists(std::filesystem::path const & directoryPath) override
    {
        FileSystem::EnsureDirectoryExists(directoryPath);
    }

    std::unique_ptr<BinaryReadStream> OpenBinaryInputStream(std::filesystem::path const & filePath) override
    {
        return std::make_unique<FileBinaryReadStream>(filePath);
    }

    std::unique_ptr<TextReadStream> OpenTextInputStream(std::filesystem::path const & filePath) override
    {
        return std::make_unique<FileTextReadStream>(filePath);
    }

    std::unique_ptr<BinaryWriteStream> OpenBinaryOutputStream(std::filesystem::path const & filePath) override
    {
        return std::make_unique<FileBinaryWriteStream>(filePath);
    }

    std::unique_ptr<TextWriteStream> OpenTextOutputStream(std::filesystem::path const & filePath) override
    {
        return std::make_unique<FileTextWriteStream>(filePath);
    }

    std::vector<std::filesystem::path> ListFiles(std::filesystem::path const & directoryPath) override
    {
        return FileSystem::ListFiles(directoryPath);
    }

    void DeleteFile(std::filesystem::path const & filePath) override
    {
        FileSystem::DeleteFile(filePath);
    }

    void RenameFile(
        std::filesystem::path const & oldFilePath,
        std::filesystem::path const & newFilePath) override
    {
        FileSystem::RenameFile(oldFilePath, newFilePath);
    }
};

