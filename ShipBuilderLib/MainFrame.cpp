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

#include <cassert>

namespace ShipBuilder {

int constexpr ButtonMargin = 4;

static std::string const ClearMaterialName = "Clear";
ImageSize constexpr MaterialSwathSize(80, 100);

int constexpr MaxVisualizationTransparency = 128;

MainFrame::MainFrame(
    wxApp * mainApp,
    wxIcon const & icon,
    ResourceLocator const & resourceLocator,
    LocalizationManager const & localizationManager,
    MaterialDatabase const & materialDatabase,
    ShipTexturizer const & shipTexturizer,
    std::function<void(std::optional<std::filesystem::path>)> returnToGameFunctor)
    : mMainApp(mainApp)
    , mReturnToGameFunctor(std::move(returnToGameFunctor))
    , mOpenGLManager()
    , mController()
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

    SetIcon(icon);
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

            // Visualizations panel
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
                    wxPanel * panel = CreateVisualizationsPanel(mMainPanel);

                    tmpVSizer->Add(
                        panel,
                        0,
                        wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT,
                        4);
                }

                {
                    wxPanel * panel = CreateVisualizationDetailsPanel(mMainPanel);

                    tmpVSizer->Add(
                        panel,
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
            wxMenuItem * loadShipMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Load Ship...") + wxS("\tCtrl+O"), _("Load a ship"), wxITEM_NORMAL);
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
            wxMenuItem * saveShipAsMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Save Ship As..."), _("Save the current ship to a different file"), wxITEM_NORMAL);
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
            wxMenuItem * menuItem = new wxMenuItem(editMenu, wxID_ANY, _("Auto-Trim"), _("Removes empty space around the ship"), wxITEM_NORMAL);
            editMenu->Append(menuItem);
            Connect(menuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnEditAutoTrimMenuItem);
        }

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
            wxMenuItem * menuItem = new wxMenuItem(editMenu, wxID_ANY, _("Resize Ship...") + wxS("\tCtrl+R"), _("Resize the ship"), wxITEM_NORMAL);
            editMenu->Append(menuItem);
            Connect(menuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnEditResizeShipMenuItem);
        }

        {
            wxMenuItem * menuItem = new wxMenuItem(editMenu, wxID_ANY, _("Ship Properties...") + wxS("\tCtrl+P"), _("Edit the ship properties"), wxITEM_NORMAL);
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

    {
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
    }

    //
    // Initialize OpenGL manager
    //

    mOpenGLManager = std::make_unique<OpenGLManager>(
        *mWorkCanvas,
        IsStandAlone());

    //
    // Reconciliate with workbench state
    //

    ReconciliateUIWithWorkbenchState();
}

void MainFrame::OpenForNewShip()
{
    // Enqueue deferred action: New ship
    assert(!mInitialAction.has_value());
    mInitialAction.emplace(
        [this]()
        {
            DoNewShip();
        });

    // Open ourselves
    Open();
}

void MainFrame::OpenForLoadShip(std::filesystem::path const & shipFilePath)
{
    // Enqueue deferred action: Load ship
    assert(!mInitialAction.has_value());
    mInitialAction.emplace(
        [=]()
        {
            if (!DoLoadShip(shipFilePath))
            {
                // No luck loading ship...
                // ...just create new ship
                DoNewShip();
            }
        });

    // Open ourselves
    Open();
}

//
// IUserInterface
//

void MainFrame::RefreshView()
{
    assert(mWorkCanvas);

    mWorkCanvas->Refresh();
}

void MainFrame::OnViewModelChanged(ViewModel const & viewModel)
{
    ReconciliateUIWithViewModel(viewModel);
}

void MainFrame::OnShipSizeChanged(ShipSpaceSize const & shipSize)
{
    ReconciliateUIWithShipSize(shipSize);
}

void MainFrame::OnShipNameChanged(Model const & model)
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

    std::string const & newName = model.GetShipMetadata().ShipName;

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

    ReconciliateUIWithShipTitle(newName, model.GetIsDirty());
}

void MainFrame::OnLayerPresenceChanged(Model const & model)
{
    ReconciliateUIWithLayerPresence(model);
}

void MainFrame::OnModelDirtyChanged(Model const & model)
{
    ReconciliateUIWithModelDirtiness(model);
}

void MainFrame::OnStructuralMaterialChanged(MaterialPlaneType plane, StructuralMaterial const * material)
{
    ReconciliateUIWithStructuralMaterial(plane, material);
}

void MainFrame::OnElectricalMaterialChanged(MaterialPlaneType plane, ElectricalMaterial const * material)
{
    ReconciliateUIWithElectricalMaterial(plane, material);
}

void MainFrame::OnRopesMaterialChanged(MaterialPlaneType plane, StructuralMaterial const * material)
{
    ReconciliateUIWithRopesMaterial(plane, material);
}

void MainFrame::OnCurrentToolChanged(std::optional<ToolType> tool)
{
    ReconciliateUIWithSelectedTool(tool);
}

void MainFrame::OnPrimaryVisualizationChanged(VisualizationType primaryVisualization)
{
    ReconciliateUIWithPrimaryVisualizationSelection(primaryVisualization);
}

void MainFrame::OnGameVisualizationModeChanged(GameVisualizationModeType mode)
{
    ReconciliateUIWithGameVisualizationModeSelection(mode);
}

void MainFrame::OnStructuralLayerVisualizationModeChanged(StructuralLayerVisualizationModeType mode)
{
    ReconciliateUIWithStructuralLayerVisualizationModeSelection(mode);
}

void MainFrame::OnElectricalLayerVisualizationModeChanged(ElectricalLayerVisualizationModeType mode)
{
    ReconciliateUIWithElectricalLayerVisualizationModeSelection(mode);
}

void MainFrame::OnRopesLayerVisualizationModeChanged(RopesLayerVisualizationModeType mode)
{
    ReconciliateUIWithRopesLayerVisualizationModeSelection(mode);
}

void MainFrame::OnTextureLayerVisualizationModeChanged(TextureLayerVisualizationModeType mode)
{
    ReconciliateUIWithTextureLayerVisualizationModeSelection(mode);
}

void MainFrame::OnOtherVisualizationsOpacityChanged(float opacity)
{
    ReconciliateUIWithOtherVisualizationsOpacity(opacity);
}

