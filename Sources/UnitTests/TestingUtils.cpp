#include "TestingUtils.h"

#include <cassert>
#include <cmath>
#include <set>

TestAssetManager::TestAssetManager(std::vector<TestTextureDatabase> textureDatabases)
    : mTextureDatabases(std::move(textureDatabases))
{
    // Verify no dupe DBs, and no dupe frames in each DB
    std::set<std::string> encounteredDbNames;
    for (auto const & db : textureDatabases)
    {
        assert(encounteredDbNames.count(db.DatabaseName) == 0);

        std::set<std::string> encounteredFrameFilenames;
        for (auto const & frameInfo : db.FrameInfos)
        {
            assert(encounteredFrameFilenames.count(frameInfo.FrameFilename) == 0);

            encounteredFrameFilenames.insert(frameInfo.FrameFilename);
        }

        encounteredDbNames.insert(db.DatabaseName);
    }
}

picojson::value TestAssetManager::LoadTetureDatabaseSpecification(std::string const & databaseName)
{

}

ImageSize TestAssetManager::GetTextureDatabaseFrameSize(std::string const & databaseName, std::string const & frameFileName)
{}

RgbaImageData TestAssetManager::LoadTextureDatabaseFrameRGBA(std::string const & databaseName, std::string const & frameFileName)
{}

std::vector<std::string> TestAssetManager::EnumerateTextureDatabaseFrames(std::string const & databaseName)
{}

::testing::AssertionResult ApproxEquals(float a, float b, float tolerance)
{
    if (std::abs(a - b) <= tolerance)
    {
        return ::testing::AssertionSuccess();
    }
    else
    {
        return ::testing::AssertionFailure() << "Result " << a << " too different than expected value " << b;
    }
}

float DivideByTwo(float value)
{
    return value / 2.0f;
}

// TODOTEST
////StructuralMaterial MakeTestStructuralMaterial(std::string name, rgbColor colorKey)
////{
////    return StructuralMaterial(
////        colorKey,
////        name,
////        rgbaColor::zero());
////}
////
////ElectricalMaterial MakeTestElectricalMaterial(std::string name, rgbColor colorKey, bool isInstanced)
////{
////    return ElectricalMaterial(
////        colorKey,
////        name,
////        rgbColor::zero(),
////        isInstanced);
////}