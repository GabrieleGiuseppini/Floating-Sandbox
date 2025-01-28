#include <Core/IAssetManager.h>
#include <Core/ImageData.h>
#include <Core/MemoryStreams.h>

#include <Game/FileSystem.h>
// TODOTEST
//#include <Game/Materials.h>

#include <map>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

// Test texture database in storage
struct TestTextureDatabase
{
    struct DatabaseFrameInfo
    {
        IAssetManager::AssetDescriptor AssetDescriptor;
        ImageSize FrameSize;
    };

    std::string DatabaseName;
    std::vector<DatabaseFrameInfo> FrameInfos;
    std::string DatabaseJson;
};

class TestAssetManager : public IAssetManager
{
public:

    std::vector<TestTextureDatabase> TestTextureDatabases;

public:

    TestAssetManager() = default;

    picojson::value LoadTetureDatabaseSpecification(std::string const & databaseName) override;
    ImageSize GetTextureDatabaseFrameSize(std::string const & databaseName, std::string const & frameRelativePath) override;
    RgbaImageData LoadTextureDatabaseFrameRGBA(std::string const & databaseName, std::string const & frameRelativePath) override;
    std::vector<AssetDescriptor> EnumerateTextureDatabaseFrames(std::string const & databaseName) override;

    picojson::value LoadTetureAtlasSpecification(std::string const & textureDatabaseName) override;
    RgbaImageData LoadTextureAtlasImageRGBA(std::string const & textureDatabaseName) override;

private:

    TestTextureDatabase const & GetDatabase(std::string const & databaseName);
};

class TestFileSystem : public IFileSystem
{
public:

    struct FileInfo
    {
        std::shared_ptr<memory_streambuf> StreamBuf;
        std::filesystem::file_time_type LastModified;
    };

    using FileMap = std::map<std::filesystem::path, FileInfo>;

    TestFileSystem()
        : mFileMap()
    {}

    FileMap & GetFileMap()
    {
        return mFileMap;
    }

    void PrepareTestFile(
        std::filesystem::path testFilePath,
        std::string content = "",
        std::filesystem::file_time_type lastModified = std::filesystem::file_time_type::clock::now())
    {
        auto & fileInfoEntry = mFileMap[testFilePath];
        fileInfoEntry.StreamBuf = std::make_shared<memory_streambuf>(content);
        fileInfoEntry.LastModified = lastModified;
    }

    std::string GetTestFileContent(std::filesystem::path testFilePath)
    {
        auto const it = mFileMap.find(testFilePath);
        if (it == mFileMap.end())
        {
            throw std::logic_error("File path '" + testFilePath.string() + "' does not exist in test file system");
        }

        if (!it->second.StreamBuf)
        {
            throw std::logic_error("GetTestFileContents() invoked for file with no stream: " + testFilePath.string());
        }

        return std::string(
            it->second.StreamBuf->data(),
            it->second.StreamBuf->size());
    }

    ///////////////////////////////////////////////////////////
    // IFileSystem
    ///////////////////////////////////////////////////////////

    bool Exists(std::filesystem::path const & path) override
    {
        auto it = mFileMap.find(path);
        return (it != mFileMap.end());
    }

    std::filesystem::file_time_type GetLastModifiedTime(std::filesystem::path const & path) override
    {
        auto const it = mFileMap.find(path);
        if (it == mFileMap.end())
        {
            throw std::logic_error("File path '" + path.string() + "' does not exist in test file system");
        }

        return it->second.LastModified;
    }

    void EnsureDirectoryExists(std::filesystem::path const & /*directoryPath*/) override
    {
        // Nop
    }

    std::shared_ptr<std::istream> OpenInputStream(std::filesystem::path const & filePath) override
    {
        auto it = mFileMap.find(filePath);
        if (it != mFileMap.end())
        {
            it->second.StreamBuf->rewind();
            return std::make_shared<std::istream>(it->second.StreamBuf.get());
        }
        else
        {
            return std::shared_ptr<std::istream>();
        }
    }

    std::shared_ptr<std::ostream> OpenOutputStream(std::filesystem::path const & filePath) override
    {
        auto streamBuf = std::make_shared<memory_streambuf>();
        mFileMap[filePath].StreamBuf = streamBuf;
        mFileMap[filePath].LastModified = std::filesystem::file_time_type::clock::now();

        return std::make_shared<std::ostream>(streamBuf.get());
    }

    std::vector<std::filesystem::path> ListFiles(std::filesystem::path const & directoryPath) override
    {
        std::vector<std::filesystem::path> filePaths;

        for (auto const & kv : mFileMap)
        {
            if (IsParentOf(directoryPath, kv.first))
                filePaths.push_back(kv.first);
        }

        return filePaths;
    }

    void DeleteFile(std::filesystem::path const & filePath) override
    {
        auto it = mFileMap.find(filePath);
        if (it == mFileMap.end())
            throw std::logic_error("File path '" + filePath.string() + "' does not exist in test file system");

        mFileMap.erase(it);
    }

    void RenameFile(
        std::filesystem::path const & oldFilePath,
        std::filesystem::path const & newFilePath) override
    {
        auto oldIt = mFileMap.find(oldFilePath);
        if (oldIt == mFileMap.end())
            throw std::logic_error("File path '" + oldFilePath.string() + "' does not exist in test file system");

        auto newIt = mFileMap.find(newFilePath);
        if (newIt != mFileMap.end())
            throw std::logic_error("File path '" + newFilePath.string() + "' already exists in test file system");

        mFileMap[newFilePath] = oldIt->second;
        mFileMap.erase(oldIt);
    }

private:

    static bool IsParentOf(
        std::filesystem::path const & directoryPath,
        std::filesystem::path const & childPath)
    {
        auto childIt = childPath.begin();
        for (auto const & element : directoryPath)
        {
            if (childIt == childPath.end() || *childIt != element)
                return false;

            ++childIt;
        }

        return true;
    }

    FileMap mFileMap;
};

class MockFileSystem : public IFileSystem
{
public:

    MOCK_METHOD1(Exists, bool(std::filesystem::path const & path));
    MOCK_METHOD1(GetLastModifiedTime, std::filesystem::file_time_type(std::filesystem::path const & path));
    MOCK_METHOD1(EnsureDirectoryExists, void(std::filesystem::path const & directoryPath));
    MOCK_METHOD1(OpenOutputStream, std::shared_ptr<std::ostream>(std::filesystem::path const & filePath));
    MOCK_METHOD1(OpenInputStream, std::shared_ptr<std::istream>(std::filesystem::path const & filePath));
    MOCK_METHOD1(ListFiles, std::vector<std::filesystem::path>(std::filesystem::path const & directoryPath));
    MOCK_METHOD1(DeleteFile, void(std::filesystem::path const & filePath));
    MOCK_METHOD2(RenameFile, void(std::filesystem::path const & oldFilePath, std::filesystem::path const & newFilePath));
};

::testing::AssertionResult ApproxEquals(float a, float b, float tolerance);

float DivideByTwo(float value);

// TODOTEST
//StructuralMaterial MakeTestStructuralMaterial(std::string name, rgbColor colorKey);
//ElectricalMaterial MakeTestElectricalMaterial(std::string name, rgbColor colorKey, bool isInstanced = false);