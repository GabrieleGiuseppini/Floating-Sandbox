/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-23
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ResizeDialog.h"

#include "../WorkbenchState.h"

#include <UILib/WxHelpers.h>

#include <wx/gbsizer.h>
#include <wx/statbmp.h>

#include <cassert>

namespace ShipBuilder {

ResizeDialog::ResizeDialog(
    wxWindow * parent,
    ResourceLocator const & resourceLocator)
    : mResourceLocator(resourceLocator)
    , mSourceSize(0, 0)
{
    Create(
        parent,
        wxID_ANY,
        wxEmptyString,
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED | wxSTAY_ON_TOP);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    dialogVSizer->AddSpacer(20);

    // Top ribbon
    {
        wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

        // Size boxes
        {
            int constexpr TextCtrlWidth = 60;

            wxGridBagSizer * sizer = new wxGridBagSizer(5, 5);

            // Old size
            {
                // Label
                {
                    auto label = new wxStaticText(this, wxID_ANY, _("Original Size"));

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
                    auto label = new wxStaticText(this, wxID_ANY, _("New Size"));

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
                        WorkbenchState::GetMaxShipDimension(),
                        0, // Temporary
                        wxEmptyString,
                        [this](int value)
                        {
                            if (mTargetSizeDimensionLockButton->GetValue())
                            {
                                // Calculate height when preserving source aspect ratio
                                int const newHeight = std::max(
                                    static_cast<int>(std::round(static_cast<float>(value * mSourceSize.height) / static_cast<float>(mSourceSize.width))),
                                    1);
                                mTargetHeightSpinBox->SetValue(newHeight);
                            }

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
                        WorkbenchState::GetMaxShipDimension(),
                        0, // Temporary
                        wxEmptyString,
                        [this](int value)
                        {
                            if (mTargetSizeDimensionLockButton->GetValue())
                            {
                                // Calculate width when preserving source aspect ratio
                                int const newWidth = std::max(
                                    static_cast<int>(std::round(static_cast<float>(value * mSourceSize.width) / static_cast<float>(mSourceSize.height))),
                                    1);
                                mTargetWidthSpinBox->SetValue(newWidth);
                            }

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
                    [this](bool isChecked)
                    {
                        if (isChecked)
                        {
                            // Calculate height when preserving source aspect ratio
                            int const newHeight = std::max(
                                static_cast<int>(std::round(static_cast<float>(mTargetWidthSpinBox->GetValue() * mSourceSize.height) / static_cast<float>(mSourceSize.width))),
                                1);
                            mTargetHeightSpinBox->SetValue(newHeight);

                            // Tell viz controller
                            mShipResizeVisualizationControl->SetTargetSize(
                                IntegralRectSize(
                                    mTargetWidthSpinBox->GetValue(),
                                    newHeight));
                        }
                    });

                mTargetSizeDimensionLockButton->SetValue(true);

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
            int constexpr BaseButtonSize = 20;

            wxGridBagSizer * sizer = new wxGridBagSizer(2, 2);

            for (int y = 0; y < 3; ++y)
            {
                for (int x = 0; x < 3; ++x)
                {
                    IntegralCoordinates anchorCoordinates(x, y);

                    auto const buttonSize = wxSize(
                        x == 1 ? 2 * BaseButtonSize : BaseButtonSize,
                        y == 1 ? 2 * BaseButtonSize : BaseButtonSize);

                    auto button = new wxToggleButton(this, wxID_ANY, wxEmptyString, wxDefaultPosition, buttonSize);

                    button->Bind(
                        wxEVT_TOGGLEBUTTON,
                        [this, button, anchorCoordinates](wxCommandEvent &)
                        {
                            // Tell control
                            mShipResizeVisualizationControl->SetAnchor(anchorCoordinates);

                            // Reconciliate UI
                            ReconciliateUIWithAnchorCoordinates(anchorCoordinates);
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
            wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT,
            10);
    }

    dialogVSizer->AddSpacer(20);

    // Visualization
    {
        mShipResizeVisualizationControl = new ShipResizeVisualizationControl(
            this,
            400,
            200,
            [this]()
            {
                ReconciliateUIWithAnchorCoordinates(std::nullopt);
            });

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

bool ResizeDialog::ShowModalForResize(
    RgbaImageData const & image,
    IntegralRectSize const & targetSize)
{
    mSourceSize = IntegralRectSize(image.Size.width, image.Size.height);

    ReconciliateUI(image, targetSize, ModeType::ForResize);

    return wxDialog::ShowModal() == wxID_OK;
}

bool ResizeDialog::ShowModalForTexture(
    RgbaImageData const & image,
    IntegralRectSize const & targetSize)
{
    mSourceSize = IntegralRectSize(image.Size.width, image.Size.height);

    ReconciliateUI(image, targetSize, ModeType::ForTexture);

    return wxDialog::ShowModal() == wxID_OK;
}

IntegralRectSize ResizeDialog::GetTargetSize() const
{
    return IntegralRectSize(
        mTargetWidthSpinBox->GetValue(),
        mTargetHeightSpinBox->GetValue());
}

IntegralCoordinates ResizeDialog::GetOffset() const
{
    auto const topLeftOffset = mShipResizeVisualizationControl->GetOffset();
    auto const targetSize = GetTargetSize();

    return IntegralCoordinates(
        topLeftOffset.x,
        targetSize.height - (topLeftOffset.y + mSourceSize.height));
}

void ResizeDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    mShipResizeVisualizationControl->Deinitialize();
    EndModal(wxID_OK);
}

void ResizeDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    mShipResizeVisualizationControl->Deinitialize();
    EndModal(wxID_CANCEL);
}

void ResizeDialog::ReconciliateUIWithAnchorCoordinates(std::optional<IntegralCoordinates> const & anchorCoordinates)
{
    // Reconciliate toggle state
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 3; ++x)
        {
            bool const isSelected = (anchorCoordinates.has_value() && y == anchorCoordinates->y && x == anchorCoordinates->x);
            if (mAnchorButtons[y * 3 + x]->GetValue() != isSelected)
            {
                mAnchorButtons[y * 3 + x]->SetValue(isSelected);
            }
        }
    }
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

    // Source size
    mSourceWidthTextCtrl->SetValue(std::to_string(image.Size.width));
    mSourceHeightTextCtrl->SetValue(std::to_string(image.Size.height));

    // Target size
    mTargetWidthSpinBox->SetValue(targetSize.width);
    mTargetWidthSpinBox->Enable(mode == ModeType::ForResize);
    mTargetHeightSpinBox->SetValue(targetSize.height);
    mTargetHeightSpinBox->Enable(mode == ModeType::ForResize);
    mTargetSizeDimensionLockButton->Enable(mode == ModeType::ForResize);

    // Anchor - centered
    IntegralCoordinates centerAnchorCoordinates(1, 1);
    ReconciliateUIWithAnchorCoordinates(centerAnchorCoordinates);

    // Viz control
    mShipResizeVisualizationControl->Initialize(
        image,
        targetSize,
        centerAnchorCoordinates); // Anchor - centered
}

}