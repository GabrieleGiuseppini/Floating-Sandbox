/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-08-27
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MainFrame.h"

#include <GameCore/Log.h>
#include <GameCore/UserGameException.h>
#include <GameCore/Version.h>

#include <GameOpenGL/GameOpenGL.h>

#include <Game/ImageFileTools.h>
#include <Game/ShipDeSerializer.h>

#include <UILib/BitmapButton.h>
#include <UILib/BitmapToggleButton.h>
#include <UILib/EditSpinBox.h>
#include <UILib/WxHelpers.h>

#include <wx/button.h>
#include <wx/cursor.h>
#include <wx/gbsizer.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/tglbtn.h>
#include <wx/utils.h>

#ifdef _MSC_VER
 // Nothing to do here - we use RC files
#else
#include "Resources/ShipBBB.xpm"
#endif

#include <cassert>

namespace ShipBuilder {


int constexpr ButtonMargin = 4;

ImageSize constexpr MaterialSwathSize(80, 100);

int const MinLayerTransparency = 0;
int constexpr MaxLayerTransparency = 100;


MainFrame::MainFrame(
    wxApp * mainApp,
    ResourceLocator const & resourceLocator,
    LocalizationManager const & localizationManager,
    MaterialDatabase const & materialDatabase,
    ShipTexturizer const & shipTexturizer,
    std::function<void(std::optional<std::filesystem::path>)> returnToGameFunctor)
    : mMainApp(mainApp)
    , mReturnToGameFunctor(std::move(returnToGameFunctor))
    , mResourceLocator(resourceLocator)
    , mLocalizationManager(localizationManager)
    , mMaterialDatabase(materialDatabase)
    , mShipTexturizer(shipTexturizer)
    , mWorkCanvasHScrollBar(nullptr)
    , mWorkCanvasVScrollBar(nullptr)
    // State
    , mIsMouseCapturedByWorkCanvas(false)
    , mWorkbenchState(materialDatabase)
{
    Create(
        nullptr,
        wxID_ANY,
        std::string(APPLICATION_NAME " ShipBuilder " APPLICATION_VERSION_SHORT_STR),
        wxDefaultPosition,
        wxDefaultSize,
        wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxRESIZE_BORDER | wxSYSTEM_MENU | wxCAPTION | wxCLIP_CHILDREN | wxMAXIMIZE
        | (IsStandAlone() ? wxCLOSE_BOX : 0));

    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);

    SetIcon(wxICON(BBB_SHIP_ICON));
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    Maximize();
    Centre();

    //
    // Load static bitmaps
    //

    mNullMaterialBitmap = WxHelpers::LoadBitmap("null_material", MaterialSwathSize, mResourceLocator);

    //
    // Setup main frame
    //
    // Row 0: [File Panel] [Ship Settings] [Tool Settings]
    // Row 1: [Layers Panel]  |  [Work Canvas]
    //        [Toolbar Panel] |
    // Row 2: [           Status Bar                  ]

    mMainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxSizer * mainVSizer = new wxBoxSizer(wxVERTICAL);

    // Row 0
    {
        wxBoxSizer * row0HSizer = new wxBoxSizer(wxHORIZONTAL);

        // File panel
        {
            wxPanel * filePanel = CreateFilePanel(mMainPanel);

            row0HSizer->Add(
                filePanel,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);
        }

        // Spacer
        {
            wxStaticLine * vLine = new wxStaticLine(mMainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);

            row0HSizer->Add(
                vLine,
                0,
                wxEXPAND | wxLEFT | wxRIGHT,
                2 * ButtonMargin);
        }

        // Ship settings panel
        {
            wxPanel * shipSettingsPanel = CreateShipSettingsPanel(mMainPanel);

            row0HSizer->Add(
                shipSettingsPanel,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);
        }

        // Spacer
        {
            wxStaticLine * vLine = new wxStaticLine(mMainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);

            row0HSizer->Add(
                vLine,
                0,
                wxEXPAND | wxLEFT | wxRIGHT,
                2 * ButtonMargin);
        }

        // Tool settings panel
        {
            wxPanel * toolSettingsPanel = CreateToolSettingsPanel(mMainPanel);

            row0HSizer->Add(
                toolSettingsPanel,
                1, // Expand horizontally
                wxALIGN_CENTER_VERTICAL,
                0);
        }

        mainVSizer->Add(
            row0HSizer,
            0,
            wxEXPAND, // Expand horizontally
            0);
    }

