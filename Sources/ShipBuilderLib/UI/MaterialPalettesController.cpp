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
    // Model

    mStructuralLayerMaterialDatabase = std::make_unique<MaterialDatabase<LayerType::Structural>>(
        defaultMaterialDatabase.GetStructuralMaterialColorMap(),
        StandardSystemPaths::GetInstance().GetUserGameRootFolderPath());

    mElectricalLayerMaterialDatabase = std::make_unique<MaterialDatabase<LayerType::Electrical>>(
        defaultMaterialDatabase.GetElectricalMaterialColorMap(),
        StandardSystemPaths::GetInstance().GetUserGameRootFolderPath());

    mRopesLayerMaterialDatabase = std::make_unique<MaterialDatabase<LayerType::Ropes>>(
        defaultMaterialDatabase.GetStructuralMaterialColorMap(),
        StandardSystemPaths::GetInstance().GetUserGameRootFolderPath());

    // View

    mStructuralLayerMaterialPaletteBrowser = std::make_unique<MaterialPaletteBrowser<LayerType::Structural>>(
        parent,
        *this,
        defaultMaterialDatabase.GetStructuralMaterialPalette(),
        mStructuralLayerMaterialDatabase->GetUserCustomMaterials(),
        shipTexturizer,
        soundController,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.0f, 0.33f));

    progressCallback(0.33f, ProgressMessageType::LoadingMaterialPalette);

    mElectricalLayerMaterialPaletteBrowser = std::make_unique<MaterialPaletteBrowser<LayerType::Electrical>>(
        parent,
        *this,
        defaultMaterialDatabase.GetElectricalMaterialPalette(),
        mElectricalLayerMaterialDatabase->GetUserCustomMaterials(),
        shipTexturizer,
        soundController,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.33f, 0.33f));

    progressCallback(0.66f, ProgressMessageType::LoadingMaterialPalette);

    mRopesLayerMaterialPaletteBrowser = std::make_unique<MaterialPaletteBrowser<LayerType::Ropes>>(
        parent,
        *this,
        defaultMaterialDatabase.GetRopeMaterialPalette(),
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

////////////////////////////////////////////////////////////

template<LayerType TLayerType>
void MaterialPalettesController::OnNewUserCustomMaterial(typename LayerTypeTraits<TLayerType>::material_type const * baseMaterial)
{
    using TMaterial = typename LayerTypeTraits<TLayerType>::material_type;
    using TVariantOverridesType = typename TMaterial::VariantOverridesType;

    // Make new override
    TVariantOverridesType overrides = baseMaterial->MakeStartingVariantOverrides();

    // Run dialog
    std::optional<TVariantOverridesType> resultOverrides;
    if constexpr (TLayerType == LayerType::Structural)
    {
        resultOverrides = mStructuralLayerMaterialPaletteBrowser->RunNewCustomMaterial(overrides, baseMaterial);
    }
    else if constexpr (TLayerType == LayerType::Electrical)
    {
        resultOverrides = mElectricalLayerMaterialPaletteBrowser->RunNewCustomMaterial(overrides, baseMaterial);
    }
    else
    {
        static_assert(TLayerType == LayerType::Ropes);

        resultOverrides = mRopesLayerMaterialPaletteBrowser->RunNewCustomMaterial(overrides, baseMaterial);
    }

    if (resultOverrides.has_value())
    {
        //
        // Make material
        //

        std::unique_ptr<TMaterial> customMaterial = baseMaterial->MakeCustomMaterial(*resultOverrides);

        //
        // Store material in DB and add to palette
        //

        if constexpr (TLayerType == LayerType::Structural)
        {
            TMaterial const * customMaterialPtr = mStructuralLayerMaterialDatabase->AddUserCustomMaterial(std::move(customMaterial));
            mStructuralLayerMaterialPaletteBrowser->AddUserCustomMaterials({ customMaterialPtr });
        }
        else if constexpr (TLayerType == LayerType::Electrical)
        {
            TMaterial const * customMaterialPtr = mElectricalLayerMaterialDatabase->AddUserCustomMaterial(std::move(customMaterial));
            mElectricalLayerMaterialPaletteBrowser->AddUserCustomMaterials({ customMaterialPtr });
        }
        else
        {
            static_assert(TLayerType == LayerType::Ropes);

            TMaterial const * customMaterialPtr = mRopesLayerMaterialDatabase->AddUserCustomMaterial(std::move(customMaterial));
            mRopesLayerMaterialPaletteBrowser->AddUserCustomMaterials({ customMaterialPtr });
        }
    }
}

}