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

    progressCallback(0.33f, ProgressMessageType::LoadingMaterialPalette);

    mElectricalMaterialPaletteBrowser = std::make_unique<MaterialPaletteBrowser<LayerType::Electrical>>(
        parent,
        *this,
        defaultMaterialDatabase.GetElectricalMaterialPalette(),
        shipTexturizer,
        soundController,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.33f, 0.33f));

    progressCallback(0.66f, ProgressMessageType::LoadingMaterialPalette);

    mRopesMaterialPaletteBrowser = std::make_unique<MaterialPaletteBrowser<LayerType::Ropes>>(
        parent,
        *this,
        defaultMaterialDatabase.GetRopeMaterialPalette(),
        shipTexturizer,
        soundController,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.66f, 0.33f));

    progressCallback(1.0f, ProgressMessageType::LoadingMaterialPalette);

    mStructuralMaterialEditDialog = std::make_unique<MaterialEditDialog<StructuralMaterial>>(parent);
    mElectricalMaterialEditDialog = std::make_unique<MaterialEditDialog<ElectricalMaterial>>(parent);
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

void MaterialPalettesController::OnNewCustomStructuralMaterial(StructuralMaterial const * baseMaterial)
{
    OnNewCustomMaterial<LayerType::Structural>(baseMaterial);
}

void MaterialPalettesController::OnNewCustomElectricalMaterial(ElectricalMaterial const * baseMaterial)
{
    OnNewCustomMaterial<LayerType::Electrical>(baseMaterial);
}

void MaterialPalettesController::OnNewCustomRopesMaterial(StructuralMaterial const * baseMaterial)
{
    OnNewCustomMaterial<LayerType::Ropes>(baseMaterial);
}

////////////////////////////////////////////////////////////

template<LayerType TLayerType>
void MaterialPalettesController::OnNewCustomMaterial(typename LayerTypeTraits<TLayerType>::material_type const * baseMaterial)
{
    using TVariantOverridesType = typename LayerTypeTraits<TLayerType>::material_type::VariantOverridesType;

    // Make new override
    TVariantOverridesType variantOverrides = baseMaterial->MakeStartingVariantOverrides();

    // Run dialog
    std::optional<TVariantOverridesType> result;
    if constexpr (TLayerType == LayerType::Structural || TLayerType == LayerType::Ropes)
    {
        result = mStructuralMaterialEditDialog->RunForNew(variantOverrides, baseMaterial);
    }
    else
    {
        static_assert(TLayerType == LayerType::Electrical);

        result = mElectricalMaterialEditDialog->RunForNew(variantOverrides, baseMaterial);
    }

    if (result.has_value())
    {
        // TODO
    }
}

}