    // Row 1
    {
        wxBoxSizer * row1HSizer = new wxBoxSizer(wxHORIZONTAL);

        // Col 0
        {
            wxBoxSizer * row1Col0VSizer = new wxBoxSizer(wxVERTICAL);

            // Layers panel
            {
                wxSizer * tmpVSizer = new wxBoxSizer(wxVERTICAL);

                {
                    wxStaticLine * line = new wxStaticLine(mMainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);

                    tmpVSizer->Add(
                        line,
                        0,
                        wxEXPAND,
                        0);
                }

                {
                    wxPanel * layersPanel = CreateLayersPanel(mMainPanel);

                    tmpVSizer->Add(
                        layersPanel,
                        0,
                        wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT,
                        4);
                }

                {
                    wxStaticLine * line = new wxStaticLine(mMainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);

                    tmpVSizer->Add(
                        line,
                        0,
                        wxEXPAND,
                        0);
                }

                row1Col0VSizer->Add(
                    tmpVSizer,
                    0,
                    0,
                    0);
            }

            // Toolbar panel
            {
                wxPanel * toolbarPanel = CreateToolbarPanel(mMainPanel);

                row1Col0VSizer->Add(
                    toolbarPanel,
                    0,
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            row1HSizer->Add(
                row1Col0VSizer,
                0,
                0,
                0);
        }

        // Col 1
        {
            // Work panel
            {
                wxPanel * workPanel = CreateWorkPanel(mMainPanel);

                row1HSizer->Add(
                    workPanel,
                    1, // Use all available H space
                    wxEXPAND, // Also expand vertically
                    0);
            }
        }

        mainVSizer->Add(
            row1HSizer,
            1, // Take all vertical room
            wxEXPAND, // Expand horizontally
            0);
    }

    // Row 2
    {
        // Status bar
        {
            mStatusBar = new StatusBar(mMainPanel, mResourceLocator);

            mainVSizer->Add(
                mStatusBar,
                0,
                wxEXPAND, // Expand horizontally
                0);
        }
    }

    mMainPanel->SetSizer(mainVSizer);

    //
    // Setup menu
    //

    wxMenuBar * mainMenuBar = new wxMenuBar();

#ifdef __WXGTK__
    // Note: on GTK we build an accelerator table for plain keys because of https://trac.wxwidgets.org/ticket/17611
    std::vector<wxAcceleratorEntry> acceleratorEntries;
#define ADD_PLAIN_ACCELERATOR_KEY(key, menuItem) \
        acceleratorEntries.push_back(MakePlainAcceleratorKey(int((key)), (menuItem)));
#else
#define ADD_PLAIN_ACCELERATOR_KEY(key, menuItem) \
        (void)menuItem;
#endif

    // File
    {
        wxMenu * fileMenu = new wxMenu();

        {
            wxMenuItem * newShipMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("New Ship") + wxS("\tCtrl+N"), _("Create a new empty ship"), wxITEM_NORMAL);
            fileMenu->Append(newShipMenuItem);
            Connect(newShipMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnNewShip);
        }

        {
            wxMenuItem * loadShipMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Load Ship") + wxS("\tCtrl+O"), _("Load a ship"), wxITEM_NORMAL);
            fileMenu->Append(loadShipMenuItem);
            Connect(loadShipMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnLoadShip);
        }

        {
            mSaveShipMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Save Ship") + wxS("\tCtrl+S"), _("Save the current ship"), wxITEM_NORMAL);
            fileMenu->Append(mSaveShipMenuItem);
            Connect(mSaveShipMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSaveShip);
        }

        if (!IsStandAlone())
        {
            mSaveAndGoBackMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Save Ship and Return to Game"), _("Save the current ship and return to the simulator"), wxITEM_NORMAL);
            fileMenu->Append(mSaveAndGoBackMenuItem);
            Connect(mSaveAndGoBackMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSaveAndGoBack);
        }
        else
        {
            mSaveAndGoBackMenuItem = nullptr;
        }

        {
            wxMenuItem * saveShipAsMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Save Ship As"), _("Save the current ship to a different file"), wxITEM_NORMAL);
            fileMenu->Append(saveShipAsMenuItem);
            Connect(saveShipAsMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSaveShipAs);
        }

        if (!IsStandAlone())
        {
            wxMenuItem * quitAndGoBackMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Quit and Return to Game"), _("Discard the current ship and return to the simulator"), wxITEM_NORMAL);
            fileMenu->Append(quitAndGoBackMenuItem);
            Connect(quitAndGoBackMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnQuitAndGoBack);
        }

        if (IsStandAlone())
        {
            wxMenuItem * quitMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Quit") + wxS("\tAlt+F4"), _("Quit the builder"), wxITEM_NORMAL);
            fileMenu->Append(quitMenuItem);
            Connect(quitMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnQuit);
        }

        mainMenuBar->Append(fileMenu, _("&File"));
    }

    // Edit
    {
        wxMenu * editMenu = new wxMenu();

        {
            wxMenuItem * menuItem = new wxMenuItem(editMenu, wxID_ANY, _("Resize Ship") + wxS("\tCtrl+R"), _("Resize the ship"), wxITEM_NORMAL);
            editMenu->Append(menuItem);
            Connect(menuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShipCanvasResize);
        }

        {
            wxMenuItem * menuItem = new wxMenuItem(editMenu, wxID_ANY, _("Ship Properties") + wxS("\tCtrl+P"), _("Edit the ship properties"), wxITEM_NORMAL);
            editMenu->Append(menuItem);
            Connect(menuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShipProperties);
        }

        {
            mUndoMenuItem = new wxMenuItem(editMenu, wxID_ANY, _("Undo") + wxS("\tCtrl+Z"), _("Undo the last edit operation"), wxITEM_NORMAL);
            editMenu->Append(mUndoMenuItem);
            Connect(mUndoMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnUndo);
        }

        mainMenuBar->Append(editMenu, _("&Edit"));
    }

    // View

    {
        wxMenu * viewMenu = new wxMenu();

        wxMenuItem * zoomInMenuItem = new wxMenuItem(viewMenu, wxID_ANY, _("Zoom In") + wxS("\t+"), wxEmptyString, wxITEM_NORMAL);
        viewMenu->Append(zoomInMenuItem);
        Connect(zoomInMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnZoomIn);
        ADD_PLAIN_ACCELERATOR_KEY('+', zoomInMenuItem);
        ADD_PLAIN_ACCELERATOR_KEY(WXK_NUMPAD_ADD, zoomInMenuItem);

        wxMenuItem * zoomOutMenuItem = new wxMenuItem(viewMenu, wxID_ANY, _("Zoom Out") + wxS("\t-"), wxEmptyString, wxITEM_NORMAL);
        viewMenu->Append(zoomOutMenuItem);
        Connect(zoomOutMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnZoomOut);
        ADD_PLAIN_ACCELERATOR_KEY('-', zoomOutMenuItem);
        ADD_PLAIN_ACCELERATOR_KEY(WXK_NUMPAD_SUBTRACT, zoomOutMenuItem);

        viewMenu->Append(new wxMenuItem(viewMenu, wxID_SEPARATOR));

        wxMenuItem * resetViewMenuItem = new wxMenuItem(viewMenu, wxID_ANY, _("Reset View") + wxS("\tHOME"), wxEmptyString, wxITEM_NORMAL);
        viewMenu->Append(resetViewMenuItem);
        Connect(resetViewMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnResetView);
        ADD_PLAIN_ACCELERATOR_KEY(WXK_HOME, resetViewMenuItem);

        mainMenuBar->Append(viewMenu, _("&View"));
    }

    // Options
    {
        wxMenu * optionsMenu = new wxMenu();

        wxMenuItem * openLogWindowMenuItem = new wxMenuItem(optionsMenu, wxID_ANY, _("Open Log Window") + wxS("\tCtrl+L"), wxEmptyString, wxITEM_NORMAL);
        optionsMenu->Append(openLogWindowMenuItem);
        Connect(openLogWindowMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnOpenLogWindowMenuItemSelected);

        mainMenuBar->Append(optionsMenu, _("&Options"));
    }

    SetMenuBar(mainMenuBar);

#ifdef __WXGTK__
    // Set accelerator
    wxAcceleratorTable accel(acceleratorEntries.size(), acceleratorEntries.data());
    SetAcceleratorTable(accel);
#endif

    //
    // Setup material palettes
    //

    mStructuralMaterialPalette = std::make_unique<MaterialPalette<StructuralMaterial>>(
        this,
        mMaterialDatabase.GetStructuralMaterialPalette(),
        mShipTexturizer,
        mResourceLocator);

    mStructuralMaterialPalette->Bind(fsEVT_STRUCTURAL_MATERIAL_SELECTED, &MainFrame::OnStructuralMaterialSelected, this);

    mElectricalMaterialPalette = std::make_unique<MaterialPalette<ElectricalMaterial>>(
        this,
        mMaterialDatabase.GetElectricalMaterialPalette(),
        mShipTexturizer,
        mResourceLocator);

    mElectricalMaterialPalette->Bind(fsEVT_ELECTRICAL_MATERIAL_SELECTED, &MainFrame::OnElectricalMaterialSelected, this);

    //
    // Create view
    //

    if (IsStandAlone())
    {
        // Initialize OpenGL
        GameOpenGL::InitOpenGL();
    }

    mView = std::make_unique<View>(
        ShipSpaceSize(0, 0), // We don't have a ship yet
        DisplayLogicalSize(
            mWorkCanvas->GetSize().GetWidth(),
            mWorkCanvas->GetSize().GetHeight()),
        mWorkCanvas->GetContentScaleFactor(),
        false, // Grid enabled
        [this]()
        {
            mWorkCanvas->SwapBuffers();
        },
        mResourceLocator);

    mView->UploadBackgroundTexture(
        ImageFileTools::LoadImageRgba(
            mResourceLocator.GetBitmapFilePath("shipbuilder_background")));
}

void MainFrame::OpenForNewShip()
{
    // New ship
    // TODO: date in ship name, via static helper method
    DoNewShip("MyShip-TODO");

    // Open ourselves
    Open();
}

void MainFrame::OpenForLoadShip(std::filesystem::path const & shipFilePath)
{
    // Load ship
    DoLoadShip(shipFilePath);

    // Open ourselves
    Open();
}

//
// IUserInterface
//

void MainFrame::RefreshView()
{
    if (mWorkCanvas)
    {
        mWorkCanvas->Refresh();
    }
}

void MainFrame::OnViewModelChanged()
{
    if (mController)
    {
        ReconciliateUIWithViewModel();
    }
}

void MainFrame::OnShipMetadataChanged(ShipMetadata const & shipMetadata)
{
    if (mController)
    {
        ReconciliateUIWithShipMetadata(shipMetadata);
    }
}

void MainFrame::OnShipSizeChanged(ShipSpaceSize const & shipSize)
{
    if (mController)
    {
        ReconciliateUIWithShipSize(shipSize);
    }
}

void MainFrame::OnLayerPresenceChanged()
{
    if (mController)
    {
        ReconciliateUIWithLayerPresence();
    }
}

void MainFrame::OnPrimaryLayerChanged(LayerType primaryLayer)
{
    if (mController)
    {
        ReconciliateUIWithPrimaryLayerSelection(primaryLayer);
    }
}

void MainFrame::OnModelDirtyChanged()
{
    if (mController)
    {
        ReconciliateUIWithModelDirtiness();
    }
}

void MainFrame::OnWorkbenchStateChanged()
{
    if (mController)
    {
        ReconciliateUIWithWorkbenchState();
    }
}

void MainFrame::OnCurrentToolChanged(std::optional<ToolType> tool)
{
    if (mController)
    {
        ReconciliateUIWithSelectedTool(tool);
    }
}

void MainFrame::OnUndoStackStateChanged()
{
    if (mController)
    {
        ReconciliateUIWithUndoStackState();
    }
}

void MainFrame::OnToolCoordinatesChanged(std::optional<ShipSpaceCoordinates> coordinates)
{
    if (coordinates.has_value())
    {
        if (mController)
        {
            // Flip coordinates: we show zero at top, just to be consistent with drawing software
            coordinates->FlipY(mController->GetModelController().GetModel().GetShipSize().height);
        }
        else
        {
            coordinates.reset();
        }
    }

    mStatusBar->SetToolCoordinates(coordinates);
}

ShipSpaceCoordinates MainFrame::GetMouseCoordinates() const
{
    wxMouseState const mouseState = wxGetMouseState();
    int x = mouseState.GetX();
    int y = mouseState.GetY();

    // Convert to work canvas coordinates
    assert(mWorkCanvas);
    mWorkCanvas->ScreenToClient(&x, &y);

    return mView->ScreenToShipSpace({ x, y });
}

std::optional<ShipSpaceCoordinates> MainFrame::GetMouseCoordinatesIfInWorkCanvas() const
{
    //
    // This function basically simulates the mouse<->focused window logic, returning mouse
    // coordinates only if the work canvas would legitimately receive a mouse event
    //

    wxMouseState const mouseState = wxGetMouseState();
    int x = mouseState.GetX();
    int y = mouseState.GetY();

    // Convert to work canvas coordinates
    assert(mWorkCanvas);
    mWorkCanvas->ScreenToClient(&x, &y);

    // Check if in canvas (but not captured by scrollbars), or if simply captured
    if ((mWorkCanvas->HitTest(x, y) == wxHT_WINDOW_INSIDE && !mWorkCanvasHScrollBar->HasCapture() && !mWorkCanvasVScrollBar->HasCapture())
        || mIsMouseCapturedByWorkCanvas)
    {
        return mView->ScreenToShipSpace({ x, y });
    }
    else
    {
        return std::nullopt;
    }
}

void MainFrame::SetToolCursor(wxImage const & cursorImage)
{
    mWorkCanvas->SetCursor(wxCursor(cursorImage));
}

void MainFrame::ResetToolCursor()
{
    mWorkCanvas->SetCursor(wxNullCursor);
}

void MainFrame::ScrollIntoViewIfNeeded(DisplayLogicalCoordinates const & workCanvasDisplayLogicalCoordinates)
{
    // TODOTEST
    LogMessage("TODOTEST: MainFrame::ScrollIntoViewIfNeeded(", workCanvasDisplayLogicalCoordinates.ToString(), ") WorkCanvasSize=(",
        mWorkCanvas->GetSize().GetWidth(), ", ", mWorkCanvas->GetSize().GetHeight());

    // TODO: if we have to scroll, invoke Controller::PanCamera which will notify us back
}

/////////////////////////////////////////////////////////////////////

wxPanel * MainFrame::CreateFilePanel(wxWindow * parent)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

