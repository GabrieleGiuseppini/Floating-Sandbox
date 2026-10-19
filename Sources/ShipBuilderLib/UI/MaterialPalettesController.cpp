/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-06-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MaterialPalettesController.h"

#include <UILib/StandardSystemPaths.h>

namespace ShipBuilder {

MaterialPalettesController::MaterialPalettesController(
    wxWindow * parent,
    wxIcon const & icon,
    std::function<void(StructuralMaterial const * material, MaterialPlaneType plane)> onStructuralLayerMaterialSelected,
    std::function<void(ElectricalMaterial const * material, MaterialPlaneType plane)> onElectricalLayerMaterialSelected,
    std::function<void(StructuralMaterial const * material, MaterialPlaneType plane)> onRopeLayerMaterialSelected,
    DefaultMaterialDatabase const & defaultMaterialDatabase,
    ShipTexturizer const & shipTexturizer,
    ISoundController * soundController,
    WorkbenchState const & workbenchState,
    GameAssetManager const & gameAssetManager,
    ProgressCallback const & progressCallback)
    : mDefaultMaterialDatabase(defaultMaterialDatabase)
    , mWorkbenchState(workbenchState)
    , mOnStructuralLayerMaterialSelected(std::move(onStructuralLayerMaterialSelected))
    , mOnElectricalLayerMaterialSelected(std::move(onElectricalLayerMaterialSelected))
    , mOnRopeLayerMaterialSelected(std::move(onRopeLayerMaterialSelected))
    , mLastOpenedPaletteBrowser(nullptr)
{
    // Model

    mStructuralLayerMaterialDatabase = std::make_unique<MaterialDatabase<LayerType::Structural>>(
        mDefaultMaterialDatabase.GetStructuralMaterialColorMap(),
        StandardSystemPaths::GetInstance().GetUserGameRootFolderPath());

    mElectricalLayerMaterialDatabase = std::make_unique<MaterialDatabase<LayerType::Electrical>>(
        mDefaultMaterialDatabase.GetElectricalMaterialColorMap(),
        StandardSystemPaths::GetInstance().GetUserGameRootFolderPath());

    mRopesLayerMaterialDatabase = std::make_unique<MaterialDatabase<LayerType::Ropes>>(
        mDefaultMaterialDatabase.GetStructuralMaterialColorMap(),
        StandardSystemPaths::GetInstance().GetUserGameRootFolderPath());

    // View

    mStructuralLayerMaterialPaletteBrowser = std::make_unique<MaterialPaletteBrowser<LayerType::Structural>>(
        parent,
        icon,
        *this,
        mDefaultMaterialDatabase.GetStructuralMaterialPalette(),
        mStructuralLayerMaterialDatabase->GetUserCustomMaterials(),
        shipTexturizer,
        soundController,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.0f, 0.33f));

    progressCallback(0.33f, ProgressMessageType::LoadingMaterialPalette);

    mElectricalLayerMaterialPaletteBrowser = std::make_unique<MaterialPaletteBrowser<LayerType::Electrical>>(
        parent,
        icon,
        *this,
        mDefaultMaterialDatabase.GetElectricalMaterialPalette(),
        mElectricalLayerMaterialDatabase->GetUserCustomMaterials(),
        shipTexturizer,
        soundController,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.33f, 0.33f));

    progressCallback(0.66f, ProgressMessageType::LoadingMaterialPalette);

    mRopesLayerMaterialPaletteBrowser = std::make_unique<MaterialPaletteBrowser<LayerType::Ropes>>(
        parent,
        icon,
        *this,
        mDefaultMaterialDatabase.GetRopeMaterialPalette(),
        mRopesLayerMaterialDatabase->GetUserCustomMaterials(),
        shipTexturizer,
        soundController,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.66f, 0.33f));

    progressCallback(1.0f, ProgressMessageType::LoadingMaterialPalette);
}

