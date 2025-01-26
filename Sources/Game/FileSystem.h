/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-10-05
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Core/GameException.h>
#include <Core/Log.h>
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

    static std::shared_ptr<std::istream> OpenInputStream(std::filesystem::path const & filePath)
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

    static std::shared_ptr<std::ostream> OpenOutputStream(std::filesystem::path const & filePath)
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

    static std::string LoadTextFile(std::filesystem::path const & filePath)
    {
        std::ifstream file(filePath.string(), std::ios::in);
        if (!file.is_open())
        {
            throw GameException("Cannot open file \"" + filePath.string() + "\" for reading");
        }

        std::stringstream ss;
        ss << file.rdbuf();

        std::string content = ss.str();

        // For some reason, the preferences file sometimes is made of all null characters
        content.erase(
            std::find(content.begin(), content.end(), '\0'),
            content.end());

        return content;
    }

    static std::vector<std::string> LoadTextFileLines(std::filesystem::path const & filePath)
    {
        std::ifstream file(filePath.string(), std::ios::in);
        if (!file.is_open())
        {
            throw GameException("Cannot open file \"" + filePath.string() + "\" for reading");
        }

        std::string line;
        std::vector<std::string> lines;
        while (std::getline(file, line))
        {
            lines.emplace_back(line);
        }

        return lines;
    }

    static void SaveTextFile(
        std::string const & content,
        std::filesystem::path const & filePath)
    {
        // Make sure directory exists
        EnsureDirectoryExists(filePath.parent_path());

        // Open file
        std::ofstream outputFile(filePath, std::ios_base::out | std::ios_base::trunc);
        if (!outputFile.is_open())
        {
            throw GameException("Cannot open file \"" + filePath.string() + "\" for writing");
        }

        // Dump
        outputFile << content;

        // Close
        outputFile.flush();
        outputFile.close();
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

    std::shared_ptr<std::istream> OpenInputStream(std::filesystem::path const & filePath) override
    {
        return FileSystem::OpenInputStream(filePath);
    }

    std::shared_ptr<std::ostream> OpenOutputStream(std::filesystem::path const & filePath) override
    {
        return FileSystem::OpenOutputStream(filePath);
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