    {
        // New ship
        {
            auto button = new BitmapButton(
                panel,
                mResourceLocator.GetIconFilePath("new_ship_button"),
                [this]()
                {
                    NewShip();
                },
                _("Create a new empty ship"));

            sizer->Add(button, 0, wxALL, ButtonMargin);
        }

        sizer->AddStretchSpacer();

        // Load ship
        {
            auto button = new BitmapButton(
                panel,
                mResourceLocator.GetIconFilePath("load_ship_button"),
                [this]()
                {
                    LoadShip();
                },
                _("Load a ship"));

            sizer->Add(button, 0, wxALL, ButtonMargin);
        }

        sizer->AddStretchSpacer();

        // Save ship
        {
            mSaveShipButton = new BitmapButton(
                panel,
                mResourceLocator.GetIconFilePath("save_ship_button"),
                [this]()
                {
                    SaveShip();
                },
                _("Save the current ship"));

            sizer->Add(mSaveShipButton, 0, wxALL, ButtonMargin);
        }

        sizer->AddStretchSpacer();

        // Save As ship
        {
            auto button = new BitmapButton(
                panel,
                mResourceLocator.GetIconFilePath("save_ship_as_button"),
                [this]()
                {
                    SaveShipAs();
                },
                _("Save the current ship to a different file"));

            sizer->Add(button, 0, wxALL, ButtonMargin);
        }

        // Save and return to game
        if (!IsStandAlone())
        {
            auto saveAndReturnToGameButton = new BitmapButton(
                panel,
                mResourceLocator.GetIconFilePath("save_and_return_to_game_button"),
                [this]()
                {
                    SaveAndSwitchBackToGame();
                },
                _("Save the current ship and return to the simulator"));

            sizer->Add(saveAndReturnToGameButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, ButtonMargin);
        }

        // Quit and return to game
        if (!IsStandAlone())
        {
            auto quitAndReturnToGameButton = new BitmapButton(
                panel,
                mResourceLocator.GetIconFilePath("quit_and_return_to_game_button"),
                [this]()
                {
                    QuitAndSwitchBackToGame();
                },
                _("Discard the current ship and return to the simulator"));

            sizer->Add(quitAndReturnToGameButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, ButtonMargin);
        }
    }

    panel->SetSizerAndFit(sizer);

    return panel;
}

wxPanel * MainFrame::CreateShipSettingsPanel(wxWindow * parent)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

    {
        // Resize button
        {
            auto button = new BitmapButton(
                panel,
                mResourceLocator.GetIconFilePath("resize_button"),
                [this]()
                {
                    OpenShipCanvasResize();
                },
                _("Resize the ship"));

            sizer->Add(button, 0, wxALL, ButtonMargin);
        }

        // Metadata button
        {
            auto button = new BitmapButton(
                panel,
                mResourceLocator.GetIconFilePath("metadata_button"),
                [this]()
                {
                    OpenShipProperties();
                },
                _("Edit the ship properties"));

            sizer->Add(button, 0, wxALL, ButtonMargin);
        }
    }

    panel->SetSizerAndFit(sizer);

    return panel;
}

wxPanel * MainFrame::CreateToolSettingsPanel(wxWindow * parent)
{
    std::uint32_t MaxPencilSize = 8;

    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    mToolSettingsPanelsSizer = new wxBoxSizer(wxHORIZONTAL);

    mToolSettingsPanelsSizer->AddSpacer(20);

    {
        // Structural pencil
        {
            wxPanel * tsPanel = CreateToolSettingsToolSizePanel(
                panel,
                _("Pencil size:"),
                _("The size of the pencil tool."),
                1,
                MaxPencilSize,
                mWorkbenchState.GetStructuralPencilToolSize(),
                [this](std::uint32_t value)
                {
                    mWorkbenchState.SetStructuralPencilToolSize(value);
                });

            mToolSettingsPanelsSizer->Add(
                tsPanel,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);

            mToolSettingsPanels.emplace_back(
                ToolType::StructuralPencil,
                tsPanel);
        }

        // Structural eraser
        {
            wxPanel * tsPanel = CreateToolSettingsToolSizePanel(
                panel,
                _("Eraser size:"),
                _("The size of the eraser tool."),
                1,
                MaxPencilSize,
                mWorkbenchState.GetStructuralEraserToolSize(),
                [this](std::uint32_t value)
                {
                    mWorkbenchState.SetStructuralEraserToolSize(value);
                });

            mToolSettingsPanelsSizer->Add(
                tsPanel,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);

            mToolSettingsPanels.emplace_back(
                ToolType::StructuralEraser,
                tsPanel);
        }
    }

    mToolSettingsPanelsSizer->AddStretchSpacer(1);

    panel->SetSizerAndFit(mToolSettingsPanelsSizer);

    return panel;
}

