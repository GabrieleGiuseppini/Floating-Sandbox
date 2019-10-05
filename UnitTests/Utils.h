#include <GameCore/FileSystem.h>
#include <GameCore/MemoryStreams.h>

#include <map>
#include <vector>

#include "gtest/gtest.h"

::testing::AssertionResult ApproxEquals(float a, float b, float tolerance);

class TestFileSystem : public IFileSystem
{
public:

    using FileMap = std::map<std::filesystem::path, std::shared_ptr<memory_streambuf>>;

    TestFileSystem()
        : mFileMap()
    {}

    FileMap & GetFileMap()
    {
        return mFileMap;
    }

    ///////////////////////////////////////////////////////////
    // IFileSystem
    ///////////////////////////////////////////////////////////

    std::shared_ptr<std::istream> OpenInputStream(std::filesystem::path const & filePath) override
    {
        auto it = mFileMap.find(filePath);
        if (it != mFileMap.end())
        {
            return std::make_shared<std::istream>(it->second.get());
        }
        else
        {
            return std::shared_ptr<std::istream>();
        }
    }

    std::shared_ptr<std::ostream> OpenOutputStream(std::filesystem::path const & filePath) override
    {
        auto streamBuf = std::make_shared<memory_streambuf>();        
        mFileMap[filePath] = streamBuf;

        return std::make_shared<std::ostream>(streamBuf.get());
    }

private:

    FileMap mFileMap;
};