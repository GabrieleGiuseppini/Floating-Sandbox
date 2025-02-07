/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Layers.h"
#include "MaterialDatabase.h"
#include "Physics/Physics.h"
#include "ShipAutoTexturizationSettings.h"

#include <Core/GameTypes.h>
#include <Core/IAssetManager.h>
#include <Core/ImageData.h>
#include <Core/Vectors.h>

#include <cassert>
#include <optional>
#include <unordered_map>

class ShipTexturizer
{
public:

    static int constexpr MaxHighDefinitionTextureSize = 4096; // Max texture size for low-end gfx cards

public:

    ShipTexturizer(
        MaterialDatabase const & materialDatabase,
        IAssetManager & assetManager);

    static int CalculateHighDefinitionTextureMagnificationFactor(
        ShipSpaceSize const & shipSize,
        int maxTextureSize);

    RgbaImageData MakeAutoTexture(
        StructuralLayerData const & structuralLayer,
        std::optional<ShipAutoTexturizationSettings> const & settings,
        int maxTextureSize,
        IAssetManager & assetManager) const;

    void AutoTexturizeInto(
        StructuralLayerData const & structuralLayer,
        ShipSpaceRect const & structuralLayerRegion,
        RgbaImageData & targetTextureImage,
        int magnificationFactor,
        ShipAutoTexturizationSettings const & settings,
        IAssetManager & assetManager) const;

    RgbaImageData MakeInteriorViewTexture(
        Physics::Triangles const & triangles,
        Physics::Points const & points,
        ShipSpaceSize const & shipSize,
        RgbaImageData const & backgroundTexture) const;

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

    static std::unordered_map<std::string, std::string> MakeMaterialTextureNameToTextureRelativePathMap(
        MaterialDatabase const & materialDatabase,
        IAssetManager & assetManager);

    static float MaterialTextureMagnificationToPixelConversionFactor(float magnification);

    RgbaImageData MakeMaterialTextureSample(
        std::optional<ShipAutoTexturizationSettings> const & settings,
        ImageSize const & sampleSize,
        rgbaColor const & renderColor,
        std::optional<std::string> const & textureName,
        IAssetManager & assetManager) const;

    inline Vec2fImageData const & GetMaterialTexture(
        std::optional<std::string> const & textureName,
        IAssetManager & assetManager) const;

    void ResetMaterialTextureCacheUseCounts() const;

    void PurgeMaterialTextureCache(size_t maxSize) const;

    inline void DrawTriangleFloorInto(
        ElementIndex triangleIndex,
        Physics::Points const & points,
        Physics::Triangles const & triangles,
        vec2f const & textureSizeF,
        int const floorThickness,
        RgbaImageData & targetTextureImage) const;

    inline void DrawHVEdgeFloorInto(
        int xStart,
        int xEnd, // Included
        int xIncr,
        int xLimitIncr,
        int yStart,
        int yEnd, // Included
        int yIncr,
        RgbaImageData & targetTextureImage) const;

    inline void DrawDEdgeFloorInto(
        int xStart,
        int xEnd, // Included
        int xIncr,
        int xLimitIncr,
        int absoluteMinX,
        int absoluteMaxX,
        int yStart,
        int yEnd, // Included
        int yIncr,
        RgbaImageData & targetTextureImage) const;

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

    std::unordered_map<std::string, std::string> const mMaterialTextureNameToTextureRelativePathMap;

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
