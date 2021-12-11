/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-23
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ResizeDialog.h"

#include <UILib/WxHelpers.h>

#include <cassert>

namespace ShipBuilder {

ResizeDialog::ResizeDialog(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
    : mResourceLocator(resourceLocator)
{
    Create(
        parent,
        wxID_ANY,
        wxEmptyString,
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    dialogVSizer->AddSpacer(20);

    // Size boxes
    {
        int constexpr TextCtrlWidth = 60;
        int const MaxDimension = 10000;

        wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

        // Orig size
        {
            {
                mSourceWidthTextCtrl = new wxTextCtrl(
                    this,
                    wxID_ANY,
                    wxEmptyString,
                    wxDefaultPosition,
                    wxSize(TextCtrlWidth, -1),
                    wxTE_CENTRE);

                mSourceWidthTextCtrl->Enable(false);

                hSizer->Add(mSourceWidthTextCtrl, 0, wxALIGN_CENTER_VERTICAL, 0);
            }

            hSizer->AddSpacer(5);

            {
                mSourceHeightTextCtrl = new wxTextCtrl(
                    this,
                    wxID_ANY,
                    wxEmptyString,
                    wxDefaultPosition,
                    wxSize(TextCtrlWidth, -1),
                    wxTE_CENTRE);

                mSourceHeightTextCtrl->Enable(false);

                hSizer->Add(mSourceHeightTextCtrl, 0, wxALIGN_CENTER_VERTICAL, 0);
            }
        }

        hSizer->AddSpacer(30);

        // Target size
        {
            {
                mTargetWidthSpinBox = new EditSpinBox<int>(
                    this,
                    TextCtrlWidth,
                    1,
                    MaxDimension,
                    0, // Temporary
                    wxEmptyString,
                    [this](int value)
                    {
                        mShipResizeVisualizationControl->SetTargetSize(
                            IntegralRectSize(
                                value,
                                mTargetHeightSpinBox->GetValue()));
                    });

                hSizer->Add(mTargetWidthSpinBox, 0, wxALIGN_CENTER_VERTICAL, 0);
            }

            {
                mTargetSizeDimensionLockButton = new BitmapToggleButton(
                    this,
                    resourceLocator.GetBitmapFilePath("locked_vertical_small"),
                    [this]()
                    {
                        // TODO
                    });

                hSizer->Add(
                    mTargetSizeDimensionLockButton,
                    0,
                    wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT,
                    4);
            }

            {
                mTargetHeightSpinBox = new EditSpinBox<int>(
                    this,
                    TextCtrlWidth,
                    1,
                    MaxDimension,
                    0, // Temporary
                    wxEmptyString,
                    [this](int value)
                    {
                        mShipResizeVisualizationControl->SetTargetSize(
                            IntegralRectSize(
                                mTargetWidthSpinBox->GetValue(),
                                value));
                    });

                hSizer->Add(mTargetHeightSpinBox, 0, wxALIGN_CENTER_VERTICAL, 0);
            }
        }

        dialogVSizer->Add(
            hSizer,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);
    }

    dialogVSizer->AddSpacer(20);

    // Visualization
    {
        mShipResizeVisualizationControl = new ShipResizeVisualizationControl(this, 400, 200);

        dialogVSizer->Add(
            mShipResizeVisualizationControl, 
            0, 
            wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT,
            10);
    }

    dialogVSizer->AddSpacer(20);

    // Buttons
    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddSpacer(20);

        {
            auto button = new wxButton(this, wxID_ANY, _("OK"));
            button->Bind(wxEVT_BUTTON, &ResizeDialog::OnOkButton, this);
            buttonsSizer->Add(button, 0);
        }

        buttonsSizer->AddSpacer(20);

        {
            auto button = new wxButton(this, wxID_ANY, _("Cancel"));
            button->Bind(wxEVT_BUTTON, &ResizeDialog::OnCancelButton, this);
            buttonsSizer->Add(button, 0);
        }

        buttonsSizer->AddSpacer(20);

        dialogVSizer->Add(buttonsSizer, 0, wxALIGN_CENTER_HORIZONTAL);
    }

    dialogVSizer->AddSpacer(20);

    //
    // Finalize dialog
    //

    SetSizerAndFit(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

int ResizeDialog::ShowModalForResize(
    RgbaImageData const & image,
    IntegralRectSize const & targetSize)
{
    // TODOTEST
    //mSessionData.emplace(ModeType::ForResize, targetSize);

    ReconciliateUI(image, targetSize, ModeType::ForResize);

    return wxDialog::ShowModal();
}

int ResizeDialog::ShowModalForTexture(
    RgbaImageData const & image,
    IntegralRectSize const & targetSize)
{
    // TODOTEST
    //mSessionData.emplace(ModeType::ForTexture, targetSize);

    ReconciliateUI(image, targetSize, ModeType::ForTexture);

    return wxDialog::ShowModal();
}

void ResizeDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    // TODO: copy result from control
    // TODO: expose result
    // TODO: or not, getter on dialog could call getter on control

    // TODOTEST
    //mSessionData.reset();
    mShipResizeVisualizationControl->Deinitialize();
    EndModal(wxID_OK);
}

void ResizeDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    // TODOTEST
    //mSessionData.reset();
    mShipResizeVisualizationControl->Deinitialize();
    EndModal(wxID_CANCEL);
}

void ResizeDialog::ReconciliateUI(
    RgbaImageData const & image,
    IntegralRectSize const & targetSize,
    ModeType mode)
{
    // Title
    switch (mode)
    {
        case ModeType::ForResize:
        {
            SetTitle(_("Resize Ship"));
            break;
        }

        case ModeType::ForTexture:
        {
            SetTitle(_("Center Texture"));
            break;
        }
    }

    // TODOTEST
    //assert(mSessionData);

    // Source size
    mSourceWidthTextCtrl->SetValue(std::to_string(image.Size.width));
    mSourceHeightTextCtrl->SetValue(std::to_string(image.Size.height));

    // Target size
    mTargetWidthSpinBox->SetValue(targetSize.width);
    mTargetWidthSpinBox->Enable(mode == ModeType::ForResize);
    mTargetHeightSpinBox->SetValue(targetSize.height);
    mTargetHeightSpinBox->Enable(mode == ModeType::ForResize);
    mTargetSizeDimensionLockButton->Enable(mode == ModeType::ForResize);

    // Viz control
    mShipResizeVisualizationControl->Initialize(
        image,
        targetSize,
        IntegralCoordinates(0, 0)); // tODO: using calc function
}

}