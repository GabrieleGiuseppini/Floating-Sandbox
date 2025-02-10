/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-20
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ShipPropertiesEditDialog.h"

#include "NewPasswordDialog.h"
#include "ShipNameSuggestionDialog.h"

#include <UILib/WxHelpers.h>

#include <Game/GameVersion.h>

#include <Simulation/SimulationParameters.h>
#include <Simulation/ShipDefinitionFormatDeSerializer.h>

#include <Core/ExponentialSliderCore.h>
#include <Core/LinearSliderCore.h>

#include <wx/gbsizer.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/utils.h>

#include <cassert>

namespace ShipBuilder {

int const PanelInternalMargin = 20;
int const VerticalSeparatorSize = 20;
int const NumericEditBoxWidth = 100;

ShipPropertiesEditDialog::ShipPropertiesEditDialog(
    wxWindow * parent,
    ShipNameNormalizer const & shipNameNormalizer,
    GameAssetManager const & gameAssetManager)
    : mParent(parent)
    , mShipNameNormalizer(shipNameNormalizer)
    , mGameAssetManager(gameAssetManager)
{
    Create(
        parent,
        wxID_ANY,
        _("Ship Properties"),
        wxDefaultPosition,
        wxSize(600, 200),
        wxCAPTION | wxCLOSE_BOX | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    wxNotebook * notebook = new wxNotebook(
        this,
        wxID_ANY,
        wxPoint(-1, -1),
        wxSize(-1, -1),
        wxNB_TOP);

    {
        auto panel = new wxPanel(notebook);

        PopulateMetadataPanel(panel);

        notebook->AddPage(panel, _("Metadata"));
    }

    {
        auto panel = new wxPanel(notebook);

        PopulateDescriptionPanel(panel);

        notebook->AddPage(panel, _("Description"));
    }

    {
        auto panel = new wxPanel(notebook);

        PopulatePhysicsDataPanel(panel);

        notebook->AddPage(panel, _("Physics"));
    }

    {
        mAutoTexturizationPanel = new wxPanel(notebook);

        PopulateAutoTexturizationPanel(mAutoTexturizationPanel);

        notebook->AddPage(mAutoTexturizationPanel, _("Auto-Texturization"));
    }

    {
        auto panel = new wxPanel(notebook);

        PopulatePasswordProtectionPanel(panel);

        notebook->AddPage(panel, _("Password Protection"));
    }

    dialogVSizer->Add(notebook, 1, wxEXPAND);

    dialogVSizer->AddSpacer(20);

    // Buttons
    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddSpacer(20);

        {
            mOkButton = new wxButton(this, wxID_ANY, _("OK"));
            mOkButton->Bind(wxEVT_BUTTON, &ShipPropertiesEditDialog::OnOkButton, this);
            buttonsSizer->Add(mOkButton, 0);
        }

        buttonsSizer->AddSpacer(20);

        {
            auto button = new wxButton(this, wxID_ANY, _("Cancel"));
            button->Bind(wxEVT_BUTTON, &ShipPropertiesEditDialog::OnCancelButton, this);
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

void ShipPropertiesEditDialog::ShowModal(
    Controller & controller,
    ShipMetadata const & shipMetadata,
    ShipPhysicsData const & shipPhysicsData,
    std::optional<ShipAutoTexturizationSettings> const & shipAutoTexturizationSettings,
    RgbaImageData const & shipVisualization,
    bool hasTexture)
{
    mSessionData.emplace(
        controller,
        shipMetadata,
        shipPhysicsData,
        shipAutoTexturizationSettings,
        shipVisualization,
        hasTexture);

    ReconciliateUI();

    wxDialog::ShowModal();
}

void ShipPropertiesEditDialog::PopulateMetadataPanel(wxPanel * panel)
{
    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    auto explanationFont = panel->GetFont();
    explanationFont.SetPointSize(explanationFont.GetPointSize() - 2);
    explanationFont.SetStyle(wxFontStyle::wxFONTSTYLE_ITALIC);

    // Ship name
    {
        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Ship Name"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            vSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }

        {
            mShipNameTextCtrl = new wxTextCtrl(
                panel,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                wxSize(350, -1),
                wxTE_CENTRE | wxTE_PROCESS_ENTER);

            mShipNameTextCtrl->Bind(
                wxEVT_TEXT,
                [this](wxCommandEvent & event)
                {
                    OnDirty();
                    event.Skip();
                });

            mShipNameTextCtrl->Bind(
                wxEVT_TEXT_ENTER,
                [this](wxCommandEvent &)
                {
                    mShipNameTextCtrl->Navigate();
                });

            auto font = panel->GetFont();
            font.SetPointSize(font.GetPointSize() + 2);
            mShipNameTextCtrl->SetFont(font);

            vSizer->Add(mShipNameTextCtrl, 0, wxALL | wxEXPAND, 0);
        }

        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Name of the ship, e.g. \"R.M.S. Titanic\""), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            label->SetFont(explanationFont);

            vSizer->Add(label, 0, wxALL | wxEXPAND, 0);
        }
    }

    vSizer->AddSpacer(VerticalSeparatorSize);

    // Author(s)
    {
        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Author(s)"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            vSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }

        {
            mShipAuthorTextCtrl = new wxTextCtrl(
                panel,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                wxSize(150, -1),
                wxTE_CENTRE | wxTE_PROCESS_ENTER);

            mShipAuthorTextCtrl->Bind(
                wxEVT_TEXT,
                [this](wxCommandEvent & event)
                {
                    OnDirty();
                    event.Skip();
                });

            mShipAuthorTextCtrl->Bind(
                wxEVT_TEXT_ENTER,
                [this](wxCommandEvent &)
                {
                    mShipAuthorTextCtrl->Navigate();
                });

            vSizer->Add(mShipAuthorTextCtrl, 0, wxALL | wxEXPAND, 0);
        }

        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Author(s), e.g. \"Ellen Ripley; David Gahan\""), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            label->SetFont(explanationFont);

            vSizer->Add(label, 0, wxALL | wxEXPAND, 0);
        }
    }

