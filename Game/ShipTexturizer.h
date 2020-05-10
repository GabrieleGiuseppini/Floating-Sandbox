/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "MaterialDatabase.h"
#include "ResourceLocator.h"
#include "ShipBuildTypes.h"

#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/Vectors.h>

#include <cassert>
#include <filesystem>
#include <optional>
#include <unordered_map>

class ShipTexturizer
{
public:

    ShipTexturizer(ResourceLocator const & resourceLocator);

    ShipAutoTexturizationMode GetAutoTexturizationMode() const
    {
        return 	mAutoTexturizationMode;
    }

    void SetAutoTexturizationMode(ShipAutoTexturizationMode value)
    {
        mAutoTexturizationMode = value;
    }

    float GetMaterialTextureMagnification() const
    {
        return mMaterialTextureMagnification;
    }

    void SetMaterialTextureMagnification(float value)
    {
        assert(value != 0.0f);

        mMaterialTextureMagnification = value;
        mMaterialTextureWorldToPixelConversionFactor = MaterialTextureMagnificationToPixelConversionFactor(mMaterialTextureMagnification);
    }

    float GetMaterialTextureTransparency() const
    {
        return 1.0f - mMaterialTextureAlpha;
    }

    void SetMaterialTextureTransparency(float value)
    {
        mMaterialTextureAlpha = 1.0f - value;
    }

    void VerifyMaterialDatabase(MaterialDatabase const & materialDatabase) const;

    RgbaImageData Texturize(
        ImageSize const & structureSize,
        ShipBuildPointIndexMatrix const & pointMatrix, // One more point on each side, to avoid checking for boundaries
        std::vector<ShipBuildPoint> const & points) const;

private:

    static std::unordered_map<std::string, std::filesystem::path> MakeMaterialTextureNameToTextureFilePathMap(std::filesystem::path const materialTexturesFolderPath);

    static float MaterialTextureMagnificationToPixelConversionFactor(float magnification);

    inline Vec3fImageData const & GetMaterialTexture(std::optional<std::string> const & textureName) const;

    void ResetMaterialTextureCacheUseCounts() const;

    void PurgeMaterialTextureCache(size_t maxSize) const;

    inline vec3f SampleTexture(
        Vec3fImageData const & texture,
        float pixelX,
        float pixelY) const;

private:

    //
    // Settings that we are the storage of
    //

    ShipAutoTexturizationMode mAutoTexturizationMode;
    float mMaterialTextureMagnification;
    float mMaterialTextureAlpha;

    //
    // Material textures
    //

    std::filesystem::path const mMaterialTexturesFolderPath;

    std::unordered_map<std::string, std::filesystem::path> const mMaterialTextureNameToTextureFilePathMap;

    float mMaterialTextureWorldToPixelConversionFactor;

    struct CachedTexture
    {
        Vec3fImageData Texture;
        size_t UseCount;

        CachedTexture(Vec3fImageData && texture)
            : Texture(std::move(texture))
            , UseCount(0)
        {}
    };

    mutable std::unordered_map<std::string, CachedTexture> mMaterialTextureCache;
};
