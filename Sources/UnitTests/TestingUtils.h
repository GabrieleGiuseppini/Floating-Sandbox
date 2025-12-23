#include <Core/IAssetManager.h>
#include <Core/ImageData.h>
#include <Core/MemoryStreams.h>

#include <Simulation/Materials.h>

#include <Game/FileSystem.h>

#include <filesystem>
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

    picojson::value LoadTextureDatabaseSpecification(std::string const & databaseName) const override;
    ImageSize GetTextureDatabaseFrameSize(std::string const & databaseName, std::string const & frameRelativePath) const override;
    RgbaImageData LoadTextureDatabaseFrameRGBA(std::string const & databaseName, std::string const & frameRelativePath) const override;
    std::vector<AssetDescriptor> EnumerateTextureDatabaseFrames(std::string const & databaseName) const override;

    std::string GetMaterialTextureRelativePath(std::string const & materialTextureName) const override;
    RgbImageData LoadMaterialTexture(std::string const & frameRelativePath) const override;

    picojson::value LoadTextureAtlasSpecification(std::string const & textureDatabaseName) const override;
    RgbaImageData LoadTextureAtlasImageRGBA(std::string const & textureDatabaseName) const override;

    std::vector<AssetDescriptor> EnumerateShaders(std::string const & shaderSetName) const override;
    std::string LoadShader(std::string const & shaderSetName, std::string const & shaderRelativePath) const override;

    std::vector<AssetDescriptor> EnumerateFonts(std::string const & fontSetName) const override;
    std::unique_ptr<BinaryReadStream> LoadFont(std::string const & fontSetName, std::string const & fontRelativePath) const override;

    picojson::value LoadStructuralMaterialDatabase() const override;
    picojson::value LoadElectricalMaterialDatabase() const override;
    picojson::value LoadFishSpeciesDatabase() const override;
    picojson::value LoadNpcDatabase() const override;

private:

    TestTextureDatabase const & GetDatabase(std::string const & databaseName) const;
};

class TestFileSystem : public IFileSystem
{
private:

    class TestMemoryBinaryWriteStream final : public BinaryWriteStream
    {
    public:

        TestMemoryBinaryWriteStream(std::vector<std::uint8_t> & data)
            : mData(data)
        {}

        std::uint8_t const * GetData() const
        {
            return mData.data();
        }

        size_t GetSize() const
        {
            return mData.size();
        }

        void Write(std::uint8_t const * buffer, size_t size) override
        {
            size_t const oldSize = mData.size();
            mData.resize(mData.size() + size);
            std::memcpy(&(mData[oldSize]), buffer, size);
        }

        MemoryBinaryReadStream MakeReadStreamCopy() const
        {
            std::vector<std::uint8_t> copy = mData;
            return MemoryBinaryReadStream(std::move(copy));
        }

    private:

        std::vector<std::uint8_t> & mData;
    };

    class TestMemoryTextWriteStream final : public TextWriteStream
    {
    public:

        TestMemoryTextWriteStream(std::string & data)
            : mData(data)
        {}

        std::string const & GetData() const
        {
            return mData;
        }

        void Write(std::string const & content) override
        {
            mData += content;
        }

        MemoryTextReadStream MakeReadStreamCopy() const
        {
            std::string copy = mData;
            return MemoryTextReadStream(std::move(copy));
        }

    private:

        std::string & mData;
    };

public:

    struct FileInfo
    {
        std::vector<std::uint8_t> BinaryContent;
        std::string TextContent;
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
        fileInfoEntry.TextContent = content;
        fileInfoEntry.LastModified = lastModified;
    }

    std::string GetTestFileContent(std::filesystem::path testFilePath)
    {
        auto const it = mFileMap.find(testFilePath);
        if (it == mFileMap.end())
        {
            throw std::logic_error("File path '" + testFilePath.string() + "' does not exist in test file system");
        }

        return it->second.TextContent;
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

    std::unique_ptr<BinaryReadStream> OpenBinaryInputStream(std::filesystem::path const & filePath) override
    {
        auto it = mFileMap.find(filePath);
        if (it == mFileMap.end())
        {
            throw std::logic_error("File path '" + filePath.string() + "' does not exist in test file system");
        }

        std::vector<std::uint8_t> copy = it->second.BinaryContent;
        return std::make_unique<MemoryBinaryReadStream>(std::move(copy));
    }

    std::unique_ptr<TextReadStream> OpenTextInputStream(std::filesystem::path const & filePath) override
    {
        auto it = mFileMap.find(filePath);
        if (it == mFileMap.end())
        {
            throw std::logic_error("File path '" + filePath.string() + "' does not exist in test file system");
        }

        std::string copy = it->second.TextContent;
        return std::make_unique<MemoryTextReadStream>(std::move(copy));
    }

    std::unique_ptr<BinaryWriteStream> OpenBinaryOutputStream(std::filesystem::path const & filePath) override
    {
        FileInfo & fi = mFileMap[filePath];
        fi.BinaryContent.clear();
        fi.LastModified = std::filesystem::file_time_type::clock::now();

        return std::make_unique<TestMemoryBinaryWriteStream>(fi.BinaryContent);
    }

    std::unique_ptr<TextWriteStream> OpenTextOutputStream(std::filesystem::path const & filePath) override
    {
        FileInfo & fi = mFileMap[filePath];
        fi.TextContent.clear();
        fi.LastModified = std::filesystem::file_time_type::clock::now();

        return std::make_unique<TestMemoryTextWriteStream>(fi.TextContent);
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
    MOCK_METHOD1(OpenBinaryOutputStream, std::unique_ptr<BinaryWriteStream>(std::filesystem::path const & filePath));
    MOCK_METHOD1(OpenTextOutputStream, std::unique_ptr<TextWriteStream>(std::filesystem::path const & filePath));
    MOCK_METHOD1(OpenBinaryInputStream, std::unique_ptr<BinaryReadStream>(std::filesystem::path const & filePath));
    MOCK_METHOD1(OpenTextInputStream, std::unique_ptr<TextReadStream>(std::filesystem::path const & filePath));
    MOCK_METHOD1(ListFiles, std::vector<std::filesystem::path>(std::filesystem::path const & directoryPath));
    MOCK_METHOD1(DeleteFile, void(std::filesystem::path const & filePath));
    MOCK_METHOD2(RenameFile, void(std::filesystem::path const & oldFilePath, std::filesystem::path const & newFilePath));
};

::testing::AssertionResult ApproxEquals(float a, float b, float tolerance);

float DivideByTwo(float value);

StructuralMaterial MakeTestStructuralMaterial(std::string name, rgbColor colorKey);
ElectricalMaterial MakeTestElectricalMaterial(std::string name, rgbColor colorKey, bool isInstanced = false);