    vSizer->AddSpacer(VerticalSeparatorSize);

    // Art Credits
    {
        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Texture Credits"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            vSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }

        {
            mArtCreditsTextCtrl = new wxTextCtrl(
                panel,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                wxSize(150, -1),
                wxTE_CENTRE | wxTE_PROCESS_ENTER);

            mArtCreditsTextCtrl->Bind(
                wxEVT_TEXT,
                [this](wxCommandEvent & event)
                {
                    OnDirty();
                    event.Skip();
                });

            mArtCreditsTextCtrl->Bind(
                wxEVT_TEXT_ENTER,
                [this](wxCommandEvent &)
                {
                mArtCreditsTextCtrl->Navigate();
                });

            vSizer->Add(mArtCreditsTextCtrl, 0, wxALL | wxEXPAND, 0);
        }

        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Author(s) of the texture - if different than the ship author, e.g. \"Neurodancer (Shipbucket.com)\""), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            label->SetFont(explanationFont);

            vSizer->Add(label, 0, wxALL | wxEXPAND, 0);
        }
    }

    vSizer->AddSpacer(VerticalSeparatorSize);

    // Year Built
    {
        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Year Built"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            vSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }

        {
            mYearBuiltTextCtrl = new wxTextCtrl(
                panel,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                wxSize(100, -1),
                wxTE_CENTRE | wxTE_PROCESS_ENTER);

            mYearBuiltTextCtrl->Bind(
                wxEVT_TEXT,
                [this](wxCommandEvent & event)
                {
                    OnDirty();
                    event.Skip();
                });

            mYearBuiltTextCtrl->Bind(
                wxEVT_TEXT_ENTER,
                [this](wxCommandEvent &)
                {
                    mYearBuiltTextCtrl->Navigate();
                });

            vSizer->Add(mYearBuiltTextCtrl, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 0);
        }

        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Year in which the ship was built, e.g. \"1911\""), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            label->SetFont(explanationFont);

            vSizer->Add(label, 0, wxALL | wxEXPAND, 0);
        }
    }

    // Finalize
    auto marginSizer = new wxBoxSizer(wxVERTICAL);
    marginSizer->Add(vSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, PanelInternalMargin);
    panel->SetSizer(marginSizer);
}