LayersMaterialReplacementMaps MaterialPalettesController::OnShipLoaded(CustomMaterialsPod && shipCustomMaterials)
{
    //
    // First off, if any of these materials is current in the workbench state, synthesize a toggle to the "clear" material
    //

    for (auto const & m : mStructuralLayerMaterialDatabase->GetShipCustomMaterials())
    {
        PreCustomMaterialDeleted<LayerType::Structural>(m.get());
    }

    for (auto const & m : mElectricalLayerMaterialDatabase->GetShipCustomMaterials())
    {
        PreCustomMaterialDeleted<LayerType::Electrical>(m.get());
    }

    for (auto const & m : mRopesLayerMaterialDatabase->GetShipCustomMaterials())
    {
        PreCustomMaterialDeleted<LayerType::Ropes>(m.get());
    }

    //
    // Replace ship custom materials on both model & view
    //

    MaterialReplacementMap<StructuralMaterial> structuralLayerMaterialReplacementMap = mStructuralLayerMaterialDatabase->SetShipCustomMaterials(std::move(shipCustomMaterials.CustomStructuralLayerMaterials));
    MaterialReplacementMap<ElectricalMaterial> electricalLayerMaterialReplacementMap = mElectricalLayerMaterialDatabase->SetShipCustomMaterials(std::move(shipCustomMaterials.CustomElectricalLayerMaterials));
    MaterialReplacementMap<StructuralMaterial> ropesLayerMaterialReplacementMap = mRopesLayerMaterialDatabase->SetShipCustomMaterials(std::move(shipCustomMaterials.CustomRopesLayerMaterials));

    mStructuralLayerMaterialPaletteBrowser->SetShipCustomMaterials(mStructuralLayerMaterialDatabase->GetShipCustomMaterials());
    mElectricalLayerMaterialPaletteBrowser->SetShipCustomMaterials(mElectricalLayerMaterialDatabase->GetShipCustomMaterials());
    mRopesLayerMaterialPaletteBrowser->SetShipCustomMaterials(mRopesLayerMaterialDatabase->GetShipCustomMaterials());

    return LayersMaterialReplacementMaps(
        std::move(structuralLayerMaterialReplacementMap),
        std::move(electricalLayerMaterialReplacementMap),
        std::move(ropesLayerMaterialReplacementMap));
}

void MaterialPalettesController::Close()
{
    if (mLastOpenedPaletteBrowser != nullptr)
    {
        mLastOpenedPaletteBrowser->Close();
    }
}

bool MaterialPalettesController::CanLoseFocus() const
{
    if (mLastOpenedPaletteBrowser == nullptr)
    {
        return false;
    }
    else
    {
        return mLastOpenedPaletteBrowser->CanLoseFocus();
    }
}

void MaterialPalettesController::OnStructuralMaterialSelected(
    StructuralMaterial const * material,
    MaterialPlaneType plane)
{
    OnMaterialSelected<LayerType::Structural>(material, plane);
}

void MaterialPalettesController::OnElectricalMaterialSelected(
    ElectricalMaterial const * material,
    MaterialPlaneType plane)
{
    OnMaterialSelected<LayerType::Electrical>(material, plane);
}

void MaterialPalettesController::OnRopesMaterialSelected(
    StructuralMaterial const * material,
    MaterialPlaneType plane)
{
    OnMaterialSelected<LayerType::Ropes>(material, plane);
}

void MaterialPalettesController::OnNewUserCustomStructuralMaterial(StructuralMaterial const * baseMaterial)
{
    OnNewUserCustomMaterial<LayerType::Structural>(baseMaterial);
}

void MaterialPalettesController::OnNewUserCustomElectricalMaterial(ElectricalMaterial const * baseMaterial)
{
    OnNewUserCustomMaterial<LayerType::Electrical>(baseMaterial);
}

void MaterialPalettesController::OnNewUserCustomRopesMaterial(StructuralMaterial const * baseMaterial)
{
    OnNewUserCustomMaterial<LayerType::Ropes>(baseMaterial);
}

void MaterialPalettesController::OnEditCustomStructuralMaterial(
    StructuralMaterial const * customMaterial,
    std::optional<CustomMaterialRegisterKindType> registerKind)
{
    OnEditCustomMaterial<LayerType::Structural>(customMaterial, registerKind);
}

void MaterialPalettesController::OnEditCustomElectricalMaterial(
    ElectricalMaterial const * customMaterial,
    std::optional<CustomMaterialRegisterKindType> registerKind)
{
    OnEditCustomMaterial<LayerType::Electrical>(customMaterial, registerKind);
}

void MaterialPalettesController::OnEditCustomRopesMaterial(
    StructuralMaterial const * customMaterial,
    std::optional<CustomMaterialRegisterKindType> registerKind)
{
    OnEditCustomMaterial<LayerType::Ropes>(customMaterial, registerKind);
}

////////////////////////////////////////////////////////////

template<LayerType TLayerType>
void MaterialPalettesController::OnMaterialSelected(
    typename LayerTypeTraits<TLayerType>::material_type const * material,
    MaterialPlaneType plane)
{
    if constexpr (TLayerType == LayerType::Structural)
    {
        mOnStructuralLayerMaterialSelected(material, plane);
    }
    else if constexpr (TLayerType == LayerType::Electrical)
    {
        mOnElectricalLayerMaterialSelected(material, plane);
    }
    else
    {
        static_assert(TLayerType == LayerType::Ropes);

        mOnRopeLayerMaterialSelected(material, plane);
    }
}

