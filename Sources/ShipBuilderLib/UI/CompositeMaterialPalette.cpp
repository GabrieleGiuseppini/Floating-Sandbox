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
        [&progressCallback](float progress, ProgressMessageType message)
        {
            // 0.0 -> 0.33
            progressCallback(progress * 0.3333f, message);
        });

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
        [&progressCallback](float progress, ProgressMessageType message)
        {
            // 0.33 -> 0.66
            progressCallback(0.3333f + progress * 0.3333f, message);
        });

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
        [&progressCallback](float progress, ProgressMessageType message)
        {
            // 0.66 -> 1.0
            progressCallback(0.6666f + progress * 0.3333f, message);
        });

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