/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-06-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MaterialPalettesController.h"

namespace ShipBuilder {

MaterialPalettesController::MaterialPalettesController(
    wxWindow * parent,
    std::function<void(StructuralMaterial const * material, MaterialPlaneType plane)> onStructuralLayerMaterialSelected,
    std::function<void(ElectricalMaterial const * material, MaterialPlaneType plane)> onElectricalLayerMaterialSelected,
    std::function<void(StructuralMaterial const * material, MaterialPlaneType plane)> onRopeLayerMaterialSelected,
    DefaultMaterialDatabase const & defaultMaterialDatabase,
    ShipTexturizer const & shipTexturizer,
    ISoundController * soundController,
    GameAssetManager const & gameAssetManager,
    ProgressCallback const & progressCallback)
    : mOnStructuralLayerMaterialSelected(std::move(onStructuralLayerMaterialSelected))
    , mOnElectricalLayerMaterialSelected(std::move(onElectricalLayerMaterialSelected))
    , mOnRopeLayerMaterialSelected(std::move(onRopeLayerMaterialSelected))
    , mLastOpenedPalette(nullptr)
{
    mStructuralMaterialPaletteBrowser = std::make_unique<MaterialPaletteBrowser<LayerType::Structural>>(
        parent,
        *this,
        defaultMaterialDatabase.GetStructuralMaterialPalette(),
        shipTexturizer,
        soundController,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.0f, 0.33f));

    mElectricalMaterialPaletteBrowser = std::make_unique<MaterialPaletteBrowser<LayerType::Electrical>>(
        parent,
        *this,
        defaultMaterialDatabase.GetElectricalMaterialPalette(),
        shipTexturizer,
        soundController,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.33f, 0.33f));

    mRopesMaterialPaletteBrowser = std::make_unique<MaterialPaletteBrowser<LayerType::Ropes>>(
        parent,
        *this,
        defaultMaterialDatabase.GetRopeMaterialPalette(),
        shipTexturizer,
        soundController,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.66f, 0.33f));

    progressCallback(1.0f, ProgressMessageType::LoadingMaterialPalette);
}

bool MaterialPalettesController::IsOpen() const
{
    if (mLastOpenedPalette == nullptr)
    {
        return false;
    }
    else
    {
        return mLastOpenedPalette->IsOpen();
    }
}

void MaterialPalettesController::OnStructuralMaterialSelected(
    StructuralMaterial const * material,
    MaterialPlaneType plane)
{
    mOnStructuralLayerMaterialSelected(material, plane);
}

void MaterialPalettesController::OnElectricalMaterialSelected(
    ElectricalMaterial const * material,
    MaterialPlaneType plane)
{
    mOnElectricalLayerMaterialSelected(material, plane);
}

void MaterialPalettesController::OnRopesMaterialSelected(
    StructuralMaterial const * material,
    MaterialPlaneType plane)
{
    mOnRopeLayerMaterialSelected(material, plane);
}

}