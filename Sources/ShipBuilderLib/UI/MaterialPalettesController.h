/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-06-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "../ShipBuilderTypes.h"

#include "IMaterialPalettesController.h"
#include "MaterialDatabase.h"
#include "MaterialPaletteBrowser.h"

#include <Game/GameAssetManager.h>
#include <Game/ISoundController.h>

#include <Simulation/CustomMaterialsPod.h>
#include <Simulation/DefaultMaterialDatabase.h>
#include <Simulation/Layers.h>
#include <Simulation/ShipTexturizer.h>

#include <Core/GameTypes.h>
#include <Core/ProgressCallback.h>

#include <wx/window.h>

#include <functional>
#include <memory>

namespace ShipBuilder {

class MaterialPalettesController final :
    public IMaterialPalette,
    public IMaterialPalettesController
{
public:

    MaterialPalettesController(
        wxWindow * parent,
        std::function<void(StructuralMaterial const * material, MaterialPlaneType plane)> onStructuralLayerMaterialSelected,
        std::function<void(ElectricalMaterial const * material, MaterialPlaneType plane)> onElectricalLayerMaterialSelected,
        std::function<void(StructuralMaterial const * material, MaterialPlaneType plane)> onRopeLayerMaterialSelected,
        DefaultMaterialDatabase const & defaultMaterialDatabase,
        ShipTexturizer const & shipTexturizer,
        ISoundController * soundController,
        GameAssetManager const & gameAssetManager,
        ProgressCallback const & progressCallback);

    LayersMaterialReplacementMaps OnShipLoaded(CustomMaterialsPod && shipCustomMaterials);

    template<LayerType TLayer>
    void Open(
        wxRect const & referenceArea,
        MaterialPlaneType materialPlane,
        typename LayerTypeTraits<TLayer>::material_type const * initialMaterial)
    {
        if constexpr (TLayer == LayerType::Structural)
        {
            mStructuralLayerMaterialPaletteBrowser->Open(
                referenceArea,
                materialPlane,
                initialMaterial);

            mLastOpenedPalette = mStructuralLayerMaterialPaletteBrowser.get();
        }
        else if constexpr (TLayer == LayerType::Electrical)
        {
            mElectricalLayerMaterialPaletteBrowser->Open(
                referenceArea,
                materialPlane,
                initialMaterial);

            mLastOpenedPalette = mElectricalLayerMaterialPaletteBrowser.get();
        }
        else
        {
            assert(TLayer == LayerType::Ropes);

            mRopesLayerMaterialPaletteBrowser->Open(
                referenceArea,
                materialPlane,
                initialMaterial);

            mLastOpenedPalette = mRopesLayerMaterialPaletteBrowser.get();
        }
    }

    bool IsOpen() const override;

    //
    // IMaterialPalettesController
    //

    void OnStructuralMaterialSelected(StructuralMaterial const * material, MaterialPlaneType plane) override;
    void OnElectricalMaterialSelected(ElectricalMaterial const * material, MaterialPlaneType plane) override;
    void OnRopesMaterialSelected(StructuralMaterial const * material, MaterialPlaneType plane) override;

    void OnNewUserCustomStructuralMaterial(StructuralMaterial const * baseMaterial) override;
    void OnNewUserCustomElectricalMaterial(ElectricalMaterial const * baseMaterial) override;
    void OnNewUserCustomRopesMaterial(StructuralMaterial const * baseMaterial) override;

private:

    template<LayerType TLayerType>
    void OnNewUserCustomMaterial(typename LayerTypeTraits<TLayerType>::material_type const * baseMaterial);

private:

    std::function<void(StructuralMaterial const * material, MaterialPlaneType plane)> const mOnStructuralLayerMaterialSelected;
    std::function<void(ElectricalMaterial const * material, MaterialPlaneType plane)> const mOnElectricalLayerMaterialSelected;
    std::function<void(StructuralMaterial const * material, MaterialPlaneType plane)> const mOnRopeLayerMaterialSelected;

    // Model
    std::unique_ptr<MaterialDatabase<LayerType::Structural>> mStructuralLayerMaterialDatabase;
    std::unique_ptr<MaterialDatabase<LayerType::Electrical>> mElectricalLayerMaterialDatabase;
    std::unique_ptr<MaterialDatabase<LayerType::Ropes>> mRopesLayerMaterialDatabase;

    // View
    std::unique_ptr<MaterialPaletteBrowser<LayerType::Structural>> mStructuralLayerMaterialPaletteBrowser;
    std::unique_ptr<MaterialPaletteBrowser<LayerType::Electrical>> mElectricalLayerMaterialPaletteBrowser;
    std::unique_ptr<MaterialPaletteBrowser<LayerType::Ropes>> mRopesLayerMaterialPaletteBrowser;

    IMaterialPalette const * mLastOpenedPalette;
};

}