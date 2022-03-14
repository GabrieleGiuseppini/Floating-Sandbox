/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-08-27
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MainFrame.h"

#include "AskPasswordDialog.h"
#include "WaterlineAnalyzerDialog.h"

#include <UILib/HighlightableTextButton.h>
#include <UILib/EditSpinBox.h>
#include <UILib/ImageLoadDialog.h>
#include <UILib/ImageSaveDialog.h>
#include <UILib/UnderConstructionDialog.h>
#include <UILib/WxHelpers.h>

#include <Game/ImageFileTools.h>
#include <Game/ShipDeSerializer.h>

#include <GameOpenGL/GameOpenGL.h>

#include <GameCore/Log.h>
#include <GameCore/UserGameException.h>
#include <GameCore/Utils.h>
#include <GameCore/Version.h>

#include <wx/button.h>
#include <wx/cursor.h>
#include <wx/gbsizer.h>
#include <wx/msgdlg.h>
#include <wx/ribbon/art.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/tglbtn.h>
#include <wx/utils.h>
#include <wx/wupdlock.h>

#include <cassert>

namespace ShipBuilder {

int const RibbonToolbarButtonMargin = 6;
int constexpr ButtonMargin = 4;
int constexpr LabelMargin = 3;

static std::string const ClearMaterialName = "Clear";
ImageSize constexpr MaterialSwathSize(50, 65);

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
    // UI State
    , mIsMouseInWorkCanvas(false)
    , mIsMouseCapturedByWorkCanvas(false)
    , mIsShiftKeyDown(false)
    // State
    , mWorkbenchState(materialDatabase)
{
    Create(
        nullptr,
        wxID_ANY,
        std::string(APPLICATION_NAME " ShipBuilder " APPLICATION_VERSION_SHORT_STR),
        wxDefaultPosition,
        wxDefaultSize,
        wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxRESIZE_BORDER | wxCAPTION | wxCLIP_CHILDREN | wxMAXIMIZE
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
    // Row 0: [                    Ribbon                 ]
    // Row 1: [Viz Mode Header Panel] |    [Work Canvas]
    //          [Viz Details Panel]   |
    //            [Toolbar Panel]     |
    // Row 2: [                   Status Bar               ]
    //

    mMainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxSizer * mainVSizer = new wxBoxSizer(wxVERTICAL);

    // Row 0
    {
        wxBoxSizer * row0HSizer = new wxBoxSizer(wxHORIZONTAL);

        // Ribbon
        {
            mMainRibbonBar = new wxRibbonBar(mMainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxRIBBON_BAR_FLOW_HORIZONTAL | wxRIBBON_BAR_SHOW_PAGE_LABELS);

            // Configure look'n'feel
            {
                auto * artProvider = mMainRibbonBar->GetArtProvider();

                auto const backgroundColor = GetBackgroundColour();

                artProvider->SetColor(wxRIBBON_ART_PAGE_BACKGROUND_COLOUR, backgroundColor);
                artProvider->SetColor(wxRIBBON_ART_PAGE_BACKGROUND_GRADIENT_COLOUR, backgroundColor);
                artProvider->SetColor(wxRIBBON_ART_PAGE_BACKGROUND_TOP_COLOUR, backgroundColor);
                artProvider->SetColor(wxRIBBON_ART_PAGE_BACKGROUND_TOP_GRADIENT_COLOUR, backgroundColor);

                artProvider->SetColor(wxRIBBON_ART_PAGE_HOVER_BACKGROUND_COLOUR, backgroundColor);
                artProvider->SetColor(wxRIBBON_ART_PAGE_HOVER_BACKGROUND_GRADIENT_COLOUR, backgroundColor);
                artProvider->SetColor(wxRIBBON_ART_PAGE_HOVER_BACKGROUND_TOP_COLOUR, backgroundColor);
                artProvider->SetColor(wxRIBBON_ART_PAGE_HOVER_BACKGROUND_TOP_GRADIENT_COLOUR, backgroundColor);

                artProvider->SetColor(wxRIBBON_ART_TAB_CTRL_BACKGROUND_COLOUR, backgroundColor);
                artProvider->SetColor(wxRIBBON_ART_TAB_CTRL_BACKGROUND_GRADIENT_COLOUR, backgroundColor);
            }

            CreateMainRibbonPage(mMainRibbonBar);
            CreateLayersRibbonPage(mMainRibbonBar);
            CreateEditRibbonPage(mMainRibbonBar);

            mMainRibbonBar->SetActivePage(1); // We start with the widest of the pages
            mMainRibbonBar->Realize();

            row0HSizer->Add(
                mMainRibbonBar,
                0, // Maintain Width (that of the widest tab)
                0, // Maintain H
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

            // Visualization Mode Header panel
            {
                wxPanel * panel = CreateVisualizationModeHeaderPanel(mMainPanel);

                row1Col0VSizer->Add(
                    panel,
                    0, // Maintain own height
                    wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP,
                    ButtonMargin + ButtonMargin); // This is the widest panel, hence we set a border only around it
            }

            // Spacer
            {
                row1Col0VSizer->AddSpacer(8);
            }

            // Visualizations panel
            {
                wxPanel * panel = CreateVisualizationDetailsPanel(mMainPanel);

                row1Col0VSizer->Add(
                    panel,
                    1, // Expand vertically, a little
                    wxALIGN_CENTER_HORIZONTAL,
                    0);
            }

            // Separator
            {
                wxStaticLine * line = new wxStaticLine(mMainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);

                row1Col0VSizer->Add(
                    line,
                    0,
                    wxEXPAND | wxTOP | wxBOTTOM,
                    8);
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
                    wxEXPAND | wxTOP | wxBOTTOM,
                    8);
            }

            // Undo panel
            {
                wxPanel * undoPanel = CreateUndoPanel(mMainPanel);

                row1Col0VSizer->Add(
                    undoPanel,
                    2, // Expand vertically
                    wxEXPAND, // Expand horizontally
                    0);
            }

            row1HSizer->Add(
                row1Col0VSizer,
                0, // Maintain own width
                wxEXPAND, // Expand vertically
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
            mStatusBar = new StatusBar(mMainPanel, mWorkbenchState.GetDisplayUnitsSystem(), mResourceLocator);

            mainVSizer->Add(
                mStatusBar,
                0,
                wxEXPAND, // Expand horizontally
                0);
        }
    }

    mMainPanel->SetSizer(mainVSizer);

    //
    // Initialize accelerator
    //

    // Add Open Log Window
    AddAcceleratorKey(wxACCEL_CTRL, (int)'L',
        [this]()
        {
            if (!mLoggingDialog)
            {
                mLoggingDialog = std::make_unique<LoggingDialog>(this);
            }

            mLoggingDialog->Open();
        });

    wxAcceleratorTable acceleratorTable(mAcceleratorEntries.size(), mAcceleratorEntries.data());
    SetAcceleratorTable(acceleratorTable);

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

    //
    // Create dialogs
    //

    mShipLoadDialog = std::make_unique<ShipLoadDialog>(
        this,
        mResourceLocator);
}

void MainFrame::OpenForNewShip(std::optional<UnitsSystem> displayUnitsSystem)
{
    // Set units system
    if (displayUnitsSystem.has_value())
    {
        mWorkbenchState.SetDisplayUnitsSystem(*displayUnitsSystem);
        ReconciliateUIWithDisplayUnitsSystem(*displayUnitsSystem);
    }

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

void MainFrame::OpenForLoadShip(
    std::filesystem::path const & shipFilePath,
    std::optional<UnitsSystem> displayUnitsSystem)
{
    // Set units system
    if (displayUnitsSystem.has_value())
    {
        mWorkbenchState.SetDisplayUnitsSystem(*displayUnitsSystem);
        ReconciliateUIWithDisplayUnitsSystem(*displayUnitsSystem);
    }

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

void MainFrame::OnShipScaleChanged(ShipSpaceToWorldSpaceCoordsRatio const & scale)
{
    ReconciliateUIWithShipScale(scale);
}

void MainFrame::OnShipNameChanged(IModelObservable const & model)
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

    ReconciliateUIWithShipTitle(newName, model.IsDirty());
}

void MainFrame::OnLayerPresenceChanged(IModelObservable const & model)
{
    ReconciliateUIWithLayerPresence(model);
}

void MainFrame::OnModelDirtyChanged(IModelObservable const & model)
{
    ReconciliateUIWithModelDirtiness(model);
}

void MainFrame::OnModelMacroPropertiesUpdated(ModelMacroProperties const & properties)
{
    ReconciliateUIWithModelMacroProperties(properties);
}

void MainFrame::OnStructuralMaterialChanged(StructuralMaterial const * material, MaterialPlaneType plane)
{
    ReconciliateUIWithStructuralMaterial(material, plane);
}

void MainFrame::OnElectricalMaterialChanged(ElectricalMaterial const * material, MaterialPlaneType plane)
{
    ReconciliateUIWithElectricalMaterial(material, plane);
}

void MainFrame::OnRopesMaterialChanged(StructuralMaterial const * material, MaterialPlaneType plane)
{
    ReconciliateUIWithRopesMaterial(material, plane);
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

void MainFrame::OnVisualWaterlineMarkersEnablementChanged(bool isEnabled)
{
    ReconciliateUIWithVisualWaterlineMarkersEnablement(isEnabled);
}

void MainFrame::OnVisualGridEnablementChanged(bool isEnabled)
{
    ReconciliateUIWithVisualGridEnablement(isEnabled);
}

void MainFrame::OnUndoStackStateChanged(UndoStack & undoStack)
{
    ReconciliateUIWithUndoStackState(undoStack);
}

void MainFrame::OnToolCoordinatesChanged(std::optional<ShipSpaceCoordinates> coordinates, ShipSpaceSize const & shipSize)
{
    if (coordinates.has_value())
    {
        // Flip coordinates: we show zero at top, just to be consistent with drawing software
        coordinates->FlipY(shipSize.height);
    }

    mStatusBar->SetToolCoordinates(coordinates);
}

void MainFrame::OnSampledMaterialChanged(std::optional<std::string> materialName)
{
    mStatusBar->SetSampledMaterial(materialName);
}

void MainFrame::OnMeasuredWorldLengthChanged(std::optional<int> length)
{
    mStatusBar->SetMeasuredWorldLength(length);
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

wxRibbonPage * MainFrame::CreateMainRibbonPage(wxRibbonBar * parent)
{
    wxRibbonPage * page = new wxRibbonPage(parent, wxID_ANY, _("File"));

    CreateMainFileRibbonPanel(page);
    CreateMainViewRibbonPanel(page);

    return page;
}

wxRibbonPanel * MainFrame::CreateMainFileRibbonPanel(wxRibbonPage * parent)
{
    wxRibbonPanel * panel = new wxRibbonPanel(parent, wxID_ANY, _("File"), wxNullBitmap, wxDefaultPosition, wxDefaultSize,
        wxRIBBON_PANEL_NO_AUTO_MINIMISE);

    wxGridBagSizer * panelGridSizer = new wxGridBagSizer(RibbonToolbarButtonMargin, RibbonToolbarButtonMargin + RibbonToolbarButtonMargin);

    // New ship
    {
        auto * button = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("new_ship_button"),
            _("New Ship"),
            [this]()
            {
                NewShip();
            },
            _("Create a new empty ship (Ctrl+N)."));

        panelGridSizer->Add(button);

        AddAcceleratorKey(wxACCEL_CTRL, (int)'N',
            [this]()
            {
                // With keys we have no insurance of a controller
                if (mController)
                {
                    NewShip();
                }
            });
    }

    // Load ship
    {
        auto * button = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("load_ship_button"),
            _("Load Ship"),
            [this]()
            {
                LoadShip();
            },
            _("Load a ship (Ctrl+O)."));

        panelGridSizer->Add(button);

        AddAcceleratorKey(wxACCEL_CTRL, (int)'O',
            [this]()
            {
                // With keys we have no insurance of a controller
                if (mController)
                {
                    LoadShip();
                }
            });
    }

    // Save ship
    {
        mSaveShipButton = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("save_ship_button"),
            _("Save Ship"),
            [this]()
            {
                SaveShip();
            },
            _("Save the current ship."));

        panelGridSizer->Add(mSaveShipButton);

        AddAcceleratorKey(wxACCEL_CTRL, (int)'S',
            [this]()
            {
                // With keys we have no insurance of a controller
                if (mController)
                {
                    SaveShip();
                }
            });
    }

    // Save As ship
    {
        auto * button = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("save_ship_as_button"),
            _("Save Ship As"),
            [this]()
            {
                SaveShipAs();
            },
            _("Save the current ship to a different file."));

        panelGridSizer->Add(button);
    }

    // Save and return to game
    if (!IsStandAlone())
    {
        mSaveShipAndGoBackButton = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("save_and_return_to_game_button"),
            _("Save And Return"),
            [this]()
            {
                SaveAndSwitchBackToGame();
            },
            _("Save the current ship and return to the simulator."));

        panelGridSizer->Add(mSaveShipAndGoBackButton);
    }
    else
    {
        mSaveShipAndGoBackButton = nullptr;
    }

    // Quit and return to game
    if (!IsStandAlone())
    {
        auto * button = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("quit_and_return_to_game_button"),
            _("Abandon And Return"),
            [this]()
            {
                QuitAndSwitchBackToGame();
            },
            _("Discard the current ship and return to the simulator."));

        panelGridSizer->Add(button);
    }

