/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-08-27
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MainFrame.h"

#include <GameCore/Log.h>
#include <GameCore/Version.h>

#include <GameOpenGL/GameOpenGL.h>

#include <UILib/BitmapButton.h>
#include <UILib/BitmapToggleButton.h>
#include <UILib/WxHelpers.h>

#include <wx/button.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/tglbtn.h>

#ifdef _MSC_VER
 // Nothing to do here - we use RC files
#else
#include "Resources/ShipBBB.xpm"
#endif

#include <sstream>

int constexpr ButtonMargin = 4;

namespace ShipBuilder {

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

    SetIcon(wxICON(BBB_SHIP_ICON));
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    Maximize();
    Centre();

    //
    // Setup main frame
    //

    mMainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    {
        wxPanel * filePanel = CreateFilePanel(mMainPanel);

        gridSizer->Add(
            filePanel,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL,
            0);
    }

    {
        wxPanel * toolSettingsPanel = CreateToolSettingsPanel(mMainPanel);

        gridSizer->Add(
            toolSettingsPanel,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL,
            0);
    }

    {
        wxPanel * gamePanel = CreateGamePanel(mMainPanel);

        gridSizer->Add(
            gamePanel,
            wxGBPosition(0, 2),
            wxGBSpan(1, 1),
            0,
            0);
    }

    {
        wxPanel * viewPanel = CreateViewPanel(mMainPanel);

        gridSizer->Add(
            viewPanel,
            wxGBPosition(1, 0),
            wxGBSpan(1, 1),
            0,
            0);
    }

    {
        wxPanel * toolbarPanel = CreateToolbarPanel(mMainPanel);

        gridSizer->Add(
            toolbarPanel,
            wxGBPosition(2, 0),
            wxGBSpan(1, 1),
            wxEXPAND | wxALIGN_TOP | wxALIGN_CENTER_HORIZONTAL,
            0);
    }

    {
        wxPanel * workPanel = CreateWorkPanel(mMainPanel);

        gridSizer->Add(
            workPanel,
            wxGBPosition(1, 1),
            wxGBSpan(2, 2),
            wxEXPAND,
            0);
    }

    {
        mStatusBar = new wxStatusBar(mMainPanel, wxID_ANY, 0);
        mStatusBar->SetFieldsCount(1);

        gridSizer->Add(
            mStatusBar,
            wxGBPosition(3, 0),
            wxGBSpan(1, 3),
            wxEXPAND,
            0);
    }

    gridSizer->AddGrowableRow(2, 1);
    gridSizer->AddGrowableCol(1, 1);

    mMainPanel->SetSizer(gridSizer);

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

        if (!IsStandAlone())
        {
            wxMenuItem * saveAndGoBackMenuItem = new wxMenuItem(fileMenu, wxID_ANY, _("Save and Return to Game"), _("Save the current ship and return to the simulator"), wxITEM_NORMAL);
            fileMenu->Append(saveAndGoBackMenuItem);
            Connect(saveAndGoBackMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSaveAndGoBack);

            saveAndGoBackMenuItem->Enable(false); // Only enabled when dirty
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
    // Setup material palette
    //

    mMaterialPalette = std::make_unique<MaterialPalette>(
        mMaterialDatabase,
        mShipTexturizer,
        mWorkbenchState);

    //
    // Create view
    //

    if (IsStandAlone())
    {
        // Initialize OpenGL
        GameOpenGL::InitOpenGL();
    }

    mView = std::make_unique<View>(
        DisplayLogicalSize(
            mWorkCanvas->GetSize().GetWidth(),
            mWorkCanvas->GetSize().GetHeight()),
        mWorkCanvas->GetContentScaleFactor(),
        [this]()
        {
            mWorkCanvas->SwapBuffers();
        },
        mResourceLocator);

    //
    // Create controller
    //

    mController = std::make_unique<Controller>(
        *mView,
        mWorkbenchState,
        *this);

    //
    // Initialize UI
    //

    SyncWorkbenchStateToUI();
}

void MainFrame::OpenForNewShip()
{
    mController->CreateNewShip();

    Open();
}

void MainFrame::OpenForShip(std::filesystem::path const & shipFilePath)
{
    mController->LoadShip(shipFilePath);

    Open();
}

void MainFrame::DisplayToolCoordinates(std::optional<WorkSpaceCoordinates> coordinates)
{
    std::stringstream ss;

    if (coordinates.has_value())
    {
        ss << coordinates->x << ", " << coordinates->y;
    }

    mStatusBar->SetStatusText(ss.str(), 0);
}

void MainFrame::OnWorkSpaceSizeChanged()
{
    RecalculatePanning();
}

/////////////////////////////////////////////////////////////////////

wxPanel * MainFrame::CreateFilePanel(wxWindow * parent)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

    {
        {
            wxButton * button = new wxButton(panel, wxID_ANY, "Some");
            sizer->Add(button, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
        }

        {
            wxButton * button = new wxButton(panel, wxID_ANY, "Button");
            sizer->Add(button, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
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
                _T("Save the current ship and return to the simulator"));

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
                _T("Discard the current ship and return to the simulator"));

            sizer->Add(quitAndReturnToGameButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, ButtonMargin);
        }
    }

    panel->SetSizerAndFit(sizer);

    return panel;
}

