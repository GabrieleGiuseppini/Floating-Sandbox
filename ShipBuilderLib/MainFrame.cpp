/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-08-27
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MainFrame.h"

#include <GameCore/ImageSize.h>
#include <GameCore/Log.h>
#include <GameCore/Version.h>

#include <GameOpenGL/GameOpenGL.h>

#include <Game/ImageFileTools.h>

#include <UILib/BitmapButton.h>
#include <UILib/BitmapToggleButton.h>
#include <UILib/WxHelpers.h>

#include <wx/button.h>
#include <wx/gbsizer.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/tglbtn.h>

#ifdef _MSC_VER
 // Nothing to do here - we use RC files
#else
#include "Resources/ShipBBB.xpm"
#endif

#include <cassert>
#include <sstream>

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
    // Row 0: [File Panel] [Tool Settings] [Game Panel]
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

        row0HSizer->AddStretchSpacer(1);

        // Tool settings panel
        {
            wxPanel * toolSettingsPanel = CreateToolSettingsPanel(mMainPanel);

            row0HSizer->Add(
                toolSettingsPanel,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);
        }

        row0HSizer->AddStretchSpacer(1);

        // Game panel
        {
            wxPanel * gamePanel = CreateGamePanel(mMainPanel);

            row0HSizer->Add(
                gamePanel,
                0,
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
            mStatusBar = new wxStatusBar(mMainPanel, wxID_ANY, 0);
            mStatusBar->SetFieldsCount(1);

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
            wxMenuItem * newShipMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("New Ship\tCtrl+N"), _("Create a new empty ship"), wxITEM_NORMAL);
            fileMenu->Append(newShipMenuItem);
            Connect(newShipMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnNewShip);
        }

        {
            wxMenuItem * loadShipMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Load Ship\tCtrl+O"), _("Load a ship"), wxITEM_NORMAL);
            fileMenu->Append(loadShipMenuItem);
            Connect(loadShipMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnLoadShip);
        }

        {
            mSaveShipMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Save Ship\tCtrl+S"), _("Save the current ship"), wxITEM_NORMAL);
            fileMenu->Append(mSaveShipMenuItem);
            Connect(mSaveShipMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSaveShip);
        }

        if (!IsStandAlone())
        {
            mSaveAndGoBackMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Save and Return to Game"), _("Save the current ship and return to the simulator"), wxITEM_NORMAL);
            fileMenu->Append(mSaveAndGoBackMenuItem);
            Connect(mSaveAndGoBackMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSaveAndGoBack);
        }
        else
        {
            mSaveAndGoBackMenuItem = nullptr;
        }

        if (!IsStandAlone())
        {
            wxMenuItem * quitAndGoBackMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Quit and Return to Game"), _("Discard the current ship and return to the simulator"), wxITEM_NORMAL);
            fileMenu->Append(quitAndGoBackMenuItem);
            Connect(quitAndGoBackMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnQuitAndGoBack);
        }

        if (IsStandAlone())
        {
            wxMenuItem * quitMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Quit") + wxS("\tAlt-F4"), _("Quit the builder"), wxITEM_NORMAL);
            fileMenu->Append(quitMenuItem);
            Connect(quitMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnQuit);
        }

        mainMenuBar->Append(fileMenu, _("&File"));
    }

    // Edit
    {
        wxMenu * editMenu = new wxMenu();

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
        WorkSpaceSize(0, 0), // We don't have a workspace yet
        DisplayLogicalSize(
            mWorkCanvas->GetSize().GetWidth(),
            mWorkCanvas->GetSize().GetHeight()),
        mWorkCanvas->GetContentScaleFactor(),
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
    // Shut off UI updated during new controller creation
    mController.reset();

    // Create controller - will fire UI reconciliations
    mController = Controller::CreateNew(
        *mView,
        mWorkbenchState,
        *this);

    // Open ourselves
    Open();
}

void MainFrame::OpenForLoadShip(std::filesystem::path const & shipFilePath)
{
    // Load ship
    // TODO: we load the ship here, so errors may occur while we still have a controller;
    // then, pass loaded data to Controller

    // Shut off UI updated during new controller creation
    mController.reset();

    // Create controller - will fire UI reconciliations
    mController = Controller::CreateForShip(
        /* TODO: loaded ship ,*/
        *mView,
        mWorkbenchState,
        *this);

    // Open ourselves
    Open();
}

//
// IUserInterface
//

void MainFrame::RefreshView()
{
    Refresh();
}

void MainFrame::OnViewModelChanged()
{
    if (mController)
    {
        RecalculateWorkCanvasPanning();
    }
}

