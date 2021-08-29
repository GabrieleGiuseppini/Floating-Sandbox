/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "MaterialDatabase.h"
#include "ResourceLocator.h"
#include "ShipFactoryTypes.h"

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

    ShipTexturizer(
        MaterialDatabase const & materialDatabase,
        ResourceLocator const & resourceLocator);

    RgbaImageData Texturize(
        std::optional<ShipAutoTexturizationSettings> const & shipDefinitionSettings,
        ImageSize const & structureSize,
        ShipFactoryPointIndexMatrix const & pointMatrix, // One more point on each side, to avoid checking for boundaries
        std::vector<ShipFactoryPoint> const & points) const;

    //
    // Settings
    //

    ShipAutoTexturizationSettings const & GetSharedSettings() const
    {
        return mSharedSettings;
    }

    ShipAutoTexturizationSettings & GetSharedSettings()
    {
        return mSharedSettings;
    }

    void SetSharedSettings(ShipAutoTexturizationSettings const & sharedSettings)
    {
        mSharedSettings = sharedSettings;
    }

    bool GetDoForceSharedSettingsOntoShipSettings() const
    {
        return mDoForceSharedSettingsOntoShipSettings;
    }

    void SetDoForceSharedSettingsOntoShipSettings(bool value)
    {
        mDoForceSharedSettingsOntoShipSettings = value;
    }

private:

    static std::unordered_map<std::string, std::filesystem::path> MakeMaterialTextureNameToTextureFilePathMap(
        MaterialDatabase const & materialDatabase,
        ResourceLocator const & resourceLocator);

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

    ShipAutoTexturizationSettings mSharedSettings;
    bool mDoForceSharedSettingsOntoShipSettings;

    //
    // Material textures
    //

    std::unordered_map<std::string, std::filesystem::path> const mMaterialTextureNameToTextureFilePathMap;

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