wxPanel * MainFrame::CreateViewPanel(wxWindow * parent)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    // TODOTEST
    panel->SetBackgroundColour(*wxYELLOW);

    wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

    {
        {
            wxSlider * slider = new wxSlider(panel, wxID_ANY, 0, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_VERTICAL);
            sizer->Add(slider, 0, wxLEFT | wxRIGHT, 4);
        }
    }

    panel->SetSizerAndFit(sizer);

    return panel;
}

wxPanel * MainFrame::CreateToolbarPanel(wxWindow * parent)
{
    wxPanel * panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);

    // Tools

    {
        wxGridBagSizer * toolsSizer = new wxGridBagSizer(3, 3);

        // Pencil
        {
            auto button = new BitmapToggleButton(
                panel,
                mResourceLocator.GetIconFilePath("pencil_icon"),
                [this]()
                {
                    // TODOHERE
                    //SetTool(ToolType::Pencil);
                },
                _T("Draw individual structure particles"));

            toolsSizer->Add(
                button,
                wxGBPosition(0, 0),
                wxGBSpan(1, 1),
                0,
                0);
        }

        // Eraser
        {
            auto button = new BitmapToggleButton(
                panel,
                mResourceLocator.GetIconFilePath("eraser_icon"),
                [this]()
                {
                    // TODOHERE
                    //SetTool(ToolType::Eraser);
                },
                _T("Erase individual structure particles"));

            toolsSizer->Add(
                button,
                wxGBPosition(0, 1),
                wxGBSpan(1, 1),
                0,
                0);
        }

        sizer->Add(
            toolsSizer,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);
    }

    sizer->AddSpacer(15);

    // Palette

    {
        wxBoxSizer * paletteSizer = new wxBoxSizer(wxVERTICAL);

        // Foreground
        {
            mForegroundMaterialStaticBitmap = new wxStaticBitmap(
                panel,
                wxID_ANY,
                WxHelpers::MakeEmptyBitmap(),
                wxDefaultPosition, wxDefaultSize,
                wxBORDER_SUNKEN);

            mForegroundMaterialStaticBitmap->Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&MainFrame::OnForegroundMaterialSwath, this);

            paletteSizer->Add(
                mForegroundMaterialStaticBitmap,
                0,
                0,
                0);
        }

        // Background
        {
            mBackgroundMaterialStaticBitmap = new wxStaticBitmap(
                panel,
                wxID_ANY,
                WxHelpers::MakeEmptyBitmap(),
                wxDefaultPosition, wxDefaultSize,
                wxBORDER_SUNKEN);

            mBackgroundMaterialStaticBitmap->Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&MainFrame::OnBackgroundMaterialSwath, this);

            paletteSizer->Add(
                mBackgroundMaterialStaticBitmap,
                0,
                0,
                0);
        }

        sizer->Add(
            paletteSizer,
            0,
            wxALIGN_CENTER_HORIZONTAL,
            0);
    }

    panel->SetSizerAndFit(sizer);

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
        // TODO: connect

        sizer->Add(
            mWorkCanvasVScrollBar,
            1, // Occupy all space
            wxEXPAND, // Stretch as much as available
            0);
    }

    // H-scrollbar

    {
        mWorkCanvasHScrollBar = new wxScrollBar(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_HORIZONTAL);
        // TODO: connect

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

void MainFrame::OnForegroundMaterialSwath(wxMouseEvent & event)
{
    // TODO: open mMaterialPalette
    // If returns true: has changed workbenchState, so start OnWorkbenchStateChanged cascade
}

void MainFrame::OnBackgroundMaterialSwath(wxMouseEvent & event)
{
    // TODO: open mMaterialPalette
    // If returns true: has changed workbenchState, so start OnWorkbenchStateChanged cascade
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

    if (mView)
    {
        mView->SetDisplayLogicalSize(
            DisplayLogicalSize(
                event.GetSize().GetX(),
                event.GetSize().GetY()));
    }

    RecalculatePanning();

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
    // TODO: fw to controller
}

void MainFrame::OnWorkCanvasCaptureMouseLost(wxMouseCaptureLostEvent & /*event*/)
{
    // TODO: fw to controller, who will de-initialize the current tool
    // (as if lmouseup)
}

void MainFrame::OnWorkCanvasMouseLeftWindow(wxMouseEvent & /*event*/)
{
    if (!mIsMouseCapturedByWorkCanvas)
    {
        this->DisplayToolCoordinates(std::nullopt);
    }
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

void MainFrame::OnOpenLogWindowMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mLoggingDialog)
    {
        mLoggingDialog = std::make_unique<LoggingDialog>(this);
    }

    mLoggingDialog->Open();
}

void MainFrame::Open()
{
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
    // Hide self
    Show(false);

    // Invoke functor to go back
    assert(mReturnToGameFunctor);
    mReturnToGameFunctor(std::move(shipFilePath));
}

void MainFrame::RecalculatePanning()
{
    // TODOHERE
}

void MainFrame::SyncWorkbenchStateToUI()
{
    // Populate swaths in Toolbar
    // TODOHERE
    //wxStaticBitmap * mForegroundMaterialStaticBitmap;
    //wxStaticBitmap * mBackgroundMaterialStaticBitmap;

    // TODO: Populate settings in ToolSettings toolbar
}

}