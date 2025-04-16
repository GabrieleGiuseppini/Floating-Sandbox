/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-06-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "CompositeMaterialPalette.h"

namespace ShipBuilder {

CompositeMaterialPalette::CompositeMaterialPalette(
    wxWindow * parent,
    std::function<void(fsStructuralMaterialSelectedEvent const & event)> onStructuralLayerMaterialSelected,
    std::function<void(fsElectricalMaterialSelectedEvent const & event)> onElectricalLayerMaterialSelected,
    std::function<void(fsStructuralMaterialSelectedEvent const & event)> onRopeLayerMaterialSelected,
    MaterialDatabase const & materialDatabase,
    ShipTexturizer const & shipTexturizer,
    GameAssetManager const & gameAssetManager,
    ProgressCallback const & progressCallback)
    : mOnStructuralLayerMaterialSelected(std::move(onStructuralLayerMaterialSelected))
    , mOnElectricalLayerMaterialSelected(std::move(onElectricalLayerMaterialSelected))
    , mOnRopeLayerMaterialSelected(std::move(onRopeLayerMaterialSelected))
    , mLastOpenedPalette(nullptr)
{
    mStructuralMaterialPalette = std::make_unique<MaterialPalette<LayerType::Structural>>(
        parent,
        materialDatabase.GetStructuralMaterialPalette(),
        shipTexturizer,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.0f, 0.33f));

    mStructuralMaterialPalette->Bind(
        fsEVT_STRUCTURAL_MATERIAL_SELECTED,
        [this](fsStructuralMaterialSelectedEvent & event)
        {
            mOnStructuralLayerMaterialSelected(event);
        });

    mElectricalMaterialPalette = std::make_unique<MaterialPalette<LayerType::Electrical>>(
        parent,
        materialDatabase.GetElectricalMaterialPalette(),
        shipTexturizer,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.33f, 0.33f));

    mElectricalMaterialPalette->Bind(
        fsEVT_ELECTRICAL_MATERIAL_SELECTED,
        [this](fsElectricalMaterialSelectedEvent & event)
        {
            mOnElectricalLayerMaterialSelected(event);
        });

    mRopesMaterialPalette = std::make_unique<MaterialPalette<LayerType::Ropes>>(
        parent,
        materialDatabase.GetRopeMaterialPalette(),
        shipTexturizer,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.66f, 0.33f));

    mRopesMaterialPalette->Bind(
        fsEVT_STRUCTURAL_MATERIAL_SELECTED,
        [this](fsStructuralMaterialSelectedEvent & event)
        {
            mOnRopeLayerMaterialSelected(event);
        });

    progressCallback(1.0f, ProgressMessageType::LoadingMaterialPalette);
}

bool CompositeMaterialPalette::IsOpen() const
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

}