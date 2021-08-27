/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-08-27
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MainFrame.h"

#include <GameCore/Log.h>
#include <GameCore/Version.h>

#include <GameOpenGL/GameOpenGL.h>

#include <wx/gbsizer.h>
#include <wx/sizer.h>

#ifdef _MSC_VER
 // Nothing to do here - we use RC files
#else
#include "Resources/ShipBBB.xpm"
#endif

namespace ShipBuilder {

MainFrame::MainFrame(
    wxApp * mainApp,
    ResourceLocator const & resourceLocator,
    LocalizationManager const & localizationManager)
    : mIsStandAlone(true) // TODO
    , mMainApp(mainApp)
    , mResourceLocator(resourceLocator)
    , mLocalizationManager(localizationManager)
    , mIsMouseCapturedByWorkCanvas(false)
{
    Create(
        nullptr,
        wxID_ANY,
        std::string(APPLICATION_NAME " ShipBuilder " APPLICATION_VERSION_SHORT_STR),
        wxDefaultPosition,
        wxDefaultSize,
        wxDEFAULT_FRAME_STYLE | wxMAXIMIZE);

    SetIcon(wxICON(BBB_SHIP_ICON));
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    Maximize();
    Centre();

    //
    // Setup UI
    //

    mMainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    {
        wxPanel * filePanel = CreateFilePanel();

        gridSizer->Add(
            filePanel,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            0,
            0);
    }

    {
        wxPanel * toolSettingsPanel = CreateToolSettingsPanel();

        gridSizer->Add(
            toolSettingsPanel,
            wxGBPosition(0, 1),
            wxGBSpan(1, 1),
            0,
            0);
    }

    {
        wxPanel * gamePanel = CreateGamePanel();

        gridSizer->Add(
            gamePanel,
            wxGBPosition(0, 2),
            wxGBSpan(1, 1),
            0,
            0);
    }

    {
        wxPanel * viewPanel = CreateViewPanel();

        gridSizer->Add(
            viewPanel,
            wxGBPosition(1, 0),
            wxGBSpan(1, 1),
            0,
            0);
    }

    {
        wxPanel * toolbarPanel = CreateToolbarPanel();

        gridSizer->Add(
            toolbarPanel,
            wxGBPosition(2, 0),
            wxGBSpan(1, 1),
            0,
            0);
    }

    {
        wxPanel * workPanel = CreateWorkPanel();

        gridSizer->Add(
            workPanel,
            wxGBPosition(1, 1),
            wxGBSpan(2, 2),
            wxEXPAND,
            0);
    }

    gridSizer->AddGrowableCol(1);
    gridSizer->AddGrowableCol(2);

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

        if (mIsStandAlone)
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
    // Create view
    //

    if (mIsStandAlone)
    {
        // Initialize OpenGL
        GameOpenGL::InitOpenGL();
    }

    mView = std::make_unique<View>(
        mWorkCanvas->GetContentScaleFactor(),
        [this]()
        {
            //LogMessage("TODOTEST: Swapping buffers...");
            mWorkCanvas->SwapBuffers();
            //LogMessage("TODOTEST: ...buffers swapped.");
        },
        mResourceLocator);
}

void MainFrame::Open()
{
    Show(true);
}

/////////////////////////////////////////////////////////////////////

wxPanel * MainFrame::CreateFilePanel()
{
    wxPanel * panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

    {
        // TODO
    }

    panel->SetSizer(sizer);

    return panel;
}

wxPanel * MainFrame::CreateToolSettingsPanel()
{
    wxPanel * panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

    {
        // TODO
    }

    panel->SetSizer(sizer);

    return panel;
}

wxPanel * MainFrame::CreateGamePanel()
{
    wxPanel * panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

    {
        // TODO
    }

    panel->SetSizer(sizer);

    return panel;
}

wxPanel * MainFrame::CreateViewPanel()
{
    wxPanel * panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

    {
        // TODO
    }

    panel->SetSizer(sizer);

    return panel;
}

wxPanel * MainFrame::CreateToolbarPanel()
{
    wxPanel * panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

    {
        // TODO
    }

    panel->SetSizer(sizer);

    return panel;
}

wxPanel * MainFrame::CreateWorkPanel()
{
    wxPanel * panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

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

        mWorkCanvas->Connect(wxEVT_SIZE, (wxObjectEventFunction)&MainFrame::OnWorkCanvasResize, 0, this);
        mWorkCanvas->Connect(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&MainFrame::OnWorkCanvasLeftDown, 0, this);
        mWorkCanvas->Connect(wxEVT_LEFT_UP, (wxObjectEventFunction)&MainFrame::OnWorkCanvasLeftUp, 0, this);
        mWorkCanvas->Connect(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&MainFrame::OnWorkCanvasRightDown, 0, this);
        mWorkCanvas->Connect(wxEVT_RIGHT_UP, (wxObjectEventFunction)&MainFrame::OnWorkCanvasRightUp, 0, this);
        mWorkCanvas->Connect(wxEVT_MOTION, (wxObjectEventFunction)&MainFrame::OnWorkCanvasMouseMove, 0, this);
        mWorkCanvas->Connect(wxEVT_MOUSEWHEEL, (wxObjectEventFunction)&MainFrame::OnWorkCanvasMouseWheel, 0, this);

        sizer->Add(
            mWorkCanvas.get(),
            1, // Occupy all horizontal space
            wxEXPAND, // Stretch vertically as much as available
            0);

        //
        // Create GL context, and make it current on the canvas
        //

        mGLContext = std::make_unique<wxGLContext>(mWorkCanvas.get());
        mGLContext->SetCurrent(*mWorkCanvas);
    }

    panel->SetSizer(sizer);

    return panel;
}

void MainFrame::OnWorkCanvasResize(wxSizeEvent & event)
{
    LogMessage("OnWorkCanvasResize: ", event.GetSize().GetX(), "x", event.GetSize().GetY());

    // TODO: fw to controller

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

    // TODO: fw to controller

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

    // TODO: fw to controller
}

void MainFrame::OnWorkCanvasRightDown(wxMouseEvent & /*event*/)
{
    // TODO: fw to controller

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

    // TODO: fw to controller
}

void MainFrame::OnWorkCanvasMouseMove(wxMouseEvent & event)
{
    // TODO: fw to controller
}

void MainFrame::OnWorkCanvasMouseWheel(wxMouseEvent & event)
{
    // TODO: fw to controller
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

}