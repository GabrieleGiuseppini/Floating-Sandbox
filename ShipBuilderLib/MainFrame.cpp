/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-08-27
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MainFrame.h"

#include "AskPasswordDialog.h"

#include <GameCore/Log.h>
#include <GameCore/UserGameException.h>
#include <GameCore/Utils.h>
#include <GameCore/Version.h>

#include <GameOpenGL/GameOpenGL.h>

#include <Game/ImageFileTools.h>
#include <Game/ShipDeSerializer.h>

#include <UILib/BitmapButton.h>
#include <UILib/BitmapToggleButton.h>
#include <UILib/HighlightableTextButton.h>
#include <UILib/EditSpinBox.h>
#include <UILib/ImageLoadDialog.h>
#include <UILib/UnderConstructionDialog.h>
#include <UILib/WxHelpers.h>

#include <wx/button.h>
#include <wx/cursor.h>
#include <wx/gbsizer.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/tglbtn.h>
#include <wx/utils.h>
#include <wx/wupdlock.h>

#ifdef _MSC_VER
 // Nothing to do here - we use RC files
#else
#include "Resources/ShipBBB.xpm"
#endif

#include <cassert>

namespace ShipBuilder {


int constexpr ButtonMargin = 4;

ImageSize constexpr MaterialSwathSize(80, 100);

int constexpr MaxLayerTransparency = 128;

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
    , mIsMouseInWorkCanvas(false)
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
                4 * ButtonMargin);
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
                4 * ButtonMargin);
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
                    wxPanel * layersVisualizationPanel = CreateLayersVisualizationPanel(mMainPanel);

                    tmpVSizer->Add(
                        layersVisualizationPanel,
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

            // Separator
            {
                wxStaticLine * line = new wxStaticLine(mMainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);

                row1Col0VSizer->Add(
                    line,
                    0,
                    wxTOP | wxBOTTOM | wxEXPAND,
                    2);
            }

            // Undo panel
            {
                wxPanel * undoPanel = CreateUndoPanel(mMainPanel);

                row1Col0VSizer->Add(
                    undoPanel,
                    1, // Expand vertically
                    wxLEFT | wxRIGHT | wxEXPAND, // Expand horizontally
                    4);
            }

            row1HSizer->Add(
                row1Col0VSizer,
                0,
                wxEXPAND,
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
            Connect(mSaveShipMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSaveShipMenuItem);
        }

        if (!IsStandAlone())
        {
            mSaveAndGoBackMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Save Ship and Return to Game"), _("Save the current ship and return to the simulator"), wxITEM_NORMAL);
            fileMenu->Append(mSaveAndGoBackMenuItem);
            Connect(mSaveAndGoBackMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSaveAndGoBackMenuItem);
        }
        else
        {
            mSaveAndGoBackMenuItem = nullptr;
        }

        {
            wxMenuItem * saveShipAsMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Save Ship As"), _("Save the current ship to a different file"), wxITEM_NORMAL);
            fileMenu->Append(saveShipAsMenuItem);
            Connect(saveShipAsMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSaveShipAsMenuItem);
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
            mUndoMenuItem = new wxMenuItem(editMenu, wxID_ANY, _("Undo") + wxS("\tCtrl+Z"), _("Undo the last edit operation"), wxITEM_NORMAL);
            editMenu->Append(mUndoMenuItem);
            Connect(mUndoMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnEditUndoMenuItem);
        }

        editMenu->Append(new wxMenuItem(editMenu, wxID_SEPARATOR));

        {
            wxMenuItem * menuItem = new wxMenuItem(editMenu, wxID_ANY, _("Flip Ship Horizontally"), _("Flip the ship horizontally"), wxITEM_NORMAL);
            editMenu->Append(menuItem);
            Connect(menuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnEditFlipHorizontallyMenuItem);
        }

        {
            wxMenuItem * menuItem = new wxMenuItem(editMenu, wxID_ANY, _("Flip Ship Vertically"), _("Flip the ship vertically"), wxITEM_NORMAL);
            editMenu->Append(menuItem);
            Connect(menuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnEditFlipVerticallyMenuItem);
        }

        {
            wxMenuItem * menuItem = new wxMenuItem(editMenu, wxID_ANY, _("Resize Ship") + wxS("\tCtrl+R"), _("Resize the ship"), wxITEM_NORMAL);
            editMenu->Append(menuItem);
            Connect(menuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnEditResizeShipMenuItem);
        }

        {
            wxMenuItem * menuItem = new wxMenuItem(editMenu, wxID_ANY, _("Ship Properties") + wxS("\tCtrl+P"), _("Edit the ship properties"), wxITEM_NORMAL);
            editMenu->Append(menuItem);
            Connect(menuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnEditShipPropertiesMenuItem);
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

    mStructuralMaterialPalette = std::make_unique<MaterialPalette<LayerType::Structural>>(
        this,
        mMaterialDatabase.GetStructuralMaterialPalette(),
        mShipTexturizer,
        mResourceLocator);

    mStructuralMaterialPalette->Bind(fsEVT_STRUCTURAL_MATERIAL_SELECTED, &MainFrame::OnStructuralMaterialSelected, this);

    mElectricalMaterialPalette = std::make_unique<MaterialPalette<LayerType::Electrical>>(
        this,
        mMaterialDatabase.GetElectricalMaterialPalette(),
        mShipTexturizer,
        mResourceLocator);

    mElectricalMaterialPalette->Bind(fsEVT_ELECTRICAL_MATERIAL_SELECTED, &MainFrame::OnElectricalMaterialSelected, this);

    mRopesMaterialPalette = std::make_unique<MaterialPalette<LayerType::Ropes>>(
        this,
        mMaterialDatabase.GetRopeMaterialPalette(),
        mShipTexturizer,
        mResourceLocator);

    mRopesMaterialPalette->Bind(fsEVT_STRUCTURAL_MATERIAL_SELECTED, &MainFrame::OnRopeMaterialSelected, this);

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
        [this]()
        {
            mWorkCanvas->SwapBuffers();
        },
        mResourceLocator);

    mView->UploadBackgroundTexture(
        ImageFileTools::LoadImageRgba(
            mResourceLocator.GetBitmapFilePath("shipbuilder_background")));

    // Sync UI with view parameters

    mOtherLayersOpacitySlider->SetValue(
        OtherLayersOpacityToSlider(mView->GetOtherLayersOpacity()));
}