void ShipPropertiesEditDialog::PopulateDescriptionPanel(wxPanel * panel)
{
    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    auto explanationFont = panel->GetFont();
    explanationFont.SetPointSize(explanationFont.GetPointSize() - 2);
    explanationFont.SetStyle(wxFontStyle::wxFONTSTYLE_ITALIC);

    {
        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Description"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            vSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }

        {
            mDescriptionTextCtrl = new wxTextCtrl(
                panel,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                wxSize(200, 300),
                wxTE_MULTILINE);

            mDescriptionTextCtrl->Bind(
                wxEVT_TEXT,
                [this](wxCommandEvent & event)
                {
                    OnDirty();
                    event.Skip();
                });

            vSizer->Add(mDescriptionTextCtrl, 0, wxALL | wxEXPAND, 0);
        }

        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Description of the ship, as long or short as you want, or empty to leave the ship without a description"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            label->SetFont(explanationFont);

            vSizer->Add(label, 0, wxALL | wxEXPAND, 0);
        }
    }

    // Finalize
    auto marginSizer = new wxBoxSizer(wxVERTICAL);
    marginSizer->Add(vSizer, 0, wxEXPAND | wxALL, PanelInternalMargin);
    panel->SetSizer(marginSizer);
}

void ShipPropertiesEditDialog::PopulatePhysicsDataPanel(wxPanel * panel)
{
    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    auto explanationFont = panel->GetFont();
    explanationFont.SetPointSize(explanationFont.GetPointSize() - 2);
    explanationFont.SetStyle(wxFontStyle::wxFONTSTYLE_ITALIC);

    // Offset
    {
        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Offset"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            vSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }

        {
            wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

            {
                auto label = new wxStaticText(panel, wxID_ANY, "X:", wxDefaultPosition, wxDefaultSize,
                    wxALIGN_RIGHT);

                hSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL, 0);
            }

            hSizer->AddSpacer(3);

            {
                mOffsetXEditSpinBox = new EditSpinBox<float>(
                    panel,
                    NumericEditBoxWidth,
                    -SimulationParameters::HalfMaxWorldWidth,
                    SimulationParameters::HalfMaxWorldWidth,
                    0.0f, // Temporary
                    wxEmptyString,
                    [this](float value)
                    {
                        // Tell viz control
                        mShipOffsetVisualizationControl->SetOffsetX(value);

                        // Tell slider
                        mOffsetXSlider->SetValue(static_cast<int>(value));

                        OnDirty();
                    });

                hSizer->Add(mOffsetXEditSpinBox, 0, wxALIGN_CENTER_VERTICAL, 0);
            }

            hSizer->AddSpacer(10);

            {
                auto label = new wxStaticText(panel, wxID_ANY, "Y:", wxDefaultPosition, wxDefaultSize,
                    wxALIGN_RIGHT);

                hSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL, 0);
            }

            hSizer->AddSpacer(3);

            {
                mOffsetYEditSpinBox = new EditSpinBox<float>(
                    panel,
                    NumericEditBoxWidth,
                    -SimulationParameters::HalfMaxWorldHeight,
                    SimulationParameters::HalfMaxWorldHeight,
                    0.0f, // Temporary
                    wxEmptyString,
                    [this](float value)
                    {
                        // Tell viz control
                        mShipOffsetVisualizationControl->SetOffsetY(value);

                        // Tell slider
                        mOffsetYSlider->SetValue(static_cast<int>(value));

                        OnDirty();
                    });

                hSizer->Add(mOffsetYEditSpinBox, 0, wxALIGN_CENTER_VERTICAL, 0);
            }

            vSizer->Add(hSizer, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }

        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Horizontal and vertical offset of the ship, in meters"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            label->SetFont(explanationFont);

            vSizer->Add(label, 0, wxALL | wxEXPAND, 0);
        }

        vSizer->AddSpacer(10);

        // Viz control and sliders
        {
            int constexpr SliderWidth = 20;

            auto * gSizer = new wxFlexGridSizer(2, 3, 0, 0);

            // Row 1

            {
                gSizer->AddSpacer(SliderWidth);
            }

            {
                mShipOffsetVisualizationControl = new ShipOffsetVisualizationControl(
                    panel,
                    400,
                    250,
                    0.0f,
                    0.0f);

                gSizer->Add(mShipOffsetVisualizationControl, 0, 0, 0);
            }

            {
                mOffsetYSlider = new wxSlider(
                    panel,
                    wxID_ANY,
                    0, // Temporary
                    -200,
                    200,
                    wxDefaultPosition,
                    wxDefaultSize,
                    wxSL_VERTICAL | wxSL_INVERSE);

                mOffsetYSlider->Bind(
                    wxEVT_SLIDER,
                    [this](wxEvent &)
                    {
                        float const value = static_cast<float>(mOffsetYSlider->GetValue());

                        // Tell viz control
                        mShipOffsetVisualizationControl->SetOffsetY(value);

                        // Tell edit box - marking it as dirty
                        mOffsetYEditSpinBox->ChangeValue(value);

                        OnDirty();
                    });

                gSizer->Add(mOffsetYSlider, 0, wxEXPAND, 0);
            }

            // Row 2

            {
                gSizer->AddSpacer(SliderWidth);
            }

            {
                mOffsetXSlider = new wxSlider(
                    panel,
                    wxID_ANY,
                    0, // Temporary
                    -200,
                    200,
                    wxDefaultPosition,
                    wxDefaultSize,
                    wxSL_HORIZONTAL);

                mOffsetXSlider->Bind(
                    wxEVT_SLIDER,
                    [this](wxEvent &)
                    {
                        float const value = static_cast<float>(mOffsetXSlider->GetValue());

                        // Tell viz control
                        mShipOffsetVisualizationControl->SetOffsetX(value);

                        // Tell edit box - marking it as dirty
                        mOffsetXEditSpinBox->ChangeValue(value);

                        OnDirty();
                    });

                gSizer->Add(mOffsetXSlider, 0, wxEXPAND, 0);
            }

            {
                gSizer->AddSpacer(SliderWidth);
            }

            vSizer->Add(gSizer, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }
    }

    vSizer->AddSpacer(VerticalSeparatorSize);

    // Internal pressure
    {
        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Internal Pressure"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            vSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }

        {
            mInternalPressureEditSpinBox = new EditSpinBox<float>(
                panel,
                NumericEditBoxWidth,
                0.0f,
                10000.0f,
                0.0f,
                wxEmptyString,
                [this](float)
                {
                    OnDirty();
                });

            vSizer->Add(mInternalPressureEditSpinBox, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 0);
        }

        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Internal pressure that the ship is initially spawned with, in atmospheres"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            label->SetFont(explanationFont);

            vSizer->Add(label, 0, wxALL | wxEXPAND, 0);
        }
    }

    // Finalize
    auto marginSizer = new wxBoxSizer(wxVERTICAL);
    marginSizer->Add(vSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, PanelInternalMargin);
    panel->SetSizer(marginSizer);
}