    // Quit
    if (IsStandAlone())
    {
        auto * button = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("quit_button"),
            _("Quit"),
            [this]()
            {
                Quit();
            },
            _("Quit and leave the program (ALT+F4)."));

        panelGridSizer->Add(button);

        AddAcceleratorKey(wxACCEL_ALT, (int)WXK_F4,
            [this]()
            {
                Quit();
            });
    }

    // Wrap in a sizer just for margins
    {
        wxSizer * tmpSizer = new wxBoxSizer(wxVERTICAL); // Arbitrary

        tmpSizer->Add(
            panelGridSizer,
            0,
            wxALL,
            RibbonToolbarButtonMargin);

        panel->SetSizerAndFit(tmpSizer);
    }

    return panel;
}

wxRibbonPanel * MainFrame::CreateMainViewRibbonPanel(wxRibbonPage * parent)
{
    wxRibbonPanel * panel = new wxRibbonPanel(parent, wxID_ANY, _("View"), wxNullBitmap, wxDefaultPosition, wxDefaultSize,
        wxRIBBON_PANEL_NO_AUTO_MINIMISE);

    wxGridBagSizer * panelGridSizer = new wxGridBagSizer(RibbonToolbarButtonMargin, RibbonToolbarButtonMargin + RibbonToolbarButtonMargin);

    // Zoom In
    {
        mZoomInButton = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("zoom_in_medium"),
            _("Zoom In"),
            [this]()
            {
                ZoomIn();
            },
            _("Magnify the view (+)."));

        panelGridSizer->Add(mZoomInButton);

        AddAcceleratorKey(wxACCEL_NORMAL, (int)'+',
            [this]()
            {
                // With keys we have no insurance of a controller
                if (mController)
                {
                    ZoomIn();
                }
            });
    }

    // Zoom Out
    {
        mZoomOutButton = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("zoom_out_medium"),
            _("Zoom Out"),
            [this]()
            {
                ZoomOut();
            },
            _("Reduce the view (-)."));

        panelGridSizer->Add(mZoomOutButton);

        AddAcceleratorKey(wxACCEL_NORMAL, (int)'-',
            [this]()
            {
                // With keys we have no insurance of a controller
                if (mController)
                {
                    ZoomOut();
                }
            });
    }

    // Reset View
    {
        auto * button = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("zoom_reset_medium"),
            _("Reset View"),
            [this]()
            {
                ResetView();
            },
            _("Restore view settings to their defaults (HOME)."));

        panelGridSizer->Add(button);

        AddAcceleratorKey(wxACCEL_NORMAL, (int)WXK_HOME,
            [this]()
            {
                // With keys we have no insurance of a controller
                if (mController)
                {
                    ResetView();
                }
            });
    }

    // Wrap in a sizer just for margins
    {
        wxSizer * tmpSizer = new wxBoxSizer(wxVERTICAL); // Arbitrary

        tmpSizer->Add(
            panelGridSizer,
            0,
            wxALL,
            RibbonToolbarButtonMargin);

        panel->SetSizerAndFit(tmpSizer);
    }

    return panel;
}

wxRibbonPage * MainFrame::CreateLayersRibbonPage(wxRibbonBar * parent)
{
    wxRibbonPage * page = new wxRibbonPage(parent, wxID_ANY, _("Layers"));

    // Structural
    {
        CreateLayerRibbonPanel(page, LayerType::Structural);
    }

    // Electrical
    {
        CreateLayerRibbonPanel(page, LayerType::Electrical);
    }

    // Ropes
    {
        CreateLayerRibbonPanel(page, LayerType::Ropes);
    }

    // Texture
    {
        CreateLayerRibbonPanel(page, LayerType::Texture);
    }

    return page;
}

