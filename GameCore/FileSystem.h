/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-10-05
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

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
			for (auto const & entryIt : std::filesystem::directory_iterator(directoryPath))
			{
				if (std::filesystem::is_regular_file(entryIt.path()))
				{
					filePaths.push_back(entryIt.path());
				}
			}
		}

        return filePaths;
    }

    virtual void DeleteFile(std::filesystem::path const & filePath) override
    {
        std::filesystem::remove(filePath);
    }
};