void MainFrame::OnWorkSpaceSizeChanged(WorkSpaceSize const & workSpaceSize)
{
    if (mController)
    {
        ReconciliateUIWithWorkSpaceSize(workSpaceSize);
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

void MainFrame::OnModelDirtyChanged(bool isDirty)
{
    if (mController)
    {
        ReconciliateUIWithModelDirtiness(isDirty);
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

void MainFrame::OnToolCoordinatesChanged(std::optional<WorkSpaceCoordinates> coordinates)
{
    std::stringstream ss;

    if (coordinates.has_value())
    {
        ss << coordinates->x << ", " << coordinates->y;
    }

    mStatusBar->SetStatusText(ss.str(), 0);
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
                    wxCommandEvent dummy;
                    OnNewShip(dummy);
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
                    wxCommandEvent dummy;
                    OnNewShip(dummy);
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
                    wxCommandEvent dummy;
                    OnSaveShip(dummy);
                },
                _("Save the current ship"));

            sizer->Add(mSaveShipButton, 0, wxALL, ButtonMargin);
        }
    }

    panel->SetSizerAndFit(sizer);

    return panel;
}

wxPanel * MainFrame::CreateToolSettingsPanel(wxWindow * parent)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

    {
        {
            wxButton * button = new wxButton(panel, wxID_ANY, "Some");
            sizer->Add(button, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
        }

        {
            wxButton * button = new wxButton(panel, wxID_ANY, "Tool");
            sizer->Add(button, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
        }

        {
            wxButton * button = new wxButton(panel, wxID_ANY, "Settings");
            sizer->Add(button, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
        }
    }

    panel->SetSizerAndFit(sizer);

    return panel;
}

wxPanel * MainFrame::CreateGamePanel(wxWindow * parent)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

    {
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
                                            mController->NewStructuralLayer();
                                            break;
                                        }

                                        case LayerType::Texture:
                                        {
                                            assert(false);
                                            break;
                                        }
                                    }

                                    // Switch primary layer to this one
                                    mController->SelectPrimaryLayer(layer);
                                },
                                "Make a new empty layer");

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

                    {
                        auto * loadButton = new BitmapButton(
                            panel,
                            mResourceLocator.GetBitmapFilePath("open_layer_button"),
                            [this, layer]()
                            {
                                // TODO
                                LogMessage("Open layer ", static_cast<uint32_t>(layer));
                            },
                            "Import this layer from a file");

                        layerManagerSizer->Add(
                            loadButton,
                            wxGBPosition(iRow * 3 + 1, 1),
                            wxGBSpan(1, 1),
                            wxLEFT | wxRIGHT,
                            10);
                    }

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

                                    // Switch primary layer if it was this one
                                    if (mController->GetPrimaryLayer() == layer)
                                    {
                                        mController->SelectPrimaryLayer(LayerType::Structural);
                                    }
                                },
                                "Remove this layer");

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

                    {
                        auto * saveButton = new BitmapButton(
                            panel,
                            mResourceLocator.GetBitmapFilePath("save_layer_button"),
                            [this, layer]()
                            {
                                // TODO
                                LogMessage("Save layer ", static_cast<uint32_t>(layer));
                            },
                            "Export this layer to a file");

                        layerManagerSizer->Add(
                            saveButton,
                            wxGBPosition(iRow * 3 + 1, 2),
                            wxGBSpan(1, 1),
                            0,
                            0);

                        mLayerSaveButtons[iLayer] = saveButton;
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

            rootVSizer->Add(
                mOtherLayersOpacitySlider,
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
                mController->SetTool(tool);
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
                    wxSize(MaterialSwathSize.Width, MaterialSwathSize.Height),
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
                    wxSize(MaterialSwathSize.Width, MaterialSwathSize.Height),
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
                    wxSize(MaterialSwathSize.Width, MaterialSwathSize.Height),
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
                    wxSize(MaterialSwathSize.Width, MaterialSwathSize.Height),
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

    // V-scrollbar

    {
        mWorkCanvasVScrollBar = new wxScrollBar(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);

        mWorkCanvasVScrollBar->Bind(
            wxEVT_SCROLL_THUMBTRACK,
            [this](wxScrollEvent & /*event*/)
            {
                if (mController)
                {
                    mController->SetCamera(
                        mWorkCanvasHScrollBar->GetThumbPosition(),
                        mWorkCanvasVScrollBar->GetThumbPosition());
                }
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
            [this](wxScrollEvent & /*event*/)
            {
                if (mController)
                {
                    mController->SetCamera(
                        mWorkCanvasHScrollBar->GetThumbPosition(),
                        mWorkCanvasVScrollBar->GetThumbPosition());
                }
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

void MainFrame::OnWorkCanvasPaint(wxPaintEvent & /*event*/)
{
    LogMessage("OnWorkCanvasPaint");

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
        mController->OnMouseMove(DisplayLogicalCoordinates(event.GetX(), event.GetY()));
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
    if (mController)
    {
        mController->OnMouseOut();
    }
}

void MainFrame::OnWorkCanvasMouseLeftWindow(wxMouseEvent & /*event*/)
{
    if (!mIsMouseCapturedByWorkCanvas)
    {
        if (mController)
        {
            mController->OnMouseOut();
        }
    }
}

void MainFrame::OnNewShip(wxCommandEvent & /*event*/)
{
    if (mController->GetModelController().GetModel().GetIsDirty())
    {
        // Ask user if they really want
        if (!AskUserIfSure(_("Are you sure you want to discard your changes?")))
        {
            return;
        }
    }

    // TODO
}

void MainFrame::OnLoadShip(wxCommandEvent & /*event*/)
{
    if (mController->GetModelController().GetModel().GetIsDirty())
    {
        // Ask user if they really want
        if (!AskUserIfSure(_("Are you sure you want to discard your changes?")))
        {
            return;
        }
    }

    // TODO
}

void MainFrame::OnSaveShip(wxCommandEvent & /*event*/)
{
    // TODO
}

void MainFrame::OnSaveAndGoBack(wxCommandEvent & /*event*/)
{
    SaveAndSwitchBackToGame();
}

void MainFrame::OnQuitAndGoBack(wxCommandEvent & /*event*/)
{
    if (mController->GetModelController().GetModel().GetIsDirty())
    {
        // Ask user if they really want
        if (!AskUserIfSure(_("Are you sure you want to discard your changes?")))
        {
            return;
        }
    }

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
        if (!AskUserIfSure(_("Are you sure you want to discard your changes?")))
        {
            event.Veto();
            return;
        }
    }

    event.Skip();
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

    // Sync UI
    ReconciliateUI();

    // Show us
    Show(true);

    // Make ourselves the topmost frame
    mMainApp->SetTopWindow(this);
}

void MainFrame::SaveAndSwitchBackToGame()
{
    // TODO: SaveShipDialog
    // TODO: if success: save via Controller::SaveShip() and provide new file path
    // TODO: else: cancel operation (i.e. nop)

    std::filesystem::path const TODOPath = mResourceLocator.GetInstalledShipFolderPath() / "Lifeboat.shp";
    SwitchBackToGame(TODOPath);
}

void MainFrame::QuitAndSwitchBackToGame()
{
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
    int result = wxMessageBox(_("Are you sure you want to discard your changes?"), _("Are You Sure?"), wxOK | wxCANCEL);
    return (result == wxOK);
}

void MainFrame::RecalculateWorkCanvasPanning()
{
    assert(mView);

    //
    // We populate the scollbar with work space coordinates
    //

    WorkSpaceCoordinates const cameraPos = mView->GetCameraWorkSpacePosition();
    WorkSpaceSize const cameraThumbSize = mView->GetCameraThumbSize();
    WorkSpaceSize const cameraRange = mView->GetCameraRange();

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

void MainFrame::ReconciliateUI()
{
    assert(!!mController);

    ReconciliateUIWithLayerPresence();
    ReconciliateUIWithWorkSpaceSize(mController->GetModelController().GetModel().GetWorkSpaceSize());
    ReconciliateUIWithPrimaryLayerSelection(mController->GetPrimaryLayer());
    ReconciliateUIWithModelDirtiness(mController->GetModelController().GetModel().GetIsDirty());
    ReconciliateUIWithWorkbenchState();
    ReconciliateUIWithSelectedTool(mController->GetTool());
}

void MainFrame::ReconciliateUIWithWorkSpaceSize(WorkSpaceSize const & workSpaceSize)
{
    assert(mController);

    // Notify view of new workspace size
    // Note: might cause a view model change that would not be
    // notified via OnViewModelChanged
    mView->SetWorkSpaceSize(workSpaceSize);

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

        if (mLayerSaveButtons[iLayer] != nullptr
            && mLayerSaveButtons[iLayer]->IsEnabled() != hasLayer)
        {
            mLayerSaveButtons[iLayer]->Enable(hasLayer);
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

void MainFrame::ReconciliateUIWithModelDirtiness(bool isDirty)
{
    assert(mController);

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
                rgbaColor(foreElectricalMaterial->RenderColor),
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
                rgbaColor(backElectricalMaterial->RenderColor),
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
}

}