void MainFrame::OpenForNewShip()
{
    // New ship
    DoNewShip();

    // Open ourselves
    Open();
}

void MainFrame::OpenForLoadShip(std::filesystem::path const & shipFilePath)
{
    // Register idle event for delayed load
    mOpenForLoadShipFilePath = shipFilePath;
    Bind(wxEVT_IDLE, (wxObjectEventFunction)&MainFrame::OnOpenForLoadShipIdleEvent, this);

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

void MainFrame::OnShipNameChanged(std::string const & newName)
{
    if (mController)
    {
        //
        // Ship filename workflow
        //
        // - If we have a current ship file path,
        // - And the file exists,
        // - And its filename is different than the filename that comes from the new ship name,
        // - And this new filename does not exist:
        // 
        // - Ask user if wants to rename file
        //

        std::string const newShipFilename = Utils::MakeFilenameSafeString(newName) + ShipDeSerializer::GetShipDefinitionFileExtension();

        if (mCurrentShipFilePath.has_value()
            && std::filesystem::exists(*mCurrentShipFilePath)
            && newShipFilename != mCurrentShipFilePath->filename().string())
        {
            std::filesystem::path const newFilepath = mCurrentShipFilePath->parent_path() / newShipFilename;
            if (!std::filesystem::exists(newFilepath)
                && AskUserIfRename(newShipFilename))
            {
                // Rename
                std::filesystem::rename(*mCurrentShipFilePath, newFilepath);
                mCurrentShipFilePath = newFilepath;
            }
            else
            {
                // Doesn't want to rename, hence at this moment the ship has no backing file anymore...
                // ...clear filename, so that Save will become SaveAs
                mCurrentShipFilePath.reset();
            }
        }

        //
        // Reconciliate UI
        //

        ReconciliateUIWithShipName(newName);
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

void MainFrame::OnStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType mode)
{
    if (mController)
    {
        ReconciliateUIWithStructuralLayerVisualizationModeSelection(mode);
    }
}

void MainFrame::OnElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType mode)
{
    if (mController)
    {
        ReconciliateUIWithElectricalLayerVisualizationModeSelection(mode);
    }
}

void MainFrame::OnRopesLayerVisualizationMode(RopesLayerVisualizationModeType mode)
{
    if (mController)
    {
        ReconciliateUIWithRopesLayerVisualizationModeSelection(mode);
    }
}

void MainFrame::OnTextureLayerVisualizationMode(TextureLayerVisualizationModeType mode)
{
    if (mController)
    {
        ReconciliateUIWithTextureLayerVisualizationModeSelection(mode);
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
            coordinates->FlipY(mController->GetShipSize().height);
        }
        else
        {
            coordinates.reset();
        }
    }

    mStatusBar->SetToolCoordinates(coordinates);
}

