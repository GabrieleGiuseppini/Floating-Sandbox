/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2026-01-07
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ResizeTextureDialog.h"

#include "../WorkbenchState.h"

#include <UILib/WxHelpers.h>

#include <wx/gbsizer.h>
#include <wx/statbmp.h>

#include <cassert>

namespace ShipBuilder {

void ResizeTextureDialog::InternalCreateLayout(
    wxBoxSizer * dialogVSizer,
    GameAssetManager const & /*gameAssetManager*/)
{
    // Checkboxes
    {
        {
            mMaintainAspectRatioCheckBox = new wxCheckBox(this, wxID_ANY, _("Maintain aspect ratio"));
            mMaintainAspectRatioCheckBox->SetToolTip(_("Maintains the original aspect ratio of the texture, filling-in extra space with transparent data."));

            mMaintainAspectRatioCheckBox->Bind(
                wxEVT_CHECKBOX,
                [this](wxCommandEvent & event)
                {
                    // Tell control
                    mShipTextureResizeVisualizationControl->SetDoMaintainAspectRatio(event.IsChecked());
                });

            dialogVSizer->Add(
                mMaintainAspectRatioCheckBox,
                0,
                wxALIGN_LEFT | wxLEFT,
                10);
        }

        {
            mOptimizeForStructureCheckBox = new wxCheckBox(this, wxID_ANY, _("Optimize for structure"));
            mOptimizeForStructureCheckBox->SetToolTip(_("Slightly stretch texture to optimize coverage by the structure layer."));

            // We want it on by default
            mOptimizeForStructureCheckBox->SetValue(true);

            dialogVSizer->Add(
                mOptimizeForStructureCheckBox,
                0,
                wxALIGN_LEFT | wxLEFT,
                10);
        }
    }

    dialogVSizer->AddSpacer(20);

    // Visualization
    {
        mShipTextureResizeVisualizationControl = new ShipTextureResizeVisualizationControl(
            this,
            400,
            200);

        dialogVSizer->Add(
            mShipTextureResizeVisualizationControl,
            0,
            wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT,
            10);
    }
}

void ResizeTextureDialog::InternalReconciliateUI(
    RgbaImageData const & image,
    ShipSpaceSize const & shipSize)
{
    mShipSize = shipSize;

    // We always want to maintain aspect ratio
    bool const doMaintainAspectRatio = true;
    mMaintainAspectRatioCheckBox->SetValue(doMaintainAspectRatio);

    // Enable aspect ratio checkbox only if needed
    auto const fittedSize = image.Size.ResizeToAspectRatioOf(shipSize);
    mMaintainAspectRatioCheckBox->Enable(fittedSize != image.Size);

    // Viz control
    mShipTextureResizeVisualizationControl->Initialize(
        image,
        shipSize,
        doMaintainAspectRatio);
}

void ResizeTextureDialog::InternalOnClose()
{
    mShipTextureResizeVisualizationControl->Deinitialize();
}

}