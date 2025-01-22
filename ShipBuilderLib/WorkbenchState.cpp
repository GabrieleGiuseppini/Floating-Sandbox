/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "WorkbenchState.h"

#include <UILib/StandardSystemPaths.h>

#include <Game/Version.h>

#include <GameCore/Utils.h>

#include <cassert>

namespace ShipBuilder {

WorkbenchState::WorkbenchState(
    MaterialDatabase const & materialDatabase,
    IUserInterface & userInterface)
    : mClipboardManager(userInterface)
    ////////////////////////
    , mNewShipSize(0, 0) // Set later
{
    // Default structural foreground material: first structural material
    assert(!materialDatabase.GetStructuralMaterialPalette().Categories.empty()
        && !materialDatabase.GetStructuralMaterialPalette().Categories[0].SubCategories.empty()
        && !materialDatabase.GetStructuralMaterialPalette().Categories[0].SubCategories[0].Materials.empty());
    mStructuralForegroundMaterial = &(materialDatabase.GetStructuralMaterialPalette().Categories[0].SubCategories[0].Materials[0].get());

    // Default structural background material: none
    mStructuralBackgroundMaterial = nullptr;

    // Default electrical foreground material: first electrical material
    assert(!materialDatabase.GetElectricalMaterialPalette().Categories.empty()
        && !materialDatabase.GetElectricalMaterialPalette().Categories[0].SubCategories.empty()
        && !materialDatabase.GetElectricalMaterialPalette().Categories[0].SubCategories[0].Materials.empty());
    mElectricalForegroundMaterial = &(materialDatabase.GetElectricalMaterialPalette().Categories[0].SubCategories[0].Materials[0].get());

    // Default electrical background material: none
    mElectricalBackgroundMaterial = nullptr;

    // Default ropes foreground material: first ropes material
    assert(materialDatabase.GetRopeMaterialPalette().Categories.size() > 0
        && !materialDatabase.GetRopeMaterialPalette().Categories[0].SubCategories.empty()
        && !materialDatabase.GetRopeMaterialPalette().Categories[0].SubCategories[0].Materials.empty());
    mRopesForegroundMaterial = &(materialDatabase.GetRopeMaterialPalette().Categories[0].SubCategories[0].Materials[0].get());

    // Default ropes background material: second ropes material
    assert(materialDatabase.GetRopeMaterialPalette().Categories.size() > 1
        && !materialDatabase.GetRopeMaterialPalette().Categories[1].SubCategories.empty()
        && !materialDatabase.GetRopeMaterialPalette().Categories[0].SubCategories[0].Materials.empty());
    mRopesBackgroundMaterial = &(materialDatabase.GetRopeMaterialPalette().Categories[1].SubCategories[0].Materials[0].get());

    //
    // Default tool settings
    //

    mStructuralPencilToolSize = 1;
    mStructuralRectangleLineSize = 1;
    mStructuralRectangleFillMode = FillMode::NoFill;
    mStructuralEraserToolSize = 4;
    mElectricalEraserToolSize = 1;
    mStructuralLineToolSize = 1;
    mStructuralLineToolIsHullMode = true;
    mStructuralFloodToolIsContiguous = true;
    mTextureMagicWandTolerance = 0;
    mTextureMagicWandIsAntiAliased = true;
    mTextureMagicWandIsContiguous = true;
    mTextureEraserToolSize = 4;
    mSelectionIsAllLayers = false;
    mPasteIsTransparent = false;

    //
    // Default visualization settings
    //

    mCanvasBackgroundColor = rgbColor(255, 255, 255);
    mPrimaryVisualization = GetDefaultPrimaryVisualization();
    mGameVisualizationMode = GameVisualizationModeType::AutoTexturizationMode; // Will be changed (by Controller) to Texture when loading a ship with texture
    mStructuralLayerVisualizationMode = StructuralLayerVisualizationModeType::PixelMode;
    mElectricalLayerVisualizationMode = ElectricalLayerVisualizationModeType::PixelMode;
    mRopesLayerVisualizationMode = RopesLayerVisualizationModeType::LinesMode;
    mExteriorTextureLayerVisualizationMode = ExteriorTextureLayerVisualizationModeType::MatteMode;
    mInteriorTextureLayerVisualizationMode = InteriorTextureLayerVisualizationModeType::MatteMode;
    mOtherVisualizationsOpacity = 0.75f;
    mIsWaterlineMarkersEnabled = false;
    mIsGridEnabled = false;

    //
    // Misc
    //

    mDisplayUnitsSystem = UnitsSystem::SI_Celsius;
    mNewShipSize = ShipSpaceSize(200, 100);

    //
    // Load preferences
    //

    try
    {
        LoadPreferences();
    }
    catch (...)
    {
        // Ignore
    }
}

WorkbenchState::~WorkbenchState()
{
    //
    // Save preferences
    //

    try
    {
        SavePreferences();
    }
    catch (...)
    {
        // Ignore
    }
}

std::filesystem::path WorkbenchState::GetPreferencesFilePath()
{
    return StandardSystemPaths::GetInstance().GetUserGameRootFolderPath() / "shipbuilder_preferences.json";
}

std::optional<picojson::object> WorkbenchState::LoadPreferencesRootObject()
{
    auto const preferencesFilePath = GetPreferencesFilePath();

    if (std::filesystem::exists(preferencesFilePath))
    {
        auto const preferencesRootValue = Utils::ParseJSONFile(preferencesFilePath);

        if (preferencesRootValue.is<picojson::object>())
        {
            return preferencesRootValue.get<picojson::object>();
        }
    }

    return std::nullopt;
}

void WorkbenchState::LoadPreferences()
{
    auto const preferencesRootObject = LoadPreferencesRootObject();
    if (preferencesRootObject.has_value())
    {
        //
        // Load version
        //

        Version settingsVersion(Version::Zero());
        if (auto versionIt = preferencesRootObject->find("version");
            versionIt != preferencesRootObject->end() && versionIt->second.is<std::string>())
        {
            settingsVersion = Version::FromString(versionIt->second.get<std::string>());
        }

        //
        // Display units system
        //

        if (auto displayUnitsSystemIt = preferencesRootObject->find("display_units_system");
            displayUnitsSystemIt != preferencesRootObject->end() && displayUnitsSystemIt->second.is<std::int64_t>())
        {
            mDisplayUnitsSystem = static_cast<UnitsSystem>(displayUnitsSystemIt->second.get<std::int64_t>());
        }

        //
        // Ship load directories
        //

        if (auto shipLoadDirectoriesIt = preferencesRootObject->find("ship_load_directories");
            shipLoadDirectoriesIt != preferencesRootObject->end() && shipLoadDirectoriesIt->second.is<picojson::array>())
        {
            mShipLoadDirectories.clear();

            auto shipLoadDirectories = shipLoadDirectoriesIt->second.get<picojson::array>();
            for (auto const & shipLoadDirectory : shipLoadDirectories)
            {
                if (shipLoadDirectory.is<std::string>())
                {
                    auto shipLoadDirectoryPath = std::filesystem::path(shipLoadDirectory.get<std::string>());

                    // Make sure dir still exists, and it's not in the vector already
                    if (std::filesystem::exists(shipLoadDirectoryPath)
                        && std::find(
                            mShipLoadDirectories.cbegin(),
                            mShipLoadDirectories.cend(),
                            shipLoadDirectoryPath) == mShipLoadDirectories.cend())
                    {
                        mShipLoadDirectories.push_back(shipLoadDirectoryPath);
                    }
                }
            }
        }

        //
        // Canvas BG color
        //

        if (auto canvasBackgroundColorIt = preferencesRootObject->find("canvas_background_color");
            canvasBackgroundColorIt != preferencesRootObject->end() && canvasBackgroundColorIt->second.is<std::string>())
        {
            mCanvasBackgroundColor = rgbColor::fromString(canvasBackgroundColorIt->second.get<std::string>());
        }
    }
}

void WorkbenchState::SavePreferences() const
{
    picojson::object preferencesRootObject;

    // Add version
    preferencesRootObject["version"] = picojson::value(Version::CurrentVersion().ToString());

    // Add display units system
    preferencesRootObject["display_units_system"] = picojson::value(static_cast<std::int64_t>(mDisplayUnitsSystem));

    // Add ship load directories
    {
        picojson::array shipLoadDirectories;
        for (auto shipDir : mShipLoadDirectories)
        {
            shipLoadDirectories.push_back(picojson::value(shipDir.string()));
        }

        preferencesRootObject["ship_load_directories"] = picojson::value(shipLoadDirectories);
    }

    // Add canvas background color
    preferencesRootObject["canvas_background_color"] = picojson::value(mCanvasBackgroundColor.toString());

    // Save
    Utils::SaveJSONFile(
        picojson::value(preferencesRootObject),
        GetPreferencesFilePath());
}


}