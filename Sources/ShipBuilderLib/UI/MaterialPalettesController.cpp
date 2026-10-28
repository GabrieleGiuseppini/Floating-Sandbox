/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-06-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MaterialPalettesController.h"

#include <UILib/StandardSystemPaths.h>

#include <Game/GameVersion.h>

#include <wx/filedlg.h>
#include <wx/msgdlg.h>

#include <algorithm>
#include <set>

namespace ShipBuilder {

MaterialPalettesController::MaterialPalettesController(
    wxWindow * parent,
    wxIcon const & icon,
    std::function<void(StructuralMaterial const * material, MaterialPlaneType plane)> onStructuralLayerMaterialSelected,
    std::function<void(ElectricalMaterial const * material, MaterialPlaneType plane)> onElectricalLayerMaterialSelected,
    std::function<void(StructuralMaterial const * material, MaterialPlaneType plane)> onRopeLayerMaterialSelected,
    DefaultMaterialDatabase const & defaultMaterialDatabase,
    std::function<Controller &()> controllerGetter,
    ShipTexturizer const & shipTexturizer,
    ISoundController * soundController,
    WorkbenchState const & workbenchState,
    GameAssetManager const & gameAssetManager,
    ProgressCallback const & progressCallback)
    : mDefaultMaterialDatabase(defaultMaterialDatabase)
    , mWorkbenchState(workbenchState)
    , mControllerGetter(std::move(controllerGetter))
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

bool MaterialPalettesController::IsAnyCustomMaterialChecked() const
{
    return GetPaletteBrowser<LayerType::Structural>().IsAnyCustomMaterialChecked()
        || GetPaletteBrowser<LayerType::Electrical>().IsAnyCustomMaterialChecked()
        || GetPaletteBrowser<LayerType::Ropes>().IsAnyCustomMaterialChecked();
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

void MaterialPalettesController::OnEditCustomStructuralMaterial(StructuralMaterial const * customMaterial)
{
    OnEditCustomMaterial<LayerType::Structural>(customMaterial);
}

void MaterialPalettesController::OnEditCustomElectricalMaterial(ElectricalMaterial const * customMaterial)
{
    OnEditCustomMaterial<LayerType::Electrical>(customMaterial);
}

void MaterialPalettesController::OnEditCustomRopesMaterial(StructuralMaterial const * customMaterial)
{
    OnEditCustomMaterial<LayerType::Ropes>(customMaterial);
}

void MaterialPalettesController::OnRemoveCustomStructuralMaterials(std::vector<StructuralMaterial const *> customMaterials)
{
    OnRemoveCustomMaterials<LayerType::Structural>(std::move(customMaterials));
}

void MaterialPalettesController::OnRemoveCustomElectricalMaterials(std::vector<ElectricalMaterial const *> customMaterials)
{
    OnRemoveCustomMaterials<LayerType::Electrical>(std::move(customMaterials));
}

void MaterialPalettesController::OnRemoveCustomRopesMaterials(std::vector<StructuralMaterial const *> customMaterials)
{
    OnRemoveCustomMaterials<LayerType::Ropes>(std::move(customMaterials));
}

void MaterialPalettesController::OnImportCustomStructuralShipMaterials(std::vector<StructuralMaterial const *> customMaterials)
{
    OnImportCustomShipMaterials<LayerType::Structural>(std::move(customMaterials));
}

void MaterialPalettesController::OnImportCustomElectricalShipMaterials(std::vector<ElectricalMaterial const *> customMaterials)
{
    OnImportCustomShipMaterials<LayerType::Electrical>(std::move(customMaterials));
}

void MaterialPalettesController::OnImportCustomRopesShipMaterials(std::vector<StructuralMaterial const *> customMaterials)
{
    OnImportCustomShipMaterials<LayerType::Ropes>(std::move(customMaterials));
}

void MaterialPalettesController::OnExportCheckedCustomMaterials() const
{
    //
    // Ask path
    //

    wxFileDialog saveAsDialog(
        nullptr,
        _("Save custom materials"),
        StandardSystemPaths::GetInstance().GetUserDesktopFolderPath().string()
            + std::filesystem::path::preferred_separator, // See https://forums.wxwidgets.org/viewtopic.php?t=42293
        "my_custom_materials.json",
        _("Custom material files") + wxS(" (*.json)|*.json"),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
        wxDefaultPosition,
        wxDefaultSize);

    auto const res = saveAsDialog.ShowModal();
    if (res == wxID_OK)
    {
        //
        // Get checked materials from all layers
        //

        std::vector<StructuralMaterial const *> structuralMaterials = GetPaletteBrowser<LayerType::Structural>().GetAllCustomMaterialsChecked();
        std::vector<ElectricalMaterial const *> electricalMaterials = GetPaletteBrowser<LayerType::Electrical>().GetAllCustomMaterialsChecked();
        std::vector<StructuralMaterial const *> ropesMaterials = GetPaletteBrowser<LayerType::Ropes>().GetAllCustomMaterialsChecked();

        //
        // Export
        //

        BaseMaterialDatabase::ExportCustomMaterials(
            structuralMaterials,
            electricalMaterials,
            ropesMaterials,
            std::filesystem::path(saveAsDialog.GetPath().ToStdString()));
    }
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
void MaterialPalettesController::OnEditCustomMaterial(typename LayerTypeTraits<TLayerType>::material_type const * customMaterial)
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
        TMaterial const * const newCustomMaterialPtr = newCustomMaterial.get();

        //
        // Replace material in DB
        //

        std::unique_ptr<TMaterial const> oldCustomMaterial = GetDatabase<TLayerType>().ReplaceCustomMaterial(customMaterial, std::move(newCustomMaterial));
        TMaterial const * const oldCustomMaterialPtr = oldCustomMaterial.get();

        //
        // If any of these materials is current in the workbench state, synthesize a toggle to the new material
        //

        PreCustomMaterialUpdated<TLayerType>(oldCustomMaterialPtr, newCustomMaterialPtr);

        //
        // Bake Undo action for material
        //

        auto materialUndoAction = [this, newCustomMaterialPtr, old = std::move(oldCustomMaterial)]() mutable
            {
                RestoreCustomMaterialReplacementForUndo<TLayerType>(newCustomMaterialPtr, std::move(old));
            };

        //
        // Replace in ship (if exists)
        //

        Controller & controller = mControllerGetter();

        wxString undoTitle;
        if constexpr (TLayerType == LayerType::Structural)
        {
            undoTitle = _("Structural Material Edit");
        }
        else if constexpr (TLayerType == LayerType::Electrical)
        {
            undoTitle = _("Electrical Material Edit");
        }
        else
        {
            static_assert(TLayerType == LayerType::Ropes);
            undoTitle = _("Ropes Material Edit");
        }

        controller.ReplaceCustomMaterials<TLayerType>(
            std::map<TMaterial const *, TMaterial const *>({ { oldCustomMaterialPtr, newCustomMaterialPtr } }),
            std::move(materialUndoAction),
            std::move(undoTitle));

        //
        // Update material in palette
        //

        GetPaletteBrowser<TLayerType>().ReplaceCustomMaterial(oldCustomMaterialPtr, newCustomMaterialPtr);
    }
}

template<LayerType TLayerType>
void MaterialPalettesController::OnRemoveCustomMaterials(std::vector<typename LayerTypeTraits<TLayerType>::material_type const *> customMaterials)
{
    using TMaterial = typename LayerTypeTraits<TLayerType>::material_type;

    //
    // Check if in-use by ship
    //

    Controller & controller = mControllerGetter();

    std::set<TMaterial const *> shipSet;
    if constexpr (TLayerType == LayerType::Structural)
    {
        shipSet = controller.GetModelController().GetStructuralMaterialsInUse(customMaterials);
    }
    else if constexpr (TLayerType == LayerType::Electrical)
    {
        shipSet = controller.GetModelController().GetElectricalMaterialsInUse(customMaterials);
    }
    else
    {
        static_assert(TLayerType == LayerType::Ropes);
        shipSet = controller.GetModelController().GetRopesMaterialsInUse(customMaterials);
    }

    if (!shipSet.empty())
    {
        //
        // Ask user how to proceed
        //

        wxMessageDialog dialog(
            nullptr,
            _("One or more of the materials you are about to delete are being used in the current ship. Do you want to keep these materials, or would you rather delete the corresponding particles instead?"),
            ApplicationName,
            wxYES_NO | wxCANCEL | wxSTAY_ON_TOP | wxCENTRE);

        dialog.SetYesNoCancelLabels(
            _("Keep the Materials"),
            _("Remove Particles from Ship"),
            _("Abort"));

        auto const result = dialog.ShowModal();
        if (result == wxID_YES)
        {
            // Keep materials

            // Prune delenda
            for (auto delendaIt = customMaterials.begin(); delendaIt != customMaterials.end(); /*incremented in loop*/)
            {
                if (shipSet.find(*delendaIt) != shipSet.end())
                {
                    delendaIt = customMaterials.erase(delendaIt);
                }
                else
                {
                    ++delendaIt;
                }
            }

            // Empty ship set - so we won't remove these
            shipSet.clear();
        }
        else if (result == wxID_NO)
        {
            // Remove particles from ship

            // Nothing here, we'll remove particles and then delete all
        }
        else
        {
            // Abort
            return;
        }
    }

    if (customMaterials.empty())
    {
        // Nothing left to do
        return;
    }

    //
    // Remove from DB
    //

    auto [removedUserMaterials, removedShipMaterials] = GetDatabase<TLayerType>().ExtractCustomMaterials(customMaterials);

    //
    // If any of these materials is current in the workbench state, synthesize a toggle to the new material
    //

    for (TMaterial const * delendumMaterial : customMaterials)
    {
        PreCustomMaterialDeleted<TLayerType>(delendumMaterial);
    }

    //
    // Bake Undo action for material
    //

    auto materialUndoAction = [this, removedUserMaterials = std::move(removedUserMaterials), removedShipMaterials = std::move(removedShipMaterials)]() mutable
        {
            RestoreCustomMaterialsDeletionForUndo<TLayerType>(std::move(removedUserMaterials), std::move(removedShipMaterials));
        };

    //
    // Replace in ship (if any)
    //

    wxString undoTitle;
    if constexpr (TLayerType == LayerType::Structural)
    {
        undoTitle = _("Structural Materials Delete");
    }
    else if constexpr (TLayerType == LayerType::Electrical)
    {
        undoTitle = _("Electrical Materials Delete");
    }
    else
    {
        static_assert(TLayerType == LayerType::Ropes);
        undoTitle = _("Ropes Materials Delete");
    }

    std::map<TMaterial const *, TMaterial const *> shipMaterialsReplacementMap;
    for (TMaterial const * sm : shipSet)
    {
        shipMaterialsReplacementMap.emplace(sm, nullptr);
    }

    controller.ReplaceCustomMaterials<TLayerType>(
        shipMaterialsReplacementMap,
        std::move(materialUndoAction),
        std::move(undoTitle));

    //
    // Remove materials from palette
    //

    GetPaletteBrowser<TLayerType>().RemoveCustomMaterials(customMaterials);
}

template<LayerType TLayerType>
void MaterialPalettesController::OnImportCustomShipMaterials(std::vector<typename LayerTypeTraits<TLayerType>::material_type const *> customMaterials)
{
    using TMaterial = typename LayerTypeTraits<TLayerType>::material_type;

    //
    // Move in DB
    //

    GetDatabase<TLayerType>().MoveShipCustomMaterialsToUser(customMaterials);

    //
    // Bake Undo action
    //

    auto materialUndoAction = [this, customMaterials](Controller &)
        {
            RestoreCustomMaterialsShipToUserMoveForUndo<TLayerType>(customMaterials);
        };

    auto & controller = mControllerGetter();

    controller.StoreUndoAction(
        _("Materials Import"),
        1, // Nominal
        controller.GetModelController().GetDirtyState(),
        std::move(materialUndoAction));

    //
    // Move in palette
    //

    GetPaletteBrowser<TLayerType>().MoveCustomMaterialsRegister(customMaterials, CustomMaterialRegisterKindType::User);
}

template<LayerType TLayerType>
void MaterialPalettesController::RestoreCustomMaterialReplacementForUndo(
    typename LayerTypeTraits<TLayerType>::material_type const * customMaterialToReplace,
    std::unique_ptr<typename LayerTypeTraits<TLayerType>::material_type const> && customMaterialToRestore)
{
    using TMaterial = typename LayerTypeTraits<TLayerType>::material_type;

    TMaterial const * const newCustomMaterialPtr = customMaterialToRestore.get();

    //
    // Replace material in DB
    //

    std::unique_ptr<TMaterial const> oldCustomMaterial = GetDatabase<TLayerType>().ReplaceCustomMaterial(customMaterialToReplace, std::move(customMaterialToRestore));
    TMaterial const * const oldCustomMaterialPtr = oldCustomMaterial.get();

    //
    // If any of these materials is current in the workbench state, synthesize a toggle to the new material
    //

    PreCustomMaterialUpdated<TLayerType>(oldCustomMaterialPtr, newCustomMaterialPtr);

    //
    // Update material in palette
    //

    GetPaletteBrowser<TLayerType>().ReplaceCustomMaterial(oldCustomMaterialPtr, newCustomMaterialPtr);
}

template<LayerType TLayerType>
void MaterialPalettesController::RestoreCustomMaterialsDeletionForUndo(
    std::vector<std::unique_ptr<typename LayerTypeTraits<TLayerType>::material_type const>> && userCustomMaterials,
    std::vector<std::unique_ptr<typename LayerTypeTraits<TLayerType>::material_type const>> && shipCustomMaterials)
{
    using TMaterial = typename LayerTypeTraits<TLayerType>::material_type;

    // Save pointers

    std::vector<TMaterial const *> userCustomMaterialPtrs;
    std::transform(
        userCustomMaterials.cbegin(),
        userCustomMaterials.cend(),
        std::back_inserter(userCustomMaterialPtrs),
        [](auto & uq_ptr) -> TMaterial const *
        {
            return uq_ptr.get();
        });

    std::vector<TMaterial const *> shipCustomMaterialPtrs;
    std::transform(
        shipCustomMaterials.cbegin(),
        shipCustomMaterials.cend(),
        std::back_inserter(shipCustomMaterialPtrs),
        [](auto & uq_ptr) -> TMaterial const *
        {
            return uq_ptr.get();
        });

    //
    // Re-add materials to DB
    //

    GetDatabase<TLayerType>().AddUserCustomMaterials(std::move(userCustomMaterials));
    GetDatabase<TLayerType>().AddShipCustomMaterials(std::move(shipCustomMaterials));

    //
    // Re-add materials to palettes
    //

    GetPaletteBrowser<TLayerType>().AddUserCustomMaterials(userCustomMaterialPtrs);
    GetPaletteBrowser<TLayerType>().AddShipCustomMaterials(shipCustomMaterialPtrs);
}

template<LayerType TLayerType>
void MaterialPalettesController::RestoreCustomMaterialsShipToUserMoveForUndo(std::vector<typename LayerTypeTraits<TLayerType>::material_type const *> const & movedMaterials)
{
    //
    // Move in DB
    //

    GetDatabase<TLayerType>().MoveUserCustomMaterialsToShip(movedMaterials);

    //
    // Move in palette
    //

    GetPaletteBrowser<TLayerType>().MoveCustomMaterialsRegister(movedMaterials, CustomMaterialRegisterKindType::Ship);
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