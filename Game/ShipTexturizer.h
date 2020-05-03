/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "MaterialDatabase.h"
#include "ResourceLocator.h"
#include "ShipDefinition.h"

#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>

#include <filesystem>
#include <unordered_map>

class ShipTexturizer
{
public:

    ShipTexturizer(ResourceLocator const & resourceLocator)
        : mMaterialTexturesFolderPath(resourceLocator.GetMaterialTexturesFolderPath())
        , mTextureCache()
        , mAutoTexturizationMode(ShipAutoTexturizationMode::FlatStructure)
    {}

    inline ShipAutoTexturizationMode GetAutoTexturizationMode() const
    {
        return 	mAutoTexturizationMode;
    }

    void SetAutoTexturizationMode(ShipAutoTexturizationMode value)
    {
        mAutoTexturizationMode = value;
    }

    void VerifyMaterialDatabase(MaterialDatabase const & materialDatabase) const;

    RgbaImageData Texturize(ShipDefinition const & shipDefinition) const;

private:

    std::filesystem::path const mMaterialTexturesFolderPath;

    struct CachedTexture
    {
        RgbaImageData Texture;
        size_t UseCount;

        CachedTexture(RgbaImageData && texture)
            : Texture(std::move(texture))
            , UseCount(0)
        {}
    };

    mutable std::unordered_map<std::string, CachedTexture> mTextureCache;

    // We are the storage for this setting
    ShipAutoTexturizationMode mAutoTexturizationMode;
};