wxRibbonPanel * MainFrame::CreateLayerRibbonPanel(wxRibbonPage * parent, LayerType layer)
{
    wxString const sureQuestion = _("The current changes to the layer will be lost; are you sure you want to continue?");

    std::string panelLabel;
    switch (layer)
    {
        case LayerType::Electrical:
        {
            panelLabel = _("Electrical Layer");
            break;
        }

        case LayerType::Ropes:
        {
            panelLabel = _("Ropes Layer");
            break;
        }

        case LayerType::Structural:
        {
            panelLabel = _("Structural Layer");
            break;
        }

        case LayerType::Texture:
        {
            panelLabel = _("Texture Layer");
            break;
        }
    }

    wxRibbonPanel * panel = new wxRibbonPanel(parent, wxID_ANY, panelLabel, wxNullBitmap, wxDefaultPosition, wxDefaultSize,
        wxRIBBON_PANEL_NO_AUTO_MINIMISE);

    wxGridBagSizer * panelGridSizer = new wxGridBagSizer(RibbonToolbarButtonMargin, RibbonToolbarButtonMargin + RibbonToolbarButtonMargin);

    int iColumn = 0;

    // Selector
    {
        VisualizationType visualization{ 0 };
        std::string buttonBitmapName;
        switch (layer)
        {
            case LayerType::Electrical:
            {
                visualization = VisualizationType::ElectricalLayer;
                buttonBitmapName = "electrical_layer";
                break;
            }

            case LayerType::Ropes:
            {
                visualization = VisualizationType::RopesLayer;
                buttonBitmapName = "ropes_layer";
                break;
            }

            case LayerType::Structural:
            {
                visualization = VisualizationType::StructuralLayer;
                buttonBitmapName = "structural_layer";
                break;
            }

            case LayerType::Texture:
            {
                visualization = VisualizationType::TextureLayer;
                buttonBitmapName = "texture_layer";
                break;
            }
        }

        auto * button = new RibbonToolbarButton<BitmapRadioButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetBitmapFilePath(buttonBitmapName),
            _("Edit"),
            [this, visualization]()
            {
                mController->SelectPrimaryVisualization(visualization);
            },
            _("Select this layer as the primary layer."));

        panelGridSizer->Add(
            button,
            wxGBPosition(0, iColumn++),
            wxGBSpan(2, 1),
            0,
            0);

        mVisualizationSelectButtons[static_cast<size_t>(visualization)] = button;
    }

    // Game mode viz
    if (layer == LayerType::Structural)
    {
        auto * button = new RibbonToolbarButton<BitmapRadioButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetBitmapFilePath("game_visualization"),
            _("Game View"),
            [this]()
            {
                mController->SelectPrimaryVisualization(VisualizationType::Game);
            },
            _("View the structure as it is rendered by the simulator."));

        panelGridSizer->Add(
            button,
            wxGBPosition(0, iColumn++),
            wxGBSpan(2, 1),
            0,
            0);

        mVisualizationSelectButtons[static_cast<size_t>(VisualizationType::Game)] = button;
    }

    int iCurrentButton = 0;

    // New/Open
    {
        auto const clickHandler = [this, layer, sureQuestion]()
        {
            if (mController->GetModelController().HasLayer(layer)
                && mController->GetModelController().IsLayerDirty(layer))
            {
                if (!AskUserIfSure(sureQuestion))
                {
                    // Changed their mind
                    return;
                }
            }

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
                    ImportTextureLayerFromImage();

                    break;
                }
            }
        };

        RibbonToolbarButton<BitmapButton> * newButton;
        if (layer != LayerType::Texture)
        {
            newButton = new RibbonToolbarButton<BitmapButton>(
                panel,
                wxHORIZONTAL,
                mResourceLocator.GetBitmapFilePath("new_layer_button"),
                _("Add/Clear"),
                clickHandler,
                _("Add or clean this layer."));
        }
        else
        {
            newButton = new RibbonToolbarButton<BitmapButton>(
                panel,
                wxHORIZONTAL,
                mResourceLocator.GetBitmapFilePath("open_image_button"),
                _("From Image"),
                clickHandler,
                _("Import this layer from an image file."));
        }

        panelGridSizer->Add(
            newButton,
            wxGBPosition(iCurrentButton % 2, iColumn + iCurrentButton / 2),
            wxGBSpan(1, 1),
            0,
            0);

        ++iCurrentButton;
    }

    // Import
    {
        auto * importButton = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxHORIZONTAL,
            mResourceLocator.GetBitmapFilePath("open_layer_button"),
            _("Import"),
            [this, layer, sureQuestion]()
            {
                if (mController->GetModelController().HasLayer(layer)
                    && mController->GetModelController().IsLayerDirty(layer))
                {
                    if (!AskUserIfSure(sureQuestion))
                    {
                        // Changed their mind
                        return;
                    }
                }

                ImportLayerFromShip(layer);
            },
            _("Import this layer from another ship."));

        panelGridSizer->Add(
            importButton,
            wxGBPosition(iCurrentButton % 2, iColumn + iCurrentButton / 2),
            wxGBSpan(1, 1),
            0,
            0);

        ++iCurrentButton;
    }

    // Delete
    {
        RibbonToolbarButton<BitmapButton> * deleteButton;

        if (layer != LayerType::Structural)
        {
            deleteButton = new RibbonToolbarButton<BitmapButton>(
                panel,
                wxHORIZONTAL,
                mResourceLocator.GetBitmapFilePath("delete_layer_button"),
                _("Remove"),
                [this, layer, sureQuestion]()
                {
                    switch (layer)
                    {
                        case LayerType::Electrical:
                        {
                            assert(mController->GetModelController().HasLayer(LayerType::Electrical));

                            if (mController->GetModelController().IsLayerDirty(LayerType::Electrical))
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
                            assert(mController->GetModelController().HasLayer(LayerType::Ropes));

                            if (mController->GetModelController().IsLayerDirty(LayerType::Ropes))
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

                        case LayerType::Texture:
                        {
                            assert(mController->GetModelController().HasLayer(LayerType::Texture));

                            if (mController->GetModelController().IsLayerDirty(LayerType::Texture))
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

                        case LayerType::Structural:
                        {
                            assert(false);
                            break;
                        }
                    }
                },
                _("Remove this layer from the ship."));

            panelGridSizer->Add(
                deleteButton,
                wxGBPosition(iCurrentButton % 2, iColumn + iCurrentButton / 2),
                wxGBSpan(1, 1),
                0,
                0);

            ++iCurrentButton;
        }
        else
        {
            deleteButton = nullptr;
        }

        mLayerDeleteButtons[static_cast<size_t>(layer)] = deleteButton;
    }

    // Export
    {
        RibbonToolbarButton<BitmapButton> * exportButton;

        if (layer == LayerType::Structural
            || layer == LayerType::Texture)
        {
            exportButton = new RibbonToolbarButton<BitmapButton>(
                panel,
                wxHORIZONTAL,
                mResourceLocator.GetBitmapFilePath("save_layer_button"),
                _("Export"),
                [this, layer]()
                {
                    ImageSaveDialog dlg(this);
                    auto const ret = dlg.ShowModal();
                    if (ret == wxID_OK)
                    {
                        if (layer == LayerType::Structural)
                        {
                            ShipDeSerializer::SaveStructuralLayerImage(
                                mController->GetModelController().GetStructuralLayer(),
                                dlg.GetChosenFilepath());
                        }
                        else
                        {
                            assert(layer == LayerType::Texture);

                            ImageFileTools::SavePngImage(
                                mController->GetModelController().GetTextureLayer().Buffer,
                                dlg.GetChosenFilepath());
                        }
                    }
                },
                _("Export this layer to a file."));

            panelGridSizer->Add(
                exportButton,
                wxGBPosition(iCurrentButton % 2, iColumn + iCurrentButton / 2),
                wxGBSpan(1, 1),
                0,
                0);

            ++iCurrentButton;
        }
        else
        {
            exportButton = nullptr;
        }

        mLayerExportButtons[static_cast<size_t>(layer)] = exportButton;
    }

    // Wrap in a sizer just for margins
    {
        wxSizer * tmpSizer = new wxBoxSizer(wxVERTICAL); // Arbitrary

        tmpSizer->Add(
            panelGridSizer,
            0,
            wxALL,
            RibbonToolbarButtonMargin);

        panel->SetSizerAndFit(tmpSizer);
    }

    return panel;
}

wxRibbonPage * MainFrame::CreateEditRibbonPage(wxRibbonBar * parent)
{
    wxRibbonPage * page = new wxRibbonPage(parent, wxID_ANY, _("Edit"));

    CreateEditUndoRibbonPanel(page);
    CreateEditShipRibbonPanel(page);
    CreateEditAnalysisRibbonPanel(page);
    CreateEditToolSettingsRibbonPanel(page);

    return page;
}

wxRibbonPanel * MainFrame::CreateEditUndoRibbonPanel(wxRibbonPage * parent)
{
    wxRibbonPanel * panel = new wxRibbonPanel(parent, wxID_ANY, _("Undo"), wxNullBitmap, wxDefaultPosition, wxDefaultSize,
        wxRIBBON_PANEL_NO_AUTO_MINIMISE);

    wxGridBagSizer * panelGridSizer = new wxGridBagSizer(RibbonToolbarButtonMargin, RibbonToolbarButtonMargin + RibbonToolbarButtonMargin);

    // Undo
    {
        mUndoButton = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("undo_medium"),
            _("Undo"),
            [this]()
            {
                assert(mController);
                mController->UndoLast();
            },
            _("Undo the last edit operation (Ctrl+Z)."));

        panelGridSizer->Add(mUndoButton);

        AddAcceleratorKey(wxACCEL_CTRL, (int)'Z',
            [this]()
            {
                // With keys we have no insurance of either a controller or a stack
                if (mController)
                {
                    mController->TryUndoLast();
                }
            });
    }

    // Wrap in a sizer just for margins
    {
        wxSizer * tmpSizer = new wxBoxSizer(wxVERTICAL); // Arbitrary

        tmpSizer->Add(
            panelGridSizer,
            0,
            wxALL,
            RibbonToolbarButtonMargin);

        panel->SetSizerAndFit(tmpSizer);
    }

    return panel;
}

