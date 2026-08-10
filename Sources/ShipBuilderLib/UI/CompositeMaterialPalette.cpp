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
        materialDatabase.GetStructuralMaterialPalette(),
        shipTexturizer,
        soundController,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.0f, 0.33f));

    mStructuralMaterialPaletteBrowser->Bind(
        fsEVT_STRUCTURAL_MATERIAL_SELECTED,
        [this](fsStructuralMaterialSelectedEvent & event)
        {
            mOnStructuralLayerMaterialSelected(event);
        });

    mElectricalMaterialPaletteBrowser = std::make_unique<MaterialPaletteBrowser<LayerType::Electrical>>(
        parent,
        materialDatabase.GetElectricalMaterialPalette(),
        shipTexturizer,
        soundController,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.33f, 0.33f));

    mElectricalMaterialPaletteBrowser->Bind(
        fsEVT_ELECTRICAL_MATERIAL_SELECTED,
        [this](fsElectricalMaterialSelectedEvent & event)
        {
            mOnElectricalLayerMaterialSelected(event);
        });

    mRopesMaterialPaletteBrowser = std::make_unique<MaterialPaletteBrowser<LayerType::Ropes>>(
        parent,
        materialDatabase.GetRopeMaterialPalette(),
        shipTexturizer,
        soundController,
        gameAssetManager,
        progressCallback.MakeSubCallback(0.66f, 0.33f));

    mRopesMaterialPaletteBrowser->Bind(
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