void ShipPropertiesEditDialog::PopulateAutoTexturizationPanel(wxPanel * panel)
{
    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    auto explanationFont = panel->GetFont();
    explanationFont.SetPointSize(explanationFont.GetPointSize() - 2);
    explanationFont.SetStyle(wxFontStyle::wxFONTSTYLE_ITALIC);

    // Main radio
    {
        wxGridBagSizer * gSizer = new wxGridBagSizer(10, 5);

        {
            mAutoTexturizationSettingsOffButton = new BitmapRadioButton(
                panel,
                mGameAssetManager.GetPngImageFilePath("x_medium"),
                [this]()
                {
                    mAutoTexturizationSettingsOnButton->SetValue(false);
                    mAutoTexturizationSettingsPanel->Enable(false);

                    mIsAutoTexturizationSettingsDirty = true;

                    OnDirty();
                },
                _("Use the global auto-texturization settings."));

            gSizer->Add(
                mAutoTexturizationSettingsOffButton,
                wxGBPosition(0, 0),
                wxGBSpan(1, 1),
                0,
                0);
        }

        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Use the global auto-texturization settings of the simulator."), wxDefaultPosition, wxDefaultSize,
                wxALIGN_LEFT);

            label->SetFont(explanationFont);

            gSizer->Add(
                label,
                wxGBPosition(0, 1),
                wxGBSpan(1, 1),
                0,
                0);
        }

        {
            mAutoTexturizationSettingsOnButton = new BitmapRadioButton(
                panel,
                mGameAssetManager.GetPngImageFilePath("ship_file_medium"),
                [this]()
                {
                    mAutoTexturizationSettingsOffButton->SetValue(false);
                    mAutoTexturizationSettingsPanel->Enable(true);

                    mIsAutoTexturizationSettingsDirty = true;

                    OnDirty();
                },
                _("Set auto-texturization settings."));

            gSizer->Add(
                mAutoTexturizationSettingsOnButton,
                wxGBPosition(1, 0),
                wxGBSpan(1, 1),
                0,
                0);
        }

        {
            auto label = new wxStaticText(panel, wxID_ANY, _("Set ship-specific auto-texturization settings."), wxDefaultPosition, wxDefaultSize,
                wxALIGN_LEFT);

            label->SetFont(explanationFont);

            gSizer->Add(
                label,
                wxGBPosition(1, 1),
                wxGBSpan(1, 1),
                0,
                0);
        }

        vSizer->Add(gSizer, 0, wxALIGN_CENTER_HORIZONTAL, 0);
    }

    vSizer->AddSpacer(VerticalSeparatorSize);

    // Settings panel
    {
        mAutoTexturizationSettingsPanel = new wxPanel(panel);

        wxSizer * settingsPanelVSizer = new wxBoxSizer(wxVERTICAL);

        // Texturization Mode
        {
            wxStaticBoxSizer * texturizationModeBoxSizer = new wxStaticBoxSizer(wxVERTICAL, mAutoTexturizationSettingsPanel, _("Mode"));

            {
                wxGridBagSizer * gSizer = new wxGridBagSizer(10, 5);

                {
                    mFlatStructureAutoTexturizationModeButton = new BitmapRadioButton(
                        texturizationModeBoxSizer->GetStaticBox(),
                        mGameAssetManager.GetPngImageFilePath("auto_texturization_particle"),
                        [this]()
                        {
                            mMaterialTexturesAutoTexturizationModeButton->SetValue(false);

                            mMaterialTextureMagnificationSlider->Enable(false);
                            mMaterialTextureTransparencySlider->Enable(false);

                            mIsAutoTexturizationSettingsDirty = true;

                            OnDirty();
                        },
                        _("Flat Structure mode."));

                    gSizer->Add(
                        mFlatStructureAutoTexturizationModeButton,
                        wxGBPosition(0, 0),
                        wxGBSpan(1, 1),
                        0,
                        0);
                }

                {
                    auto label = new wxStaticText(texturizationModeBoxSizer->GetStaticBox(), wxID_ANY, _("Flat Structure mode: generates a texture using the particles' matte colors."),
                        wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);

                    label->SetFont(explanationFont);

                    gSizer->Add(
                        label,
                        wxGBPosition(0, 1),
                        wxGBSpan(1, 1),
                        0,
                        0);
                }

                {
                    mMaterialTexturesAutoTexturizationModeButton = new BitmapRadioButton(
                        texturizationModeBoxSizer->GetStaticBox(),
                        mGameAssetManager.GetPngImageFilePath("auto_texturization_material"),
                        [this]()
                        {
                            mFlatStructureAutoTexturizationModeButton->SetValue(false);

                            mMaterialTextureMagnificationSlider->Enable(true);
                            mMaterialTextureTransparencySlider->Enable(true);

                            mIsAutoTexturizationSettingsDirty = true;

                            OnDirty();
                        },
                        _("Material Textures mode."));

                    gSizer->Add(
                        mMaterialTexturesAutoTexturizationModeButton,
                        wxGBPosition(1, 0),
                        wxGBSpan(1, 1),
                        0,
                        0);
                }

                {
                    auto label = new wxStaticText(texturizationModeBoxSizer->GetStaticBox(), wxID_ANY, _("Material Textures mode: generates a texture using material-specific textures."),
                        wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);

                    label->SetFont(explanationFont);

                    gSizer->Add(
                        label,
                        wxGBPosition(1, 1),
                        wxGBSpan(1, 1),
                        0,
                        0);
                }

                texturizationModeBoxSizer->Add(gSizer, 0, wxALL, 10);
            }

            settingsPanelVSizer->Add(
                texturizationModeBoxSizer,
                0,
                wxEXPAND,
                0);
        }

        settingsPanelVSizer->AddSpacer(VerticalSeparatorSize);

        // Material Texture Magnification
        {
            mMaterialTextureMagnificationSlider = new SliderControl<float>(
                mAutoTexturizationSettingsPanel,
                SliderControl<float>::DirectionType::Horizontal,
                -1,
                -1,
                _("Texture Magnification"),
                _("Changes the level of detail of materials' textures."),
                [this](float)
                {
                    mIsAutoTexturizationSettingsDirty = true;

                    OnDirty();
                },
                std::make_unique<ExponentialSliderCore>(
                    0.1f,
                    1.0f,
                    2.0f));

            settingsPanelVSizer->Add(
                mMaterialTextureMagnificationSlider,
                0,
                wxEXPAND, // Expand horizontally
                0);
        }

        {
            auto label = new wxStaticText(mAutoTexturizationSettingsPanel, wxID_ANY, _("The level of detail of materials' textures"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            label->SetFont(explanationFont);

            settingsPanelVSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }

        settingsPanelVSizer->AddSpacer(VerticalSeparatorSize);

        // Material Texture Transparency
        {
            mMaterialTextureTransparencySlider = new SliderControl<float>(
                mAutoTexturizationSettingsPanel,
                SliderControl<float>::DirectionType::Horizontal,
                -1,
                -1,
                _("Texture Transparency"),
                _("Changes the transparency of materials' textures."),
                [this](float)
                {
                    mIsAutoTexturizationSettingsDirty = true;

                    OnDirty();
                },
                std::make_unique<LinearSliderCore>(
                    0.0f,
                    1.0f));

            settingsPanelVSizer->Add(
                mMaterialTextureTransparencySlider,
                0,
                wxEXPAND, // Expand horizontally
                0);
        }

        {
            auto label = new wxStaticText(mAutoTexturizationSettingsPanel, wxID_ANY, _("The transparency of materials' textures"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_CENTER);

            label->SetFont(explanationFont);

            settingsPanelVSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL, 0);
        }

        mAutoTexturizationSettingsPanel->SetSizerAndFit(settingsPanelVSizer);

        vSizer->Add(mAutoTexturizationSettingsPanel, 0, wxEXPAND, 0);
    }

    // Finalize
    auto marginSizer = new wxBoxSizer(wxVERTICAL);
    marginSizer->Add(vSizer, 0, wxEXPAND | wxALL, PanelInternalMargin);
    panel->SetSizer(marginSizer);
}

void ShipPropertiesEditDialog::PopulatePasswordProtectionPanel(wxPanel * panel)
{
    auto explanationFont = panel->GetFont();
    explanationFont.SetPointSize(explanationFont.GetPointSize() - 2);
    explanationFont.SetStyle(wxFontStyle::wxFONTSTYLE_ITALIC);

    wxGridBagSizer * gSizer = new wxGridBagSizer(10, 5);

    {
        mPasswordOnButton = new BitmapRadioButton(
            panel,
            mGameAssetManager.GetPngImageFilePath("protected_medium"),
            [this]()
            {
                OnSetPassword();
            },
            _("Set a password to protect edits to this ship."));

        gSizer->Add(
            mPasswordOnButton,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            0,
            0);
    }

    {
        auto label = new wxStaticText(panel, wxID_ANY, _("Set a password to prevent unauthorized people from making changes to this ship."), wxDefaultPosition, wxDefaultSize,
            wxALIGN_LEFT);

        label->SetFont(explanationFont);

        gSizer->Add(
            label,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            0,
            0);
    }

    {
        mPasswordOffButton = new BitmapRadioButton(
            panel,
            mGameAssetManager.GetPngImageFilePath("unprotected_medium"),
            [this]()
            {
                OnClearPassword();
            },
            _("Remove the password lock."));

        gSizer->Add(
            mPasswordOffButton,
            wxGBPosition(1, 0),
            wxGBSpan(1, 1),
            0,
            0);
    }

    {
        auto label = new wxStaticText(panel, wxID_ANY, _("Clear the password to allow everyone to make changes to this ship."), wxDefaultPosition, wxDefaultSize,
            wxALIGN_LEFT);

        label->SetFont(explanationFont);

        gSizer->Add(
            label,
            wxGBPosition(1, 1),
            wxGBSpan(1, 1),
            0,
            0);
    }

    // Finalize
    auto marginSizer = new wxBoxSizer(wxVERTICAL);
    marginSizer->Add(gSizer, 0, wxEXPAND | wxALL, PanelInternalMargin);
    panel->SetSizer(marginSizer);
}

void ShipPropertiesEditDialog::OnSetPassword()
{
    // Ask password
    NewPasswordDialog dialog(this, mGameAssetManager);
    auto const result = dialog.ShowModal();
    if (result == wxID_OK)
    {
        // Changed

        mPasswordHash = ShipDefinitionFormatDeSerializer::CalculatePasswordHash(dialog.GetPassword());
        mIsPasswordHashModified = true;
        OnDirty();

        ReconciliateUIWithPassword();
    }
    else
    {
        // Unchanged
    }
}

void ShipPropertiesEditDialog::OnClearPassword()
{
    auto const result = wxMessageBox(_("Are you sure you want to remove password protection for this ship, allowing everyone to make changes to it?"), ApplicationName,
        wxICON_EXCLAMATION | wxYES_NO | wxCENTRE);

    if (result == wxYES)
    {
        // Changed

        mPasswordHash.reset();
        mIsPasswordHashModified = true;
        OnDirty();

        ReconciliateUIWithPassword();
    }
}

void ShipPropertiesEditDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    //
    // Inspect dirty flags and communicate parts to Controller
    //

    std::optional<ShipMetadata> metadata;
    std::optional<ShipPhysicsData> physicsData;
    std::optional<std::optional<ShipAutoTexturizationSettings>> autoTexturizationSettings;

    if (IsMetadataDirty())
    {
        //
        // Populate new
        //

        auto shipName = MakeString(mShipNameTextCtrl->GetValue());
        assert(shipName.has_value() && shipName->length() > 0);

        // Normalize ship name, if user has changed it
        if (mShipNameTextCtrl->IsModified())
        {
            auto const normalizedShipName = mShipNameNormalizer.NormalizeName(*shipName);
            if (normalizedShipName != *shipName)
            {
                ShipNameSuggestionDialog dlg(mParent, mGameAssetManager);
                if (dlg.AskUserIfAcceptsSuggestedName(normalizedShipName))
                {
                    shipName = normalizedShipName;
                }
            }
        }

        metadata = ShipMetadata(
            *shipName,
            MakeString(mShipAuthorTextCtrl->GetValue()),
            MakeString(mArtCreditsTextCtrl->GetValue()),
            MakeString(mYearBuiltTextCtrl->GetValue()),
            MakeString(mDescriptionTextCtrl->GetValue()),
            mSessionData->Metadata.Scale, // CODEWORK: not editable in this version
            mSessionData->Metadata.DoHideElectricalsInPreview,
            mSessionData->Metadata.DoHideHDInPreview,
            mPasswordHash);
    }

    if (IsPhysicsDataDirty())
    {
        //
        // Populate new
        //

        physicsData = ShipPhysicsData(
            vec2f(
                mOffsetXEditSpinBox->GetValue(),
                mOffsetYEditSpinBox->GetValue()),
            mInternalPressureEditSpinBox->GetValue());
    }

    if (IsAutoTexturizationSettingsDirty())
    {
        //
        // Populate new
        //

        std::optional<ShipAutoTexturizationSettings> actualSettings;
        if (mAutoTexturizationSettingsOnButton->GetValue())
        {
            ShipAutoTexturizationModeType const mode = mFlatStructureAutoTexturizationModeButton->GetValue()
                ? ShipAutoTexturizationModeType::FlatStructure
                : ShipAutoTexturizationModeType::MaterialTextures;

            actualSettings = ShipAutoTexturizationSettings(
                mode,
                mMaterialTextureMagnificationSlider->GetValue(),
                mMaterialTextureTransparencySlider->GetValue());
        }

        autoTexturizationSettings = actualSettings;
    }

    if (metadata.has_value()
        || physicsData.has_value()
        || autoTexturizationSettings.has_value())
    {
        mSessionData->BuilderController.SetShipProperties(
            std::move(metadata),
            std::move(physicsData),
            std::move(autoTexturizationSettings));
    }

    //
    // Close dialog
    //

    mSessionData.reset();
    mShipOffsetVisualizationControl->Deinitialize();
    EndModal(0);
}

void ShipPropertiesEditDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    mSessionData.reset();
    mShipOffsetVisualizationControl->Deinitialize();
    EndModal(-1);
}

void ShipPropertiesEditDialog::OnDirty()
{
    // We assume at least one of the controls is dirty

    auto const doEnable = MakeString(mShipNameTextCtrl->GetValue()).has_value();
    if (mOkButton->IsEnabled() != doEnable)
    {
        mOkButton->Enable(doEnable);
    }
}

void ShipPropertiesEditDialog::ReconciliateUI()
{
    assert(mSessionData);

    //
    // Metadata
    //

    mShipNameTextCtrl->ChangeValue(mSessionData->Metadata.ShipName);

    mShipAuthorTextCtrl->ChangeValue(mSessionData->Metadata.Author.value_or(wxGetUserName().ToStdString()));

    if (mSessionData->HasTexture)
    {
        mArtCreditsTextCtrl->ChangeValue(mSessionData->Metadata.ArtCredits.value_or(""));
        mArtCreditsTextCtrl->Enable(true);
    }
    else
    {
        mArtCreditsTextCtrl->ChangeValue(wxEmptyString);
        mArtCreditsTextCtrl->Enable(false);
    }

    mYearBuiltTextCtrl->ChangeValue(mSessionData->Metadata.YearBuilt.value_or(""));

    //
    // Description
    //

    mDescriptionTextCtrl->ChangeValue(mSessionData->Metadata.Description.value_or(""));

    //
    // Physics
    //

    mOffsetXEditSpinBox->SetValue(mSessionData->PhysicsData.Offset.x);
    mOffsetYEditSpinBox->SetValue(mSessionData->PhysicsData.Offset.y);

    mShipOffsetVisualizationControl->Initialize(
        mSessionData->ShipVisualization,
        mSessionData->Metadata.Scale,
        mSessionData->PhysicsData.Offset.x,
        mSessionData->PhysicsData.Offset.y);

    mOffsetXSlider->SetValue(static_cast<int>(mSessionData->PhysicsData.Offset.x));
    mOffsetYSlider->SetValue(static_cast<int>(mSessionData->PhysicsData.Offset.y));

    mInternalPressureEditSpinBox->SetValue(mSessionData->PhysicsData.InternalPressure);

    //
    // Auto-Texturization
    //

    mAutoTexturizationPanel->Enable(!mSessionData->HasTexture);

    if (mSessionData->AutoTexturizationSettings.has_value())
    {
        mAutoTexturizationSettingsOffButton->SetValue(false);
        mAutoTexturizationSettingsOnButton->SetValue(true);
        mAutoTexturizationSettingsPanel->Enable(true);
    }
    else
    {
        mAutoTexturizationSettingsOffButton->SetValue(true);
        mAutoTexturizationSettingsOnButton->SetValue(false);
        mAutoTexturizationSettingsPanel->Enable(false);
    }

    // Get settings from ship or, if not set, get defaults
    ShipAutoTexturizationSettings const settings = mSessionData->AutoTexturizationSettings.value_or(ShipAutoTexturizationSettings());

    switch (settings.Mode)
    {
        case ShipAutoTexturizationModeType::FlatStructure:
        {
            mFlatStructureAutoTexturizationModeButton->SetValue(true);
            mMaterialTexturesAutoTexturizationModeButton->SetValue(false);

            mMaterialTextureMagnificationSlider->Enable(false);
            mMaterialTextureTransparencySlider->Enable(false);

            break;
        }

        case ShipAutoTexturizationModeType::MaterialTextures:
        {
            mFlatStructureAutoTexturizationModeButton->SetValue(false);
            mMaterialTexturesAutoTexturizationModeButton->SetValue(true);

            mMaterialTextureMagnificationSlider->Enable(true);
            mMaterialTextureTransparencySlider->Enable(true);

            break;
        }
    }

    mMaterialTextureMagnificationSlider->SetValue(settings.MaterialTextureMagnification);
    mMaterialTextureTransparencySlider->SetValue(settings.MaterialTextureTransparency);

    mIsAutoTexturizationSettingsDirty = false;

    //
    // Password protection
    //

    mPasswordHash = mSessionData->Metadata.Password;
    mIsPasswordHashModified = false;
    ReconciliateUIWithPassword();

    //
    // Buttons
    //

    mOkButton->Enable(false);
}

