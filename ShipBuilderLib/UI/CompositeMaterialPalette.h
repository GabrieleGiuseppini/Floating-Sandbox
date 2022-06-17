/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-06-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "MaterialPalette.h"

#include <Game/Layers.h>
#include <Game/MaterialDatabase.h>
#include <Game/ResourceLocator.h>
#include <Game/ShipTexturizer.h>

#include <GameCore/GameTypes.h>
#include <GameCore/ProgressCallback.h>

#include <wx/window.h>

#include <functional>
#include <memory>

namespace ShipBuilder {

class CompositeMaterialPalette final : public IMaterialPalette
{
public:

    CompositeMaterialPalette(
        wxWindow * parent,
        std::function<void(fsStructuralMaterialSelectedEvent const & event)> onStructuralLayerMaterialSelected,
        std::function<void(fsElectricalMaterialSelectedEvent const & event)> onElectricalLayerMaterialSelected,
        std::function<void(fsStructuralMaterialSelectedEvent const & event)> onRopeLayerMaterialSelected,
        MaterialDatabase const & materialDatabase,
        ShipTexturizer const & shipTexturizer,
        ResourceLocator const & resourceLocator,
        ProgressCallback const & progressCallback);

    template<LayerType TLayer>
    void Open(
        wxRect const & referenceArea,
        MaterialPlaneType materialPlane,
        typename LayerTypeTraits<TLayer>::material_type const * initialMaterial)
    {
        if constexpr (TLayer == LayerType::Structural)
        {
            mStructuralMaterialPalette->Open(
                referenceArea,
                materialPlane,
                initialMaterial);

            mLastOpenedPalette = mStructuralMaterialPalette.get();
        }
        else if constexpr (TLayer == LayerType::Electrical)
        {
            mElectricalMaterialPalette->Open(
                referenceArea,
                materialPlane,
                initialMaterial);

            mLastOpenedPalette = mElectricalMaterialPalette.get();
        }
        else
        {
            assert(TLayer == LayerType::Ropes);

            mRopesMaterialPalette->Open(
                referenceArea,
                materialPlane,
                initialMaterial);

            mLastOpenedPalette = mRopesMaterialPalette.get();
        }
    }

    bool IsOpen() const override;

private:

    std::function<void(fsStructuralMaterialSelectedEvent const & event)> const mOnStructuralLayerMaterialSelected;
    std::function<void(fsElectricalMaterialSelectedEvent const & event)> const mOnElectricalLayerMaterialSelected;
    std::function<void(fsStructuralMaterialSelectedEvent const & event)> const mOnRopeLayerMaterialSelected;

    std::unique_ptr<MaterialPalette<LayerType::Structural>> mStructuralMaterialPalette;
    std::unique_ptr<MaterialPalette<LayerType::Electrical>> mElectricalMaterialPalette;
    std::unique_ptr<MaterialPalette<LayerType::Ropes>> mRopesMaterialPalette;

    IMaterialPalette const * mLastOpenedPalette;
};

}