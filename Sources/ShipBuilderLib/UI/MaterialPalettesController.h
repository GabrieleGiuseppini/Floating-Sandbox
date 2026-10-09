/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-06-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "IMaterialPalettesController.h"
#include "MaterialPaletteBrowser.h"

#include <Game/GameAssetManager.h>
#include <Game/ISoundController.h>

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

    template<LayerType TLayer>
    void Open(
        wxRect const & referenceArea,
        MaterialPlaneType materialPlane,
        typename LayerTypeTraits<TLayer>::material_type const * initialMaterial)
    {
        if constexpr (TLayer == LayerType::Structural)
        {
            mStructuralMaterialPaletteBrowser->Open(
                referenceArea,
                materialPlane,
                initialMaterial);

            mLastOpenedPalette = mStructuralMaterialPaletteBrowser.get();
        }
        else if constexpr (TLayer == LayerType::Electrical)
        {
            mElectricalMaterialPaletteBrowser->Open(
                referenceArea,
                materialPlane,
                initialMaterial);

            mLastOpenedPalette = mElectricalMaterialPaletteBrowser.get();
        }
        else
        {
            assert(TLayer == LayerType::Ropes);

            mRopesMaterialPaletteBrowser->Open(
                referenceArea,
                materialPlane,
                initialMaterial);

            mLastOpenedPalette = mRopesMaterialPaletteBrowser.get();
        }
    }

    bool IsOpen() const override;

    //
    // IMaterialPalettesController
    //

    void OnStructuralMaterialSelected(StructuralMaterial const * material, MaterialPlaneType plane) override;
    void OnElectricalMaterialSelected(ElectricalMaterial const * material, MaterialPlaneType plane) override;
    void OnRopesMaterialSelected(StructuralMaterial const * material, MaterialPlaneType plane) override;

private:

    std::function<void(StructuralMaterial const * material, MaterialPlaneType plane)> const mOnStructuralLayerMaterialSelected;
    std::function<void(ElectricalMaterial const * material, MaterialPlaneType plane)> const mOnElectricalLayerMaterialSelected;
    std::function<void(StructuralMaterial const * material, MaterialPlaneType plane)> const mOnRopeLayerMaterialSelected;

    std::unique_ptr<MaterialPaletteBrowser<LayerType::Structural>> mStructuralMaterialPaletteBrowser;
    std::unique_ptr<MaterialPaletteBrowser<LayerType::Electrical>> mElectricalMaterialPaletteBrowser;
    std::unique_ptr<MaterialPaletteBrowser<LayerType::Ropes>> mRopesMaterialPaletteBrowser;

    IMaterialPalette const * mLastOpenedPalette;
};

}