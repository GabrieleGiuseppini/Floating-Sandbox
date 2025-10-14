/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-06-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "PreferencesDialog.h"

#include <UILib/WxHelpers.h>

#include <wx/button.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>

#include <array>

namespace ShipBuilder {

PreferencesDialog::PreferencesDialog(
    wxWindow * parent,
    GameAssetManager const & gameAssetManager)
{
    int constexpr Margin = 20;

    Create(
        parent,
        wxID_ANY,
        _("Preferences"),
        wxDefaultPosition,
        wxSize(880, 700), // We fit anyway
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED);

    Bind(wxEVT_CLOSE_WINDOW, &PreferencesDialog::OnCloseWindow, this);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    dialogVSizer->AddSpacer(Margin);

    // Controls
    {
        wxGridBagSizer * sizer = new wxGridBagSizer(5, 10);

        int constexpr SeparatorColumnWidth = 10;

        // New ship size
        {
            // Label
            {
                auto label = new wxStaticText(this, wxID_ANY, _("New Ship Size:"));

                sizer->Add(
                    label,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);
            }

            {
                sizer->Add(SeparatorColumnWidth, 0, wxGBPosition(0, 1), wxGBSpan(1, 1));
            }

            // Width icon
            {
                auto icon = new wxStaticBitmap(this, wxID_ANY, WxHelpers::LoadBitmap("width_icon_small", gameAssetManager));

                sizer->Add(
                    icon,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
            }

            // Width
            {
                mNewShipSizeWidthSpinBox = new EditSpinBox<int>(
                    this,
                    60,
                    1,
                    WorkbenchState::GetMaxShipDimension(),
                    0, // Temporary
                    _("The width of the canvas."),
                    [this](int)
                    {
                        // No need to react
                    });

                sizer->Add(
                    mNewShipSizeWidthSpinBox,
                    wxGBPosition(0, 3),
                    wxGBSpan(1, 1),
                    wxALIGN_CENTER_VERTICAL);
            }

            // Height icon
            {
                auto icon = new wxStaticBitmap(this, wxID_ANY, WxHelpers::LoadBitmap("height_icon_small", gameAssetManager));

                sizer->Add(
                    icon,
                    wxGBPosition(0, 4),
                    wxGBSpan(1, 1),
                    wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
            }

            // Height
            {
                mNewShipSizeHeightSpinBox = new EditSpinBox<int>(
                    this,
                    60,
                    1,
                    WorkbenchState::GetMaxShipDimension(),
                    0, // Temporary
                    _("The height of the canvas."),
                    [this](int)
                    {
                        // No need to react
                    });

                sizer->Add(
                    mNewShipSizeHeightSpinBox,
                    wxGBPosition(0, 5),
                    wxGBSpan(1, 1),
                    wxALIGN_CENTER_VERTICAL);
            }
        }

        // Texture alignment optimizations
        {
            // Label
            {
                auto label = new wxStaticText(this, wxID_ANY, _("Optimize exterior layer texture on import:"));

                sizer->Add(
                    label,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);
            }

            {
                sizer->Add(SeparatorColumnWidth, 0, wxGBPosition(1, 1), wxGBSpan(1, 1));
            }

            // Checkbox
            {
                mDoTextureAlignmentOptimizationCheckBox = new wxCheckBox(this, wxID_ANY, wxEmptyString);

                mDoTextureAlignmentOptimizationCheckBox->SetToolTip(_("When enabled, textures imported into the exterior layer are slightly modified to optimize coverage by the structure layer."));

                mDoTextureAlignmentOptimizationCheckBox->Bind(
                    wxEVT_CHECKBOX,
                    [this](wxCommandEvent &)
                    {
                        // No need to react
                    });

                sizer->Add(
                    mDoTextureAlignmentOptimizationCheckBox,
                    wxGBPosition(1, 2),
                    wxGBSpan(1, 1),
                    wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);
            }
        }

        // Canvas BG color
        {
            // Label
            {
                auto label = new wxStaticText(this, wxID_ANY, _("Canvas Background Color:"));

                sizer->Add(
                    label,
                    wxGBPosition(2, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);
            }

            {
                sizer->Add(SeparatorColumnWidth, 0, wxGBPosition(2, 1), wxGBSpan(1, 1));
            }

            // Color picker
            {
                mCanvasBackgroundColorColourPicker = new wxColourPickerCtrl(this, wxID_ANY, wxColour("WHITE"), wxDefaultPosition, wxDefaultSize);

                mCanvasBackgroundColorColourPicker->SetToolTip(_("Sets the color of the canvas background."));

                mCanvasBackgroundColorColourPicker->Bind(
                    wxEVT_COLOURPICKER_CHANGED,
                    [this](wxColourPickerEvent & event)
                    {
                        auto color = event.GetColour();

                        OnCanvasBackgroundColorSelected(rgbColor(color.Red(), color.Green(), color.Blue()));

                        // Change combo to unselected
                        mPresetColorsComboBox->SetSelection(wxNOT_FOUND);
                    });

                sizer->Add(
                    mCanvasBackgroundColorColourPicker,
                    wxGBPosition(2, 2),
                    wxGBSpan(1, 2),
                    wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
            }

            // Label
            {
                auto label = new wxStaticText(this, wxID_ANY, _("Preset Colors:"));

                sizer->Add(
                    label,
                    wxGBPosition(2, 4),
                    wxGBSpan(1, 2),
                    wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
            }

            // Preset colors
            {
                mPresetColorsComboBox = new wxBitmapComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(160, -1), wxArrayString(), wxCB_READONLY);

                mPresetColorsComboBox->SetToolTip(_("Sets a predefined color for the canvas background."));

                // Populate

                static std::array<std::tuple<rgbColor, wxString>, 4> const PredefinedColors = {
                    std::make_tuple(rgbColor(255, 255, 255), _("Default")),
                    std::make_tuple(rgbColor(148, 216, 255) , _("Blue Sky")),
                    std::make_tuple(rgbColor(4, 244, 4) , _("Green Screen")),
                    std::make_tuple(rgbColor(244, 4, 4) , _("Red Screen"))
                };

                for (auto const & entry : PredefinedColors)
                {
                    mPresetColorsComboBox->Append(std::get<1>(entry), WxHelpers::MakeMatteBitmap(rgbaColor(std::get<0>(entry), 255), rgbaColor(0, 0, 0, 255), ImageSize(40, 20)));
                }

                mPresetColorsComboBox->Bind(
                    wxEVT_COMBOBOX,
                    [this](wxCommandEvent & /*event*/)
                    {
                        auto const selection = mPresetColorsComboBox->GetSelection();
                        if (selection != wxNOT_FOUND)
                        {
                            auto const bgColor = std::get<0>(PredefinedColors[static_cast<size_t>(selection)]);

                            OnCanvasBackgroundColorSelected(bgColor);

                            mCanvasBackgroundColorColourPicker->SetColour(wxColour(bgColor.r, bgColor.g, bgColor.b));
                        }
                    });

                sizer->Add(
                    mPresetColorsComboBox,
                    wxGBPosition(2, 6),
                    wxGBSpan(1, 1),
                    wxALIGN_CENTER_VERTICAL);
            }
        }

        dialogVSizer->Add(
            sizer,
            0,
            wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT,
            Margin);
    }

    dialogVSizer->AddSpacer(Margin);

    // Buttons
    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddSpacer(Margin);

        {
            auto button = new wxButton(this, wxID_ANY, _("OK"));
            button->Bind(wxEVT_BUTTON, &PreferencesDialog::OnOkButton, this);
            buttonsSizer->Add(button, 0);
        }

        buttonsSizer->AddSpacer(Margin);

        {
            auto button = new wxButton(this, wxID_ANY, _("Cancel"));
            button->Bind(wxEVT_BUTTON, &PreferencesDialog::OnCancelButton, this);
            buttonsSizer->Add(button, 0);
        }

        buttonsSizer->AddSpacer(Margin);

        dialogVSizer->Add(
            buttonsSizer,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);
    }

    dialogVSizer->AddSpacer(Margin);

    //
    // Finalize dialog
    //

    SetSizerAndFit(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

void PreferencesDialog::ShowModal(
    Controller & controller,
    WorkbenchState const & workbenchState)
{
    //
    // Create session
    //

    mSessionData.emplace(
        controller,
        workbenchState.GetCanvasBackgroundColor());

    ReconciliateUI(workbenchState);

    wxDialog::ShowModal();
}

void PreferencesDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    //
    // Store values
    //

    assert(mSessionData.has_value());

    mSessionData->BuilderController.SetNewShipSize(
        ShipSpaceSize(
            mNewShipSizeWidthSpinBox->GetValue(),
            mNewShipSizeHeightSpinBox->GetValue()));

    mSessionData->BuilderController.SetDoTextureAlignmentOptimization(mDoTextureAlignmentOptimizationCheckBox->GetValue());

    // No need to store canvas BG color, as we set it on-the-fly

    //
    // Close dialog
    //

    mSessionData.reset();

    EndModal(0);
}

void PreferencesDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    OnCancel();
}

void PreferencesDialog::OnCloseWindow(wxCloseEvent & event)
{
    OnCancel();

    event.Skip();
}

void PreferencesDialog::OnCancel()
{
    //
    // Revert background color to the one saved in session
    //

    assert(mSessionData.has_value());

    mSessionData->BuilderController.SetCanvasBackgroundColor(mSessionData->OriginalCanvasBackgroundColor);

    //
    // Close dialog
    //

    mSessionData.reset();

    EndModal(-1);
}

void PreferencesDialog::OnCanvasBackgroundColorSelected(rgbColor const & color)
{
    assert(mSessionData.has_value());

    mSessionData->BuilderController.SetCanvasBackgroundColor(color);
}

void PreferencesDialog::ReconciliateUI(WorkbenchState const & workbenchState)
{
    assert(mSessionData);

    // New ship size
    mNewShipSizeWidthSpinBox->SetValue(workbenchState.GetNewShipSize().width);
    mNewShipSizeHeightSpinBox->SetValue(workbenchState.GetNewShipSize().height);

    // Texture alignment optimization
    mDoTextureAlignmentOptimizationCheckBox->SetValue(workbenchState.GetDoTextureAlignmentOptimization());

    // Canvas background color
    auto const bgColor = workbenchState.GetCanvasBackgroundColor();
    mCanvasBackgroundColorColourPicker->SetColour(wxColour(bgColor.r, bgColor.g, bgColor.b));
    mPresetColorsComboBox->SetSelection(wxNOT_FOUND);
}

}