void MainFrame::OnVisualGridEnablementChanged(bool isEnabled)
{
    ReconciliateUIWithVisualGridEnablement(isEnabled);
}

void MainFrame::OnUndoStackStateChanged(UndoStack & undoStack)
{
    ReconciliateUIWithUndoStackState(undoStack);
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

DisplayLogicalSize MainFrame::GetDisplaySize() const
{
    assert(mWorkCanvas);

    return DisplayLogicalSize(
        mWorkCanvas->GetSize().GetWidth(),
        mWorkCanvas->GetSize().GetHeight());
}

int MainFrame::GetLogicalToPhysicalPixelFactor() const
{
    assert(mWorkCanvas);

    return mWorkCanvas->GetContentScaleFactor();
}

void MainFrame::SwapRenderBuffers()
{
    assert(mWorkCanvas);

    mWorkCanvas->SwapBuffers();
}

DisplayLogicalCoordinates MainFrame::GetMouseCoordinates() const
{
    wxMouseState const mouseState = wxGetMouseState();
    int x = mouseState.GetX();
    int y = mouseState.GetY();

    // Convert to work canvas coordinates
    assert(mWorkCanvas);
    mWorkCanvas->ScreenToClient(&x, &y);

    return DisplayLogicalCoordinates(x, y);
}

bool MainFrame::IsMouseInWorkCanvas() const
{
    return IsLogicallyInWorkCanvas(GetMouseCoordinates());
}

std::optional<DisplayLogicalCoordinates> MainFrame::GetMouseCoordinatesIfInWorkCanvas() const
{
    //
    // This function basically simulates the mouse<->focused window logic, returning mouse
    // coordinates only if the work canvas would legitimately receive a mouse event
    //

    DisplayLogicalCoordinates const mouseCoords = GetMouseCoordinates();    
    if (IsLogicallyInWorkCanvas(mouseCoords))
    {
        return mouseCoords;
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

/////////////////////////////////////////////////////////////////////

wxAcceleratorEntry MainFrame::MakePlainAcceleratorKey(int key, wxMenuItem * menuItem)
{
    auto const keyId = wxNewId();
    wxAcceleratorEntry entry(wxACCEL_NORMAL, key, keyId);
    this->Bind(
        wxEVT_MENU,
        [menuItem, this](wxCommandEvent &)
    {
        // Toggle menu - if it's checkable
        if (menuItem->IsCheckable())
        {
            menuItem->Check(!menuItem->IsChecked());
        }

        // Fire menu event handler
        wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, menuItem->GetId());
        ::wxPostEvent(this, evt);
    },
        keyId);

    return entry;
}

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

wxPanel * MainFrame::CreateVisualizationsPanel(wxWindow * parent)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * rootVSizer = new wxBoxSizer(wxVERTICAL);

    rootVSizer->AddSpacer(10);

    // Visualizations management
    {
        wxGridBagSizer * visualizationManagerSizer = new wxGridBagSizer(0, 0);

        {
            auto const createButtonRow = [&](VisualizationType visualization, int iRow)
            {
                wxString const sureQuestion = _("The current changes to the layer will be lost; are you sure you want to continue?");

                // Selector
                {
                    std::string buttonBitmapName;
                    wxString buttonTooltip;
                    switch (visualization)
                    {
                        case VisualizationType::Game:
                        {
                            buttonBitmapName = "game_visualization";
                            buttonTooltip = _("Game view");
                            break;
                        }

                        case VisualizationType::ElectricalLayer:
                        {
                            buttonBitmapName = "electrical_layer";
                            buttonTooltip = _("Electrical layer");
                            break;
                        }

                        case VisualizationType::RopesLayer:
                        {
                            buttonBitmapName = "ropes_layer";
                            buttonTooltip = _("Ropes layer");
                            break;
                        }

                        case VisualizationType::StructuralLayer:
                        {
                            buttonBitmapName = "structural_layer";
                            buttonTooltip = _("Structural layer");
                            break;
                        }

                        case VisualizationType::TextureLayer:
                        {
                            buttonBitmapName = "texture_layer";
                            buttonTooltip = _("Texture layer");
                            break;
                        }
                    }

                    auto * selectorButton = new BitmapToggleButton(
                        panel,
                        mResourceLocator.GetBitmapFilePath(buttonBitmapName),
                        [this, visualization]()
                        {
                            mController->SelectPrimaryVisualization(visualization);
                        },
                        buttonTooltip);

                    visualizationManagerSizer->Add(
                        selectorButton,
                        wxGBPosition(iRow * 3, 0),
                        wxGBSpan(2, 1),
                        wxALIGN_CENTER_VERTICAL,
                        0);

                    mVisualizationSelectButtons[static_cast<size_t>(visualization)] = selectorButton;
                }

                // New
                if (visualization != VisualizationType::Game)
                {
                    BitmapButton * newButton;

                    if (visualization != VisualizationType::TextureLayer)
                    {
                        newButton = new BitmapButton(
                            panel,
                            mResourceLocator.GetBitmapFilePath("new_layer_button"),
                            [this, visualization, sureQuestion]()
                            {
                                switch (visualization)
                                {
                                    case VisualizationType::ElectricalLayer:
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

                                    case VisualizationType::RopesLayer:
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

                                    case VisualizationType::StructuralLayer:
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

                                    case VisualizationType::Game:
                                    case VisualizationType::TextureLayer:
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

                    visualizationManagerSizer->Add(
                        newButton,
                        wxGBPosition(iRow * 3, 1),
                        wxGBSpan(1, 1),
                        wxLEFT | wxRIGHT,
                        10);
                }

                // Import
                if (visualization != VisualizationType::Game)
                {
                    // TODO: also here ask user if sure when the layer is dirty

                    auto * importButton = new BitmapButton(
                        panel,
                        mResourceLocator.GetBitmapFilePath("open_layer_button"),
                        [this, visualization]()
                        {
                            // TODO
                            UnderConstructionDialog::Show(this, mResourceLocator);
                        },
                        _("Import this layer from another ship."));

                    visualizationManagerSizer->Add(
                        importButton,
                        wxGBPosition(iRow * 3 + 1, 1),
                        wxGBSpan(1, 1),
                        wxLEFT | wxRIGHT,
                        10);
                }

                // Delete
                if (visualization != VisualizationType::Game)
                {
                    BitmapButton * deleteButton;

                    if (visualization != VisualizationType::StructuralLayer)
                    {
                        deleteButton = new BitmapButton(
                            panel,
                            mResourceLocator.GetBitmapFilePath("delete_layer_button"),
                            [this, visualization, sureQuestion]()
                            {
                                switch (visualization)
                                {
                                    case VisualizationType::ElectricalLayer:
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

                                    case VisualizationType::RopesLayer:
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

                                    case VisualizationType::TextureLayer:
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

                                    case VisualizationType::Game:
                                    case VisualizationType::StructuralLayer:
                                    {
                                        assert(false);
                                        break;
                                    }
                                }
                            },
                            _("Remove this layer."));

                        visualizationManagerSizer->Add(
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

                    mLayerDeleteButtons[static_cast<size_t>(VisualizationToLayer(visualization))] = deleteButton;
                }

                // Export
                if (visualization != VisualizationType::Game)
                {
                    BitmapButton * exportButton;

                    if (visualization == VisualizationType::StructuralLayer
                        || visualization == VisualizationType::TextureLayer)
                    {
                        exportButton = new BitmapButton(
                            panel,
                            mResourceLocator.GetBitmapFilePath("save_layer_button"),
                            [this, visualization]()
                            {
                                // TODO
                                UnderConstructionDialog::Show(this, mResourceLocator);
                            },
                            _("Export this layer to a file."));

                        visualizationManagerSizer->Add(
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

                    mLayerExportButtons[static_cast<size_t>(VisualizationToLayer(visualization))] = exportButton;
                }

                // Spacer
                visualizationManagerSizer->Add(
                    new wxGBSizerItem(
                        -1,
                        12,
                        wxGBPosition(iRow * 3 + 2, 0),
                        wxGBSpan(1, LayerCount)));
            };

            createButtonRow(VisualizationType::Game, 0);

            createButtonRow(VisualizationType::StructuralLayer, 1);

            createButtonRow(VisualizationType::ElectricalLayer, 2);

            createButtonRow(VisualizationType::RopesLayer, 3);

            createButtonRow(VisualizationType::TextureLayer, 4);
        }

        rootVSizer->Add(
            visualizationManagerSizer,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);
    }

    panel->SetSizerAndFit(rootVSizer);

    return panel;
}

wxPanel * MainFrame::CreateVisualizationDetailsPanel(wxWindow * parent)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * rootHSizer = new wxBoxSizer(wxHORIZONTAL);

    // Other visualizations opacity slider
    {
        mOtherVisualizationsOpacitySlider = new wxSlider(panel, wxID_ANY, 0, 0, MaxVisualizationTransparency,
            wxDefaultPosition, wxDefaultSize, wxSL_VERTICAL | wxSL_INVERSE);

        mOtherVisualizationsOpacitySlider->Bind(
            wxEVT_SLIDER,
            [this](wxCommandEvent &)
            {
                assert(mController);
                float const opacity = OtherVisualizationsOpacitySliderToOpacity(mOtherVisualizationsOpacitySlider->GetValue());
                mController->SetOtherVisualizationsOpacity(opacity);
            });

        rootHSizer->Add(
            mOtherVisualizationsOpacitySlider,
            0, // Retain horizontal width
            wxEXPAND, // Expand vertically
            0);
    }

    rootHSizer->AddSpacer(15);

    // Viz modifiers
    {
        mVisualizationModePanelsSizer = new wxBoxSizer(wxVERTICAL);

        mVisualizationModePanelsSizer->AddSpacer(7);

        // Game viz mode
        {
            wxPanel * gameVisualizationModesPanel = new wxPanel(panel);

            wxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

            // None mode
            {
                mGameVisualizationNoneModeButton = new BitmapToggleButton(
                    gameVisualizationModesPanel,
                    mResourceLocator.GetBitmapFilePath("x_small"),
                    [this]()
                    {
                        assert(mController);
                        mController->SetGameVisualizationMode(GameVisualizationModeType::None);

                        DeviateFocus();
                    },
                    _("No visualization: the game view is not drawn."));

                vSizer->Add(
                    mGameVisualizationNoneModeButton,
                    0, // Retain V size
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }


            vSizer->AddSpacer(5);

            // Auto-texturization mode
            {
                mGameVisualizationAutoTexturizationModeButton = new BitmapToggleButton(
                    gameVisualizationModesPanel,
                    mResourceLocator.GetBitmapFilePath("autotexturization_mode_icon_small"),
                    [this]()
                    {
                        assert(mController);
                        mController->SetGameVisualizationMode(GameVisualizationModeType::AutoTexturizationMode);

                        DeviateFocus();
                    },
                    _("Auto-texturization mode: view ship as when it is auto-texturized."));

                vSizer->Add(
                    mGameVisualizationAutoTexturizationModeButton,
                    0, // Retain V size
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            vSizer->AddSpacer(5);

            // Texture mode
            {
                mGameVisualizationTextureModeButton = new BitmapToggleButton(
                    gameVisualizationModesPanel,
                    mResourceLocator.GetBitmapFilePath("structural_texture_mode_icon_small"),
                    [this]()
                    {
                        assert(mController);
                        mController->SetGameVisualizationMode(GameVisualizationModeType::TextureMode);

                        DeviateFocus();
                    },
                    _("Texture mode: view ship with texture."));

                vSizer->Add(
                    mGameVisualizationTextureModeButton,
                    0, // Retain V size
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            gameVisualizationModesPanel->SetSizerAndFit(vSizer);

            mVisualizationModePanelsSizer->Add(
                gameVisualizationModesPanel,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);

            mVisualizationModePanels[static_cast<size_t>(VisualizationType::Game)] = gameVisualizationModesPanel;
        }

        // Structure viz mode
        {
            wxPanel * structuralLayerVisualizationModesPanel = new wxPanel(panel);

            wxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

            // None mode
            {
                mStructuralLayerVisualizationNoneModeButton = new BitmapToggleButton(
                    structuralLayerVisualizationModesPanel,
                    mResourceLocator.GetBitmapFilePath("x_small"),
                    [this]()
                    {
                        assert(mController);
                        mController->SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType::None);

                        DeviateFocus();
                    },
                    _("No visualization: the structural layer is not drawn."));

                vSizer->Add(
                    mStructuralLayerVisualizationNoneModeButton,
                    0, // Retain V size
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            vSizer->AddSpacer(ButtonMargin);

            // Mesh mode
            {
                mStructuralLayerVisualizationMeshModeButton = new BitmapToggleButton(
                    structuralLayerVisualizationModesPanel,
                    mResourceLocator.GetBitmapFilePath("mesh_mode_icon_small"),
                    [this]()
                    {
                        assert(mController);
                        mController->SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType::MeshMode);

                        DeviateFocus();
                    },
                    _("Mesh mode: view the structural mesh as it is induced by the structure particles."));

                vSizer->Add(
                    mStructuralLayerVisualizationMeshModeButton,
                    0, // Retain V size
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            vSizer->AddSpacer(ButtonMargin);

            // Pixel mode
            {
                mStructuralLayerVisualizationPixelModeButton = new BitmapToggleButton(
                    structuralLayerVisualizationModesPanel,
                    mResourceLocator.GetBitmapFilePath("pixel_mode_icon_small"),
                    [this]()
                    {
                        assert(mController);
                        mController->SetStructuralLayerVisualizationMode(StructuralLayerVisualizationModeType::PixelMode);

                        DeviateFocus();
                    },
                    _("Pixel mode: view structural particles as pixels."));

                vSizer->Add(
                    mStructuralLayerVisualizationPixelModeButton,
                    0, // Retain V size
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            structuralLayerVisualizationModesPanel->SetSizerAndFit(vSizer);

            mVisualizationModePanelsSizer->Add(
                structuralLayerVisualizationModesPanel,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);

            mVisualizationModePanels[static_cast<size_t>(VisualizationType::StructuralLayer)] = structuralLayerVisualizationModesPanel;
        }

        // Electrical viz mode
        {
            wxPanel * electricalLayerVisualizationModesPanel = new wxPanel(panel);

            wxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

            // None mode
            {
                mElectricalLayerVisualizationNoneModeButton = new BitmapToggleButton(
                    electricalLayerVisualizationModesPanel,
                    mResourceLocator.GetBitmapFilePath("x_small"),
                    [this]()
                    {
                        assert(mController);
                        mController->SetElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType::None);

                        DeviateFocus();
                    },
                    _("No visualization: the electrical layer is not drawn."));

                vSizer->Add(
                    mElectricalLayerVisualizationNoneModeButton,
                    0, // Retain V size
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            vSizer->AddSpacer(ButtonMargin);

            // Pixel mode
            {
                mElectricalLayerVisualizationPixelModeButton = new BitmapToggleButton(
                    electricalLayerVisualizationModesPanel,
                    mResourceLocator.GetBitmapFilePath("pixel_mode_icon_small"),
                    [this]()
                    {
                        assert(mController);
                        mController->SetElectricalLayerVisualizationMode(ElectricalLayerVisualizationModeType::PixelMode);

                        DeviateFocus();
                    },
                    _("Pixel mode: view electrical particles as pixels."));

                vSizer->Add(
                    mElectricalLayerVisualizationPixelModeButton,
                    0, // Retain V size
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            electricalLayerVisualizationModesPanel->SetSizerAndFit(vSizer);

            mVisualizationModePanelsSizer->Add(
                electricalLayerVisualizationModesPanel,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);

            mVisualizationModePanels[static_cast<size_t>(VisualizationType::ElectricalLayer)] = electricalLayerVisualizationModesPanel;
        }

        // Ropes viz mode
        {
            wxPanel * ropesLayerVisualizationModesPanel = new wxPanel(panel);

            wxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

            // None mode
            {
                mRopesLayerVisualizationNoneModeButton = new BitmapToggleButton(
                    ropesLayerVisualizationModesPanel,
                    mResourceLocator.GetBitmapFilePath("x_small"),
                    [this]()
                    {
                        assert(mController);
                        mController->SetRopesLayerVisualizationMode(RopesLayerVisualizationModeType::None);

                        DeviateFocus();
                    },
                    _("No visualization: the ropes layer is not drawn."));

                vSizer->Add(
                    mRopesLayerVisualizationNoneModeButton,
                    0, // Retain V size
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            vSizer->AddSpacer(ButtonMargin);

            // Lines mode
            {
                mRopesLayerVisualizationLinesModeButton = new BitmapToggleButton(
                    ropesLayerVisualizationModesPanel,
                    mResourceLocator.GetBitmapFilePath("lines_mode_icon_small"),
                    [this]()
                    {
                        assert(mController);
                        mController->SetRopesLayerVisualizationMode(RopesLayerVisualizationModeType::LinesMode);

                        DeviateFocus();
                    },
                    _("Lines mode: view ropes as lines."));

                vSizer->Add(
                    mRopesLayerVisualizationLinesModeButton,
                    0, // Retain V size
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            ropesLayerVisualizationModesPanel->SetSizerAndFit(vSizer);

            mVisualizationModePanelsSizer->Add(
                ropesLayerVisualizationModesPanel,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);

            mVisualizationModePanels[static_cast<size_t>(VisualizationType::RopesLayer)] = ropesLayerVisualizationModesPanel;
        }

        // Texture viz mode
        {
            wxPanel * textureLayerVisualizationModesPanel = new wxPanel(panel);

            wxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

            // None mode
            {
                mTextureLayerVisualizationNoneModeButton = new BitmapToggleButton(
                    textureLayerVisualizationModesPanel,
                    mResourceLocator.GetBitmapFilePath("x_small"),
                    [this]()
                    {
                        assert(mController);
                        mController->SetTextureLayerVisualizationMode(TextureLayerVisualizationModeType::None);

                        DeviateFocus();
                    },
                    _("No visualization: the texture layer is not drawn."));

                vSizer->Add(
                    mTextureLayerVisualizationNoneModeButton,
                    0, // Retain V size
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            vSizer->AddSpacer(ButtonMargin);

            // Matte mode
            {
                mTextureLayerVisualizationMatteModeButton = new BitmapToggleButton(
                    textureLayerVisualizationModesPanel,
                    mResourceLocator.GetBitmapFilePath("texture_mode_icon_small"),
                    [this]()
                    {
                        assert(mController);
                        mController->SetTextureLayerVisualizationMode(TextureLayerVisualizationModeType::MatteMode);

                        DeviateFocus();
                    },
                    _("Matte mode: view texture as an opaque image."));

                vSizer->Add(
                    mTextureLayerVisualizationMatteModeButton,
                    0, // Retain V size
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            textureLayerVisualizationModesPanel->SetSizerAndFit(vSizer);

            mVisualizationModePanelsSizer->Add(
                textureLayerVisualizationModesPanel,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);

            mVisualizationModePanels[static_cast<size_t>(VisualizationType::TextureLayer)] = textureLayerVisualizationModesPanel;
        }

        mVisualizationModePanelsSizer->AddSpacer(10);

        mVisualizationModePanelsSizer->AddStretchSpacer(1);

        // View grid button
        {
            auto bitmap = WxHelpers::LoadBitmap("view_grid_button", mResourceLocator);
            mViewGridButton = new wxBitmapToggleButton(panel, wxID_ANY, bitmap, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
            mViewGridButton->SetToolTip(_("Enable/Disable the visual guides."));
            mViewGridButton->Bind(
                wxEVT_TOGGLEBUTTON,
                [this](wxCommandEvent & event)
                {
                    assert(mController);
                    mController->EnableVisualGrid(event.IsChecked());

                    DeviateFocus();
                });

            mVisualizationModePanelsSizer->Add(
                mViewGridButton,
                0, // Retain vertical width
                wxALIGN_CENTER_HORIZONTAL, // Do not expand vertically
                0);
        }

        mVisualizationModePanelsSizer->AddSpacer(7);

        // Before we freeze this panel's size, make only its "Game"
        // viz mode panel visible, which is currently the tallest
        for (size_t iVisualization = 0; iVisualization < VisualizationCount; ++iVisualization)
        {
            mVisualizationModePanelsSizer->Show(mVisualizationModePanels[iVisualization], iVisualization == static_cast<size_t>(VisualizationType::Game));
        }

        rootHSizer->Add(
            mVisualizationModePanelsSizer,
            0, // Retain horizontal width
            wxEXPAND, // Expand vertically
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
            wxGridBagSizer * toolsSizer = new wxGridBagSizer(ButtonMargin, ButtonMargin);

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
            wxGridBagSizer * toolsSizer = new wxGridBagSizer(ButtonMargin, ButtonMargin);

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
            wxGridBagSizer * toolsSizer = new wxGridBagSizer(ButtonMargin, ButtonMargin);

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
            wxGridBagSizer * toolsSizer = new wxGridBagSizer(ButtonMargin, ButtonMargin);

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
    //LogMessage("OnWorkCanvasPaint (with", (!mController ? "out" : ""), " controller, with", (!mInitialAction ? "out" : ""), " initial action)");

    // Execute initial action, if any
    if (mInitialAction.has_value())
    {
        // Extract and cleanup initial action - so a ShowModal coming from within it and kicking off a new OnPaint does not enter an infinite loop
        auto const initialAction = std::move(*mInitialAction);
        mInitialAction.reset();

        // Execute
        try
        {
            initialAction();
        }
        catch (std::exception const & e)
        {
            ShowError(std::string(e.what()));

            // No need to continue, bail out
            BailOut();
        }
    }

    // Render, if we have a controller
    if (mController)
    {
        mController->Render();
    }
}

void MainFrame::OnWorkCanvasResize(wxSizeEvent & event)
{
    LogMessage("OnWorkCanvasResize (with", (!mController ? "out" : ""), " controller): ", event.GetSize().GetX(), "x", event.GetSize().GetY());

    if (mController)
    {
        mController->OnWorkCanvasResized(GetWorkCanvasSize());
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
    if (mController)
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
    }

    event.Skip();
}

void MainFrame::OnEditUndoMenuItem(wxCommandEvent & /*event*/)
{
    mController->UndoLast();
}

void MainFrame::OnEditAutoTrimMenuItem(wxCommandEvent & /*event*/)
{
    mController->AutoTrim();
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
    assert(mController);

    mController->SetStructuralMaterial(event.GetMaterialPlane(), event.GetMaterial());
}

void MainFrame::OnElectricalMaterialSelected(fsElectricalMaterialSelectedEvent & event)
{
    assert(mController);

    mController->SetElectricalMaterial(event.GetMaterialPlane(), event.GetMaterial());
}

void MainFrame::OnRopeMaterialSelected(fsStructuralMaterialSelectedEvent & event)
{
    assert(mController);

    mController->SetRopeMaterial(event.GetMaterialPlane(), event.GetMaterial());
}

//////////////////////////////////////////////////////////////////

void MainFrame::Open()
{
    LogMessage("MainFrame::Open()");

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
    // Hide self
    Show(false);

    // Let go of controller
    mController.reset();

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

                image = image.MakeReframed(
                    ImageSize(targetSize.width, targetSize.height),
                    ImageCoordinates(originOffset.x, originOffset.y),
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

    // Make ship preview
    auto const shipPreviewImage = mController->MakePreview();

    // Start with target size == current ship size
    IntegralRectSize const initialTargetSize(mController->GetShipSize().width, mController->GetShipSize().height);

    if (!mResizeDialog->ShowModalForResize(*shipPreviewImage, initialTargetSize))
    {
        // User aborted
        return;
    }

    //
    // Resize
    //

    auto const targetSize = mResizeDialog->GetTargetSize();
    auto const originOffset = mResizeDialog->GetOffset();

    mController->ResizeShip(
        ShipSpaceSize(targetSize.width, targetSize.height),
        ShipSpaceCoordinates(originOffset.x, originOffset.y));
}

void MainFrame::OpenShipProperties()
{
    if (!mShipPropertiesEditDialog)
    {
        mShipPropertiesEditDialog = std::make_unique<ShipPropertiesEditDialog>(this, mResourceLocator);
    }

    // Make ship preview
    auto const shipPreviewImage = mController->MakePreview();

    mShipPropertiesEditDialog->ShowModal(
        *mController,
        mController->GetShipMetadata(),
        mController->GetShipPhysicsData(),
        mController->GetShipAutoTexturizationSettings(),
        *shipPreviewImage,
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

    // Dispose of current controller - including its OpenGL machinery
    mController.reset();

    // Reset current ship filename
    mCurrentShipFilePath.reset();

    // Create new controller with empty ship
    mController = Controller::CreateNew(
        shipName,
        *mOpenGLManager,
        mWorkbenchState,
        *this,
        mShipTexturizer,
        mResourceLocator);
}

bool MainFrame::DoLoadShip(std::filesystem::path const & shipFilePath)
{
    //
    // Load definition
    //

    std::optional<ShipDefinition> shipDefinition;
    try
    {
        shipDefinition.emplace(ShipDeSerializer::LoadShip(shipFilePath, mMaterialDatabase));
    }
    catch (UserGameException const & exc)
    {
        ShowError(mLocalizationManager.MakeErrorMessage(exc));
        return false;
    }
    catch (std::runtime_error const & exc)
    {
        ShowError(exc.what());
        return false;
    }

    assert(shipDefinition.has_value());

    //
    // Check password
    //

    if (!AskPasswordDialog::CheckPasswordProtectedEdit(*shipDefinition, this, mResourceLocator))
    {
        return false;
    }

    //
    // Recreate controller
    //

    // Dispose of current controller - including its OpenGL machinery
    mController.reset();

    // Reset current ship filename
    mCurrentShipFilePath.reset();

    // Create new controller with ship
    mController = Controller::CreateForShip(
        std::move(*shipDefinition),
        *mOpenGLManager,
        mWorkbenchState,
        *this,
        mShipTexturizer,
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

    // Success
    return true;
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

void MainFrame::BailOut()
{
    if (IsStandAlone())
    {
        Close();
    }
    else
    {
        SwitchBackToGame(std::nullopt);
    }
}

bool MainFrame::IsLogicallyInWorkCanvas(DisplayLogicalCoordinates const & coords) const
{
    // Check if in canvas (but not captured by scrollbars), or if simply captured
    return (mWorkCanvas->HitTest(coords.x, coords.y) == wxHT_WINDOW_INSIDE && !mWorkCanvasHScrollBar->HasCapture() && !mWorkCanvasVScrollBar->HasCapture())
        || mIsMouseCapturedByWorkCanvas;
}

DisplayLogicalSize MainFrame::GetWorkCanvasSize() const
{
    return DisplayLogicalSize(
        mWorkCanvas->GetSize().GetWidth(),
        mWorkCanvas->GetSize().GetHeight());
}

void MainFrame::RecalculateWorkCanvasPanning(ViewModel const & viewModel)
{
    //
    // We populate the scollbar with work space coordinates
    //

    ShipSpaceCoordinates const cameraPos = viewModel.GetCameraShipSpacePosition();
    ShipSpaceSize const cameraRange = viewModel.GetCameraRange();
    ShipSpaceSize const cameraThumbSize = viewModel.GetCameraThumbSize();
        

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
    // Set focus on primary visualization button
    uint32_t const iPrimaryVisualization = static_cast<uint32_t>(mWorkbenchState.GetPrimaryVisualization());
    mVisualizationSelectButtons[iPrimaryVisualization]->SetFocus();
}

float MainFrame::OtherVisualizationsOpacitySliderToOpacity(int sliderValue) const
{
    float const opacity =
        static_cast<float>(sliderValue)
        / static_cast<float>(MaxVisualizationTransparency);

    return opacity;
}

int MainFrame::OtherVisualizationsOpacityToSlider(float opacityValue) const
{
    int const sliderValue = static_cast<int>(
        opacityValue * static_cast<float>(MaxVisualizationTransparency));

    return sliderValue;
}

size_t MainFrame::LayerToVisualizationIndex(LayerType layer) const
{
    switch (layer)
    {
        case LayerType::Structural:
        {
            return static_cast<size_t>(VisualizationType::StructuralLayer);
        }

        case LayerType::Electrical:
        {
            return static_cast<size_t>(VisualizationType::ElectricalLayer);
        }

        case LayerType::Ropes:
        {
            return static_cast<size_t>(VisualizationType::RopesLayer);
        }

        case LayerType::Texture:
        {
            return static_cast<size_t>(VisualizationType::TextureLayer);
        }
    }

    assert(false);
    return std::numeric_limits<size_t>::max();
}

void MainFrame::ReconciliateUIWithWorkbenchState()
{
    ReconciliateUIWithStructuralMaterial(MaterialPlaneType::Foreground, mWorkbenchState.GetStructuralForegroundMaterial());
    ReconciliateUIWithStructuralMaterial(MaterialPlaneType::Background, mWorkbenchState.GetStructuralBackgroundMaterial());
    ReconciliateUIWithElectricalMaterial(MaterialPlaneType::Foreground, mWorkbenchState.GetElectricalForegroundMaterial());
    ReconciliateUIWithElectricalMaterial(MaterialPlaneType::Background, mWorkbenchState.GetElectricalBackgroundMaterial());
    ReconciliateUIWithRopesMaterial(MaterialPlaneType::Foreground, mWorkbenchState.GetRopesForegroundMaterial());
    ReconciliateUIWithRopesMaterial(MaterialPlaneType::Background, mWorkbenchState.GetRopesBackgroundMaterial());

    ReconciliateUIWithSelectedTool(mWorkbenchState.GetCurrentToolType());

    ReconciliateUIWithPrimaryVisualizationSelection(mWorkbenchState.GetPrimaryVisualization());

    ReconciliateUIWithGameVisualizationModeSelection(mWorkbenchState.GetGameVisualizationMode());
    ReconciliateUIWithStructuralLayerVisualizationModeSelection(mWorkbenchState.GetStructuralLayerVisualizationMode());
    ReconciliateUIWithElectricalLayerVisualizationModeSelection(mWorkbenchState.GetElectricalLayerVisualizationMode());
    ReconciliateUIWithRopesLayerVisualizationModeSelection(mWorkbenchState.GetRopesLayerVisualizationMode());
    ReconciliateUIWithTextureLayerVisualizationModeSelection(mWorkbenchState.GetTextureLayerVisualizationMode());

    ReconciliateUIWithOtherVisualizationsOpacity(mWorkbenchState.GetOtherVisualizationsOpacity());

    ReconciliateUIWithVisualGridEnablement(mWorkbenchState.IsGridEnabled());
}

void MainFrame::ReconciliateUIWithViewModel(ViewModel const & viewModel)
{
    RecalculateWorkCanvasPanning(viewModel);

    // TODO: set zoom in StatusBar
}

void MainFrame::ReconciliateUIWithShipSize(ShipSpaceSize const & shipSize)
{
    // TODO: status bar
}

void MainFrame::ReconciliateUIWithShipTitle(std::string const & shipName, bool isShipDirty)
{
    SetFrameTitle(shipName, isShipDirty);
}

void MainFrame::ReconciliateUIWithLayerPresence(Model const & model)
{
    //
    // Rules
    //
    // Presence button: if HasLayer
    // New, Load: always
    // Delete, Save: if HasLayer
    // Game viz auto-texturization mode: only if texture layer not present
    // Game viz texture mode: only if texture layer present
    //

    for (uint32_t iLayer = 0; iLayer < LayerCount; ++iLayer)
    {
        bool const hasLayer = model.HasLayer(static_cast<LayerType>(iLayer));

        mVisualizationSelectButtons[LayerToVisualizationIndex(static_cast<LayerType>(iLayer))]->Enable(hasLayer);

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

    if (model.HasLayer(LayerType::Texture))
    {
        mGameVisualizationAutoTexturizationModeButton->Enable(false);
        mGameVisualizationTextureModeButton->Enable(true);
    }
    else
    {
        mGameVisualizationAutoTexturizationModeButton->Enable(true);
        mGameVisualizationTextureModeButton->Enable(false);
    }

    mVisualizationSelectButtons[static_cast<size_t>(mWorkbenchState.GetPrimaryVisualization())]->SetFocus(); // Prevent other random buttons from getting focus
}

void MainFrame::ReconciliateUIWithModelDirtiness(Model const & model)
{
    bool const isDirty = model.GetIsDirty();

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

    SetFrameTitle(model.GetShipMetadata().ShipName, isDirty);
}

void MainFrame::ReconciliateUIWithStructuralMaterial(MaterialPlaneType plane, StructuralMaterial const * material)
{
    switch (plane)
    {
        case MaterialPlaneType::Foreground:
        {
            if (material != nullptr)
            {
                wxBitmap foreStructuralBitmap = WxHelpers::MakeBitmap(
                    mShipTexturizer.MakeTextureSample(
                        std::nullopt, // Use shared settings
                        MaterialSwathSize,
                        *material));

                mStructuralForegroundMaterialSelector->SetBitmap(foreStructuralBitmap);
                mStructuralForegroundMaterialSelector->SetToolTip(material->Name);
            }
            else
            {
                mStructuralForegroundMaterialSelector->SetBitmap(mNullMaterialBitmap);
                mStructuralForegroundMaterialSelector->SetToolTip(ClearMaterialName);
            }

            break;
        }

        case MaterialPlaneType::Background:
        {
            if (material != nullptr)
            {
                wxBitmap backStructuralBitmap = WxHelpers::MakeBitmap(
                    mShipTexturizer.MakeTextureSample(
                        std::nullopt, // Use shared settings
                        MaterialSwathSize,
                        *material));

                mStructuralBackgroundMaterialSelector->SetBitmap(backStructuralBitmap);
                mStructuralBackgroundMaterialSelector->SetToolTip(material->Name);
            }
            else
            {
                mStructuralBackgroundMaterialSelector->SetBitmap(mNullMaterialBitmap);
                mStructuralBackgroundMaterialSelector->SetToolTip(ClearMaterialName);
            }

            break;
        }
    }
}

void MainFrame::ReconciliateUIWithElectricalMaterial(MaterialPlaneType plane, ElectricalMaterial const * material)
{
    switch (plane)
    {
        case MaterialPlaneType::Foreground:
        {
            if (material != nullptr)
            {
                wxBitmap foreElectricalBitmap = WxHelpers::MakeMatteBitmap(
                    rgbaColor(material->RenderColor, 255),
                    MaterialSwathSize);

                mElectricalForegroundMaterialSelector->SetBitmap(foreElectricalBitmap);
                mElectricalForegroundMaterialSelector->SetToolTip(material->Name);
            }
            else
            {
                mElectricalForegroundMaterialSelector->SetBitmap(mNullMaterialBitmap);
                mElectricalForegroundMaterialSelector->SetToolTip(ClearMaterialName);
            }

            break;
        }

        case MaterialPlaneType::Background:
        {
            if (material != nullptr)
            {
                wxBitmap backElectricalBitmap = WxHelpers::MakeMatteBitmap(
                    rgbaColor(material->RenderColor, 255),
                    MaterialSwathSize);

                mElectricalBackgroundMaterialSelector->SetBitmap(backElectricalBitmap);
                mElectricalBackgroundMaterialSelector->SetToolTip(material->Name);
            }
            else
            {
                mElectricalBackgroundMaterialSelector->SetBitmap(mNullMaterialBitmap);
                mElectricalBackgroundMaterialSelector->SetToolTip(ClearMaterialName);
            }

            break;
        }
    }
}

void MainFrame::ReconciliateUIWithRopesMaterial(MaterialPlaneType plane, StructuralMaterial const * material)
{
    switch (plane)
    {
        case MaterialPlaneType::Foreground:
        {
            if (material != nullptr)
            {
                wxBitmap foreRopesBitmap = WxHelpers::MakeBitmap(
                    mShipTexturizer.MakeTextureSample(
                        std::nullopt, // Use shared settings
                        MaterialSwathSize,
                        *material));

                mRopesForegroundMaterialSelector->SetBitmap(foreRopesBitmap);
                mRopesForegroundMaterialSelector->SetToolTip(material->Name);
            }
            else
            {
                mRopesForegroundMaterialSelector->SetBitmap(mNullMaterialBitmap);
                mRopesForegroundMaterialSelector->SetToolTip(ClearMaterialName);
            }

            break;
        }

        case MaterialPlaneType::Background:
        {
            if (material != nullptr)
            {
                wxBitmap backRopesBitmap = WxHelpers::MakeBitmap(
                    mShipTexturizer.MakeTextureSample(
                        std::nullopt, // Use shared settings
                        MaterialSwathSize,
                        *material));

                mRopesBackgroundMaterialSelector->SetBitmap(backRopesBitmap);
                mRopesBackgroundMaterialSelector->SetToolTip(material->Name);
            }
            else
            {
                mRopesBackgroundMaterialSelector->SetBitmap(mNullMaterialBitmap);
                mRopesBackgroundMaterialSelector->SetToolTip(ClearMaterialName);
            }

            break;
        }
    }
}

void MainFrame::ReconciliateUIWithSelectedTool(std::optional<ToolType> tool)
{
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

void MainFrame::ReconciliateUIWithPrimaryVisualizationSelection(VisualizationType primaryVisualization)
{
    //
    // Toggle various UI elements <-> primary viz
    //

    bool hasToggledToolPanel = false;
    bool hasToggledLayerVisualizationModePanel = false;

    auto const iPrimaryVisualization = static_cast<uint32_t>(primaryVisualization);
    for (uint32_t iVisualization = 0; iVisualization < VisualizationCount; ++iVisualization)
    {
        bool const isVisualizationSelected = (iVisualization == iPrimaryVisualization);

        // Visualization selection buttons
        if (mVisualizationSelectButtons[iVisualization]->GetValue() != isVisualizationSelected)
        {
            mVisualizationSelectButtons[iVisualization]->SetValue(isVisualizationSelected);

            if (isVisualizationSelected)
            {
                mVisualizationSelectButtons[iVisualization]->SetFocus(); // Prevent other random buttons for getting focus
            }
        }

        // Layer visualization mode panels
        if (mVisualizationModePanelsSizer->IsShown(mVisualizationModePanels[iVisualization]) != isVisualizationSelected)
        {
            mVisualizationModePanelsSizer->Show(mVisualizationModePanels[iVisualization], isVisualizationSelected);
            hasToggledLayerVisualizationModePanel = true;
        }
    }

    auto const iPrimaryLayer = static_cast<uint32_t>(VisualizationToLayer(primaryVisualization));
    for (uint32_t iLayer = 0; iLayer < LayerCount; ++iLayer)
    {
        // Toolbar panels
        bool const isLayerSelected = (iLayer == iPrimaryLayer);
        if (mToolbarPanelsSizer->IsShown(mToolbarPanels[iLayer]) != isLayerSelected)
        {
            mToolbarPanelsSizer->Show(mToolbarPanels[iLayer], isLayerSelected);
            hasToggledToolPanel = true;
        }
    }

    if (hasToggledLayerVisualizationModePanel)
    {
        mVisualizationModePanelsSizer->Layout();
    }

    if (hasToggledToolPanel)
    {
        mToolbarPanelsSizer->Layout();
    }
}

void MainFrame::ReconciliateUIWithGameVisualizationModeSelection(GameVisualizationModeType mode)
{
    mGameVisualizationNoneModeButton->SetValue(mode == GameVisualizationModeType::None);
    mGameVisualizationAutoTexturizationModeButton->SetValue(mode == GameVisualizationModeType::AutoTexturizationMode);
    mGameVisualizationTextureModeButton->SetValue(mode == GameVisualizationModeType::TextureMode);
}

void MainFrame::ReconciliateUIWithStructuralLayerVisualizationModeSelection(StructuralLayerVisualizationModeType mode)
{
    mStructuralLayerVisualizationNoneModeButton->SetValue(mode == StructuralLayerVisualizationModeType::None);
    mStructuralLayerVisualizationMeshModeButton->SetValue(mode == StructuralLayerVisualizationModeType::MeshMode);
    mStructuralLayerVisualizationPixelModeButton->SetValue(mode == StructuralLayerVisualizationModeType::PixelMode);
}

void MainFrame::ReconciliateUIWithElectricalLayerVisualizationModeSelection(ElectricalLayerVisualizationModeType mode)
{
    mElectricalLayerVisualizationNoneModeButton->SetValue(mode == ElectricalLayerVisualizationModeType::None);
    mElectricalLayerVisualizationPixelModeButton->SetValue(mode == ElectricalLayerVisualizationModeType::PixelMode);
}

void MainFrame::ReconciliateUIWithRopesLayerVisualizationModeSelection(RopesLayerVisualizationModeType mode)
{
    mRopesLayerVisualizationNoneModeButton->SetValue(mode == RopesLayerVisualizationModeType::None);
    mRopesLayerVisualizationLinesModeButton->SetValue(mode == RopesLayerVisualizationModeType::LinesMode);
}

void MainFrame::ReconciliateUIWithTextureLayerVisualizationModeSelection(TextureLayerVisualizationModeType mode)
{
    mTextureLayerVisualizationNoneModeButton->SetValue(mode == TextureLayerVisualizationModeType::None);
    mTextureLayerVisualizationMatteModeButton->SetValue(mode == TextureLayerVisualizationModeType::MatteMode);
}

void MainFrame::ReconciliateUIWithOtherVisualizationsOpacity(float opacity)
{
    auto const sliderValue = OtherVisualizationsOpacityToSlider(opacity);
    if (sliderValue != mOtherVisualizationsOpacitySlider->GetValue())
    {
        mOtherVisualizationsOpacitySlider->SetValue(sliderValue);
    }
}

void MainFrame::ReconciliateUIWithVisualGridEnablement(bool isEnabled)
{
    if (mViewGridButton->GetValue() != isEnabled)
    {
        mViewGridButton->SetValue(isEnabled);
    }
}

void MainFrame::ReconciliateUIWithUndoStackState(UndoStack & undoStack)
{
    //
    // Menu item
    //

    bool const canUndo = !undoStack.IsEmpty();
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
    for (size_t stackItemIndex = 0; stackItemIndex < undoStack.GetSize(); ++stackItemIndex)
    {
        auto button = new HighlightableTextButton(mUndoStackPanel, undoStack.GetTitleAt(stackItemIndex));
        button->SetHighlighted(stackItemIndex == undoStack.GetSize() - 1);

        button->Bind(
            wxEVT_BUTTON,
            [this, stackItemIndex](wxCommandEvent &)
            {
                assert(mController);
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