void ShipPropertiesEditDialog::ReconciliateUIWithPassword()
{
    bool const hasPassword = mPasswordHash.has_value();

    mPasswordOnButton->Enable(!hasPassword);
    mPasswordOnButton->SetValue(hasPassword);

    mPasswordOffButton->Enable(hasPassword);
    mPasswordOffButton->SetValue(!hasPassword);
}

bool ShipPropertiesEditDialog::IsMetadataDirty() const
{
    return mShipNameTextCtrl->IsModified()
        || mShipAuthorTextCtrl->IsModified()
        || mArtCreditsTextCtrl->IsModified()
        || mYearBuiltTextCtrl->IsModified()
        || mDescriptionTextCtrl->IsModified()
        || mIsPasswordHashModified;
}

bool ShipPropertiesEditDialog::IsPhysicsDataDirty() const
{
    return mOffsetXEditSpinBox->IsModified()
        || mOffsetYEditSpinBox->IsModified()
        || mInternalPressureEditSpinBox->IsModified();
}

bool ShipPropertiesEditDialog::IsAutoTexturizationSettingsDirty() const
{
    return mIsAutoTexturizationSettingsDirty;
}

std::optional<std::string> ShipPropertiesEditDialog::MakeString(wxString && value)
{
    std::string trimmedValue = value.Trim(true).Trim(false).ToStdString();
    if (trimmedValue.empty())
        return std::nullopt;
    else
        return trimmedValue;
}

}