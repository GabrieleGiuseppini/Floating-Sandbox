/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Layers.h"
#include "MaterialDatabase.h"
#include "ResourceLocator.h"
#include "ShipAutoTexturizationSettings.h"

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

    static ImageSize CalculateTextureSize(ShipSpaceSize const & shipSize);

    RgbaImageData MakeTexture(
        StructuralLayerData const & structuralLayer,
        std::optional<ShipAutoTexturizationSettings> const & shipDefinitionSettings) const;

    void Texturize(
        StructuralLayerData const & structuralLayer,
        ShipSpaceRect const & structuralLayerRegion,
        RgbaImageData & targetTextureImage,
        ShipAutoTexturizationSettings const & settings) const;

    template<typename TMaterial>
    RgbaImageData MakeTextureSample(
        std::optional<ShipAutoTexturizationSettings> const & settings,
        ImageSize const & sampleSize,
        TMaterial const & material) const
    {
        return MakeTextureSample(
            settings,
            sampleSize,
            rgbaColor(material.RenderColor, 255),
            material.MaterialTextureName);
    }

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

    RgbaImageData MakeTextureSample(
        std::optional<ShipAutoTexturizationSettings> const & settings,
        ImageSize const & sampleSize,
        rgbaColor const & renderColor,
        std::optional<std::string> const & textureName) const;

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