wxRibbonPanel * MainFrame::CreateEditShipRibbonPanel(wxRibbonPage * parent)
{
    wxRibbonPanel * panel = new wxRibbonPanel(parent, wxID_ANY, _("Ship"), wxNullBitmap, wxDefaultPosition, wxDefaultSize,
        wxRIBBON_PANEL_NO_AUTO_MINIMISE);

    wxGridBagSizer * panelGridSizer = new wxGridBagSizer(RibbonToolbarButtonMargin, RibbonToolbarButtonMargin + RibbonToolbarButtonMargin);

    // Auto-Trim
    {
        auto button = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("trim_medium"),
            _("Auto-Trim"),
            [this]()
            {
                assert(mController);
                mController->AutoTrim();
            },
            _("Remove empty space around the ship."));

        panelGridSizer->Add(button);
    }

    // Rotate 90 CW
    {
        auto button = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("rotate_90_cw_medium"),
            _("90° CW"),
            [this]()
            {
                assert(mController);
                mController->Rotate90(RotationDirectionType::Clockwise);
            },
            _("Rotate the ship 90° clockwise."));

        panelGridSizer->Add(button);
    }

    // Rotate 90 CCW
    {
        auto button = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("rotate_90_ccw_medium"),
            _("90° CCW"),
            [this]()
            {
                assert(mController);
                mController->Rotate90(RotationDirectionType::CounterClockwise);
            },
            _("Rotate the ship 90° anti-clockwise."));

        panelGridSizer->Add(button);
    }

    // Flip H
    {
        auto button = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("flip_h_medium"),
            _("Flip H"),
            [this]()
            {
                assert(mController);
                mController->Flip(DirectionType::Horizontal);
            },
            _("Flip the ship horizontally."));

        panelGridSizer->Add(button);
    }

    // Flip V
    {
        auto button = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("flip_v_medium"),
            _("Flip V"),
            [this]()
            {
                assert(mController);
                mController->Flip(DirectionType::Vertical);
            },
            _("Flip the ship vertically."));

        panelGridSizer->Add(button);
    }

    // Resize
    {
        auto button = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("resize_button"),
            _("Size"),
            [this]()
            {
                OpenShipCanvasResize();
            },
            _("Resize the ship."));

        panelGridSizer->Add(button);
    }

    // Metadata
    {
        auto button = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("metadata_button"),
            _("Properties"),
            [this]()
            {
                OpenShipProperties();
            },
            _("Edit the ship properties."));

        panelGridSizer->Add(button);
    }

    // Wrap in a sizer just for margins
    {
        wxSizer * tmpSizer = new wxBoxSizer(wxVERTICAL); // Arbitrary

        tmpSizer->Add(
            panelGridSizer,
            0,
            wxALL,
            RibbonToolbarButtonMargin);

        panel->SetSizerAndFit(tmpSizer);
    }

    return panel;
}

wxRibbonPanel * MainFrame::CreateEditAnalysisRibbonPanel(wxRibbonPage * parent)
{
    wxRibbonPanel * panel = new wxRibbonPanel(parent, wxID_ANY, _("Analysis"), wxNullBitmap, wxDefaultPosition, wxDefaultSize,
        wxRIBBON_PANEL_NO_AUTO_MINIMISE);

    wxGridBagSizer * panelGridSizer = new wxGridBagSizer(RibbonToolbarButtonMargin, RibbonToolbarButtonMargin + RibbonToolbarButtonMargin);

    // Waterline analysis
    {
        auto button = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("waterline_analysis_icon_medium"),
            _("Waterline"),
            [this]()
            {
                auto const ribbonScreenRect = mMainRibbonBar->GetScreenRect();
                wxPoint const centerScreen = wxPoint(
                    ribbonScreenRect.x + this->GetScreenRect().width / 2,
                    ribbonScreenRect.y + ribbonScreenRect.height / 2);

                WaterlineAnalyzerDialog dlg(
                    this,
                    centerScreen,
                    mController->GetModelObservable(),
                    mController->GetView(),
                    *this,
                    mWorkbenchState.IsWaterlineMarkersEnabled(),
                    mWorkbenchState.GetDisplayUnitsSystem(),
                    mResourceLocator);

                dlg.ShowModal();
            },
            _("Forecast where the ship's waterline will be once the ship is in the water."));

        panelGridSizer->Add(button);
    }

    // Validation
    {
        auto button = new RibbonToolbarButton<BitmapButton>(
            panel,
            wxVERTICAL,
            mResourceLocator.GetIconFilePath("validate_ship_button"),
            _("Validation"),
            [this]()
            {
                ValidateShip();
            },
            _("Check the ship for issues."));

        panelGridSizer->Add(button);
    }

    // Wrap in a sizer just for margins
    {
        wxSizer * tmpSizer = new wxBoxSizer(wxVERTICAL); // Arbitrary

        tmpSizer->Add(
            panelGridSizer,
            0,
            wxALL,
            RibbonToolbarButtonMargin);

        panel->SetSizerAndFit(tmpSizer);
    }

    return panel;
}

