/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ClipboardManager.h"
#include "IUserInterface.h"
#include "ShipBuilderTypes.h"

#include <Game/Materials.h>
#include <Game/MaterialDatabase.h>

#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>

#include <picojson.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace ShipBuilder {

/*
 * This class aggregates the current state of the ship builder editor.
 *
 * This state is managed by the Controller but lives in the MainFrame,
 * and thus its lifetime is longer than the Controller's.
 */
class WorkbenchState
{
public:

    WorkbenchState(
        MaterialDatabase const & materialDatabase,
        IUserInterface & userInterface);

    ~WorkbenchState();

    //
    // Components
    //

    ClipboardManager const & GetClipboardManager() const
    {
        return mClipboardManager;
    }

    ClipboardManager & GetClipboardManager()
    {
        return mClipboardManager;
    }

    //
    // Materials
    //

    StructuralMaterial const * GetStructuralForegroundMaterial() const
    {
        return mStructuralForegroundMaterial;
    }

    void SetStructuralForegroundMaterial(StructuralMaterial const * material)
    {
        mStructuralForegroundMaterial = material;
    }

    StructuralMaterial const * GetStructuralBackgroundMaterial() const
    {
        return mStructuralBackgroundMaterial;
    }

    void SetStructuralBackgroundMaterial(StructuralMaterial const * material)
    {
        mStructuralBackgroundMaterial = material;
    }

    void SetStructuralMaterial(StructuralMaterial const * material, MaterialPlaneType plane)
    {
        if (plane == MaterialPlaneType::Foreground)
        {
            SetStructuralForegroundMaterial(material);
        }
        else
        {
            assert(plane == MaterialPlaneType::Background);
            SetStructuralBackgroundMaterial(material);
        }
    }

    ElectricalMaterial const * GetElectricalForegroundMaterial() const
    {
        return mElectricalForegroundMaterial;
    }

    void SetElectricalForegroundMaterial(ElectricalMaterial const * material)
    {
        mElectricalForegroundMaterial = material;
    }

    ElectricalMaterial const * GetElectricalBackgroundMaterial() const
    {
        return mElectricalBackgroundMaterial;
    }

    void SetElectricalBackgroundMaterial(ElectricalMaterial const * material)
    {
        mElectricalBackgroundMaterial = material;
    }

    void SetElectricalMaterial(ElectricalMaterial const * material, MaterialPlaneType plane)
    {
        if (plane == MaterialPlaneType::Foreground)
        {
            SetElectricalForegroundMaterial(material);
        }
        else
        {
            assert(plane == MaterialPlaneType::Background);
            SetElectricalBackgroundMaterial(material);
        }
    }

    StructuralMaterial const * GetRopesForegroundMaterial() const
    {
        assert(mRopesForegroundMaterial != nullptr); // Ropes material is never NULL
        return mRopesForegroundMaterial;
    }

    void SetRopesForegroundMaterial(StructuralMaterial const * material)
    {
        assert(material != nullptr); // Ropes material is never NULL
        mRopesForegroundMaterial = material;
    }

    StructuralMaterial const * GetRopesBackgroundMaterial() const
    {
        assert(mRopesBackgroundMaterial != nullptr); // Ropes material is never NULL
        return mRopesBackgroundMaterial;
    }

    void SetRopesBackgroundMaterial(StructuralMaterial const * material)
    {
        assert(material != nullptr); // Ropes material is never NULL
        mRopesBackgroundMaterial = material;
    }

    void SetRopesMaterial(StructuralMaterial const * material, MaterialPlaneType plane)
    {
        assert(material != nullptr); // Ropes material is never NULL

        if (plane == MaterialPlaneType::Foreground)
        {
            SetRopesForegroundMaterial(material);
        }
        else
        {
            assert(plane == MaterialPlaneType::Background);
            SetRopesBackgroundMaterial(material);
        }
    }

    //
    // Tools settings
    //

    std::uint32_t GetStructuralPencilToolSize() const
    {
        return mStructuralPencilToolSize;
    }

    void SetStructuralPencilToolSize(std::uint32_t value)
    {
        mStructuralPencilToolSize = value;
    }

