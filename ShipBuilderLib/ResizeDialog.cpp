/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-23
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ResizeDialog.h"

#include <UILib/WxHelpers.h>

#include <wx/gbsizer.h>
#include <wx/statbmp.h>

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

    // Top ribbon
    {
        wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

        // Size boxes
        {
            int constexpr TextCtrlWidth = 60;
            int const MaxDimension = 10000;

            wxGridBagSizer * sizer = new wxGridBagSizer(5, 5);

            // Old size
            {
                // Label
                {
                    auto label = new wxStaticText(this, wxID_ANY, _T("Original Size"));

                    sizer->Add(
                        label,
                        wxGBPosition(0, 0),
                        wxGBSpan(1, 2),
                        wxALIGN_CENTER_HORIZONTAL);
                }

                // Width icon
                {
                    auto icon = new wxStaticBitmap(this, wxID_ANY, WxHelpers::LoadBitmap("width_icon_small", resourceLocator));

                    sizer->Add(
                        icon,
                        wxGBPosition(1, 0),
                        wxGBSpan(1, 1),
                        wxALIGN_CENTER_VERTICAL);
                }

                // Width
                {
                    mSourceWidthTextCtrl = new wxTextCtrl(
                        this,
                        wxID_ANY,
                        wxEmptyString,
                        wxDefaultPosition,
                        wxSize(TextCtrlWidth, -1),
                        wxTE_CENTRE);

                    mSourceWidthTextCtrl->Enable(false);

                    sizer->Add(
                        mSourceWidthTextCtrl,
                        wxGBPosition(1, 1),
                        wxGBSpan(1, 1),
                        0);
                }

                // Height icon
                {
                    auto icon = new wxStaticBitmap(this, wxID_ANY, WxHelpers::LoadBitmap("height_icon_small", resourceLocator));

                    sizer->Add(
                        icon,
                        wxGBPosition(2, 0),
                        wxGBSpan(1, 1),
                        wxALIGN_CENTER_VERTICAL);
                }

                // Height
                {
                    mSourceHeightTextCtrl = new wxTextCtrl(
                        this,
                        wxID_ANY,
                        wxEmptyString,
                        wxDefaultPosition,
                        wxSize(TextCtrlWidth, -1),
                        wxTE_CENTRE);

                    mSourceHeightTextCtrl->Enable(false);

                    sizer->Add(
                        mSourceHeightTextCtrl,
                        wxGBPosition(2, 1),
                        wxGBSpan(1, 1),
                        0);
                }
            }

            // Vertical spacer
            {
                sizer->Add(
                    18,
                    1, // Pointless
                    wxGBPosition(0, 2),
                    wxGBSpan(3, 1));
            }

            // New size
            {
                // Label
                {
                    auto label = new wxStaticText(this, wxID_ANY, _T("New Size"));

                    sizer->Add(
                        label,
                        wxGBPosition(0, 3),
                        wxGBSpan(1, 2),
                        wxALIGN_CENTER_HORIZONTAL);
                }

                // Width icon
                {
                    auto icon = new wxStaticBitmap(this, wxID_ANY, WxHelpers::LoadBitmap("width_icon_small", resourceLocator));

                    sizer->Add(
                        icon,
                        wxGBPosition(1, 3),
                        wxGBSpan(1, 1),
                        wxALIGN_CENTER_VERTICAL);
                }

                // Width
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

                    sizer->Add(
                        mTargetWidthSpinBox,
                        wxGBPosition(1, 4),
                        wxGBSpan(1, 1),
                        0);
                }

                // Height icon
                {
                    auto icon = new wxStaticBitmap(this, wxID_ANY, WxHelpers::LoadBitmap("height_icon_small", resourceLocator));

                    sizer->Add(
                        icon,
                        wxGBPosition(2, 3),
                        wxGBSpan(1, 1),
                        wxALIGN_CENTER_VERTICAL);
                }

                // Height
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

                    sizer->Add(
                        mTargetHeightSpinBox,
                        wxGBPosition(2, 4),
                        wxGBSpan(1, 1),
                        0);
                }
            }

            // Lock button
            {
                mTargetSizeDimensionLockButton = new BitmapToggleButton(
                    this,
                    resourceLocator.GetBitmapFilePath("locked_vertical_small"),
                    [this]()
                    {
                        // TODO
                    });

                sizer->Add(
                    mTargetSizeDimensionLockButton,
                    wxGBPosition(1, 5),
                    wxGBSpan(2, 1),
                    wxALIGN_CENTER_VERTICAL);
            }

            hSizer->Add(
                sizer,
                1,
                wxALIGN_CENTER_VERTICAL,
                0);
        }

        hSizer->AddSpacer(40);

        // Anchor controls
        {
            wxGridBagSizer * sizer = new wxGridBagSizer(2, 2);

            for (int y = 0; y < 3; ++y)
            {
                for (int x = 0; x < 3; ++x)
                {
                    auto button = new wxToggleButton(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(30, 30));

                    button->Bind(
                        wxEVT_BUTTON,
                        [this, x, y](wxCommandEvent &)
                        {
                            // TODOHERE
                            // TODO: invoked need also to untoggle others
                        });

                    sizer->Add(
                        button,
                        wxGBPosition(y, x),
                        wxGBSpan(1, 1),
                        0);

                    mAnchorButtons[y * 3 + x] = button;
                }
            }

            hSizer->Add(
                sizer,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);
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
        IntegralCoordinates(0, 0)); // tODO: using calc function for center
}

}