wxRibbonPanel * MainFrame::CreateEditToolSettingsRibbonPanel(wxRibbonPage * parent)
{
    std::uint32_t constexpr MaxPencilSize = 8;
    wxColor const labelColor = parent->GetArtProvider()->GetColor(wxRIBBON_ART_BUTTON_BAR_LABEL_COLOUR);

    wxRibbonPanel * ribbonPanel = new wxRibbonPanel(parent, wxID_ANY, _("Tool Settings"), wxNullBitmap, wxDefaultPosition, wxDefaultSize,
        wxRIBBON_PANEL_NO_AUTO_MINIMISE);

    mToolSettingsPanelsSizer = new wxBoxSizer(wxHORIZONTAL);

    // Structural pencil
    {
        wxPanel * dynamicPanel = new wxPanel(ribbonPanel);
        wxGridBagSizer * dynamicPanelGridSizer = new wxGridBagSizer(RibbonToolbarButtonMargin, RibbonToolbarButtonMargin + RibbonToolbarButtonMargin);

        // Label
        {
            auto * staticText = new wxStaticText(dynamicPanel, wxID_ANY, _("Pencil Size:"));
            staticText->SetForegroundColour(labelColor);

            dynamicPanelGridSizer->Add(
                staticText,
                wxGBPosition(0, 0),
                wxGBSpan(1, 1),
                wxALIGN_CENTER_VERTICAL);
        }

        // Edit spin box
        {
            EditSpinBox<std::uint32_t> * editSpinBox = new EditSpinBox<std::uint32_t>(
                dynamicPanel,
                40,
                1,
                MaxPencilSize,
                mWorkbenchState.GetStructuralPencilToolSize(),
                _("The size of the pencil tool."),
                [this](std::uint32_t value)
                {
                    mWorkbenchState.SetStructuralPencilToolSize(value);
                });

            dynamicPanelGridSizer->Add(
                editSpinBox,
                wxGBPosition(0, 1),
                wxGBSpan(1, 1),
                wxALIGN_CENTER_VERTICAL);
        }

        dynamicPanel->SetSizerAndFit(dynamicPanelGridSizer);

        // Insert in dynamic panel
        {
            mToolSettingsPanelsSizer->Add(
                dynamicPanel,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);

            mToolSettingsPanels.emplace_back(
                ToolType::StructuralPencil,
                dynamicPanel);
        }
    }

    // Structural eraser
    {
        wxPanel * dynamicPanel = new wxPanel(ribbonPanel);
        wxGridBagSizer * dynamicPanelGridSizer = new wxGridBagSizer(RibbonToolbarButtonMargin, RibbonToolbarButtonMargin + RibbonToolbarButtonMargin);

        // Label
        {
            auto * staticText = new wxStaticText(dynamicPanel, wxID_ANY, _("Eraser Size:"));
            staticText->SetForegroundColour(labelColor);

            dynamicPanelGridSizer->Add(
                staticText,
                wxGBPosition(0, 0),
                wxGBSpan(1, 1),
                wxALIGN_CENTER_VERTICAL);
        }

        // Edit spin box
        {
            EditSpinBox<std::uint32_t> * editSpinBox = new EditSpinBox<std::uint32_t>(
                dynamicPanel,
                40,
                1,
                MaxPencilSize,
                mWorkbenchState.GetStructuralEraserToolSize(),
                _("The size of the eraser tool."),
                [this](std::uint32_t value)
                {
                    mWorkbenchState.SetStructuralEraserToolSize(value);
                });

            dynamicPanelGridSizer->Add(
                editSpinBox,
                wxGBPosition(0, 1),
                wxGBSpan(1, 1),
                wxALIGN_CENTER_VERTICAL);
        }

        dynamicPanel->SetSizerAndFit(dynamicPanelGridSizer);

        // Insert in dynamic panel
        {
            mToolSettingsPanelsSizer->Add(
                dynamicPanel,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);

            mToolSettingsPanels.emplace_back(
                ToolType::StructuralEraser,
                dynamicPanel);
        }
    }

    // Structural line
    {
        wxPanel * dynamicPanel = new wxPanel(ribbonPanel);
        wxGridBagSizer * dynamicPanelGridSizer = new wxGridBagSizer(RibbonToolbarButtonMargin, RibbonToolbarButtonMargin + RibbonToolbarButtonMargin);

        // Line Size Label
        {
            auto * staticText = new wxStaticText(dynamicPanel, wxID_ANY, _("Line Size:"));
            staticText->SetForegroundColour(labelColor);

            dynamicPanelGridSizer->Add(
                staticText,
                wxGBPosition(0, 0),
                wxGBSpan(1, 1),
                wxALIGN_CENTER_VERTICAL);
        }

        // Line Size Edit spin box
        {
            EditSpinBox<std::uint32_t> * editSpinBox = new EditSpinBox<std::uint32_t>(
                dynamicPanel,
                40,
                1,
                MaxPencilSize,
                mWorkbenchState.GetStructuralLineToolSize(),
                _("The size of the line tool."),
                [this](std::uint32_t value)
                {
                    mWorkbenchState.SetStructuralLineToolSize(value);
                });

            dynamicPanelGridSizer->Add(
                editSpinBox,
                wxGBPosition(0, 1),
                wxGBSpan(1, 1),
                wxALIGN_CENTER_VERTICAL);
        }

        // Contiguity Label
        {
            auto * staticText = new wxStaticText(dynamicPanel, wxID_ANY, _("Hull Mode:"));
            staticText->SetForegroundColour(labelColor);

            dynamicPanelGridSizer->Add(
                staticText,
                wxGBPosition(1, 0),
                wxGBSpan(1, 1),
                wxALIGN_CENTER_VERTICAL);
        }

        // Contiguity Checkbox
        {
            wxCheckBox * chkBox = new wxCheckBox(dynamicPanel, wxID_ANY, wxEmptyString);

            chkBox->SetToolTip(_("When enabled, draw lines with pixel edges touching each other."));

            chkBox->SetValue(mWorkbenchState.GetStructuralLineToolIsHullMode());

            chkBox->Bind(
                wxEVT_CHECKBOX,
                [this](wxCommandEvent & event)
                {
                    mWorkbenchState.SetStructuralLineToolIsHullMode(event.IsChecked());
                });

            dynamicPanelGridSizer->Add(
                chkBox,
                wxGBPosition(1, 1),
                wxGBSpan(1, 1),
                wxALIGN_CENTER_VERTICAL);
        }

        dynamicPanel->SetSizerAndFit(dynamicPanelGridSizer);

        // Insert in dynamic panel
        {
            mToolSettingsPanelsSizer->Add(
                dynamicPanel,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);

            mToolSettingsPanels.emplace_back(
                ToolType::StructuralLine,
                dynamicPanel);
        }
    }

    // Structural flood
    {
        wxPanel * dynamicPanel = new wxPanel(ribbonPanel);
        wxGridBagSizer * dynamicPanelGridSizer = new wxGridBagSizer(RibbonToolbarButtonMargin, RibbonToolbarButtonMargin + RibbonToolbarButtonMargin);

        // Contiguous Label
        {
            auto * staticText = new wxStaticText(dynamicPanel, wxID_ANY, _("Contiguous Only:"));
            staticText->SetForegroundColour(labelColor);

            dynamicPanelGridSizer->Add(
                staticText,
                wxGBPosition(0, 0),
                wxGBSpan(1, 1),
                wxALIGN_CENTER_VERTICAL);
        }

        // Contiguous Checkbox
        {
            wxCheckBox * chkBox = new wxCheckBox(dynamicPanel, wxID_ANY, wxEmptyString);

            chkBox->SetToolTip(_("Flood only particles touching each other. When not checked, the flood tool effectively replaces a material with another."));

            chkBox->SetValue(mWorkbenchState.GetStructuralFloodToolIsContiguous());

            chkBox->Bind(
                wxEVT_CHECKBOX,
                [this](wxCommandEvent & event)
                {
                    mWorkbenchState.SetStructuralFloodToolIsContiguous(event.IsChecked());
                });

            dynamicPanelGridSizer->Add(
                chkBox,
                wxGBPosition(0, 1),
                wxGBSpan(1, 1),
                wxALIGN_CENTER_VERTICAL);
        }

        dynamicPanel->SetSizerAndFit(dynamicPanelGridSizer);

        // Insert in dynamic panel
        {
            mToolSettingsPanelsSizer->Add(
                dynamicPanel,
                0,
                wxALIGN_CENTER_VERTICAL,
                0);

            mToolSettingsPanels.emplace_back(
                ToolType::StructuralFlood,
                dynamicPanel);
        }
    }

    // Wrap in a sizer just for margins
    {
        wxSizer * tmpSizer = new wxBoxSizer(wxVERTICAL); // Arbitrary

        tmpSizer->Add(
            mToolSettingsPanelsSizer,
            1, // Stretch vertically so single-row panels have a chance of being able to be at V center
            wxALL,
            RibbonToolbarButtonMargin);

        ribbonPanel->SetSizerAndFit(tmpSizer);
    }

    // Find widest panel
    wxPanel const * widestPanel = nullptr;
    for (auto const & entry : mToolSettingsPanels)
    {
        int const width = std::get<1>(entry)->GetSize().GetWidth();
        if (widestPanel == nullptr || width > widestPanel->GetSize().GetWidth())
        {
            widestPanel = std::get<1>(entry);
        }
    }

    // Show widest panel only
    for (auto const & entry : mToolSettingsPanels)
    {
        mToolSettingsPanelsSizer->Show(
            std::get<1>(entry),
            (widestPanel == nullptr && std::get<0>(entry) == ToolType::StructuralLine)
            || (widestPanel != nullptr && std::get<1>(entry) == widestPanel));
    }

    return ribbonPanel;
}