    std::uint32_t GetStructuralEraserToolSize() const
    {
        return mStructuralEraserToolSize;
    }

    void SetStructuralEraserToolSize(std::uint32_t value)
    {
        mStructuralEraserToolSize = value;
    }

    std::uint32_t GetElectricalEraserToolSize() const
    {
        return mElectricalEraserToolSize;
    }

    void SetElectricalEraserToolSize(std::uint32_t value)
    {
        mElectricalEraserToolSize = value;
    }

    std::uint32_t GetStructuralLineToolSize() const
    {
        return mStructuralLineToolSize;
    }

    void SetStructuralLineToolSize(std::uint32_t value)
    {
        mStructuralLineToolSize = value;
    }

    bool GetStructuralLineToolIsHullMode() const
    {
        return mStructuralLineToolIsHullMode;
    }

    void SetStructuralLineToolIsHullMode(bool value)
    {
        mStructuralLineToolIsHullMode = value;
    }

    bool GetStructuralFloodToolIsContiguous() const
    {
        return mStructuralFloodToolIsContiguous;
    }

    void SetStructuralFloodToolIsContiguous(bool value)
    {
        mStructuralFloodToolIsContiguous = value;
    }

    unsigned int GetTextureMagicWandTolerance() const
    {
        return mTextureMagicWandTolerance;
    }

    void SetTextureMagicWandTolerance(unsigned int value)
    {
        mTextureMagicWandTolerance = value;
    }

    bool GetTextureMagicWandIsAntiAliased() const
    {
        return mTextureMagicWandIsAntiAliased;
    }

    void SetTextureMagicWandIsAntiAliased(bool value)
    {
        mTextureMagicWandIsAntiAliased = value;
    }

    bool GetTextureMagicWandIsContiguous() const
    {
        return mTextureMagicWandIsContiguous;
    }

    void SetTextureMagicWandIsContiguous(bool value)
    {
        mTextureMagicWandIsContiguous = value;
    }

    std::uint32_t GetTextureEraserToolSize() const
    {
        return mTextureEraserToolSize;
    }

    void SetTextureEraserToolSize(std::uint32_t value)
    {
        mTextureEraserToolSize = value;
    }

    bool GetSelectionIsAllLayers() const
    {
        return mSelectionIsAllLayers;
    }

    void SetSelectionIsAllLayers(bool value)
    {
        mSelectionIsAllLayers = value;
    }

    bool GetPasteIsTransparent() const
    {
        return mPasteIsTransparent;
    }

    void SetPasteIsTransparent(bool value)
    {
        mPasteIsTransparent = value;
    }

    //
    // Visualizations
    //

    rgbColor GetCanvasBackgroundColor() const
    {
        return mCanvasBackgroundColor;
    }

    void SetCanvasBackgroundColor(rgbColor const & color)
    {
        mCanvasBackgroundColor = color;
    }

    static VisualizationType constexpr GetDefaultPrimaryVisualization()
    {
        return VisualizationType::Game;
    }

    VisualizationType GetPrimaryVisualization() const
    {
        return mPrimaryVisualization;
    }

    void SetPrimaryVisualization(VisualizationType visualization)
    {
        mPrimaryVisualization = visualization;
    }

    GameVisualizationModeType GetGameVisualizationMode() const
    {
        return mGameVisualizationMode;
    }

    void SetGameVisualizationMode(GameVisualizationModeType mode)
    {
        mGameVisualizationMode = mode;
    }

    StructuralLayerVisualizationModeType GetStructuralLayerVisualizationMode() const
    {
        return mStructuralLayerVisualizationMode;
    }

