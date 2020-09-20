/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2020-09-14
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "DebugDialog.h"

#include <wx/button.h>
#include <wx/gbsizer.h>
#include <wx/notebook.h>
#include <wx/statbox.h>

static constexpr int Border = 10;
static int constexpr CellBorder = 8;
static int constexpr StaticBoxInsetMargin = 10;
static int constexpr StaticBoxTopMargin = 7;

DebugDialog::DebugDialog(
    wxWindow* parent,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController)
    : mParent(parent)
    , mGameController(std::move(gameController))
    , mSoundController(std::move(soundController))
{
    Create(
        mParent,
        wxID_ANY,
        _("Debug"),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxFRAME_SHAPED);

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));


    //
    // Lay the dialog out
    //

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    wxNotebook * notebook = new wxNotebook(
        this,
        wxID_ANY,
        wxPoint(-1, -1),
        wxSize(-1, -1),
        wxNB_TOP);


    //
    // Triangle
    //

    {
        wxPanel * trianglesPanel = new wxPanel(notebook);

        PopulateTrianglesPanel(trianglesPanel);

        notebook->AddPage(trianglesPanel, _("Triangles"));
    }

    dialogVSizer->Add(notebook, 1, wxEXPAND);


    //
    // Finalize dialog
    //

    SetSizer(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

void DebugDialog::Open()
{
    this->Show();
}

void DebugDialog::PopulateTrianglesPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    //
    // Destroy
    //

    {
        wxStaticBox * destroyBox = new wxStaticBox(panel, wxID_ANY, _("Destroy"));

        wxBoxSizer * destroyBoxSizer = new wxBoxSizer(wxVERTICAL);
        destroyBoxSizer->AddSpacer(StaticBoxTopMargin + StaticBoxInsetMargin);

        // Triangle Index
        {
            mTriangleIndexSpinCtrl = new wxSpinCtrl(destroyBox, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                wxSP_ARROW_KEYS | wxALIGN_CENTRE_HORIZONTAL);
            mTriangleIndexSpinCtrl->SetRange(0, std::numeric_limits<int>::max());

            destroyBoxSizer->Add(
                mTriangleIndexSpinCtrl,
                0,
                wxALIGN_CENTER_HORIZONTAL | wxALL,
                StaticBoxInsetMargin);
        }

        // Button
        {
            auto destroyButton = new wxButton(destroyBox, wxID_ANY, _("Destroy!"));
            destroyButton->Bind(
                wxEVT_BUTTON,
                [this](wxCommandEvent &)
                {
                    bool bRes = mGameController->DestroyTriangle(
                        ElementId(
                            0, // TODO: ship ID
                            static_cast<ElementIndex>(mTriangleIndexSpinCtrl->GetValue())));

                    if (!bRes)
                    {
                        mSoundController->PlayErrorSound();
                    }
                });

            destroyBoxSizer->Add(
                destroyButton,
                0,
                wxALIGN_CENTER_HORIZONTAL | wxALL,
                StaticBoxInsetMargin);
        }

        destroyBox->SetSizerAndFit(destroyBoxSizer);

        gridSizer->Add(
            destroyBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL,
            CellBorder);
    }


    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
}