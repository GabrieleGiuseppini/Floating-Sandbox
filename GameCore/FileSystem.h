/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-10-05
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <filesystem>
#include <fstream>
#include <memory>

/* 
 * Abstraction of file-system primitives to ease unit tests.
 */
struct IFileSystem
{
    virtual ~IFileSystem()
    {}

    /*
     * Opens a file for reading. Returns an empty pointer if the file does not exist.
     */
    virtual std::shared_ptr<std::istream> OpenInputStream(std::filesystem::path const & filePath) = 0;

    /*
     * Opens a file for writing. Overwrites the files if it exists already.
     */
    virtual std::shared_ptr<std::ostream> OpenOutputStream(std::filesystem::path const & filePath) = 0;
};

/*
 * IFileSystem concrete implementation working against the real file system.
 */
class FileSystem final : public IFileSystem 
{
public:

    FileSystem()
    {}

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
};
