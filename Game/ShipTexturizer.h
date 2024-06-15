/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Layers.h"
#include "MaterialDatabase.h"
#include "Physics.h"
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

    static int CalculateHighDefinitionTextureMagnificationFactor(ShipSpaceSize const & shipSize);

    RgbaImageData MakeAutoTexture(
        StructuralLayerData const & structuralLayer,
        std::optional<ShipAutoTexturizationSettings> const & settings) const;

    void AutoTexturizeInto(
        StructuralLayerData const & structuralLayer,
        ShipSpaceRect const & structuralLayerRegion,
        RgbaImageData & targetTextureImage,
        int magnificationFactor,
        ShipAutoTexturizationSettings const & settings) const;

    RgbaImageData MakeInteriorViewTexture(
        Physics::Triangles const & triangles,
        Physics::Points const & points,
        RgbaImageData const & backgroundTexture,
        float backgroundAlpha) const;


    void RenderShipInto(
        StructuralLayerData const & structuralLayer,
        ShipSpaceRect const & structuralLayerRegion,
        RgbaImageData const & sourceTextureImage,
        RgbaImageData & targetTextureImage,
        int magnificationFactor) const;

    template<typename TMaterial>
    RgbaImageData MakeMaterialTextureSample(
        std::optional<ShipAutoTexturizationSettings> const & settings,
        ImageSize const & sampleSize,
        TMaterial const & material) const
    {
        return MakeMaterialTextureSample(
            settings,
            sampleSize,
            material.RenderColor,
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

    using Vec2fImageData = ImageData<vec2f>;

private:

    static std::unordered_map<std::string, std::filesystem::path> MakeMaterialTextureNameToTextureFilePathMap(
        MaterialDatabase const & materialDatabase,
        ResourceLocator const & resourceLocator);

    static float MaterialTextureMagnificationToPixelConversionFactor(float magnification);

    RgbaImageData MakeMaterialTextureSample(
        std::optional<ShipAutoTexturizationSettings> const & settings,
        ImageSize const & sampleSize,
        rgbaColor const & renderColor,
        std::optional<std::string> const & textureName) const;

    inline Vec2fImageData const & GetMaterialTexture(std::optional<std::string> const & textureName) const;

    void ResetMaterialTextureCacheUseCounts() const;

    void PurgeMaterialTextureCache(size_t maxSize) const;

    inline rgbaColor SampleTextureBilinearConstrained(
        RgbaImageData const & texture,
        float pixelX,
        float pixelY) const;

    inline vec2f SampleTextureBilinearRepeated(
        Vec2fImageData const & texture,
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
        Vec2fImageData Texture;
        size_t UseCount;

        CachedTexture(Vec2fImageData && texture)
            : Texture(std::move(texture))
            , UseCount(0)
        {}
    };

    mutable std::unordered_map<std::string, CachedTexture> mMaterialTextureCache;
};