void MainFrame::OnError(wxString const & errorMessage) const
{
    wxMessageBox(errorMessage, _("Error"), wxICON_ERROR);
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
                    OnSaveShip();
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
                    OnSaveShipAs();
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
                    OnSaveAndGoBack();
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

        // Validate button
        {
            auto button = new BitmapButton(
                panel,
                mResourceLocator.GetIconFilePath("validate_ship_button"),
                [this]()
                {
                    ValidateShip();
                },
                _("Check the ship for issues"));

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

    {
        // Structural pencil
        {
            auto [tsPanel, sizer] = CreateToolSettingsToolSizePanel(
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

            tsPanel->SetSizerAndFit(sizer);

            mToolSettingsPanelsSizer->Add(
                tsPanel,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);

            mToolSettingsPanelsSizer->Hide(tsPanel);

            mToolSettingsPanels.emplace_back(
                ToolType::StructuralPencil,
                tsPanel);
        }

        // Structural eraser
        {
            auto [tsPanel, sizer] = CreateToolSettingsToolSizePanel(
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

            tsPanel->SetSizerAndFit(sizer);

            mToolSettingsPanelsSizer->Add(
                tsPanel,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);

            mToolSettingsPanelsSizer->Hide(tsPanel);

            mToolSettingsPanels.emplace_back(
                ToolType::StructuralEraser,
                tsPanel);
        }

        // Structural line
        {
            auto [tsPanel, sizer] = CreateToolSettingsToolSizePanel(
                panel,
                _("Line size:"),
                _("The size of the line tool."),
                1,
                MaxPencilSize,
                mWorkbenchState.GetStructuralLineToolSize(),
                [this](std::uint32_t value)
                {
                    mWorkbenchState.SetStructuralLineToolSize(value);
                });

            // Contiguity
            {
                wxCheckBox * chkBox = new wxCheckBox(tsPanel, wxID_ANY, _T("Hull mode"));

                chkBox->SetToolTip(_("When enabled, draw lines with pixel edges touching each other"));

                chkBox->SetValue(mWorkbenchState.GetStructuralLineToolIsHullMode());

                chkBox->Bind(
                    wxEVT_CHECKBOX,
                    [this](wxCommandEvent & event)
                    {
                        mWorkbenchState.SetStructuralLineToolIsHullMode(event.IsChecked());
                    });

                sizer->Add(
                    chkBox,
                    0,
                    wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT,
                    4);
            }

            tsPanel->SetSizerAndFit(sizer);

            mToolSettingsPanelsSizer->Add(
                tsPanel,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);

            mToolSettingsPanelsSizer->Hide(tsPanel);

            mToolSettingsPanels.emplace_back(
                ToolType::StructuralLine,
                tsPanel);
        }

        // Structural flood
        {
            wxPanel * tsPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);

            wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

            {
                wxCheckBox * chkBox = new wxCheckBox(tsPanel, wxID_ANY, _("Contiguous only"));

                chkBox->SetToolTip(_("Flood only particles touching each other. When not checked, the flood tool effectively replaces a material with another."));

                chkBox->SetValue(mWorkbenchState.GetStructuralFloodToolIsContiguous());

                chkBox->Bind(
                    wxEVT_CHECKBOX,
                    [this](wxCommandEvent & event)
                    {
                        mWorkbenchState.SetStructuralFloodToolIsContiguous(event.IsChecked());
                    });

                sizer->Add(
                    chkBox,
                    0,
                    wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT,
                    4);
            }

            tsPanel->SetSizerAndFit(sizer);

            mToolSettingsPanelsSizer->Add(
                tsPanel,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);

            mToolSettingsPanelsSizer->Hide(tsPanel);

            mToolSettingsPanels.emplace_back(
                ToolType::StructuralFlood,
                tsPanel);
        }
    }

    mToolSettingsPanelsSizer->AddStretchSpacer(1);

    panel->SetSizer(mToolSettingsPanelsSizer);

    return panel;
}

wxPanel * MainFrame::CreateLayersPanel(wxWindow * parent)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * rootVSizer = new wxBoxSizer(wxVERTICAL);

    rootVSizer->AddSpacer(10);

    // Layer management
    {
        wxGridBagSizer * layerManagerSizer = new wxGridBagSizer(0, 0);

        {
            auto const createButtonRow = [&](LayerType layer, int iRow)
            {
                wxString const sureQuestion = _("The current changes to the layer will be lost; are you sure you want to continue?");

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
                            [this, layer, sureQuestion]()
                            {
                                switch (layer)
                                {
                                    case LayerType::Electrical:
                                    {
                                        if (mController->HasModelLayer(LayerType::Electrical)
                                            && mController->IsModelDirty(LayerType::Electrical))
                                        {
                                            if (!AskUserIfSure(sureQuestion))
                                            {
                                                // Changed their mind
                                                return;
                                            }
                                        }

                                        mController->NewElectricalLayer();

                                        break;
                                    }

                                    case LayerType::Ropes:
                                    {
                                        if (mController->HasModelLayer(LayerType::Ropes)
                                            && mController->IsModelDirty(LayerType::Ropes))
                                        {
                                            if (!AskUserIfSure(sureQuestion))
                                            {
                                                // Changed their mind
                                                return;
                                            }
                                        }

                                        mController->NewRopesLayer();

                                        break;
                                    }

                                    case LayerType::Structural:
                                    {
                                        if (mController->HasModelLayer(LayerType::Structural)
                                            && mController->IsModelDirty(LayerType::Structural))
                                        {
                                            if (!AskUserIfSure(sureQuestion))
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
                    }
                    else
                    {
                        newButton = new BitmapButton(
                            panel,
                            mResourceLocator.GetBitmapFilePath("open_image_button"),
                            [this, sureQuestion]()
                            {
                                if (mController->HasModelLayer(LayerType::Texture)
                                    && mController->IsModelDirty(LayerType::Texture))
                                {
                                    if (!AskUserIfSure(sureQuestion))
                                    {
                                        // Changed their mind
                                        return;
                                    }
                                }

                                ImportTextureLayerFromImage();
                            },
                            _("Import this layer from an image file."));
                    }

                    layerManagerSizer->Add(
                        newButton,
                        wxGBPosition(iRow * 3, 1),
                        wxGBSpan(1, 1),
                        wxLEFT | wxRIGHT,
                        10);
                }

                // Import
                {
                    // TODO: also here ask user if sure when the layer is dirty

                    auto * importButton = new BitmapButton(
                        panel,
                        mResourceLocator.GetBitmapFilePath("open_layer_button"),
                        [this, layer]()
                        {
                            // TODO
                            UnderConstructionDialog::Show(this, mResourceLocator);
                        },
                        _("Import this layer from another ship."));

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
                            [this, layer, sureQuestion]()
                            {
                                switch (layer)
                                {
                                    case LayerType::Electrical:
                                    {
                                        assert(mController->HasModelLayer(LayerType::Electrical));

                                        if (mController->IsModelDirty(LayerType::Electrical))
                                        {
                                            if (!AskUserIfSure(sureQuestion))
                                            {
                                                // Changed their mind
                                                return;
                                            }
                                        }

                                        mController->RemoveElectricalLayer();

                                        break;
                                    }

                                    case LayerType::Ropes:
                                    {
                                        assert(mController->HasModelLayer(LayerType::Ropes));

                                        if (mController->IsModelDirty(LayerType::Ropes))
                                        {
                                            if (!AskUserIfSure(sureQuestion))
                                            {
                                                // Changed their mind
                                                return;
                                            }
                                        }

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
                                        assert(mController->HasModelLayer(LayerType::Texture));

                                        if (mController->IsModelDirty(LayerType::Texture))
                                        {
                                            if (!AskUserIfSure(sureQuestion))
                                            {
                                                // Changed their mind
                                                return;
                                            }
                                        }

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
                                UnderConstructionDialog::Show(this, mResourceLocator);
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

    panel->SetSizerAndFit(rootVSizer);

    return panel;
}

wxPanel * MainFrame::CreateLayersVisualizationPanel(wxWindow * parent)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * rootHSizer = new wxBoxSizer(wxHORIZONTAL);

    // Other layers opacity slider
    {
        mOtherLayersOpacitySlider = new wxSlider(panel, wxID_ANY, 0, 0, MaxLayerTransparency,
            wxDefaultPosition, wxDefaultSize, wxSL_VERTICAL | wxSL_INVERSE);

        mOtherLayersOpacitySlider->Bind(
            wxEVT_SLIDER,
            [this](wxCommandEvent &)
            {
                assert(mController);
                float const opacity = OtherLayersOpacitySliderToOpacity(mOtherLayersOpacitySlider->GetValue());
                mController->SetOtherLayersOpacity(opacity);
            });

        rootHSizer->Add(
            mOtherLayersOpacitySlider,
            0, // Retain horizontal width
            wxEXPAND | wxALIGN_TOP, // Expand vertically
            0);
    }

    // View modifiers
    {
        mLayerVisualizationModePanelsSizer = new wxBoxSizer(wxVERTICAL);

        // Structure
        {
            wxPanel * structuralLayerVisualizationModesPanel = new wxPanel(panel);

            wxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

            // Particle mode
            {
                mStructuralLayerParticleVisualizationModeButton = new BitmapToggleButton(
                    structuralLayerVisualizationModesPanel,
                    mResourceLocator.GetBitmapFilePath("save_layer_button"), // TODOHERE: Particle Viz Mode icon
                    [this]()
                    {
                        assert(mController);
                        
                        // TODOHERE
                        //mController->SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType::ParticleMode);

                        DeviateFocus();
                    },
                    _("Particle mode: view individual structural particles."));

                auto buttonSize = mStructuralLayerParticleVisualizationModeButton->GetBitmap().GetSize();
                buttonSize.IncBy(4, 4);
                mStructuralLayerParticleVisualizationModeButton->SetMaxSize(buttonSize);

                vSizer->Add(
                    mStructuralLayerParticleVisualizationModeButton,
                    0, // Retain V size
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            // Auto-texturization mode
            {
                mStructuralLayerAutoTexturizationVisualizationModeButton = new BitmapToggleButton(
                    structuralLayerVisualizationModesPanel,
                    mResourceLocator.GetBitmapFilePath("save_layer_button"), // TODOHERE: Same icon as auto-texturization, but smaller
                    [this]()
                    {
                        assert(mController);
                        
                        // TODOHERE
                        //mController->SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType::AutoTexturizationMode);

                        DeviateFocus();
                    },
                    _("Auto-texturization mode: view ship as when it is auto-texturized."));

                auto buttonSize = mStructuralLayerAutoTexturizationVisualizationModeButton->GetBitmap().GetSize();
                buttonSize.IncBy(4, 4);
                mStructuralLayerAutoTexturizationVisualizationModeButton->SetMaxSize(buttonSize);

                vSizer->Add(
                    mStructuralLayerAutoTexturizationVisualizationModeButton,
                    0, // Retain V size
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            // Texture mode
            {
                mStructuralLayerTextureVisualizationModeButton = new BitmapToggleButton(
                    structuralLayerVisualizationModesPanel,
                    mResourceLocator.GetBitmapFilePath("save_layer_button"), // TODOHERE: Same icon as photo, but smaller
                    [this]()
                    {
                        assert(mController);
                        
                        // TODOHERE
                        //mController->SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType::TextureMode);

                        DeviateFocus();
                    },
                    _("Texture mode: view ship with texture masked with the ship's structure."));

                vSizer->Add(
                    mStructuralLayerTextureVisualizationModeButton,
                    0, // Retain V size
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            structuralLayerVisualizationModesPanel->SetSizerAndFit(vSizer);

            mLayerVisualizationModePanelsSizer->Add(
                structuralLayerVisualizationModesPanel,
                1, // Expand vertically
                wxALIGN_CENTER_HORIZONTAL,
                0);

            mLayerVisualizationModePanels[static_cast<size_t>(LayerType::Structural)] = structuralLayerVisualizationModesPanel;
        }

        // Electrical
        {
            // Nothing at the moment
            mLayerVisualizationModePanels[static_cast<size_t>(LayerType::Electrical)] = nullptr;
        }

        // Ropes
        {
            // Nothing at the moment
            mLayerVisualizationModePanels[static_cast<size_t>(LayerType::Ropes)] = nullptr;
        }

        // Texture
        {
            // Nothing at the moment
            mLayerVisualizationModePanels[static_cast<size_t>(LayerType::Texture)] = nullptr;
        }

        mLayerVisualizationModePanelsSizer->AddStretchSpacer(1);

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
                [this](wxCommandEvent & event)
                {
                    assert(mController);
                    mController->EnableVisualGrid(event.IsChecked());

                    DeviateFocus();
                });

            mLayerVisualizationModePanelsSizer->Add(
                viewGridButton,
                0, // Retain vertical width
                wxALIGN_CENTER_HORIZONTAL, // Do not expand vertically
                0);
        }

        rootHSizer->Add(
            mLayerVisualizationModePanelsSizer,
            0, // Retain horizontal width
            wxEXPAND | wxALIGN_TOP, // Expand vertically
            0);
    }
    
    panel->SetSizerAndFit(rootHSizer);

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

            // Line
            {
                auto button = makeToolButton(
                    ToolType::StructuralLine,
                    structuralToolbarPanel,
                    "line_icon",
                    _("Draw particles in lines"));

                toolsSizer->Add(
                    button,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    0,
                    0);
            }

            // Flood
            {
                auto button = makeToolButton(
                    ToolType::StructuralFlood,
                    structuralToolbarPanel,
                    "flood_tool_icon",
                    _("Fill an area with structure particles"));

                toolsSizer->Add(
                    button,
                    wxGBPosition(1, 1),
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
                        OpenMaterialPalette(event, LayerType::Structural, MaterialPlaneType::Foreground);
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
                        OpenMaterialPalette(event, LayerType::Structural, MaterialPlaneType::Background);
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

        mToolbarPanelsSizer->Hide(structuralToolbarPanel);

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

            // Line
            {
                auto button = makeToolButton(
                    ToolType::ElectricalLine,
                    electricalToolbarPanel,
                    "line_icon",
                    _("Draw electrical elements in lines"));

                toolsSizer->Add(
                    button,
                    wxGBPosition(1, 0),
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
                        OpenMaterialPalette(event, LayerType::Electrical, MaterialPlaneType::Foreground);
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
                        OpenMaterialPalette(event, LayerType::Electrical, MaterialPlaneType::Background);
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

        mToolbarPanelsSizer->Hide(electricalToolbarPanel);

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

            // RopePencil
            {
                auto button = makeToolButton(
                    ToolType::RopePencil,
                    ropesToolbarPanel,
                    "pencil_icon",
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
                    ToolType::RopeEraser,
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

        ropesToolbarSizer->AddSpacer(15);

        // Swaths

        {
            wxBoxSizer * paletteSizer = new wxBoxSizer(wxVERTICAL);

            // Foreground
            {
                mRopesForegroundMaterialSelector = new wxStaticBitmap(
                    ropesToolbarPanel,
                    wxID_ANY,
                    WxHelpers::MakeEmptyBitmap(),
                    wxDefaultPosition,
                    wxSize(MaterialSwathSize.width, MaterialSwathSize.height),
                    wxBORDER_SUNKEN);

                mRopesForegroundMaterialSelector->Bind(
                    wxEVT_LEFT_DOWN,
                    [this](wxMouseEvent & event)
                    {
                        OpenMaterialPalette(event, LayerType::Ropes, MaterialPlaneType::Foreground);
                    });

                paletteSizer->Add(
                    mRopesForegroundMaterialSelector,
                    0,
                    0,
                    0);
            }

            paletteSizer->AddSpacer(8);

            // Background
            {
                mRopesBackgroundMaterialSelector = new wxStaticBitmap(
                    ropesToolbarPanel,
                    wxID_ANY,
                    WxHelpers::MakeEmptyBitmap(),
                    wxDefaultPosition,
                    wxSize(MaterialSwathSize.width, MaterialSwathSize.height),
                    wxBORDER_SUNKEN);

                mRopesBackgroundMaterialSelector->Bind(
                    wxEVT_LEFT_DOWN,
                    [this](wxMouseEvent & event)
                    {
                        OpenMaterialPalette(event, LayerType::Ropes, MaterialPlaneType::Background);
                    });

                paletteSizer->Add(
                    mRopesBackgroundMaterialSelector,
                    0,
                    0,
                    0);
            }

            ropesToolbarSizer->Add(
                paletteSizer,
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

        mToolbarPanelsSizer->Hide(ropesToolbarPanel);

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

        mToolbarPanelsSizer->Hide(textureToolbarPanel);

        // Store toolbar panel
        mToolbarPanels[static_cast<size_t>(LayerType::Texture)] = textureToolbarPanel;
    }

    panel->SetSizer(mToolbarPanelsSizer);

    return panel;
}

wxPanel * MainFrame::CreateUndoPanel(wxWindow * parent)
{
    mUndoStackPanel = new wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    mUndoStackPanel->SetScrollRate(0, 5);

    {
        auto font = mUndoStackPanel->GetFont();
        font.SetPointSize(font.GetPointSize() - 2);
        mUndoStackPanel->SetFont(font);
    }

    auto vSizer = new wxBoxSizer(wxVERTICAL);

    // Empty for now

    mUndoStackPanel->SetSizerAndFit(vSizer);

    return mUndoStackPanel;
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
        mWorkCanvas->Connect(wxEVT_KEY_DOWN, (wxObjectEventFunction)&MainFrame::OnWorkCanvasKeyDown, 0, this);
        mWorkCanvas->Connect(wxEVT_KEY_UP, (wxObjectEventFunction)&MainFrame::OnWorkCanvasKeyUp, 0, this);


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

std::tuple<wxPanel *, wxSizer *> MainFrame::CreateToolSettingsToolSizePanel(
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

    // Edit spin box
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

    return std::tuple(panel, sizer);
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

    auto const workCanvasSize = GetWorkCanvasSize();

    // We take care of notifying the view ourselves, as we might have a view
    // but not a controller
    if (mView)
    {
        mView->SetDisplayLogicalSize(workCanvasSize);
    }

    if (mController)
    {
        mController->OnWorkCanvasResized(workCanvasSize);
    }

    // Allow resizing to occur, this is a hook
    event.Skip();
}

void MainFrame::OnWorkCanvasLeftDown(wxMouseEvent & event)
{
    // First of all, set focus on the canvas if it has lost it - we want
    // it to receive all mouse events
    if (!mWorkCanvas->HasFocus())
    {
        mWorkCanvas->SetFocus();
    }

    // Hang on to the mouse for as long as the button is pressed
    if (!mIsMouseCapturedByWorkCanvas)
    {
        if (!mIsMouseInWorkCanvas)
        {
            // When clicking into the canvas while the mouse was out, wxWidgets sends first a LeftDown
            // followed by an EnterWindow; when this happens, the EnterWindow sees that the mouse is
            // captured and does not act on it.
            // We thus synthesize an EnterWindow here
            OnWorkCanvasMouseEnteredWindow(event);
            mIsMouseInWorkCanvas = false; // Allow next EnterWindow to think it's the first
        }

        LogMessage("Mouse captured");
        mWorkCanvas->CaptureMouse();
        mIsMouseCapturedByWorkCanvas = true;
    }

    if (mController)
    {
        mController->OnLeftMouseDown();
    }
}

void MainFrame::OnWorkCanvasLeftUp(wxMouseEvent & /*event*/)
{
    // We can now release the mouse
    if (mIsMouseCapturedByWorkCanvas)
    {
        mWorkCanvas->ReleaseMouse();
        mIsMouseCapturedByWorkCanvas = false;

        LogMessage("Mouse released");

        if (!mIsMouseInWorkCanvas)
        {
            // Simulate a window left
            if (mController)
            {
                mController->OnUncapturedMouseOut();
            }
        }
    }

    if (mController)
    {
        mController->OnLeftMouseUp();
    }
}

void MainFrame::OnWorkCanvasRightDown(wxMouseEvent & /*event*/)
{
    // Hang on to the mouse for as long as the button is pressed
    if (!mIsMouseCapturedByWorkCanvas)
    {
        LogMessage("Mouse captured");

        mWorkCanvas->CaptureMouse();
        mIsMouseCapturedByWorkCanvas = true;
    }

    if (mController)
    {
        mController->OnRightMouseDown();
    }
}

void MainFrame::OnWorkCanvasRightUp(wxMouseEvent & /*event*/)
{
    // We can now release the mouse
    if (mIsMouseCapturedByWorkCanvas)
    {
        mWorkCanvas->ReleaseMouse();
        mIsMouseCapturedByWorkCanvas = false;

        LogMessage("Mouse released");

        if (!mIsMouseInWorkCanvas)
        {
            // Simulate a window left
            if (mController)
            {
                mController->OnUncapturedMouseOut();
            }
        }
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
    LogMessage("CaptureMouseLost");

    if (mController)
    {
        mController->OnMouseCaptureLost();
    }
}

void MainFrame::OnWorkCanvasMouseLeftWindow(wxMouseEvent & /*event*/)
{
    LogMessage("LeftWindow (isCaptured=", mIsMouseCapturedByWorkCanvas, ")");

    assert(mIsMouseInWorkCanvas);
    mIsMouseInWorkCanvas = false;

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
    LogMessage("EnteredWindow (isCaptured=", mIsMouseCapturedByWorkCanvas, ")");

    assert(!mIsMouseInWorkCanvas);
    mIsMouseInWorkCanvas = true;

    if (!mIsMouseCapturedByWorkCanvas)
    {
        if (mController)
        {
            mController->OnUncapturedMouseIn();
        }
    }
}

void MainFrame::OnWorkCanvasKeyDown(wxKeyEvent & event)
{
    if (mController)
    {
        if (event.GetKeyCode() == WXK_SHIFT)
        {
            mController->OnShiftKeyDown();
        }
    }
}

void MainFrame::OnWorkCanvasKeyUp(wxKeyEvent & event)
{
    if (mController)
    {
        if (event.GetKeyCode() == WXK_SHIFT)
        {
            mController->OnShiftKeyUp();
        }
    }
}

void MainFrame::OnNewShip(wxCommandEvent & /*event*/)
{
    NewShip();
}

void MainFrame::OnLoadShip(wxCommandEvent & /*event*/)
{
    LoadShip();
}

void MainFrame::OnSaveShipMenuItem(wxCommandEvent & /*event*/)
{
    OnSaveShip();
}

void MainFrame::OnSaveShip()
{
    if (PreSaveShipCheck())
    {
        SaveShip();
    }
}

void MainFrame::OnSaveShipAsMenuItem(wxCommandEvent & /*event*/)
{
    OnSaveShipAs();
}

void MainFrame::OnSaveShipAs()
{
    if (PreSaveShipCheck())
    {
        SaveShipAs();
    }
}

void MainFrame::OnSaveAndGoBackMenuItem(wxCommandEvent & /*event*/)
{
    OnSaveAndGoBack();
}

void MainFrame::OnSaveAndGoBack()
{
    if (PreSaveShipCheck())
    {
        SaveAndSwitchBackToGame();
    }
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
    if (event.CanVeto() && mController->IsModelDirty())
    {
        // Ask user if they really want
        int result = AskUserIfSave();
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

void MainFrame::OnEditUndoMenuItem(wxCommandEvent & /*event*/)
{
    mController->UndoLast();
}

void MainFrame::OnEditFlipHorizontallyMenuItem(wxCommandEvent & /*event*/)
{
    mController->Flip(DirectionType::Horizontal);
}

void MainFrame::OnEditFlipVerticallyMenuItem(wxCommandEvent & /*event*/)
{
    mController->Flip(DirectionType::Vertical);
}

void MainFrame::OnEditResizeShipMenuItem(wxCommandEvent & /*event*/)
{
    OpenShipCanvasResize();
}

void MainFrame::OnEditShipPropertiesMenuItem(wxCommandEvent & /*event*/)
{
    OpenShipProperties();
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

void MainFrame::OnRopeMaterialSelected(fsStructuralMaterialSelectedEvent & event)
{
    if (event.GetMaterialPlane() == MaterialPlaneType::Foreground)
    {
        mWorkbenchState.SetRopesForegroundMaterial(event.GetMaterial());
    }
    else
    {
        assert(event.GetMaterialPlane() == MaterialPlaneType::Background);
        mWorkbenchState.SetRopesBackgroundMaterial(event.GetMaterial());
    }

    ReconciliateUIWithWorkbenchState();
}

//////////////////////////////////////////////////////////////////

void MainFrame::OnOpenForLoadShipIdleEvent(wxIdleEvent & /*event*/)
{
    // Unbind idle event handler
    bool result = Unbind(wxEVT_IDLE, (wxObjectEventFunction)&MainFrame::OnOpenForLoadShipIdleEvent, this);
    assert(result);
    (void)result;
    
    // Load ship
    if (!DoLoadShip(mOpenForLoadShipFilePath))
    {
        // No luck loading ship...
        // ...just create new ship
        DoNewShip();
    }
}

void MainFrame::Open()
{
    // Show us
    Show(true);

    // Make ourselves the topmost frame
    mMainApp->SetTopWindow(this);
}

void MainFrame::NewShip()
{
    if (mController->IsModelDirty())
    {
        // Ask user if they really want
        int result = AskUserIfSave();
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

    DoNewShip();
}

void MainFrame::LoadShip()
{
    if (mController->IsModelDirty())
    {
        // Ask user if they really want
        int result = AskUserIfSave();
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
        DoLoadShip(shipFilePath); // Ignore eventual failure
    }
}

bool MainFrame::PreSaveShipCheck()
{
    assert(mController);

    // Validate ship in dialog, and allow user to continue or cancel the save

    if (!mModelValidationDialog)
    {
        mModelValidationDialog = std::make_unique<ModelValidationDialog>(this, mResourceLocator);
    }

    return mModelValidationDialog->ShowModalForSaveShipValidation(*mController);
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
        Utils::MakeFilenameSafeString(mController->GetShipMetadata().ShipName),
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
    if (mController->IsModelDirty())
    {
        // Ask user if they really want
        int result = AskUserIfSave();
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

void MainFrame::ImportTextureLayerFromImage()
{
    ImageLoadDialog dlg(this);
    auto const ret = dlg.ShowModal();
    if (ret == wxID_OK)
    {
        try
        {
            auto image = ImageFileTools::LoadImageRgba(dlg.GetPath().ToStdString());

            if (image.Size.width == 0 || image.Size.height == 0)
            {
                throw GameException("The specified texture image is empty, and thus it may not be used for this ship.");
            }

            // Calculate target size == size of texture when maintaining same aspect ratio as ship's,
            // preferring to not cut
            ShipSpaceSize const & shipSize = mController->GetShipSize();
            IntegralRectSize targetSize = (image.Size.width * shipSize.height >= image.Size.height * shipSize.width)
                ? IntegralRectSize(image.Size.width, image.Size.width * shipSize.height / shipSize.width) // Keeping this width would require greater height (no clipping), and thus we want to keep this width
                : IntegralRectSize(image.Size.height * shipSize.width / shipSize.height, image.Size.height); // Keeping this width would require smaller height (hence clipping), and thus we want to keep the height instead
            
            // Check if the target size does not match the current texture size
            if (targetSize.width != image.Size.width
                || targetSize.height != image.Size.height)
            {
                //
                // Ask user how to resize
                //

                if (!mResizeDialog)
                {
                    mResizeDialog = std::make_unique<ResizeDialog>(this, mResourceLocator);
                }

                if (!mResizeDialog->ShowModalForTexture(image, targetSize))
                {
                    // User aborted
                    return;
                }

                //
                // Resize
                //

                auto const originOffset = mResizeDialog->GetOffset();

                image = image.Reframe(
                    ImageCoordinates(originOffset.x, originOffset.y),
                    ImageSize(targetSize.width, targetSize.height),
                    rgbaColor::zero());
            }

            // Set texture
            mController->SetTextureLayer(
                TextureLayerData(std::move(image)),
                std::nullopt);
        }
        catch (std::runtime_error const & exc)
        {
            ShowError(exc.what());
        }
    }
}

void MainFrame::OpenShipCanvasResize()
{
    if (!mResizeDialog)
    {
        mResizeDialog = std::make_unique<ResizeDialog>(this, mResourceLocator);
    }

    auto const & preview = mController->GetStructuralLayerVisualization();
    if (mResizeDialog->ShowModalForResize(preview, IntegralRectSize(preview.Size.width, preview.Size.height)))
    {
        // tODOHERE
    }
}

void MainFrame::OpenShipProperties()
{
    if (!mShipPropertiesEditDialog)
    {
        mShipPropertiesEditDialog = std::make_unique<ShipPropertiesEditDialog>(this, mResourceLocator);
    }

    mShipPropertiesEditDialog->ShowModal(
        *mController,
        mController->GetShipMetadata(),
        mController->GetShipPhysicsData(),
        mController->GetShipAutoTexturizationSettings(),
        mController->GetStructuralLayerVisualization(),
        mController->HasModelLayer(LayerType::Texture));
}

void MainFrame::ValidateShip()
{
    if (!mModelValidationDialog)
    {
        mModelValidationDialog = std::make_unique<ModelValidationDialog>(this, mResourceLocator);
    }

    mModelValidationDialog->ShowModalForStandAloneValidation(*mController);
}

void MainFrame::OpenMaterialPalette(
    wxMouseEvent const & event,
    LayerType layer,
    MaterialPlaneType plane)
{
    wxWindow * referenceWindow = dynamic_cast<wxWindow *>(event.GetEventObject());
    if (nullptr == referenceWindow)
        return;

    auto const referenceRect = mWorkCanvas->GetScreenRect();

    if (layer == LayerType::Structural)
    {
        mStructuralMaterialPalette->Open(
            referenceRect,
            plane,
            plane == MaterialPlaneType::Foreground
            ? mWorkbenchState.GetStructuralForegroundMaterial()
            : mWorkbenchState.GetStructuralBackgroundMaterial());
    }
    else if (layer == LayerType::Electrical)
    {
        mElectricalMaterialPalette->Open(
            referenceRect,
            plane,
            plane == MaterialPlaneType::Foreground
            ? mWorkbenchState.GetElectricalForegroundMaterial()
            : mWorkbenchState.GetElectricalBackgroundMaterial());
    }
    else
    {
        assert(layer == LayerType::Ropes);

        mRopesMaterialPalette->Open(
            referenceRect,
            plane,
            plane == MaterialPlaneType::Foreground
            ? mWorkbenchState.GetRopesForegroundMaterial()
            : mWorkbenchState.GetRopesBackgroundMaterial());
    }
}

bool MainFrame::AskUserIfSure(wxString caption)
{
    int result = wxMessageBox(caption, ApplicationName, wxICON_EXCLAMATION | wxOK | wxCANCEL | wxCENTRE);
    return (result == wxOK);
}

int MainFrame::AskUserIfSave()
{
    int result = wxMessageBox(_("Do you want to save your changes before continuing?"), ApplicationName, 
        wxICON_EXCLAMATION | wxYES_NO | wxCANCEL | wxCENTRE);
    return result;
}

bool MainFrame::AskUserIfRename(std::string const & newFilename)
{
    wxString const caption = wxString::Format(_("Do you also want to rename the ship file to \"%s\"?"), newFilename);
    int result = wxMessageBox(caption, ApplicationName, wxICON_EXCLAMATION | wxYES_NO | wxCENTRE);
    return (result == wxYES);
}

void MainFrame::ShowError(wxString const & message)
{
    wxMessageBox(message, _("Maritime Disaster"), wxICON_ERROR);
}

void MainFrame::DoNewShip()
{
    // Make name
    std::string const shipName = "MyShip-" + Utils::MakeNowDateAndTimeString();

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

bool MainFrame::DoLoadShip(std::filesystem::path const & shipFilePath)
{
    try
    {
        // Load ship
        ShipDefinition shipDefinition = ShipDeSerializer::LoadShip(shipFilePath, mMaterialDatabase);

        // Check password
        if (AskPasswordDialog::CheckPasswordProtectedEdit(shipDefinition, this, mResourceLocator))
        {
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

            // Success
            return true;
        }
    }
    catch (UserGameException const & exc)
    {
        ShowError(mLocalizationManager.MakeErrorMessage(exc));
    }
    catch (std::runtime_error const & exc)
    {
        ShowError(exc.what());
    }

    // No luck
    return false;
}

void MainFrame::DoSaveShip(std::filesystem::path const & shipFilePath)
{
    assert(mController);

    // Get ship definition
    auto shipDefinition = mController->MakeShipDefinition();

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

DisplayLogicalSize MainFrame::GetWorkCanvasSize() const
{
    return DisplayLogicalSize(
        mWorkCanvas->GetSize().GetWidth(),
        mWorkCanvas->GetSize().GetHeight());
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

float MainFrame::OtherLayersOpacitySliderToOpacity(int sliderValue)
{
    float const opacity =
        static_cast<float>(sliderValue)
        / static_cast<float>(MaxLayerTransparency);

    return opacity;
}

int MainFrame::OtherLayersOpacityToSlider(float opacityValue)
{
    int const sliderValue = static_cast<int>(
        opacityValue * static_cast<float>(MaxLayerTransparency));

    return sliderValue;
}

void MainFrame::ReconciliateUI()
{
    assert(mController);
    ReconciliateUIWithViewModel();
    ReconciliateUIWithShipName(mController->GetShipMetadata().ShipName);
    ReconciliateUIWithLayerPresence();
    ReconciliateUIWithShipSize(mController->GetShipSize());
    ReconciliateUIWithPrimaryLayerSelection(mController->GetPrimaryLayer());
    ReconciliateUIWithStructuralLayerVisualizationModeSelection(mController->GetStructuralLayerVisualizationMode());
    ReconciliateUIWithElectricalLayerVisualizationModeSelection(mController->GetElectricalLayerVisualizationMode());
    ReconciliateUIWithRopesLayerVisualizationModeSelection(mController->GetRopesLayerVisualizationMode());
    ReconciliateUIWithTextureLayerVisualizationModeSelection(mController->GetTextureLayerVisualizationMode());
    ReconciliateUIWithModelDirtiness();
    ReconciliateUIWithWorkbenchState();
    ReconciliateUIWithSelectedTool(mController->GetCurrentTool());
    ReconciliateUIWithUndoStackState();

    mMainPanel->Layout();

    assert(mWorkCanvas);
    mWorkCanvas->Refresh();
}

void MainFrame::ReconciliateUIWithViewModel()
{
    RecalculateWorkCanvasPanning();

    // TODO: set zoom in StatusBar
}

void MainFrame::ReconciliateUIWithShipName(std::string const & shipName)
{
    SetFrameTitle(shipName, mController->IsModelDirty());
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
        bool const hasLayer = mController->HasModelLayer(static_cast<LayerType>(iLayer));

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

    mOtherLayersOpacitySlider->Enable(mController->HasModelExtraLayers());

    mLayerSelectButtons[static_cast<size_t>(mController->GetPrimaryLayer())]->SetFocus(); // Prevent other random buttons for getting focus
}

void MainFrame::ReconciliateUIWithPrimaryLayerSelection(LayerType primaryLayer)
{
    assert(mController);

    //
    // Toggle various UI elements <-> primary layer
    //

    bool hasToggledToolPanel = false;
    bool hasToggledLayerVisualizationModePanel = false;

    assert(mController->HasModelLayer(primaryLayer));
    
    for (uint32_t iLayer = 0; iLayer < LayerCount; ++iLayer)
    {
        bool const isSelected = (iLayer == static_cast<uint32_t>(primaryLayer));

        // Layer selection buttons
        if (mLayerSelectButtons[iLayer]->GetValue() != isSelected)
        {
            mLayerSelectButtons[iLayer]->SetValue(isSelected);

            if (isSelected)
            {
                mLayerSelectButtons[iLayer]->SetFocus(); // Prevent other random buttons for getting focus
            }
        }

        // Layer visualization mode panels
        if (mLayerVisualizationModePanels[iLayer] != nullptr 
            && mLayerVisualizationModePanelsSizer->IsShown(mLayerVisualizationModePanels[iLayer]) != isSelected)
        {
            mLayerVisualizationModePanelsSizer->Show(mLayerVisualizationModePanels[iLayer], isSelected);
            hasToggledLayerVisualizationModePanel = true;
        }

        // Toolbar panels
        if (mToolbarPanelsSizer->IsShown(mToolbarPanels[iLayer]) != isSelected)
        {
            mToolbarPanelsSizer->Show(mToolbarPanels[iLayer], isSelected);
            hasToggledToolPanel = true;
        }
    }

    if (hasToggledLayerVisualizationModePanel)
    {
        mLayerVisualizationModePanelsSizer->Layout();
    }

    if (hasToggledToolPanel)
    {
        mToolbarPanelsSizer->Layout();
    }
}

void MainFrame::ReconciliateUIWithStructuralLayerVisualizationModeSelection(StructuralLayerVisualizationModeType mode)
{
    mStructuralLayerParticleVisualizationModeButton->SetValue(mode == StructuralLayerVisualizationModeType::ParticleMode);
    mStructuralLayerAutoTexturizationVisualizationModeButton->SetValue(mode == StructuralLayerVisualizationModeType::AutoTexturizationMode);
    mStructuralLayerTextureVisualizationModeButton->SetValue(mode == StructuralLayerVisualizationModeType::TextureMode);
}

void MainFrame::ReconciliateUIWithElectricalLayerVisualizationModeSelection(ElectricalLayerVisualizationModeType /*mode*/)
{
    // Nop at this moment
}

void MainFrame::ReconciliateUIWithRopesLayerVisualizationModeSelection(RopesLayerVisualizationModeType /*mode*/)
{
    // Nop at this moment
}

void MainFrame::ReconciliateUIWithTextureLayerVisualizationModeSelection(TextureLayerVisualizationModeType /*mode*/)
{
    // Nop at this moment
}

void MainFrame::ReconciliateUIWithModelDirtiness()
{
    assert(mController);

    bool const isDirty = mController->IsModelDirty();

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

    SetFrameTitle(mController->GetShipMetadata().ShipName, isDirty);
}

void MainFrame::ReconciliateUIWithWorkbenchState()
{
    assert(mController);

    // Populate swaths in toolbars
    {
        static std::string const ClearMaterialName = "Clear";

        {
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
        }

        {
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

        {
            auto const * foreRopesMaterial = mWorkbenchState.GetRopesForegroundMaterial();
            if (foreRopesMaterial != nullptr)
            {
                wxBitmap foreRopesBitmap = WxHelpers::MakeBitmap(
                    mShipTexturizer.MakeTextureSample(
                        std::nullopt, // Use shared settings
                        MaterialSwathSize,
                        *foreRopesMaterial));

                mRopesForegroundMaterialSelector->SetBitmap(foreRopesBitmap);
                mRopesForegroundMaterialSelector->SetToolTip(foreRopesMaterial->Name);
            }
            else
            {
                mRopesForegroundMaterialSelector->SetBitmap(mNullMaterialBitmap);
                mRopesForegroundMaterialSelector->SetToolTip(ClearMaterialName);
            }

            auto const * backRopesMaterial = mWorkbenchState.GetRopesBackgroundMaterial();
            if (backRopesMaterial != nullptr)
            {
                wxBitmap backRopesBitmap = WxHelpers::MakeBitmap(
                    mShipTexturizer.MakeTextureSample(
                        std::nullopt, // Use shared settings
                        MaterialSwathSize,
                        *backRopesMaterial));

                mRopesBackgroundMaterialSelector->SetBitmap(backRopesBitmap);
                mRopesBackgroundMaterialSelector->SetToolTip(backRopesMaterial->Name);
            }
            else
            {
                mRopesBackgroundMaterialSelector->SetBitmap(mNullMaterialBitmap);
                mRopesBackgroundMaterialSelector->SetToolTip(ClearMaterialName);
            }
        }
    }

    // Note: at this moment we're not populating settings in ToolSettings toolbar, as all controls
    // are initialized with the settings' current values, and the only source of change for the
    // values are the controls themselves. If that changes, this would be the place to reconciliate
    // the workbench state with the controls.
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

    //
    // Menu item
    //

    bool const canUndo = mController->CanUndo();
    if (mUndoMenuItem->IsEnabled() != canUndo)
    {
        mUndoMenuItem->Enable(canUndo);
    }

    //
    // Undo stack
    //

    wxWindowUpdateLocker scopedUpdateFreezer(mUndoStackPanel);

    // Clear
    mUndoStackPanel->GetSizer()->Clear(true);

    // Populate
    for (size_t stackItemIndex = 0; stackItemIndex < mController->GetUndoStackSize(); ++stackItemIndex)
    {
        auto button = new HighlightableTextButton(mUndoStackPanel, mController->GetUndoTitleAt(stackItemIndex));
        button->SetHighlighted(stackItemIndex == mController->GetUndoStackSize() - 1);

        button->Bind(
            wxEVT_BUTTON,
            [this, stackItemIndex](wxCommandEvent &)
            {
                mController->UndoUntil(stackItemIndex);
            });

        mUndoStackPanel->GetSizer()->Add(
            button,
            0,
            wxLEFT | wxRIGHT | wxEXPAND,
            4);
    }

    mUndoStackPanel->FitInside();
    mUndoStackPanel->Layout();

    // Scroll to bottom
    mUndoStackPanel->Scroll(wxDefaultCoord, mUndoStackPanel->GetScrollRange(wxVERTICAL));
}

}