    void SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType mode)
    {
        mStructuralLayerVisualizationMode = mode;
    }

    ElectricalLayerVisualizationModeType GetElectricalLayerVisualizationMode() const
    {
        return mElectricalLayerVisualizationMode;
    }

    void SetElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType mode)
    {
        mElectricalLayerVisualizationMode = mode;
    }

    RopesLayerVisualizationModeType GetRopesLayerVisualizationMode() const
    {
        return mRopesLayerVisualizationMode;
    }

    void SetRopesLayerVisualizationMode(RopesLayerVisualizationModeType mode)
    {
        mRopesLayerVisualizationMode = mode;
    }

    TextureLayerVisualizationModeType GetTextureLayerVisualizationMode() const
    {
        return mTextureLayerVisualizationMode;
    }

    void SetTextureLayerVisualizationMode(TextureLayerVisualizationModeType mode)
    {
        mTextureLayerVisualizationMode = mode;
    }

    float GetOtherVisualizationsOpacity() const
    {
        return mOtherVisualizationsOpacity;
    }

    void SetOtherVisualizationsOpacity(float value)
    {
        mOtherVisualizationsOpacity = value;
    }

    bool IsWaterlineMarkersEnabled() const
    {
        return mIsWaterlineMarkersEnabled;
    }

    void EnableWaterlineMarkers(bool value)
    {
        mIsWaterlineMarkersEnabled = value;
    }

    bool IsGridEnabled() const
    {
        return mIsGridEnabled;
    }

    void EnableGrid(bool value)
    {
        mIsGridEnabled = value;
    }

    //
    // Misc
    //

    static int constexpr GetMaxShipDimension()
    {
        return 2000;
    }

    ShipSpaceSize GetNewShipSize() const
    {
        return mNewShipSize;
    }

    void SetNewShipSize(ShipSpaceSize value)
    {
        mNewShipSize = value;
    }

    UnitsSystem GetDisplayUnitsSystem() const
    {
        return mDisplayUnitsSystem;
    }

    void SetDisplayUnitsSystem(UnitsSystem value)
    {
        mDisplayUnitsSystem = value;
    }

    std::vector<std::filesystem::path> const & GetShipLoadDirectories() const
    {
        return mShipLoadDirectories;
    }

    void AddShipLoadDirectory(std::filesystem::path shipLoadDirectory)
    {
        // Check if it's in already
        if (std::find(mShipLoadDirectories.cbegin(), mShipLoadDirectories.cend(), shipLoadDirectory) == mShipLoadDirectories.cend())
        {
            // Add in front
            mShipLoadDirectories.insert(mShipLoadDirectories.cbegin(), shipLoadDirectory);
        }
    }

private:

    static std::filesystem::path GetPreferencesFilePath();

    static std::optional<picojson::object> LoadPreferencesRootObject();

    void LoadPreferences();

    void SavePreferences() const;

private:

    // Owned components
    ClipboardManager mClipboardManager;

    // Materials
    StructuralMaterial const * mStructuralForegroundMaterial;
    StructuralMaterial const * mStructuralBackgroundMaterial;
    ElectricalMaterial const * mElectricalForegroundMaterial;
    ElectricalMaterial const * mElectricalBackgroundMaterial;
    StructuralMaterial const * mRopesForegroundMaterial;
    StructuralMaterial const * mRopesBackgroundMaterial;

    // Tool settings
    std::uint32_t mStructuralPencilToolSize;
    std::uint32_t mStructuralEraserToolSize;
    std::uint32_t mElectricalEraserToolSize;
    std::uint32_t mStructuralLineToolSize;
    bool mStructuralLineToolIsHullMode;
    bool mStructuralFloodToolIsContiguous;
    unsigned int mTextureMagicWandTolerance; // 0-100
    bool mTextureMagicWandIsAntiAliased;
    bool mTextureMagicWandIsContiguous;
    std::uint32_t mTextureEraserToolSize;
    bool mSelectionIsAllLayers;
    bool mPasteIsTransparent;

    // Visualizations
    rgbColor mCanvasBackgroundColor;
    VisualizationType mPrimaryVisualization;
    GameVisualizationModeType mGameVisualizationMode;
    StructuralLayerVisualizationModeType mStructuralLayerVisualizationMode;
    ElectricalLayerVisualizationModeType mElectricalLayerVisualizationMode;
    RopesLayerVisualizationModeType mRopesLayerVisualizationMode;
    TextureLayerVisualizationModeType mTextureLayerVisualizationMode;
    float mOtherVisualizationsOpacity;
    bool mIsWaterlineMarkersEnabled;
    bool mIsGridEnabled;

    // Misc
    ShipSpaceSize mNewShipSize;
    UnitsSystem mDisplayUnitsSystem;
    std::vector<std::filesystem::path> mShipLoadDirectories;
};

}