wxPanel * MainFrame::CreateVisualizationModeHeaderPanel(wxWindow * parent)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    mVisualizationModeHeaderPanelsSizer = new wxBoxSizer(wxVERTICAL);

    // Game viz mode
    {
        wxPanel * modePanel = new wxPanel(panel);

        auto * sizer = new wxGridBagSizer(0, 0);

        // Icon
        {
            auto * staticBitmap = new wxStaticBitmap(
                modePanel,
                wxID_ANY,
                WxHelpers::LoadBitmap("game_visualization", mResourceLocator));

            sizer->Add(
                staticBitmap,
                wxGBPosition(0, 0),
                wxGBSpan(2, 1),
                wxRIGHT,
                ButtonMargin);
        }

        // Label 1
        {
            auto * staticText = new wxStaticText(modePanel, wxID_ANY, _("Structural Layer"));

            sizer->Add(
                staticText,
                wxGBPosition(0, 1),
                wxGBSpan(1, 1),
                wxALIGN_TOP | wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        // Label 2
        {
            auto * staticText = new wxStaticText(modePanel, wxID_ANY, _("(Game View)"));

            sizer->Add(
                staticText,
                wxGBPosition(1, 1),
                wxGBSpan(1, 1),
                wxALIGN_BOTTOM | wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        modePanel->SetSizerAndFit(sizer);

        mVisualizationModeHeaderPanelsSizer->Add(
            modePanel,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);

        mVisualizationModeHeaderPanelsSizer->Hide(modePanel);

        mVisualizationModeHeaderPanels[static_cast<size_t>(VisualizationType::Game)] = modePanel;
    }

    // Structural layer mode
    {
        wxPanel * modePanel = new wxPanel(panel);

        auto * sizer = new wxGridBagSizer(0, 0);

        // Icon
        {
            auto * staticBitmap = new wxStaticBitmap(
                modePanel,
                wxID_ANY,
                WxHelpers::LoadBitmap("structural_layer", mResourceLocator));

            sizer->Add(
                staticBitmap,
                wxGBPosition(0, 0),
                wxGBSpan(2, 1),
                wxRIGHT,
                ButtonMargin);
        }

        // Label 1
        {
            auto * staticText = new wxStaticText(modePanel, wxID_ANY, _("Structural Layer"));

            sizer->Add(
                staticText,
                wxGBPosition(0, 1),
                wxGBSpan(1, 1),
                wxALIGN_TOP | wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        // Label 2
        {
            auto * staticText = new wxStaticText(modePanel, wxID_ANY, _("(Structure View)"));

            sizer->Add(
                staticText,
                wxGBPosition(1, 1),
                wxGBSpan(1, 1),
                wxALIGN_BOTTOM | wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        modePanel->SetSizerAndFit(sizer);

        mVisualizationModeHeaderPanelsSizer->Add(
            modePanel,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);

        mVisualizationModeHeaderPanels[static_cast<size_t>(VisualizationType::StructuralLayer)] = modePanel;
    }

    // Electrical layer mode
    {
        wxPanel * modePanel = new wxPanel(panel);

        auto * sizer = new wxGridBagSizer(0, 0);

        // Icon
        {
            auto * staticBitmap = new wxStaticBitmap(
                modePanel,
                wxID_ANY,
                WxHelpers::LoadBitmap("electrical_layer", mResourceLocator));

            sizer->Add(
                staticBitmap,
                wxGBPosition(0, 0),
                wxGBSpan(2, 1),
                wxRIGHT,
                ButtonMargin);
        }

        // Label
        {
            auto * staticText = new wxStaticText(modePanel, wxID_ANY, _("Electrical Layer"));

            sizer->Add(
                staticText,
                wxGBPosition(0, 1),
                wxGBSpan(2, 1),
                wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        modePanel->SetSizerAndFit(sizer);

        mVisualizationModeHeaderPanelsSizer->Add(
            modePanel,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);

        mVisualizationModeHeaderPanelsSizer->Hide(modePanel);

        mVisualizationModeHeaderPanels[static_cast<size_t>(VisualizationType::ElectricalLayer)] = modePanel;
    }

    // Ropes layer mode
    {
        wxPanel * modePanel = new wxPanel(panel);

        auto * sizer = new wxGridBagSizer(0, 0);

        // Icon
        {
            auto * staticBitmap = new wxStaticBitmap(
                modePanel,
                wxID_ANY,
                WxHelpers::LoadBitmap("ropes_layer", mResourceLocator));

            sizer->Add(
                staticBitmap,
                wxGBPosition(0, 0),
                wxGBSpan(2, 1),
                wxRIGHT,
                ButtonMargin);
        }

        // Label
        {
            auto * staticText = new wxStaticText(modePanel, wxID_ANY, _("Ropes Layer"));

            sizer->Add(
                staticText,
                wxGBPosition(0, 1),
                wxGBSpan(2, 1),
                wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        modePanel->SetSizerAndFit(sizer);

        mVisualizationModeHeaderPanelsSizer->Add(
            modePanel,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);

        mVisualizationModeHeaderPanelsSizer->Hide(modePanel);

        mVisualizationModeHeaderPanels[static_cast<size_t>(VisualizationType::RopesLayer)] = modePanel;
    }

    // Texture layer mode
    {
        wxPanel * modePanel = new wxPanel(panel);

        auto * sizer = new wxGridBagSizer(0, 0);

        // Icon
        {
            auto * staticBitmap = new wxStaticBitmap(
                modePanel,
                wxID_ANY,
                WxHelpers::LoadBitmap("texture_layer", mResourceLocator));

            sizer->Add(
                staticBitmap,
                wxGBPosition(0, 0),
                wxGBSpan(2, 1),
                wxRIGHT,
                ButtonMargin);
        }

        // Label
        {
            auto * staticText = new wxStaticText(modePanel, wxID_ANY, _("Texture Layer"));

            sizer->Add(
                staticText,
                wxGBPosition(0, 1),
                wxGBSpan(2, 1),
                wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        modePanel->SetSizerAndFit(sizer);

        mVisualizationModeHeaderPanelsSizer->Add(
            modePanel,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);

        mVisualizationModeHeaderPanelsSizer->Hide(modePanel);

        mVisualizationModeHeaderPanels[static_cast<size_t>(VisualizationType::TextureLayer)] = modePanel;
    }

    panel->SetSizerAndFit(mVisualizationModeHeaderPanelsSizer);

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
                mGameVisualizationNoneModeButton = new BitmapRadioButton(
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
                mGameVisualizationAutoTexturizationModeButton = new BitmapRadioButton(
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
                mGameVisualizationTextureModeButton = new BitmapRadioButton(
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
                mStructuralLayerVisualizationNoneModeButton = new BitmapRadioButton(
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
                mStructuralLayerVisualizationMeshModeButton = new BitmapRadioButton(
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
                mStructuralLayerVisualizationPixelModeButton = new BitmapRadioButton(
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
                mElectricalLayerVisualizationNoneModeButton = new BitmapRadioButton(
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
                mElectricalLayerVisualizationPixelModeButton = new BitmapRadioButton(
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
                mRopesLayerVisualizationNoneModeButton = new BitmapRadioButton(
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
                mRopesLayerVisualizationLinesModeButton = new BitmapRadioButton(
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
                mTextureLayerVisualizationNoneModeButton = new BitmapRadioButton(
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
                mTextureLayerVisualizationMatteModeButton = new BitmapRadioButton(
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

        // View waterline markers button
        {
            auto bitmap = WxHelpers::LoadBitmap("view_waterline_markers_button", mResourceLocator);
            mViewWaterlineMarkersButton = new wxBitmapToggleButton(panel, wxID_ANY, bitmap, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
            mViewWaterlineMarkersButton->SetToolTip(_("Enable/disable visualization of the ship's center of mass."));
            mViewWaterlineMarkersButton->Bind(
                wxEVT_TOGGLEBUTTON,
                [this](wxCommandEvent & event)
                {
                    assert(mController);
                    mController->EnableWaterlineMarkers(event.IsChecked());

                    DeviateFocus();
                });

            mVisualizationModePanelsSizer->Add(
                mViewWaterlineMarkersButton,
                0, // Retain vertical width
                wxALIGN_CENTER_HORIZONTAL, // Do not expand vertically
                0);
        }

        mVisualizationModePanelsSizer->AddSpacer(ButtonMargin);

        // View grid button
        {
            auto bitmap = WxHelpers::LoadBitmap("view_grid_button", mResourceLocator);
            mViewGridButton = new wxBitmapToggleButton(panel, wxID_ANY, bitmap, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
            mViewGridButton->SetToolTip(_("Enable/disable the visual guides."));
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

    auto const makeToolButton = [this](
        ToolType tool,
        wxPanel * toolbarPanel,
        std::string iconName,
        wxString tooltip) -> BitmapRadioButton *
    {
        auto button = new BitmapRadioButton(
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

        wxBoxSizer * structuralToolbarVSizer = new wxBoxSizer(wxVERTICAL);

        // Tools

        {
            wxGridBagSizer * toolsSizer = new wxGridBagSizer(ButtonMargin, ButtonMargin);

            // Pencil
            {
                auto button = makeToolButton(
                    ToolType::StructuralPencil,
                    structuralToolbarPanel,
                    "pencil_icon",
                    _("Draw individual structure particles."));

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
                    _("Erase individual structure particles."));

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
                    _("Draw particles in lines."));

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
                    _("Fill an area with structure particles."));

                toolsSizer->Add(
                    button,
                    wxGBPosition(1, 1),
                    wxGBSpan(1, 1),
                    0,
                    0);
            }

            // Sampler
            {
                auto button = makeToolButton(
                    ToolType::StructuralSampler,
                    structuralToolbarPanel,
                    "sampler_icon",
                    _("Sample the material under the cursor."));

                toolsSizer->Add(
                    button,
                    wxGBPosition(2, 0),
                    wxGBSpan(1, 1),
                    0,
                    0);
            }

            // MeasuringTape
            {
                auto button = makeToolButton(
                    ToolType::StructuralMeasuringTapeTool,
                    structuralToolbarPanel,
                    "measuring_tape_icon",
                    _("Measure lengths."));

                toolsSizer->Add(
                    button,
                    wxGBPosition(2, 1),
                    wxGBSpan(1, 1),
                    0,
                    0);
            }

            structuralToolbarVSizer->Add(
                toolsSizer,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        structuralToolbarVSizer->AddSpacer(15);

        // Swaths

        {
            wxBoxSizer * paletteSizer = new wxBoxSizer(wxHORIZONTAL);

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

            paletteSizer->AddSpacer(ButtonMargin);

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

            structuralToolbarVSizer->Add(
                paletteSizer,
                0,
                wxALIGN_CENTER_HORIZONTAL,
                0);
        }

        structuralToolbarPanel->SetSizerAndFit(structuralToolbarVSizer);

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
                    _("Draw individual electrical elements."));

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
                    _("Erase individual electrical elements."));

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
                    _("Draw electrical elements in lines."));

                toolsSizer->Add(
                    button,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    0,
                    0);
            }

            // Sampler
            {
                auto button = makeToolButton(
                    ToolType::ElectricalSampler,
                    electricalToolbarPanel,
                    "sampler_icon",
                    _("Sample the material under the cursor."));

                toolsSizer->Add(
                    button,
                    wxGBPosition(1, 1),
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
            wxBoxSizer * paletteSizer = new wxBoxSizer(wxHORIZONTAL);

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

            paletteSizer->AddSpacer(ButtonMargin);

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
                    _("Draw a rope between two endpoints."));

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
                    _("Erase rope endpoints."));

                toolsSizer->Add(
                    button,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    0,
                    0);
            }

            // Sampler
            {
                auto button = makeToolButton(
                    ToolType::RopeSampler,
                    ropesToolbarPanel,
                    "sampler_icon",
                    _("Sample the material under the cursor."));

                toolsSizer->Add(
                    button,
                    wxGBPosition(1, 0),
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
            wxBoxSizer * paletteSizer = new wxBoxSizer(wxHORIZONTAL);

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

            paletteSizer->AddSpacer(ButtonMargin);

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

void MainFrame::AddAcceleratorKey(int flags, int keyCode, std::function<void()> handler)
{
    auto const commandId = wxNewId();

    mAcceleratorEntries.emplace_back(flags, keyCode, commandId);

    this->Bind(
        wxEVT_MENU,
        [handler](wxCommandEvent &)
        {
            handler();
        },
        commandId);
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
            if (!mIsShiftKeyDown) // Suppress repetitions
            {
                mIsShiftKeyDown = true;

                mController->OnShiftKeyDown();
                return; // Eaten
            }
        }
    }

    event.Skip();
}

void MainFrame::OnWorkCanvasKeyUp(wxKeyEvent & event)
{
    if (mController)
    {
        if (event.GetKeyCode() == WXK_SHIFT)
        {
            if (mIsShiftKeyDown) // Suppress repetitions
            {
                mIsShiftKeyDown = false;

                mController->OnShiftKeyUp();
            }
        }
    }
}

void MainFrame::OnClose(wxCloseEvent & event)
{
    if (event.CanVeto() && !IsStandAlone())
    {
        // User pressed ALT+F4 in simulator mode...
        // ...do not allow close(), as we only allow
        // to exit with a switch *back*
        event.Veto();
        return;
    }

    if (mController)
    {
        if (event.CanVeto() && mController->GetModelController().IsDirty())
        {
            // Ask user if they really want
            int result = AskUserIfSave();
            if (result == wxYES)
            {
                if (!DoSaveShipOrSaveShipAsWithValidation())
                {
                    // Didn't end up saving
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

void MainFrame::OnStructuralMaterialSelected(fsStructuralMaterialSelectedEvent & event)
{
    assert(mController);

    mController->SetStructuralMaterial(event.GetMaterial(), event.GetMaterialPlane());
}

void MainFrame::OnElectricalMaterialSelected(fsElectricalMaterialSelectedEvent & event)
{
    assert(mController);

    mController->SetElectricalMaterial(event.GetMaterial(), event.GetMaterialPlane());
}

void MainFrame::OnRopeMaterialSelected(fsStructuralMaterialSelectedEvent & event)
{
    assert(mController);

    mController->SetRopeMaterial(event.GetMaterial(), event.GetMaterialPlane());
}

//////////////////////////////////////////////////////////////////

void MainFrame::Open()
{
    LogMessage("MainFrame::Open()");

    // Show us
    Show(true);
    Layout();

    // Select "Main" ribbon page
    mMainRibbonBar->SetActivePage(size_t(0));

    // Make ourselves the topmost frame
    mMainApp->SetTopWindow(this);
}

void MainFrame::NewShip()
{
    if (mController->GetModelController().IsDirty())
    {
        // Ask user if they really want
        int result = AskUserIfSave();
        if (result == wxYES)
        {
            if (!DoSaveShipOrSaveShipAsWithValidation())
            {
                // Didn't end up saving
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
    if (mController->GetModelController().IsDirty())
    {
        // Ask user if they really want
        int result = AskUserIfSave();
        if (result == wxYES)
        {
            if (!DoSaveShipOrSaveShipAsWithValidation())
            {
                // Didn't end up saving
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
    auto const res = mShipLoadDialog->ShowModal(mWorkbenchState.GetShipLoadDirectories());
    if (res == wxID_OK)
    {
        // Load ship
        auto const shipFilePath = mShipLoadDialog->GetChosenShipFilepath();
        DoLoadShip(shipFilePath); // Ignore eventual failure

        // Store directory in preferences
        mWorkbenchState.AddShipLoadDirectory(shipFilePath.parent_path());
    }
}

void MainFrame::SaveShip()
{
    DoSaveShipOrSaveShipAsWithValidation();
}

void MainFrame::SaveShipAs()
{
    DoSaveShipAsWithValidation();
}

void MainFrame::SaveAndSwitchBackToGame()
{
    // Save/SaveAs
    if (DoSaveShipOrSaveShipAsWithValidation())
    {
        // Return
        assert(mCurrentShipFilePath.has_value());
        assert(ShipDeSerializer::IsShipDefinitionFile(*mCurrentShipFilePath));
        SwitchBackToGame(*mCurrentShipFilePath);
    }
}

void MainFrame::QuitAndSwitchBackToGame()
{
    if (mController->GetModelController().IsDirty())
    {
        // Ask user if they really want
        int result = AskUserIfSave();
        if (result == wxYES)
        {
            if (!DoSaveShipOrSaveShipAsWithValidation())
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

void MainFrame::Quit()
{
    // Close frame
    Close();
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

void MainFrame::ImportLayerFromShip(LayerType layer)
{
    // Open ship load dialog
    auto const res = mShipLoadDialog->ShowModal(mWorkbenchState.GetShipLoadDirectories());
    if (res == wxID_OK)
    {
        auto const shipFilePath = mShipLoadDialog->GetChosenShipFilepath();

        std::optional<ShipDefinition> shipDefinition = DoLoadShipDefinitionAndCheckPassword(shipFilePath);
        if (!shipDefinition.has_value())
        {
            // No luck
            return;
        }

        switch (layer)
        {
            case LayerType::Electrical:
            {
                if (!shipDefinition->ElectricalLayer)
                {
                    ShowError(_("The selected ship does not have an electrical layer"));
                    return;
                }

                // Reframe loaded layer to fit our model's size
                ElectricalLayerData newElectricalLayer = shipDefinition->ElectricalLayer->MakeReframed(
                    mController->GetModelController().GetShipSize(),
                    ShipSpaceCoordinates(0, 0),
                    ElectricalElement(nullptr, NoneElectricalElementInstanceIndex));

                mController->SetElectricalLayer(
                    _("Import Electrical Layer"),
                    std::move(newElectricalLayer));

                break;
            }

            case LayerType::Ropes:
            {
                if (!shipDefinition->RopesLayer)
                {
                    ShowError(_("The selected ship does not have a ropes layer"));
                    return;
                }

                // Reframe loaded layer to fit our model's size
                RopesLayerData newRopesLayer = shipDefinition->RopesLayer->MakeReframed(
                    mController->GetModelController().GetShipSize(),
                    ShipSpaceCoordinates(0, 0));

                mController->SetRopesLayer(
                    _("Import Ropes Layer"),
                    std::move(newRopesLayer));

                break;
            }

            case LayerType::Structural:
            {
                // Reframe loaded layer to fit our model's size
                StructuralLayerData newStructuralLayer = shipDefinition->StructuralLayer.MakeReframed(
                    mController->GetModelController().GetShipSize(),
                    ShipSpaceCoordinates(0, 0),
                    StructuralElement(nullptr));

                mController->SetStructuralLayer(
                    _("Import Structural Layer"),
                    std::move(newStructuralLayer));

                break;
            }

            case LayerType::Texture:
            {
                if (!shipDefinition->TextureLayer)
                {
                    ShowError(_("The selected ship does not have a texture layer"));
                    return;
                }

                // No need to resize, as texture image doesn't have to match;
                // we'll leave it to the user, though, to ensure the *ratio* matches
                mController->SetTextureLayer(
                    _("Import Texture Layer"),
                    std::move(*(shipDefinition->TextureLayer.release())),
                    shipDefinition->Metadata.ArtCredits); // Import also art credits

                break;
            }
        }

        // Store directory in preferences
        mWorkbenchState.AddShipLoadDirectory(shipFilePath.parent_path());
    }
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
                ShowError(_("The specified texture image is empty, and thus it may not be used for this ship."));
                return;
            }

            // Calculate target size == size of texture when maintaining same aspect ratio as ship's,
            // preferring to not cut
            ShipSpaceSize const & shipSize = mController->GetModelController().GetShipSize();
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
                _("Import Texture Layer"),
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
    auto const shipSize = mController->GetModelController().GetShipSize();
    IntegralRectSize const initialTargetSize(shipSize.width, shipSize.height);

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
        mController->GetModelController().GetShipMetadata(),
        mController->GetModelController().GetShipPhysicsData(),
        mController->GetModelController().GetShipAutoTexturizationSettings(),
        *shipPreviewImage,
        mController->GetModelController().HasLayer(LayerType::Texture));
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

void MainFrame::ShowError(wxString const & message) const
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

    std::optional<ShipDefinition> shipDefinition = DoLoadShipDefinitionAndCheckPassword(shipFilePath);

    if (!shipDefinition.has_value())
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

    // Remember file path - but only if it's a definition file in the "official" format (not a legacy one),
    // and only if it's not a stock ship (otherwise users could overwrite game ships unknowingly)
    if (ShipDeSerializer::IsShipDefinitionFile(shipFilePath)
        && !Utils::IsFileUnderDirectory(shipFilePath, mResourceLocator.GetInstalledShipFolderPath()))
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

std::optional<ShipDefinition> MainFrame::DoLoadShipDefinitionAndCheckPassword(std::filesystem::path const & shipFilePath)
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
        return std::nullopt;
    }
    catch (std::runtime_error const & exc)
    {
        ShowError(exc.what());
        return std::nullopt;
    }

    assert(shipDefinition.has_value());

    //
    // Check password
    //

    if (!AskPasswordDialog::CheckPasswordProtectedEdit(*shipDefinition, this, mResourceLocator))
    {
        return std::nullopt;
    }

    return shipDefinition;
}

bool MainFrame::DoSaveShipOrSaveShipAsWithValidation()
{
    if (!mCurrentShipFilePath.has_value())
    {
        return DoSaveShipAsWithValidation();
    }
    else
    {
        return DoSaveShipWithValidation(*mCurrentShipFilePath);
    }
}

bool MainFrame::DoSaveShipAsWithValidation()
{
    if (DoPreSaveShipValidation())
    {
        // Open ship save dialog

        if (!mShipSaveDialog)
        {
            mShipSaveDialog = std::make_unique<ShipSaveDialog>(this);
        }

        auto const res = mShipSaveDialog->ShowModal(
            Utils::MakeFilenameSafeString(mController->GetModelController().GetShipMetadata().ShipName),
            ShipSaveDialog::GoalType::FullShip);

        if (res == wxID_OK)
        {
            // Save ship
            auto const shipFilePath = mShipSaveDialog->GetChosenShipFilepath();
            DoSaveShipWithoutValidation(shipFilePath);
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool MainFrame::DoSaveShipWithValidation(std::filesystem::path const & shipFilePath)
{
    if (DoPreSaveShipValidation())
    {
        DoSaveShipWithoutValidation(shipFilePath);
        return true;
    }
    else
    {
        return false;
    }
}

void MainFrame::DoSaveShipWithoutValidation(std::filesystem::path const & shipFilePath)
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

bool MainFrame::DoPreSaveShipValidation()
{
    assert(mController);

    // Validate ship in dialog, and allow user to continue or cancel the save

    if (!mModelValidationDialog)
    {
        mModelValidationDialog = std::make_unique<ModelValidationDialog>(this, mResourceLocator);
    }

    return mModelValidationDialog->ShowModalForSaveShipValidation(*mController);
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

void MainFrame::ZoomIn()
{
    assert(!!mController);
    mController->AddZoom(1);
}

void MainFrame::ZoomOut()
{
    assert(!!mController);
    mController->AddZoom(-1);
}

void MainFrame::ResetView()
{
    assert(!!mController);
    mController->ResetView();
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
    ReconciliateUIWithStructuralMaterial(mWorkbenchState.GetStructuralForegroundMaterial(), MaterialPlaneType::Foreground);
    ReconciliateUIWithStructuralMaterial(mWorkbenchState.GetStructuralBackgroundMaterial(), MaterialPlaneType::Background);
    ReconciliateUIWithElectricalMaterial(mWorkbenchState.GetElectricalForegroundMaterial(), MaterialPlaneType::Foreground);
    ReconciliateUIWithElectricalMaterial(mWorkbenchState.GetElectricalBackgroundMaterial(), MaterialPlaneType::Background);
    ReconciliateUIWithRopesMaterial(mWorkbenchState.GetRopesForegroundMaterial(), MaterialPlaneType::Foreground);
    ReconciliateUIWithRopesMaterial(mWorkbenchState.GetRopesBackgroundMaterial(), MaterialPlaneType::Background);

    ReconciliateUIWithSelectedTool(mWorkbenchState.GetCurrentToolType());

    ReconciliateUIWithPrimaryVisualizationSelection(mWorkbenchState.GetPrimaryVisualization());

    ReconciliateUIWithGameVisualizationModeSelection(mWorkbenchState.GetGameVisualizationMode());
    ReconciliateUIWithStructuralLayerVisualizationModeSelection(mWorkbenchState.GetStructuralLayerVisualizationMode());
    ReconciliateUIWithElectricalLayerVisualizationModeSelection(mWorkbenchState.GetElectricalLayerVisualizationMode());
    ReconciliateUIWithRopesLayerVisualizationModeSelection(mWorkbenchState.GetRopesLayerVisualizationMode());
    ReconciliateUIWithTextureLayerVisualizationModeSelection(mWorkbenchState.GetTextureLayerVisualizationMode());

    ReconciliateUIWithOtherVisualizationsOpacity(mWorkbenchState.GetOtherVisualizationsOpacity());

    ReconciliateUIWithVisualWaterlineMarkersEnablement(mWorkbenchState.IsWaterlineMarkersEnabled());
    ReconciliateUIWithVisualGridEnablement(mWorkbenchState.IsGridEnabled());

    ReconciliateUIWithDisplayUnitsSystem(mWorkbenchState.GetDisplayUnitsSystem());
}

void MainFrame::ReconciliateUIWithViewModel(ViewModel const & viewModel)
{
    // Panning
    RecalculateWorkCanvasPanning(viewModel);

    // Zoom buttons
    mZoomInButton->Enable(viewModel.GetZoom() < ViewModel::MaxZoom);
    mZoomOutButton->Enable(viewModel.GetZoom() > ViewModel::MinZoom);

    // StatusBar
    mStatusBar->SetZoom(viewModel.GetZoom());
}

void MainFrame::ReconciliateUIWithShipSize(ShipSpaceSize const & shipSize)
{
    mStatusBar->SetCanvasSize(shipSize);
}

void MainFrame::ReconciliateUIWithShipScale(ShipSpaceToWorldSpaceCoordsRatio const & scale)
{
    mStatusBar->SetShipScale(scale);
}

void MainFrame::ReconciliateUIWithShipTitle(std::string const & shipName, bool isShipDirty)
{
    SetFrameTitle(shipName, isShipDirty);
}

void MainFrame::ReconciliateUIWithLayerPresence(IModelObservable const & model)
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

void MainFrame::ReconciliateUIWithModelDirtiness(IModelObservable const & model)
{
    bool const isDirty = model.IsDirty();

    if (mSaveShipButton->IsEnabled() != isDirty)
    {
        mSaveShipButton->Enable(isDirty);
    }

    if (mSaveShipAndGoBackButton != nullptr
        && mSaveShipAndGoBackButton->IsEnabled() != isDirty)
    {
        mSaveShipAndGoBackButton->Enable(isDirty);
    }

    SetFrameTitle(model.GetShipMetadata().ShipName, isDirty);
}

void MainFrame::ReconciliateUIWithModelMacroProperties(ModelMacroProperties const & properties)
{
    mStatusBar->SetShipMass(properties.TotalMass);
}

void MainFrame::ReconciliateUIWithStructuralMaterial(StructuralMaterial const * material, MaterialPlaneType plane)
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

void MainFrame::ReconciliateUIWithElectricalMaterial(ElectricalMaterial const * material, MaterialPlaneType plane)
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

void MainFrame::ReconciliateUIWithRopesMaterial(StructuralMaterial const * material, MaterialPlaneType plane)
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
    bool hasPanel = false;
    for (auto const & entry : mToolSettingsPanels)
    {
        bool const isSelected = (tool.has_value() && std::get<0>(entry) == *tool);

        mToolSettingsPanelsSizer->Show(std::get<1>(entry), isSelected);

        hasPanel |= isSelected;
    }

    // Pickup new layout
    if (hasPanel)
    {
        mMainRibbonBar->Realize();
    }
    else
    {
        // Do not re-realize main ribbon bar, or else the panel becomes tiny
        mToolSettingsPanelsSizer->Layout();
    }

    // Tell status bar
    mStatusBar->SetCurrentToolType(tool);
}

void MainFrame::ReconciliateUIWithPrimaryVisualizationSelection(VisualizationType primaryVisualization)
{
    //
    // Toggle various UI elements <-> primary viz
    //

    bool hasToggledVisualizationModeHeaderPanel = false;
    bool hasToggledToolPanel = false;
    bool hasToggledLayerVisualizationModePanel = false;

    auto const iPrimaryVisualization = static_cast<uint32_t>(primaryVisualization);
    for (uint32_t iVisualization = 0; iVisualization < VisualizationCount; ++iVisualization)
    {
        bool const isVisualizationSelected = (iVisualization == iPrimaryVisualization);

        // Visualization mode header panels
        if (mVisualizationModeHeaderPanelsSizer->IsShown(mVisualizationModeHeaderPanels[iVisualization]) != isVisualizationSelected)
        {
            mVisualizationModeHeaderPanelsSizer->Show(mVisualizationModeHeaderPanels[iVisualization], isVisualizationSelected);
            hasToggledVisualizationModeHeaderPanel = true;
        }

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

    if (hasToggledVisualizationModeHeaderPanel)
    {
        mVisualizationModeHeaderPanelsSizer->Layout();
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

void MainFrame::ReconciliateUIWithVisualWaterlineMarkersEnablement(bool isEnabled)
{
    if (mViewWaterlineMarkersButton->GetValue() != isEnabled)
    {
        mViewWaterlineMarkersButton->SetValue(isEnabled);
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
    // Undo button
    //

    bool const canUndo = !undoStack.IsEmpty();
    if (mUndoButton->IsEnabled() != canUndo)
    {
        mUndoButton->Enable(canUndo);
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

void MainFrame::ReconciliateUIWithDisplayUnitsSystem(UnitsSystem displayUnitsSystem)
{
    mStatusBar->SetDisplayUnitsSystem(displayUnitsSystem);
}

}