#include "TestingUtils.h"

#include <Core/Utils.h>

#include <cassert>
#include <cmath>
#include <set>
#include <stdexcept>

picojson::value TestAssetManager::LoadTetureDatabaseSpecification(std::string const & databaseName) const
{
    return Utils::ParseJSONString(GetDatabase(databaseName).DatabaseJson);
}

ImageSize TestAssetManager::GetTextureDatabaseFrameSize(std::string const & databaseName, std::string const & frameRelativePath) const
{
    auto const & db = GetDatabase(databaseName);
    for (auto const & f : db.FrameInfos)
    {
        if (f.AssetDescriptor.RelativePath == frameRelativePath)
        {
            return f.FrameSize;
        }
    }

    throw std::runtime_error("Invalid test - unknown test texture database frame relative path: " + frameRelativePath);
}

RgbaImageData TestAssetManager::LoadTextureDatabaseFrameRGBA(std::string const & databaseName, std::string const & frameRelativePath) const
{
    assert(false); // Not needed by tests, so far
    (void)databaseName;
    (void)frameRelativePath;
    return RgbaImageData(0, 0);
}

std::vector<IAssetManager::AssetDescriptor> TestAssetManager::EnumerateTextureDatabaseFrames(std::string const & databaseName) const
{
    std::vector<AssetDescriptor> assetDescriptors;

    auto const & db = GetDatabase(databaseName);
    std::transform(
        db.FrameInfos.cbegin(),
        db.FrameInfos.cend(),
        std::back_inserter(assetDescriptors),
            [](auto const & fi)
            {
                return fi.AssetDescriptor;
            });

    return assetDescriptors;
}

picojson::value TestAssetManager::LoadTetureAtlasSpecification(std::string const & textureDatabaseName) const
{
    assert(false); // Not needed by tests, so far
    (void)textureDatabaseName;
    return picojson::value();
}

RgbaImageData TestAssetManager::LoadTextureAtlasImageRGBA(std::string const & textureDatabaseName) const
{
    assert(false); // Not needed by tests, so far
    (void)textureDatabaseName;
    return RgbaImageData(0, 0);
}

std::vector<IAssetManager::AssetDescriptor> TestAssetManager::EnumerateShaders(std::string const & shaderSetName) const
{
    assert(false); // Not needed by tests, so far
    (void)shaderSetName;
    return std::vector<IAssetManager::AssetDescriptor>();
}

std::string TestAssetManager::LoadShader(std::string const & shaderSetName, std::string const & shaderRelativePath) const
{
    assert(false); // Not needed by tests, so far
    (void)shaderSetName;
    (void)shaderRelativePath;
    return std::string();
}

std::vector<IAssetManager::AssetDescriptor> TestAssetManager::EnumerateFonts(std::string const & fontSetName) const
{
    assert(false); // Not needed by tests, so far
    (void)fontSetName;
    return std::vector<IAssetManager::AssetDescriptor>();
}

Buffer<std::uint8_t> TestAssetManager::LoadFont(std::string const & fontSetName, std::string const & fontRelativePath) const
{
    assert(false); // Not needed by tests, so far
    (void)fontSetName;
    (void)fontRelativePath;
    return Buffer<std::uint8_t>(0);
}

////////////////////////////////////

TestTextureDatabase const & TestAssetManager::GetDatabase(std::string const & databaseName) const
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