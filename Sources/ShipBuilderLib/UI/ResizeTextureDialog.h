/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-01-07
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "BaseResizeDialog.h"
#include "ShipTextureResizeVisualizationControl.h"

#include <Core/GameTypes.h>

#include <wx/checkbox.h>
#include <wx/dialog.h>

#include <memory>

namespace ShipBuilder {

class ResizeTextureDialog final : public BaseResizeDialog
{
public:

    static std::unique_ptr<ResizeTextureDialog> Create(
        wxWindow * parent,
        GameAssetManager const & gameAssetManager)
    {
        auto dlg = std::unique_ptr<ResizeTextureDialog>(new ResizeTextureDialog());
        dlg->CreateLayout(
            parent,
            _("Resize Texture"),
            gameAssetManager);

        return dlg;
    }

    bool GetDoMaintainAspectRatio() const
    {
        return mMaintainAspectRatioCheckBox->IsChecked();
    }

    bool GetDoOptimizeForStructure() const
    {
        return mOptimizeForStructureCheckBox->IsChecked();
    }

private:

    void InternalCreateLayout(
        wxBoxSizer * dialogVSizer,
        GameAssetManager const & gameAssetManager) override;

    void InternalReconciliateUI(
        RgbaImageData const & image,
        ShipSpaceSize const & shipSize) override;

    void InternalOnClose() override;

private:

    ResizeTextureDialog()
        : BaseResizeDialog()
        , mShipSize(0, 0) // Will be set later
    {
    }

    wxCheckBox * mMaintainAspectRatioCheckBox;
    wxCheckBox * mOptimizeForStructureCheckBox;
    ShipTextureResizeVisualizationControl * mShipTextureResizeVisualizationControl;

    ShipSpaceSize mShipSize;
};

}