template<LayerType TLayerType>
void MaterialPalettesController::OnNewUserCustomMaterial(typename LayerTypeTraits<TLayerType>::material_type const * baseMaterial)
{
    using TMaterial = typename LayerTypeTraits<TLayerType>::material_type;
    using TVariantOverridesType = typename TMaterial::VariantOverridesType;

    // Make new overrides as a starting point
    TVariantOverridesType startingOverrides = baseMaterial->MakeStartingVariantOverrides();

    // Run dialog
    std::optional<TVariantOverridesType> const resultOverrides = GetPaletteBrowser<TLayerType>().RunNewCustomMaterial(startingOverrides, baseMaterial);
    if (resultOverrides.has_value())
    {
        //
        // Make material
        //

        std::unique_ptr<TMaterial> customMaterial = baseMaterial->MakeCustomMaterial(*resultOverrides);

        //
        // Store material in DB and add to palette
        //

        TMaterial const * customMaterialPtr = GetDatabase<TLayerType>().AddUserCustomMaterial(std::move(customMaterial));
        GetPaletteBrowser<TLayerType>().AddUserCustomMaterials({ customMaterialPtr });
    }
}

template<LayerType TLayerType>
void MaterialPalettesController::OnEditCustomMaterial(
    typename LayerTypeTraits<TLayerType>::material_type const * customMaterial,
    std::optional<CustomMaterialRegisterKindType> registerKind)
{
    using TMaterial = typename LayerTypeTraits<TLayerType>::material_type;
    using TVariantOverridesType = typename TMaterial::VariantOverridesType;

    // Find base
    TMaterial const * baseMaterial;
    if constexpr (TMaterial::MaterialLayer == MaterialLayerType::Structural)
    {
        baseMaterial = mDefaultMaterialDatabase.FindStructuralMaterial(customMaterial->ColorKey);
    }
    else
    {
        static_assert(TMaterial::MaterialLayer == MaterialLayerType::Electrical);
        baseMaterial = mDefaultMaterialDatabase.FindElectricalMaterial(customMaterial->ColorKey);
    }

    assert(baseMaterial != nullptr);

    // Take starting overrides from custom material
    assert(customMaterial->VariantOverrides.has_value());
    TVariantOverridesType overrides = *customMaterial->VariantOverrides;

    // Run dialog
    std::optional<TVariantOverridesType> const resultOverrides = GetPaletteBrowser<TLayerType>().RunEditCustomMaterial(overrides, baseMaterial);
    if (resultOverrides.has_value())
    {
        //
        // Make new material
        //

        std::unique_ptr<TMaterial const> newCustomMaterial = baseMaterial->MakeCustomMaterial(*resultOverrides);
        TMaterial const * newCustomMaterialPtr = newCustomMaterial.get();

        //
        // Replace material in DB
        //

        std::unique_ptr<TMaterial const> oldCustomMaterial = GetDatabase<TLayerType>().ReplaceCustomMaterial(customMaterial, std::move(newCustomMaterial));
        TMaterial const * oldCustomMaterialPtr = oldCustomMaterial.get();

        //
        // If any of these materials is current in the workbench state, synthesize a toggle to the new material
        //

        PreCustomMaterialUpdated<TLayerType>(oldCustomMaterialPtr, newCustomMaterialPtr);

        //
        // Bake Undo action for material
        //

        auto materialUndoAction = [old = std::move(oldCustomMaterial)]()
            {
                // TODOHERE
            };

        // TODOHERE:
        // - Check if it's in-use by ship
        // - If in-use:
        //      - Replace in ship, via Controller::ReplaceCustom[Structural|Electrical|Ropes]Materials(map with old < ->new, Undo material - specific lambda)
        // - Else:
        //      - Controller::PushCustomMaterialUndoAction(Undo material - specific lambda)
        // - Update material in palette via Browser::UpdateCustomMaterial(...)

        (void)registerKind;
    }
}

template<LayerType TLayerType>
void MaterialPalettesController::PreCustomMaterialUpdated(
    typename LayerTypeTraits<TLayerType>::material_type const * oldCustomMaterial,
    typename LayerTypeTraits<TLayerType>::material_type const * newCustomMaterial)
{
    // Update workbench material selection if this is a currently-picked material

    if (mWorkbenchState.GetMaterial<TLayerType>(MaterialPlaneType::Foreground) == oldCustomMaterial)
    {
        OnMaterialSelected<TLayerType>(newCustomMaterial, MaterialPlaneType::Foreground);
    }

    if (mWorkbenchState.GetMaterial<TLayerType>(MaterialPlaneType::Background) == oldCustomMaterial)
    {
        OnMaterialSelected<TLayerType>(newCustomMaterial, MaterialPlaneType::Background);
    }
}

template<LayerType TLayerType>
void MaterialPalettesController::PreCustomMaterialDeleted(typename LayerTypeTraits<TLayerType>::material_type const * customMaterial)
{
    // Reset workbench material selection if this is a currently-picked material

    if (mWorkbenchState.GetMaterial<TLayerType>(MaterialPlaneType::Foreground) == customMaterial)
    {
        OnMaterialSelected<TLayerType>(nullptr, MaterialPlaneType::Foreground);
    }

    if (mWorkbenchState.GetMaterial<TLayerType>(MaterialPlaneType::Background) == customMaterial)
    {
        OnMaterialSelected<TLayerType>(nullptr, MaterialPlaneType::Background);
    }
}

}