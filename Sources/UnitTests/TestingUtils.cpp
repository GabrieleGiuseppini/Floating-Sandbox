#include "TestingUtils.h"

#include <Core/Utils.h>

#include <cassert>
#include <cmath>
#include <set>
#include <stdexcept>

picojson::value TestAssetManager::LoadTetureDatabaseSpecification(std::string const & databaseName)
{
    return Utils::ParseJSONString(GetDatabase(databaseName).DatabaseJson);
}

ImageSize TestAssetManager::GetTextureDatabaseFrameSize(std::string const & databaseName, std::string const & frameFileName)
{
    auto const & db = GetDatabase(databaseName);
    for (auto const & f : db.FrameInfos)
    {
        if (f.FrameFilename == frameFileName)
        {
            return f.FrameSize;
        }
    }

    throw std::runtime_error("Invalid test - unknown test texture database frame filename: " + frameFileName);
}

RgbaImageData TestAssetManager::LoadTextureDatabaseFrameRGBA(std::string const & databaseName, std::string const & frameFileName)
{
    assert(false); // Not needed by tests, so far
    (void)databaseName;
    (void)frameFileName;
    return RgbaImageData(0, 0);
}

std::vector<std::string> TestAssetManager::EnumerateTextureDatabaseFrames(std::string const & databaseName)
{
    std::vector<std::string> frameFilenames;

    auto const & db = GetDatabase(databaseName);
    std::transform(
        db.FrameInfos.cbegin(),
        db.FrameInfos.cend(),
        std::back_inserter(frameFilenames),
            [](auto const & fi)
            {
                return fi.FrameFilename;
            });

    return frameFilenames;
}

picojson::value TestAssetManager::LoadTetureAtlasSpecification(std::string const & textureDatabaseName)
{
    assert(false); // Not needed by tests, so far
    (void)textureDatabaseName;
    return picojson::value();
}

RgbaImageData TestAssetManager::LoadTextureAtlasImageRGBA(std::string const & textureDatabaseName)
{
    assert(false); // Not needed by tests, so far
    (void)textureDatabaseName;
    return RgbaImageData(0, 0);
}

TestTextureDatabase const & TestAssetManager::GetDatabase(std::string const & databaseName)
{
    for (auto const & db : TestTextureDatabases)
    {
        if (db.DatabaseName == databaseName)
        {
            return db;
        }
    }

    throw std::runtime_error("Invalid test - unknown test texture database name: " + databaseName);
}

///////////////////////////////////

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