wxPanel * MainFrame::CreateLayersPanel(wxWindow * parent)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * rootVSizer = new wxBoxSizer(wxVERTICAL);

    rootVSizer->AddSpacer(10);

    {
        // Layer management
        {
            wxGridBagSizer * layerManagerSizer = new wxGridBagSizer(0, 0);

            {
                auto const createButtonRow = [&](LayerType layer, int iRow)
                {
                    size_t iLayer = static_cast<size_t>(layer);

                    // Selector
                    {
                        std::string buttonBitmapName;
                        wxString buttonTooltip;
                        switch (layer)
                        {
                            case LayerType::Electrical:
                            {
                                buttonBitmapName = "electrical_layer";
                                buttonTooltip = _("Electrical layer");
                                break;
                            }

                            case LayerType::Ropes:
                            {
                                buttonBitmapName = "ropes_layer";
                                buttonTooltip = _("Ropes layer");
                                break;
                            }

                            case LayerType::Structural:
                            {
                                buttonBitmapName = "structural_layer";
                                buttonTooltip = _("Structural layer");
                                break;
                            }

                            case LayerType::Texture:
                            {
                                buttonBitmapName = "texture_layer";
                                buttonTooltip = _("Texture layer");
                                break;
                            }
                        }

                        auto * selectorButton = new BitmapToggleButton(
                            panel,
                            mResourceLocator.GetBitmapFilePath(buttonBitmapName),
                            [this, layer]()
                            {
                                mController->SelectPrimaryLayer(layer);
                            },
                            buttonTooltip);

                        layerManagerSizer->Add(
                            selectorButton,
                            wxGBPosition(iRow * 3, 0),
                            wxGBSpan(2, 1),
                            wxALIGN_CENTER_VERTICAL,
                            0);

                        mLayerSelectButtons[iLayer] = selectorButton;
                    }

                    // New
                    {
                        BitmapButton * newButton;

                        if (layer != LayerType::Texture)
                        {
                            newButton = new BitmapButton(
                                panel,
                                mResourceLocator.GetBitmapFilePath("new_layer_button"),
                                [this, layer]()
                                {
                                    switch (layer)
                                    {
                                        case LayerType::Electrical:
                                        {
                                            mController->NewElectricalLayer();
                                            break;
                                        }

                                        case LayerType::Ropes:
                                        {
                                            mController->NewRopesLayer();
                                            break;
                                        }

                                        case LayerType::Structural:
                                        {
                                            if (mController->GetModelController().GetModel().GetIsDirty(LayerType::Structural))
                                            {
                                                if (!AskUserIfSure(_("The current changes to the structural layer will be lost; are you sure you want to continue?")))
                                                {
                                                    // Changed their mind
                                                    return;
                                                }
                                            }

                                            mController->NewStructuralLayer();

                                            break;
                                        }

                                        case LayerType::Texture:
                                        {
                                            assert(false);
                                            break;
                                        }
                                    }
                                },
                                _("Add or clean the layer."));

                            layerManagerSizer->Add(
                                newButton,
                                wxGBPosition(iRow * 3, 1),
                                wxGBSpan(1, 1),
                                wxLEFT | wxRIGHT,
                                10);
                        }
                        else
                        {
                            newButton = nullptr;
                        }
                    }

                    // Import
                    {
                        wxString buttonTooltip;
                        switch (layer)
                        {
                            case LayerType::Electrical:
                            case LayerType::Ropes:
                            {
                                buttonTooltip = _("Import this layer from another ship.");
                                break;
                            }

                            case LayerType::Structural:
                            case LayerType::Texture:
                            {
                                buttonTooltip = _("Import this layer from another ship or from an image file.");
                                break;
                            }
                        }

                        auto * importButton = new BitmapButton(
                            panel,
                            mResourceLocator.GetBitmapFilePath("open_layer_button"),
                            [this, layer]()
                            {
                                // TODO
                                LogMessage("Import layer ", static_cast<uint32_t>(layer));
                            },
                            buttonTooltip);

                        layerManagerSizer->Add(
                            importButton,
                            wxGBPosition(iRow * 3 + 1, 1),
                            wxGBSpan(1, 1),
                            wxLEFT | wxRIGHT,
                            10);
                    }

                    // Delete
                    {
                        BitmapButton * deleteButton;

                        if (layer != LayerType::Structural)
                        {
                            deleteButton = new BitmapButton(
                                panel,
                                mResourceLocator.GetBitmapFilePath("delete_layer_button"),
                                [this, layer]()
                                {
                                    switch (layer)
                                    {
                                        case LayerType::Electrical:
                                        {
                                            mController->RemoveElectricalLayer();
                                            break;
                                        }

                                        case LayerType::Ropes:
                                        {
                                            mController->RemoveRopesLayer();
                                            break;
                                        }

                                        case LayerType::Structural:
                                        {
                                            assert(false);
                                            break;
                                        }

                                        case LayerType::Texture:
                                        {
                                            mController->RemoveTextureLayer();
                                            break;
                                        }
                                    }
                                },
                                _("Remove this layer."));

                            layerManagerSizer->Add(
                                deleteButton,
                                wxGBPosition(iRow * 3, 2),
                                wxGBSpan(1, 1),
                                0,
                                0);
                        }
                        else
                        {
                            deleteButton = nullptr;
                        }

                        mLayerDeleteButtons[iLayer] = deleteButton;
                    }

                    // Export
                    {
                        BitmapButton * exportButton;

                        if (layer == LayerType::Structural
                            || layer == LayerType::Texture)
                        {
                            exportButton = new BitmapButton(
                                panel,
                                mResourceLocator.GetBitmapFilePath("save_layer_button"),
                                [this, layer]()
                                {
                                    // TODO
                                    LogMessage("Export layer ", static_cast<uint32_t>(layer));
                                },
                                _("Export this layer to a file."));

                            layerManagerSizer->Add(
                                exportButton,
                                wxGBPosition(iRow * 3 + 1, 2),
                                wxGBSpan(1, 1),
                                0,
                                0);
                        }
                        else
                        {
                            exportButton = nullptr;
                        }

                        mLayerExportButtons[iLayer] = exportButton;
                    }

                    // Spacer
                    layerManagerSizer->Add(
                        new wxGBSizerItem(
                            -1,
                            12,
                            wxGBPosition(iRow * 3 + 2, 0),
                            wxGBSpan(1, LayerCount)));
                };

                createButtonRow(LayerType::Structural, 0);

                createButtonRow(LayerType::Electrical, 1);

                createButtonRow(LayerType::Ropes, 2);

                createButtonRow(LayerType::Texture, 3);
            }

            rootVSizer->Add(
                layerManagerSizer,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        // Misc
        {
            wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

            // Other layers opacity slider
            {
                mOtherLayersOpacitySlider = new wxSlider(panel, wxID_ANY, (MinLayerTransparency + MaxLayerTransparency) / 2, MinLayerTransparency, MaxLayerTransparency,
                    wxDefaultPosition, wxDefaultSize, wxSL_VERTICAL | wxSL_INVERSE);

                mOtherLayersOpacitySlider->Bind(
                    wxEVT_SLIDER,
                    [this](wxCommandEvent & /*event*/)
                    {
                        // TODO
                        LogMessage("Other layers opacity changed: ", mOtherLayersOpacitySlider->GetValue());
                    });

                hSizer->Add(
                    mOtherLayersOpacitySlider,
                    0, // Retain horizontal width
                    wxEXPAND | wxALIGN_TOP, // Expand vertically
                    0);
            }

            // View modifiers
            {
                wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

                // View grid button
                {
                    auto bitmap = WxHelpers::LoadBitmap("view_grid_button", mResourceLocator);
                    auto viewGridButton = new wxBitmapToggleButton(panel, wxID_ANY, bitmap);
                    auto buttonSize = bitmap.GetSize();
                    buttonSize.IncBy(4, 4);
                    viewGridButton->SetMaxSize(buttonSize);
                    viewGridButton->SetToolTip(_("Enable/Disable the visual guides."));
                    viewGridButton->Bind(
                        wxEVT_TOGGLEBUTTON,
                        [this, viewGridButton](wxCommandEvent & event)
                        {
                            assert(mController);
                            mController->EnableVisualGrid(event.IsChecked());

                            DeviateFocus();
                        });

                    vSizer->Add(
                        viewGridButton,
                        0, // Retain vertical width
                        wxALIGN_CENTER_HORIZONTAL, // Do not expand vertically
                        0);
                }

                hSizer->Add(
                    vSizer,
                    0, // Retain horizontal width
                    wxEXPAND | wxALIGN_TOP, // Expand vertically
                    0);
            }

            rootVSizer->Add(
                hSizer,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);
        }
    }

    panel->SetSizerAndFit(rootVSizer);

    return panel;
}

wxPanel * MainFrame::CreateToolbarPanel(wxWindow * parent)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    mToolbarPanelsSizer = new wxBoxSizer(wxVERTICAL);

    mToolbarPanelsSizer->AddSpacer(6);

    auto const makeToolButton = [this](
        ToolType tool,
        wxPanel * toolbarPanel,
        std::string iconName,
        wxString tooltip) -> BitmapToggleButton *
    {
        auto button = new BitmapToggleButton(
            toolbarPanel,
            mResourceLocator.GetIconFilePath(iconName),
            [this, tool]()
            {
                mController->SetCurrentTool(tool);
            },
            tooltip);

        mToolButtons[static_cast<size_t>(tool)] = button;

        return button;
    };

    //
    // Structural toolbar
    //

    {
        wxPanel * structuralToolbarPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);

        wxBoxSizer * structuralToolbarSizer = new wxBoxSizer(wxVERTICAL);

        // Tools

        {
            wxGridBagSizer * toolsSizer = new wxGridBagSizer(3, 3);

            // Pencil
            {
                auto button = makeToolButton(
                    ToolType::StructuralPencil,
                    structuralToolbarPanel,
                    "pencil_icon",
                    _("Draw individual structure particles"));

                toolsSizer->Add(
                    button,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    0,
                    0);
            }

            // Eraser
            {
                auto button = makeToolButton(
                    ToolType::StructuralEraser,
                    structuralToolbarPanel,
                    "eraser_icon",
                    _("Erase individual structure particles"));

                toolsSizer->Add(
                    button,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    0,
                    0);
            }

            structuralToolbarSizer->Add(
                toolsSizer,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        structuralToolbarSizer->AddSpacer(15);

        // Swaths

        {
            wxBoxSizer * paletteSizer = new wxBoxSizer(wxVERTICAL);

            // Foreground
            {
                mStructuralForegroundMaterialSelector = new wxStaticBitmap(
                    structuralToolbarPanel,
                    wxID_ANY,
                    WxHelpers::MakeEmptyBitmap(),
                    wxDefaultPosition,
                    wxSize(MaterialSwathSize.width, MaterialSwathSize.height),
                    wxBORDER_SUNKEN);

                mStructuralForegroundMaterialSelector->Bind(
                    wxEVT_LEFT_DOWN,
                    [this](wxMouseEvent & event)
                    {
                        OpenMaterialPalette(event, MaterialLayerType::Structural, MaterialPlaneType::Foreground);
                    });

                paletteSizer->Add(
                    mStructuralForegroundMaterialSelector,
                    0,
                    0,
                    0);
            }

            paletteSizer->AddSpacer(8);

            // Background
            {
                mStructuralBackgroundMaterialSelector = new wxStaticBitmap(
                    structuralToolbarPanel,
                    wxID_ANY,
                    WxHelpers::MakeEmptyBitmap(),
                    wxDefaultPosition,
                    wxSize(MaterialSwathSize.width, MaterialSwathSize.height),
                    wxBORDER_SUNKEN);

                mStructuralBackgroundMaterialSelector->Bind(
                    wxEVT_LEFT_DOWN,
                    [this](wxMouseEvent & event)
                    {
                        OpenMaterialPalette(event, MaterialLayerType::Structural, MaterialPlaneType::Background);
                    });

                paletteSizer->Add(
                    mStructuralBackgroundMaterialSelector,
                    0,
                    0,
                    0);
            }

            structuralToolbarSizer->Add(
                paletteSizer,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        structuralToolbarPanel->SetSizerAndFit(structuralToolbarSizer);

        mToolbarPanelsSizer->Add(
            structuralToolbarPanel,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);

        // Store toolbar panel
        mToolbarPanels[static_cast<size_t>(LayerType::Structural)] = structuralToolbarPanel;
    }

    //
    // Electrical toolbar
    //

    {
        wxPanel * electricalToolbarPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);

        wxBoxSizer * electricalToolbarSizer = new wxBoxSizer(wxVERTICAL);

        // Tools

        {
            wxGridBagSizer * toolsSizer = new wxGridBagSizer(3, 3);

            // Pencil
            {
                auto button = makeToolButton(
                    ToolType::ElectricalPencil,
                    electricalToolbarPanel,
                    "pencil_icon",
                    _("Draw individual electrical elements"));

                toolsSizer->Add(
                    button,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    0,
                    0);
            }

            // Eraser
            {
                auto button = makeToolButton(
                    ToolType::ElectricalEraser,
                    electricalToolbarPanel,
                    "eraser_icon",
                    _("Erase individual electrical elements"));

                toolsSizer->Add(
                    button,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    0,
                    0);
            }

            electricalToolbarSizer->Add(
                toolsSizer,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        electricalToolbarSizer->AddSpacer(15);

        // Swaths

        {
            wxBoxSizer * paletteSizer = new wxBoxSizer(wxVERTICAL);

            // Foreground
            {
                mElectricalForegroundMaterialSelector = new wxStaticBitmap(
                    electricalToolbarPanel,
                    wxID_ANY,
                    WxHelpers::MakeEmptyBitmap(),
                    wxDefaultPosition,
                    wxSize(MaterialSwathSize.width, MaterialSwathSize.height),
                    wxBORDER_SUNKEN);

                mElectricalForegroundMaterialSelector->Bind(
                    wxEVT_LEFT_DOWN,
                    [this](wxMouseEvent & event)
                    {
                        OpenMaterialPalette(event, MaterialLayerType::Electrical, MaterialPlaneType::Foreground);
                    });

                paletteSizer->Add(
                    mElectricalForegroundMaterialSelector,
                    0,
                    0,
                    0);
            }

            paletteSizer->AddSpacer(8);

            // Background
            {
                mElectricalBackgroundMaterialSelector = new wxStaticBitmap(
                    electricalToolbarPanel,
                    wxID_ANY,
                    WxHelpers::MakeEmptyBitmap(),
                    wxDefaultPosition,
                    wxSize(MaterialSwathSize.width, MaterialSwathSize.height),
                    wxBORDER_SUNKEN);

                mElectricalBackgroundMaterialSelector->Bind(
                    wxEVT_LEFT_DOWN,
                    [this](wxMouseEvent & event)
                    {
                        OpenMaterialPalette(event, MaterialLayerType::Electrical, MaterialPlaneType::Background);
                    });

                paletteSizer->Add(
                    mElectricalBackgroundMaterialSelector,
                    0,
                    0,
                    0);
            }

            electricalToolbarSizer->Add(
                paletteSizer,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        electricalToolbarPanel->SetSizerAndFit(electricalToolbarSizer);

        mToolbarPanelsSizer->Add(
            electricalToolbarPanel,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);

        // Store toolbar panel
        mToolbarPanels[static_cast<size_t>(LayerType::Electrical)] = electricalToolbarPanel;
    }

    //
    // Ropes toolbar
    //

    {
        wxPanel * ropesToolbarPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);

        wxBoxSizer * ropesToolbarSizer = new wxBoxSizer(wxVERTICAL);

        // Tools

        {
            wxGridBagSizer * toolsSizer = new wxGridBagSizer(3, 3);

            /* TODOHERE
            // Rope
            {
                auto button = makeToolButton(
                    ToolType::RopesHarness,
                    ropesToolbarPanel,
                    "pencil_icon", // // TODOHERE
                    _("Draw a rope between two endpoints"));

                toolsSizer->Add(
                    button,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    0,
                    0);
            }

            // Eraser
            {
                auto button = makeToolButton(
                    ToolType::RopesEraser,
                    ropesToolbarPanel,
                    "eraser_icon",
                    _("Erase rope endpoints"));

                toolsSizer->Add(
                    button,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    0,
                    0);
            }
            */

            ropesToolbarSizer->Add(
                toolsSizer,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        ropesToolbarPanel->SetSizerAndFit(ropesToolbarSizer);

        mToolbarPanelsSizer->Add(
            ropesToolbarPanel,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);

        // Store toolbar panel
        mToolbarPanels[static_cast<size_t>(LayerType::Ropes)] = ropesToolbarPanel;
    }

    //
    // Texture toolbar
    //

    {
        wxPanel * textureToolbarPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);

        wxBoxSizer * textureToolbarSizer = new wxBoxSizer(wxVERTICAL);

        // Tools

        {
            wxGridBagSizer * toolsSizer = new wxGridBagSizer(3, 3);

            // Move
            {
                // TODO
            }

            textureToolbarSizer->Add(
                toolsSizer,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        textureToolbarPanel->SetSizerAndFit(textureToolbarSizer);

        mToolbarPanelsSizer->Add(
            textureToolbarPanel,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);

        // Store toolbar panel
        mToolbarPanels[static_cast<size_t>(LayerType::Texture)] = textureToolbarPanel;
    }

    panel->SetSizerAndFit(mToolbarPanelsSizer);

    return panel;
}

wxPanel * MainFrame::CreateWorkPanel(wxWindow * parent)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxFlexGridSizer * sizer = new wxFlexGridSizer(2, 2, 0, 0);

    // GL Canvas
    {
        //
        // Create GL Canvas
        //

        int glCanvasAttributes[] =
        {
            WX_GL_RGBA,
            WX_GL_DOUBLEBUFFER,
            WX_GL_DEPTH_SIZE, 16,
            0, 0
        };

        mWorkCanvas = std::make_unique<wxGLCanvas>(panel, wxID_ANY, glCanvasAttributes);

        mWorkCanvas->Connect(wxEVT_PAINT, (wxObjectEventFunction)&MainFrame::OnWorkCanvasPaint, 0, this);
        mWorkCanvas->Connect(wxEVT_SIZE, (wxObjectEventFunction)&MainFrame::OnWorkCanvasResize, 0, this);
        mWorkCanvas->Connect(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&MainFrame::OnWorkCanvasLeftDown, 0, this);
        mWorkCanvas->Connect(wxEVT_LEFT_UP, (wxObjectEventFunction)&MainFrame::OnWorkCanvasLeftUp, 0, this);
        mWorkCanvas->Connect(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&MainFrame::OnWorkCanvasRightDown, 0, this);
        mWorkCanvas->Connect(wxEVT_RIGHT_UP, (wxObjectEventFunction)&MainFrame::OnWorkCanvasRightUp, 0, this);
        mWorkCanvas->Connect(wxEVT_MOTION, (wxObjectEventFunction)&MainFrame::OnWorkCanvasMouseMove, 0, this);
        mWorkCanvas->Connect(wxEVT_MOUSEWHEEL, (wxObjectEventFunction)&MainFrame::OnWorkCanvasMouseWheel, 0, this);
        mWorkCanvas->Connect(wxEVT_MOUSE_CAPTURE_LOST, (wxObjectEventFunction)&MainFrame::OnWorkCanvasCaptureMouseLost, 0, this);
        mWorkCanvas->Connect(wxEVT_LEAVE_WINDOW, (wxObjectEventFunction)&MainFrame::OnWorkCanvasMouseLeftWindow, 0, this);
        mWorkCanvas->Connect(wxEVT_ENTER_WINDOW, (wxObjectEventFunction)&MainFrame::OnWorkCanvasMouseEnteredWindow, 0, this);

        sizer->Add(
            mWorkCanvas.get(),
            1, // Occupy all space
            wxEXPAND, // Stretch as much as available
            0);

        //
        // Create GL context, and make it current on the canvas
        //

        mGLContext = std::make_unique<wxGLContext>(mWorkCanvas.get());
        mGLContext->SetCurrent(*mWorkCanvas);
    }

    auto const onScroll = [this]()
    {
        if (mController)
        {
            mController->SetCamera(
                mWorkCanvasHScrollBar->GetThumbPosition(),
                mWorkCanvasVScrollBar->GetThumbPosition());
        }
    };

    // V-scrollbar

    {
        mWorkCanvasVScrollBar = new wxScrollBar(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);

        mWorkCanvasVScrollBar->Bind(
            wxEVT_SCROLL_THUMBTRACK,
            [onScroll](wxScrollEvent & /*event*/)
            {
                onScroll();
            });

        mWorkCanvasVScrollBar->Bind(
            wxEVT_SCROLL_CHANGED,
            [onScroll](wxScrollEvent & /*event*/)
            {
                onScroll();
            });

        sizer->Add(
            mWorkCanvasVScrollBar,
            1, // Occupy all space
            wxEXPAND, // Stretch as much as available
            0);
    }

    // H-scrollbar

    {
        mWorkCanvasHScrollBar = new wxScrollBar(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_HORIZONTAL);

        mWorkCanvasHScrollBar->Bind(
            wxEVT_SCROLL_THUMBTRACK,
            [onScroll](wxScrollEvent & /*event*/)
            {
                onScroll();
            });

        mWorkCanvasHScrollBar->Bind(
            wxEVT_SCROLL_CHANGED,
            [onScroll](wxScrollEvent & /*event*/)
            {
                onScroll();
            });

        sizer->Add(
            mWorkCanvasHScrollBar,
            1, // Occupy all space
            wxEXPAND, // Stretch as much as available
            0);
    }

    sizer->AddGrowableCol(0);
    sizer->AddGrowableRow(0);

    panel->SetSizer(sizer);

    return panel;
}

wxPanel * MainFrame::CreateToolSettingsToolSizePanel(
    wxWindow * parent,
    wxString const & label,
    wxString const & tooltip,
    std::uint32_t minValue,
    std::uint32_t maxValue,
    std::uint32_t currentValue,
    std::function<void(std::uint32_t)> onValue)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

    // Label
    {
        auto * staticText = new wxStaticText(panel, wxID_ANY, label);

        sizer->Add(
            staticText,
            0,
            wxALIGN_CENTER_VERTICAL,
            4);
    }

    {
        EditSpinBox<std::uint32_t> * editSpinBox = new EditSpinBox<std::uint32_t>(
            panel,
            40,
            minValue,
            maxValue,
            currentValue,
            tooltip,
            std::move(onValue));

        sizer->Add(
            editSpinBox,
            0,
            wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT,
            4);
    }

    panel->SetSizerAndFit(sizer);

    return panel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MainFrame::OnWorkCanvasPaint(wxPaintEvent & /*event*/)
{
    if (mView)
    {
        mView->Render();
    }
}

void MainFrame::OnWorkCanvasResize(wxSizeEvent & event)
{
    LogMessage("OnWorkCanvasResize: ", event.GetSize().GetX(), "x", event.GetSize().GetY());

    if (mController)
    {
        mController->OnWorkCanvasResized(
            DisplayLogicalSize(
                event.GetSize().GetX(),
                event.GetSize().GetY()));
    }

    // Allow resizing to occur, this is a hook
    event.Skip();
}

void MainFrame::OnWorkCanvasLeftDown(wxMouseEvent & /*event*/)
{
    // First of all, set focus on the canvas if it has lost it - we want
    // it to receive all mouse events
    if (!mWorkCanvas->HasFocus())
    {
        mWorkCanvas->SetFocus();
    }

    if (mController)
    {
        mController->OnLeftMouseDown();
    }

    // Hang on to the mouse for as long as the button is pressed
    if (!mIsMouseCapturedByWorkCanvas)
    {
        LogMessage("TODOTEST: Captured");
        mWorkCanvas->CaptureMouse();
        mIsMouseCapturedByWorkCanvas = true;
    }
}

void MainFrame::OnWorkCanvasLeftUp(wxMouseEvent & /*event*/)
{
    // We can now release the mouse
    if (mIsMouseCapturedByWorkCanvas)
    {
        mWorkCanvas->ReleaseMouse();
        mIsMouseCapturedByWorkCanvas = false;

        LogMessage("TODOTEST: Released");
    }

    if (mController)
    {
        mController->OnLeftMouseUp();
    }
}

void MainFrame::OnWorkCanvasRightDown(wxMouseEvent & /*event*/)
{
    if (mController)
    {
        mController->OnRightMouseDown();
    }

    // Hang on to the mouse for as long as the button is pressed
    if (!mIsMouseCapturedByWorkCanvas)
    {
        LogMessage("TODOTEST: Captured");

        mWorkCanvas->CaptureMouse();
        mIsMouseCapturedByWorkCanvas = true;
    }
}

void MainFrame::OnWorkCanvasRightUp(wxMouseEvent & /*event*/)
{
    // We can now release the mouse
    if (mIsMouseCapturedByWorkCanvas)
    {
        mWorkCanvas->ReleaseMouse();
        mIsMouseCapturedByWorkCanvas = false;

        LogMessage("TODOTEST: Released");
    }

    if (mController)
    {
        mController->OnRightMouseUp();
    }
}

void MainFrame::OnWorkCanvasMouseMove(wxMouseEvent & event)
{
    if (mController)
    {
        mController->OnMouseMove(mView->ScreenToShipSpace({ event.GetX(), event.GetY() }));
    }
}

void MainFrame::OnWorkCanvasMouseWheel(wxMouseEvent & event)
{
    if (mController)
    {
        mController->AddZoom(event.GetWheelRotation() > 0 ? 1 : -1);
    }
}

void MainFrame::OnWorkCanvasCaptureMouseLost(wxMouseCaptureLostEvent & /*event*/)
{
    LogMessage("TODOTEST: CaptureMouseLost");

    if (mController)
    {
        mController->OnMouseCaptureLost();
    }
}

void MainFrame::OnWorkCanvasMouseLeftWindow(wxMouseEvent & /*event*/)
{
    LogMessage("TODOTEST: LeftWindow (captured=", mIsMouseCapturedByWorkCanvas, ")");

    if (!mIsMouseCapturedByWorkCanvas)
    {
        if (mController)
        {
            mController->OnUncapturedMouseOut();
        }
    }
}

void MainFrame::OnWorkCanvasMouseEnteredWindow(wxMouseEvent & /*event*/)
{
    LogMessage("TODOTEST: EnteredWindow (captured=", mIsMouseCapturedByWorkCanvas, ")");
}

void MainFrame::OnNewShip(wxCommandEvent & /*event*/)
{
    NewShip();
}

void MainFrame::OnLoadShip(wxCommandEvent & /*event*/)
{
    LoadShip();
}

void MainFrame::OnSaveShip(wxCommandEvent & /*event*/)
{
    SaveShip();
}

void MainFrame::OnSaveShipAs(wxCommandEvent & /*event*/)
{
    SaveShipAs();
}

void MainFrame::OnSaveAndGoBack(wxCommandEvent & /*event*/)
{
    SaveAndSwitchBackToGame();
}

void MainFrame::OnQuitAndGoBack(wxCommandEvent & /*event*/)
{
    QuitAndSwitchBackToGame();
}

void MainFrame::OnQuit(wxCommandEvent & /*event*/)
{
    // Close frame
    Close();
}

void MainFrame::OnClose(wxCloseEvent & event)
{
    if (event.CanVeto() && mController->GetModelController().GetModel().GetIsDirty())
    {
        // Ask user if they really want
        int result = AskUserIfSave(_("Do you want to save your changes before continuing?"));
        if (result == wxYES)
        {
            if (!SaveShip())
            {
                // Changed their mind
                result = wxCANCEL;
            }
        }

        if (result == wxCANCEL)
        {
            // Changed their mind
            event.Veto();
            return;
        }
    }

    // Nuke controller, now that all dependencies are still alive
    mController.reset();

    // Nuke view, now that the OpenGL context still works
    mView.reset();

    event.Skip();
}

void MainFrame::OnShipCanvasResize(wxCommandEvent & /*event*/)
{
    OpenShipCanvasResize();
}

void MainFrame::OnShipProperties(wxCommandEvent & /*event*/)
{
    OpenShipProperties();
}

void MainFrame::OnUndo(wxCommandEvent & /*event*/)
{
    mController->Undo();
}

void MainFrame::OnZoomIn(wxCommandEvent & /*event*/)
{
    assert(!!mController);
    mController->AddZoom(1);
}

void MainFrame::OnZoomOut(wxCommandEvent & /*event*/)
{
    assert(!!mController);
    mController->AddZoom(-1);
}

void MainFrame::OnResetView(wxCommandEvent & /*event*/)
{
    assert(!!mController);
    mController->ResetView();
}

void MainFrame::OnOpenLogWindowMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mLoggingDialog)
    {
        mLoggingDialog = std::make_unique<LoggingDialog>(this);
    }

    mLoggingDialog->Open();
}

void MainFrame::OnStructuralMaterialSelected(fsStructuralMaterialSelectedEvent & event)
{
    if (event.GetMaterialPlane() == MaterialPlaneType::Foreground)
    {
        mWorkbenchState.SetStructuralForegroundMaterial(event.GetMaterial());
    }
    else
    {
        assert(event.GetMaterialPlane() == MaterialPlaneType::Background);
        mWorkbenchState.SetStructuralBackgroundMaterial(event.GetMaterial());
    }

    ReconciliateUIWithWorkbenchState();
}

void MainFrame::OnElectricalMaterialSelected(fsElectricalMaterialSelectedEvent & event)
{
    if (event.GetMaterialPlane() == MaterialPlaneType::Foreground)
    {
        mWorkbenchState.SetElectricalForegroundMaterial(event.GetMaterial());
    }
    else
    {
        assert(event.GetMaterialPlane() == MaterialPlaneType::Background);
        mWorkbenchState.SetElectricalBackgroundMaterial(event.GetMaterial());
    }

    ReconciliateUIWithWorkbenchState();
}

void MainFrame::Open()
{
    assert(mController);

    // Show us
    Show(true);

    // Make ourselves the topmost frame
    mMainApp->SetTopWindow(this);
}

void MainFrame::NewShip()
{
    if (mController->GetModelController().GetModel().GetIsDirty())
    {
        // Ask user if they really want
        int result = AskUserIfSave(_("Do you want to save your changes before continuing?"));
        if (result == wxYES)
        {
            if (!SaveShip())
            {
                // Changed their mind
                result = wxCANCEL;
            }
        }

        if (result == wxCANCEL)
        {
            // Changed their mind
            return;
        }
    }

    // TODO: date in ship name, via static helper method
    DoNewShip("MyShip-TODO");
}

void MainFrame::LoadShip()
{
    if (mController->GetModelController().GetModel().GetIsDirty())
    {
        // Ask user if they really want
        int result = AskUserIfSave(_("Do you want to save your changes before continuing?"));
        if (result == wxYES)
        {
            if (!SaveShip())
            {
                // Changed their mind
                result = wxCANCEL;
            }
        }

        if (result == wxCANCEL)
        {
            // Changed their mind
            return;
        }
    }

    // Open ship load dialog

    if (!mShipLoadDialog)
    {
        mShipLoadDialog = std::make_unique<ShipLoadDialog>(
            this,
            mResourceLocator);
    }

    auto const res = mShipLoadDialog->ShowModal(mShipLoadDirectories);

    if (res == wxID_OK)
    {
        // Load ship
        auto const shipFilePath = mShipLoadDialog->GetChosenShipFilepath();
        DoLoadShip(shipFilePath);
    }
}

bool MainFrame::SaveShip()
{
    if (!mCurrentShipFilePath.has_value())
    {
        return SaveShipAs();
    }
    else
    {
        DoSaveShip(*mCurrentShipFilePath);
        return true;
    }
}

bool MainFrame::SaveShipAs()
{
    // Open ship save dialog

    if (!mShipSaveDialog)
    {
        mShipSaveDialog = std::make_unique<ShipSaveDialog>(this);
    }

    auto const res = mShipSaveDialog->ShowModal(
        mController->GetModelController().GetModel().GetShipMetadata().ShipName,
        ShipSaveDialog::GoalType::FullShip);

    if (res == wxID_OK)
    {
        // Save ship
        auto const shipFilePath = mShipSaveDialog->GetChosenShipFilepath();
        DoSaveShip(shipFilePath);
        return true;
    }
    else
    {
        return false;
    }
}

void MainFrame::SaveAndSwitchBackToGame()
{
    // Save/SaveAs
    if (SaveShip())
    {
        // Return
        assert(mCurrentShipFilePath.has_value());
        assert(ShipDeSerializer::IsShipDefinitionFile(*mCurrentShipFilePath));
        SwitchBackToGame(*mCurrentShipFilePath);
    }
}

void MainFrame::QuitAndSwitchBackToGame()
{
    if (mController->GetModelController().GetModel().GetIsDirty())
    {
        // Ask user if they really want
        int result = AskUserIfSave(_("Do you want to save your changes before continuing?"));
        if (result == wxYES)
        {
            if (!SaveShip())
            {
                // Changed their mind
                result = wxCANCEL;
            }
        }

        if (result == wxCANCEL)
        {
            // Changed their mind
            return;
        }
    }

    SwitchBackToGame(std::nullopt);
}

void MainFrame::SwitchBackToGame(std::optional<std::filesystem::path> shipFilePath)
{
    // Let go of controller
    mController.reset();

    // Hide self
    Show(false);

    // Invoke functor to go back
    assert(mReturnToGameFunctor);
    mReturnToGameFunctor(std::move(shipFilePath));
}

void MainFrame::OpenShipCanvasResize()
{
    if (!mShipCanvasResizeDialog)
    {
        mShipCanvasResizeDialog = std::make_unique<ShipCanvasResizeDialog>(this, mResourceLocator);
    }

    mShipCanvasResizeDialog->ShowModal(*mController);
}

void MainFrame::OpenShipProperties()
{
    if (!mShipPropertiesEditDialog)
    {
        mShipPropertiesEditDialog = std::make_unique<ShipPropertiesEditDialog>(this, mResourceLocator);
    }

    mShipPropertiesEditDialog->ShowModal(
        *mController,
        mController->GetModelController().GetModel().GetShipMetadata(),
        mController->GetModelController().GetModel().GetShipPhysicsData(),
        // TODOTEST: take from model
        std::nullopt,
        mController->GetModelController().GetModel().HasLayer(LayerType::Texture));
}

void MainFrame::OpenMaterialPalette(
    wxMouseEvent const & event,
    MaterialLayerType layer,
    MaterialPlaneType plane)
{
    wxWindow * referenceWindow = dynamic_cast<wxWindow *>(event.GetEventObject());
    if (nullptr == referenceWindow)
        return;

    auto const referenceRect = mWorkCanvas->GetScreenRect();

    if (layer == MaterialLayerType::Structural)
    {
        mStructuralMaterialPalette->Open(
            referenceRect,
            plane,
            plane == MaterialPlaneType::Foreground
            ? mWorkbenchState.GetStructuralForegroundMaterial()
            : mWorkbenchState.GetStructuralBackgroundMaterial());
    }
    else
    {
        assert(layer == MaterialLayerType::Electrical);

        mElectricalMaterialPalette->Open(
            referenceRect,
            plane,
            plane == MaterialPlaneType::Foreground
            ? mWorkbenchState.GetElectricalForegroundMaterial()
            : mWorkbenchState.GetElectricalBackgroundMaterial());
    }
}

bool MainFrame::AskUserIfSure(wxString caption)
{
    int result = wxMessageBox(caption, ApplicationName, wxICON_EXCLAMATION | wxOK | wxCANCEL | wxCENTRE);
    return (result == wxOK);
}

int MainFrame::AskUserIfSave(wxString caption)
{
    int result = wxMessageBox(caption, ApplicationName, wxICON_EXCLAMATION | wxYES_NO | wxCANCEL | wxCENTRE);
    return result;
}

void MainFrame::ShowError(wxString const & message)
{
    wxMessageBox(message, _("Maritime Disaster"), wxICON_ERROR);
}

void MainFrame::DoNewShip(std::string const & shipName)
{
    // Initialize controller (won't fire UI reconciliations)
    mController = Controller::CreateNew(
        shipName,
        *mView,
        mWorkbenchState,
        *this,
        mResourceLocator);

    // Reset current ship filename
    mCurrentShipFilePath.reset();

    // Reconciliate UI
    ReconciliateUI();
}

void MainFrame::DoLoadShip(std::filesystem::path const & shipFilePath)
{
    try
    {
        // Load ship
        ShipDefinition shipDefinition = ShipDeSerializer::LoadShip(shipFilePath, mMaterialDatabase);

        // Initialize controller with ship
        mController = Controller::CreateForShip(
            std::move(shipDefinition),
            *mView,
            mWorkbenchState,
            *this,
            mResourceLocator);

        // Remember file path - but only if it's a definition file in the "official" format, not a legacy one
        if (ShipDeSerializer::IsShipDefinitionFile(shipFilePath))
        {
            mCurrentShipFilePath = shipFilePath;
        }
        else
        {
            mCurrentShipFilePath.reset();
        }

        // Reconciliate UI
        ReconciliateUI();
    }
    catch (UserGameException const & exc)
    {
        ShowError(mLocalizationManager.MakeErrorMessage(exc));
    }
    catch (std::runtime_error const & exc)
    {
        ShowError(exc.what());
    }
}

void MainFrame::DoSaveShip(std::filesystem::path const & shipFilePath)
{
    assert(mController);

    // Get ship definition
    auto shipDefinition = mController->GetModelController().MakeShipDefinition();

    assert(ShipDeSerializer::IsShipDefinitionFile(shipFilePath));

    // Save ship
    ShipDeSerializer::SaveShip(
        shipDefinition,
        shipFilePath);

    // Reset current ship file path
    mCurrentShipFilePath = shipFilePath;

    // Clear dirtyness
    mController->ClearModelDirty();
}

void MainFrame::RecalculateWorkCanvasPanning()
{
    assert(mView);

    //
    // We populate the scollbar with work space coordinates
    //

    ShipSpaceCoordinates const cameraPos = mView->GetCameraShipSpacePosition();
    ShipSpaceSize const cameraThumbSize = mView->GetCameraThumbSize();
    ShipSpaceSize const cameraRange = mView->GetCameraRange();

    mWorkCanvasHScrollBar->SetScrollbar(
        cameraPos.x,
        cameraThumbSize.width,
        cameraRange.width,
        cameraThumbSize.width); // page size  == thumb

    mWorkCanvasVScrollBar->SetScrollbar(
        cameraPos.y,
        cameraThumbSize.height,
        cameraRange.height,
        cameraThumbSize.height); // page size  == thumb
}

void MainFrame::SetFrameTitle(std::string const & shipName, bool isDirty)
{
    std::string frameTitle = shipName;
    if (isDirty)
        frameTitle += "*";

    SetTitle(frameTitle);
}

void MainFrame::DeviateFocus()
{
    // Set focus on primary layer buttn
    uint32_t const iPrimaryLayer = static_cast<uint32_t>(mController->GetPrimaryLayer());
    mLayerSelectButtons[iPrimaryLayer]->SetFocus();
}

void MainFrame::ReconciliateUI()
{
    assert(mController);
    ReconciliateUIWithViewModel();
    ReconciliateUIWithShipMetadata(mController->GetModelController().GetModel().GetShipMetadata());
    ReconciliateUIWithLayerPresence();
    ReconciliateUIWithShipSize(mController->GetModelController().GetModel().GetShipSize());
    ReconciliateUIWithPrimaryLayerSelection(mController->GetPrimaryLayer());
    ReconciliateUIWithModelDirtiness();
    ReconciliateUIWithWorkbenchState();
    ReconciliateUIWithSelectedTool(mController->GetCurrentTool());
    ReconciliateUIWithUndoStackState();

    assert(mWorkCanvas);
    mWorkCanvas->Refresh();
}

void MainFrame::ReconciliateUIWithViewModel()
{
    RecalculateWorkCanvasPanning();

    // TODO: set zoom in StatusBar
}

void MainFrame::ReconciliateUIWithShipMetadata(ShipMetadata const & shipMetadata)
{
    SetFrameTitle(shipMetadata.ShipName, mController->GetModelController().GetModel().GetIsDirty());
}

void MainFrame::ReconciliateUIWithShipSize(ShipSpaceSize const & shipSize)
{
    assert(mController);

    RecalculateWorkCanvasPanning();

    // TODO: status bar
}

void MainFrame::ReconciliateUIWithLayerPresence()
{
    assert(mController);

    Model const & model = mController->GetModelController().GetModel();

    //
    // Rules
    //
    // Presence button: if HasLayer
    // New, Load: always
    // Delete, Save: if HasLayer
    // Slider: only enabled if > 1 layers
    //

    for (uint32_t iLayer = 0; iLayer < LayerCount; ++iLayer)
    {
        bool const hasLayer = model.HasLayer(static_cast<LayerType>(iLayer));

        mLayerSelectButtons[iLayer]->Enable(hasLayer);

        if (mLayerExportButtons[iLayer] != nullptr
            && mLayerExportButtons[iLayer]->IsEnabled() != hasLayer)
        {
            mLayerExportButtons[iLayer]->Enable(hasLayer);
        }

        if (mLayerDeleteButtons[iLayer] != nullptr
            && mLayerDeleteButtons[iLayer]->IsEnabled() != hasLayer)
        {
            mLayerDeleteButtons[iLayer]->Enable(hasLayer);
        }
    }

    mOtherLayersOpacitySlider->Enable(model.HasExtraLayers());

    mLayerSelectButtons[static_cast<size_t>(mController->GetPrimaryLayer())]->SetFocus(); // Prevent other random buttons for getting focus
}

void MainFrame::ReconciliateUIWithPrimaryLayerSelection(LayerType primaryLayer)
{
    assert(mController);

    // Toggle <select buttons, tool panels> <-> primary layer
    bool hasToggledToolPanel = false;
    assert(mController->GetModelController().GetModel().HasLayer(primaryLayer));
    uint32_t const iPrimaryLayer = static_cast<uint32_t>(primaryLayer);
    for (uint32_t iLayer = 0; iLayer < LayerCount; ++iLayer)
    {
        bool const isSelected = (iLayer == iPrimaryLayer);
        if (mLayerSelectButtons[iLayer]->GetValue() != isSelected)
        {
            mLayerSelectButtons[iLayer]->SetValue(isSelected);

            if (isSelected)
            {
                mLayerSelectButtons[iLayer]->SetFocus(); // Prevent other random buttons for getting focus
            }
        }

        if (mToolbarPanelsSizer->IsShown(mToolbarPanels[iLayer]) != isSelected)
        {
            mToolbarPanelsSizer->Show(mToolbarPanels[iLayer], isSelected);
            hasToggledToolPanel = true;
        }
    }

    if (hasToggledToolPanel)
    {
        mToolbarPanelsSizer->Layout();
    }
}

void MainFrame::ReconciliateUIWithModelDirtiness()
{
    assert(mController);

    bool const isDirty = mController->GetModelController().GetModel().GetIsDirty();

    if (mSaveShipMenuItem->IsEnabled() != isDirty)
    {
        mSaveShipMenuItem->Enable(isDirty);
    }

    if (mSaveAndGoBackMenuItem != nullptr
        && mSaveAndGoBackMenuItem->IsEnabled() != isDirty)
    {
        mSaveAndGoBackMenuItem->Enable(false);
    }

    if (mSaveShipButton->IsEnabled() != isDirty)
    {
        mSaveShipButton->Enable(isDirty);
    }

    SetFrameTitle(mController->GetModelController().GetModel().GetShipMetadata().ShipName, isDirty);
}

void MainFrame::ReconciliateUIWithWorkbenchState()
{
    assert(mController);

    // Populate swaths in toolbars
    {
        static std::string const ClearMaterialName = "Clear";

        auto const * foreStructuralMaterial = mWorkbenchState.GetStructuralForegroundMaterial();
        if (foreStructuralMaterial != nullptr)
        {
            wxBitmap foreStructuralBitmap = WxHelpers::MakeBitmap(
                mShipTexturizer.MakeTextureSample(
                    std::nullopt, // Use shared settings
                    MaterialSwathSize,
                    *foreStructuralMaterial));

            mStructuralForegroundMaterialSelector->SetBitmap(foreStructuralBitmap);
            mStructuralForegroundMaterialSelector->SetToolTip(foreStructuralMaterial->Name);
        }
        else
        {
            mStructuralForegroundMaterialSelector->SetBitmap(mNullMaterialBitmap);
            mStructuralForegroundMaterialSelector->SetToolTip(ClearMaterialName);
        }

        auto const * backStructuralMaterial = mWorkbenchState.GetStructuralBackgroundMaterial();
        if (backStructuralMaterial != nullptr)
        {
            wxBitmap backStructuralBitmap = WxHelpers::MakeBitmap(
                mShipTexturizer.MakeTextureSample(
                    std::nullopt, // Use shared settings
                    MaterialSwathSize,
                    *backStructuralMaterial));

            mStructuralBackgroundMaterialSelector->SetBitmap(backStructuralBitmap);
            mStructuralBackgroundMaterialSelector->SetToolTip(backStructuralMaterial->Name);
        }
        else
        {
            mStructuralBackgroundMaterialSelector->SetBitmap(mNullMaterialBitmap);
            mStructuralBackgroundMaterialSelector->SetToolTip(ClearMaterialName);
        }

        auto const * foreElectricalMaterial = mWorkbenchState.GetElectricalForegroundMaterial();
        if (foreElectricalMaterial != nullptr)
        {
            wxBitmap foreElectricalBitmap = WxHelpers::MakeMatteBitmap(
                rgbaColor(foreElectricalMaterial->RenderColor, 255),
                MaterialSwathSize);

            mElectricalForegroundMaterialSelector->SetBitmap(foreElectricalBitmap);
            mElectricalForegroundMaterialSelector->SetToolTip(foreElectricalMaterial->Name);
        }
        else
        {
            mElectricalForegroundMaterialSelector->SetBitmap(mNullMaterialBitmap);
            mElectricalForegroundMaterialSelector->SetToolTip(ClearMaterialName);
        }

        auto const * backElectricalMaterial = mWorkbenchState.GetElectricalBackgroundMaterial();
        if (backElectricalMaterial != nullptr)
        {
            wxBitmap backElectricalBitmap = WxHelpers::MakeMatteBitmap(
                rgbaColor(backElectricalMaterial->RenderColor, 255),
                MaterialSwathSize);

            mElectricalBackgroundMaterialSelector->SetBitmap(backElectricalBitmap);
            mElectricalBackgroundMaterialSelector->SetToolTip(backElectricalMaterial->Name);
        }
        else
        {
            mElectricalBackgroundMaterialSelector->SetBitmap(mNullMaterialBitmap);
            mElectricalBackgroundMaterialSelector->SetToolTip(ClearMaterialName);
        }
    }

    // TODO: Populate settings in ToolSettings toolbar
}

void MainFrame::ReconciliateUIWithSelectedTool(std::optional<ToolType> tool)
{
    assert(mController);

    // Select this tool's button and unselect the others
    for (size_t i = 0; i < mToolButtons.size(); ++i)
    {
        bool const isSelected = (tool.has_value() && i == static_cast<size_t>(*tool));

        if (mToolButtons[i]->GetValue() != isSelected)
        {
            mToolButtons[i]->SetValue(isSelected);
        }
    }

    // Show this tool's settings panel and hide the others
    for (auto const & entry : mToolSettingsPanels)
    {
        bool const isSelected = (tool.has_value() && std::get<0>(entry) == *tool);

        mToolSettingsPanelsSizer->Show(std::get<1>(entry), isSelected);
    }

    mToolSettingsPanelsSizer->Layout();
}

void MainFrame::ReconciliateUIWithUndoStackState()
{
    assert(mController);

    bool const canUndo = mController->CanUndo();
    if (mUndoMenuItem->IsEnabled() != canUndo)
    {
        mUndoMenuItem->Enable(canUndo);
    }
}

}