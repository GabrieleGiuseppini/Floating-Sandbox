/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MainFrame.h"

#include "SplashScreenDialog.h"
#include "Version.h"

#include <GameLib/GameException.h>
#include <GameLib/Log.h>
#include <GameLib/Utils.h>

#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/string.h>

#include <cassert>
#include <iomanip>
#include <map>
#include <sstream>

const long ID_MAIN_CANVAS = wxNewId();

const long ID_LOAD_SHIP_MENUITEM = wxNewId();
const long ID_RELOAD_LAST_SHIP_MENUITEM = wxNewId();
const long ID_QUIT_MENUITEM = wxNewId();

const long ID_ZOOM_IN_MENUITEM = wxNewId();
const long ID_ZOOM_OUT_MENUITEM = wxNewId();
const long ID_AMBIENT_LIGHT_UP_MENUITEM = wxNewId();
const long ID_AMBIENT_LIGHT_DOWN_MENUITEM = wxNewId();
const long ID_PAUSE_MENUITEM = wxNewId();
const long ID_RESET_VIEW_MENUITEM = wxNewId();

const long ID_SMASH_MENUITEM = wxNewId();
const long ID_SLICE_MENUITEM = wxNewId();
const long ID_GRAB_MENUITEM = wxNewId();
const long ID_SWIRL_MENUITEM = wxNewId();
const long ID_PIN_MENUITEM = wxNewId();
const long ID_TIMERBOMB_MENUITEM = wxNewId();
const long ID_RCBOMB_MENUITEM = wxNewId();
const long ID_RCBOMBDETONATE_MENUITEM = wxNewId();

const long ID_OPEN_SETTINGS_WINDOW_MENUITEM = wxNewId();
const long ID_OPEN_LOG_WINDOW_MENUITEM = wxNewId();
const long ID_SHOW_EVENT_TICKER_MENUITEM = wxNewId();
const long ID_MUTE_MENUITEM = wxNewId();

const long ID_ABOUT_MENUITEM = wxNewId();

const long ID_POSTIINITIALIZE_TIMER = wxNewId();
const long ID_GAME_TIMER = wxNewId();
const long ID_LOW_FREQUENCY_TIMER = wxNewId();

static constexpr int CursorStep = 30;
static constexpr int PowerBarThickness = 2;

MainFrame::MainFrame(wxApp * mainApp)
    : mMainApp(mainApp)
    , mResourceLoader(new ResourceLoader())
    , mGameController()
    , mSoundController()
    , mToolController()
    , mCurrentShipNames()
    , mCurrentRCBombCount(0u)
    , mIsShiftKeyDown(false)
    // Stats
    , mTotalFrameCount(0u)
    , mLastFrameCount(0u)
    , mStatsOriginTimestampReal(std::chrono::steady_clock::time_point::min())
    , mStatsLastTimestampReal(std::chrono::steady_clock::time_point::min())
    , mStatsOriginTimestampGame(GameWallClock::time_point::min())
{
    Create(
        nullptr, 
        wxID_ANY,
        GetVersionInfo(VersionFormat::Long),
        wxDefaultPosition, 
        wxDefaultSize, 
        wxDEFAULT_FRAME_STYLE, 
        _T("Main Frame"));

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    wxPanel* mainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);
    mainPanel->Bind(wxEVT_CHAR_HOOK, (wxObjectEventFunction)&MainFrame::OnKeyDown, this);

    mMainFrameSizer = new wxBoxSizer(wxVERTICAL);

    Connect(this->GetId(), wxEVT_CLOSE_WINDOW, (wxObjectEventFunction)&MainFrame::OnMainFrameClose);
    Connect(this->GetId(), wxEVT_PAINT, (wxObjectEventFunction)&MainFrame::OnPaint);


    //
    // Build main GL canvas
    //
    
    int mainGLCanvasAttributes[] = 
    {
        WX_GL_RGBA,
        WX_GL_DOUBLEBUFFER,
        WX_GL_DEPTH_SIZE,      16,
        WX_GL_STENCIL_SIZE,    1,

        // We want to use OpenGL 3.3, Core Profile        
        // TBD: Not now, my laptop does not support OpenGL 3 :-(
        // WX_GL_CORE_PROFILE,
        // WX_GL_MAJOR_VERSION,    3,
        // WX_GL_MINOR_VERSION,    3,

        0, 0 
    };

    mMainGLCanvas = std::make_unique<wxGLCanvas>(
        mainPanel, 
        ID_MAIN_CANVAS,
        mainGLCanvasAttributes,
        wxDefaultPosition,
        wxSize(640, 480),
        0L,
        _T("Main GL Canvas"));  

    mMainGLCanvas->Connect(wxEVT_SIZE, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasResize, 0, this);
    mMainGLCanvas->Connect(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasLeftDown, 0, this);
    mMainGLCanvas->Connect(wxEVT_LEFT_UP, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasLeftUp, 0, this);
    mMainGLCanvas->Connect(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasRightDown, 0, this);
    mMainGLCanvas->Connect(wxEVT_RIGHT_UP, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasRightUp, 0, this);
    mMainGLCanvas->Connect(wxEVT_MOTION, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasMouseMove, 0, this);
    mMainGLCanvas->Connect(wxEVT_MOUSEWHEEL, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasMouseWheel, 0, this);
    
    mMainFrameSizer->Add(
        mMainGLCanvas.get(),
        1,                  // Proportion
        wxALL | wxEXPAND,   // Flags
        0);                 // Border   

    // Take context for this canvas
    mMainGLCanvasContext = std::make_unique<wxGLContext>(mMainGLCanvas.get());
    mMainGLCanvasContext->SetCurrent(*mMainGLCanvas);

    //
    // Build menu
    //

    wxMenuBar * mainMenuBar = new wxMenuBar();
    

    // File

    wxMenu * fileMenu = new wxMenu();
    
    wxMenuItem * loadShipMenuItem = new wxMenuItem(fileMenu, ID_LOAD_SHIP_MENUITEM, _("Load Ship\tCtrl+O"), wxEmptyString, wxITEM_NORMAL);
    fileMenu->Append(loadShipMenuItem);
    Connect(ID_LOAD_SHIP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnLoadShipMenuItemSelected);

    wxMenuItem * reloadLastShipMenuItem = new wxMenuItem(fileMenu, ID_RELOAD_LAST_SHIP_MENUITEM, _("Reload Last Ship\tCtrl+R"), wxEmptyString, wxITEM_NORMAL);
    fileMenu->Append(reloadLastShipMenuItem);
    Connect(ID_RELOAD_LAST_SHIP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnReloadLastShipMenuItemSelected);

    wxMenuItem* quitMenuItem = new wxMenuItem(fileMenu, ID_QUIT_MENUITEM, _("Quit\tAlt-F4"), _("Quit the application"), wxITEM_NORMAL);
    fileMenu->Append(quitMenuItem);
    Connect(ID_QUIT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnQuit);

    mainMenuBar->Append(fileMenu, _("&File"));


    // Control

    wxMenu * controlsMenu = new wxMenu();

    wxMenuItem * zoomInMenuItem = new wxMenuItem(controlsMenu, ID_ZOOM_IN_MENUITEM, _("Zoom In\t+"), wxEmptyString, wxITEM_NORMAL);
    controlsMenu->Append(zoomInMenuItem);
    Connect(ID_ZOOM_IN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnZoomInMenuItemSelected);

    wxMenuItem * zoomOutMenuItem = new wxMenuItem(controlsMenu, ID_ZOOM_OUT_MENUITEM, _("Zoom Out\t-"), wxEmptyString, wxITEM_NORMAL);
    controlsMenu->Append(zoomOutMenuItem);
    Connect(ID_ZOOM_OUT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnZoomOutMenuItemSelected);

    wxMenuItem * amblientLightUpMenuItem = new wxMenuItem(controlsMenu, ID_AMBIENT_LIGHT_UP_MENUITEM, _("Bright Ambient Light\tPgUp"), wxEmptyString, wxITEM_NORMAL);    
    controlsMenu->Append(amblientLightUpMenuItem);
    Connect(ID_AMBIENT_LIGHT_UP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAmbientLightUpMenuItemSelected);

    wxMenuItem * ambientLightDownMenuItem = new wxMenuItem(controlsMenu, ID_AMBIENT_LIGHT_DOWN_MENUITEM, _("Dim Ambient Light\tPgDn"), wxEmptyString, wxITEM_NORMAL);
    controlsMenu->Append(ambientLightDownMenuItem);
    Connect(ID_AMBIENT_LIGHT_DOWN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAmbientLightDownMenuItemSelected);

    mPauseMenuItem = new wxMenuItem(controlsMenu, ID_PAUSE_MENUITEM, _("Pause\tSpace"), _("Pause the game"), wxITEM_CHECK);
    controlsMenu->Append(mPauseMenuItem);
    mPauseMenuItem->Check(false);
    Connect(ID_PAUSE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnPauseMenuItemSelected);

    controlsMenu->Append(new wxMenuItem(controlsMenu, wxID_SEPARATOR));

    wxMenuItem * resetViewMenuItem = new wxMenuItem(controlsMenu, ID_RESET_VIEW_MENUITEM, _("Reset View\tESC"), wxEmptyString, wxITEM_NORMAL);
    controlsMenu->Append(resetViewMenuItem);
    Connect(ID_RESET_VIEW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnResetViewMenuItemSelected);

    mainMenuBar->Append(controlsMenu, _("Controls"));


    // Tools

    mToolsMenu = new wxMenu();

    wxMenuItem * smashMenuItem = new wxMenuItem(mToolsMenu, ID_SMASH_MENUITEM, _("Smash\tS"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(smashMenuItem);
    Connect(ID_SMASH_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSmashMenuItemSelected);

    wxMenuItem * sliceMenuItem = new wxMenuItem(mToolsMenu, ID_SLICE_MENUITEM, _("Slice\tL"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(sliceMenuItem);
    Connect(ID_SLICE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSliceMenuItemSelected);

    wxMenuItem * grabMenuItem = new wxMenuItem(mToolsMenu, ID_GRAB_MENUITEM, _("Grab\tG"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(grabMenuItem);
    Connect(ID_GRAB_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnGrabMenuItemSelected);

    wxMenuItem * swirlMenuItem = new wxMenuItem(mToolsMenu, ID_SWIRL_MENUITEM, _("Swirl\tW"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(swirlMenuItem);
    Connect(ID_SWIRL_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSwirlMenuItemSelected);

    wxMenuItem * pinMenuItem = new wxMenuItem(mToolsMenu, ID_PIN_MENUITEM, _("Toggle Pin\tP"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(pinMenuItem);
    Connect(ID_PIN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnPinMenuItemSelected);

    wxMenuItem * timerBombMenuItem = new wxMenuItem(mToolsMenu, ID_TIMERBOMB_MENUITEM, _("Toggle Timer Bomb\tT"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(timerBombMenuItem);
    Connect(ID_TIMERBOMB_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnTimerBombMenuItemSelected);

    wxMenuItem * rcBombMenuItem = new wxMenuItem(mToolsMenu, ID_RCBOMB_MENUITEM, _("Toggle RC Bomb\tR"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(rcBombMenuItem);
    Connect(ID_RCBOMB_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnRCBombMenuItemSelected);

    mToolsMenu->Append(new wxMenuItem(mToolsMenu, wxID_SEPARATOR));

    mRCBombsDetonateMenuItem = new wxMenuItem(mToolsMenu, ID_RCBOMBDETONATE_MENUITEM, _("Detonate RC Bombs\tD"), wxEmptyString, wxITEM_NORMAL);
    mToolsMenu->Append(mRCBombsDetonateMenuItem);
    Connect(ID_RCBOMBDETONATE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnRCBombDetonateMenuItemSelected);
    mRCBombsDetonateMenuItem->Enable(false);
    
    mainMenuBar->Append(mToolsMenu, _("Tools"));

    
    // Options

    wxMenu * optionsMenu = new wxMenu();

    wxMenuItem * openSettingsWindowMenuItem = new wxMenuItem(optionsMenu, ID_OPEN_SETTINGS_WINDOW_MENUITEM, _("Open Settings Window\tCtrl+S"), wxEmptyString, wxITEM_NORMAL);   
    optionsMenu->Append(openSettingsWindowMenuItem);
    Connect(ID_OPEN_SETTINGS_WINDOW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnOpenSettingsWindowMenuItemSelected);

    wxMenuItem * openLogWindowMenuItem = new wxMenuItem(optionsMenu, ID_OPEN_LOG_WINDOW_MENUITEM, _("Open Log Window\tCtrl+L"), wxEmptyString, wxITEM_NORMAL);
    optionsMenu->Append(openLogWindowMenuItem);
    Connect(ID_OPEN_LOG_WINDOW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnOpenLogWindowMenuItemSelected);

    mShowEventTickerMenuItem = new wxMenuItem(optionsMenu, ID_SHOW_EVENT_TICKER_MENUITEM, _("Show Event Ticker\tCtrl+T"), wxEmptyString, wxITEM_CHECK);
    optionsMenu->Append(mShowEventTickerMenuItem);
    mShowEventTickerMenuItem->Check(false);
    Connect(ID_SHOW_EVENT_TICKER_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShowEventTickerMenuItemSelected);

    optionsMenu->Append(new wxMenuItem(optionsMenu, wxID_SEPARATOR));

    mMuteMenuItem = new wxMenuItem(optionsMenu, ID_MUTE_MENUITEM, _("Mute\tCtrl+M"), wxEmptyString, wxITEM_CHECK);
    optionsMenu->Append(mMuteMenuItem);
    mMuteMenuItem->Check(false);
    Connect(ID_MUTE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnMuteMenuItemSelected);

    mainMenuBar->Append(optionsMenu, _("Options"));


    // Help

    wxMenu * helpMenu = new wxMenu();

    wxMenuItem * aboutMenuItem = new wxMenuItem(helpMenu, ID_ABOUT_MENUITEM, _("About\tF1"), _("Show info about this application"), wxITEM_NORMAL);
    helpMenu->Append(aboutMenuItem);
    Connect(ID_ABOUT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAboutMenuItemSelected);

    mainMenuBar->Append(helpMenu, _("Help"));

    SetMenuBar(mainMenuBar);

    //
    // Event ticker panel
    //

    mEventTickerPanel = std::make_unique<EventTickerPanel>(mainPanel);
    
    mMainFrameSizer->Add(mEventTickerPanel.get(), 0, wxEXPAND);

    mMainFrameSizer->Hide(mEventTickerPanel.get());


    //
    // Finalize frame
    //

    mainPanel->SetSizerAndFit(mMainFrameSizer);
    
    Maximize();
    Centre();


    //
    // Initialize timers
    //

    mGameTimer = std::make_unique<wxTimer>(this, ID_GAME_TIMER);
    Connect(ID_GAME_TIMER, wxEVT_TIMER, (wxObjectEventFunction)&MainFrame::OnGameTimerTrigger);

    mLowFrequencyTimer = std::make_unique<wxTimer>(this, ID_LOW_FREQUENCY_TIMER);
    Connect(ID_LOW_FREQUENCY_TIMER, wxEVT_TIMER, (wxObjectEventFunction)&MainFrame::OnLowFrequencyTimerTrigger);


    //
    // Post a PostInitialize, so that we can complete initialization with a main loop running
    //

    mPostInitializeTimer = std::make_unique<wxTimer>(this, ID_POSTIINITIALIZE_TIMER);
    Connect(ID_POSTIINITIALIZE_TIMER, wxEVT_TIMER, (wxObjectEventFunction)&MainFrame::OnPostInitializeTrigger);
    mPostInitializeTimer->Start(0, true);
}

MainFrame::~MainFrame()
{
}

//
// App event handlers
//

void MainFrame::OnPostInitializeTrigger(wxTimerEvent & /*event*/)
{
    //
    // Create splash screen
    //

    std::unique_ptr<SplashScreenDialog> splash = std::make_unique<SplashScreenDialog>(*mResourceLoader);
    mMainApp->Yield();

#ifdef _DEBUG
    // The guy is pesky while debugging
    splash->Hide();
#endif


    //
    // Create Game controller
    //

    try
    {
        mGameController = GameController::Create(
            mResourceLoader,
            [&splash, this](float progress, std::string const & message)
            {
                splash->UpdateProgress(progress / 2.0f, message);
                this->mMainApp->Yield();
                this->mMainApp->Yield();
                this->mMainApp->Yield();
            });
    }
    catch (std::exception const & e)
    {
        Die("Error during initialization of game controller: " + std::string(e.what()));

        return;
    }

    this->mMainApp->Yield();


    //
    // Create Sound controller
    //

    try
    {
        mSoundController = std::make_unique<SoundController>(
            mResourceLoader,
            [&splash, this](float progress, std::string const & message)
            {
                splash->UpdateProgress(0.5f + progress / 2.0f, message);
                this->mMainApp->Yield();
                this->mMainApp->Yield();
                this->mMainApp->Yield();
            });
    }
    catch (std::exception const & e)
    {
        Die("Error during initialization of sound controller: " + std::string(e.what()));

        return;
    }

    this->mMainApp->Yield();


    //
    // Create Tool controller
    //

    // Set initial tool
    ToolType initialToolType = ToolType::Smash;
    mToolsMenu->Check(ID_SMASH_MENUITEM, true);

    try
    {
        mToolController = std::make_unique<ToolController>(
            initialToolType,
            this,
            mGameController,
            *mResourceLoader);
    }
    catch (std::exception const & e)
    {
        Die("Error during initialization of tool controller: " + std::string(e.what()));

        return;
    }

    this->mMainApp->Yield();


    //
    // Register game event handlers
    //

    mGameController->RegisterGameEventHandler(this);
    mGameController->RegisterGameEventHandler(mEventTickerPanel.get());
    mGameController->RegisterGameEventHandler(mSoundController.get());


    //
    // Load initial ship
    //

    auto defaultShipFilePath = mResourceLoader->GetDefaultShipDefinitionFilePath();

    try
    {
        mGameController->AddShip(defaultShipFilePath);
    }
    catch (std::exception const & e)
    {
        Die("Error during initialization: " + std::string(e.what()));

        return;
    }

    splash->UpdateProgress(1.0f, "Ready!");

    this->mMainApp->Yield();


    //
    // Close splash screen
    //

#ifndef _DEBUG

    // Make sure the splash screen shows for at least half a sec, or else it's weird
    auto start = std::chrono::steady_clock::now();
    auto end = std::chrono::steady_clock::now();
    while ((end - start) < std::chrono::milliseconds(500))
    {
        this->mMainApp->Yield();
        Sleep(10);
        end = std::chrono::steady_clock::now();
    }

#endif

    splash->Destroy();



    //
    // Start timers
    //

    mGameTimer->Start(0, true);
    mLowFrequencyTimer->Start(1000, false);


    //
    // Show ourselves now
    //

    this->Show();
}

void MainFrame::OnMainFrameClose(wxCloseEvent & /*event*/)
{
    if (!!mGameTimer)
        mGameTimer->Stop();
    if (!!mLowFrequencyTimer)
        mLowFrequencyTimer->Stop();

    Destroy();
}

void MainFrame::OnQuit(wxCommandEvent & /*event*/)
{
    Close();
}

void MainFrame::OnPaint(wxPaintEvent & event)
{
    RenderGame();

    event.Skip();
}

void MainFrame::OnKeyDown(wxKeyEvent & event)
{
    if (event.GetKeyCode() == WXK_LEFT)
    {
        // Left
        mGameController->Pan(vec2f(-20.0f, 0.0f));
    }
    else if (event.GetKeyCode() == WXK_UP)
    {
        // Up
        mGameController->Pan(vec2f(00.0f, -20.0f));
    }
    else if (event.GetKeyCode() == WXK_RIGHT)
    {
        // Right
        mGameController->Pan(vec2f(20.0f, 0.0f));
    }
    else if (event.GetKeyCode() == WXK_DOWN)
    {
        // Down
        mGameController->Pan(vec2f(0.0f, 20.0f));
    }
    
    event.Skip();
}

void MainFrame::OnGameTimerTrigger(wxTimerEvent & /*event*/)
{
    // Initialize stats, if needed
    if (mStatsOriginTimestampReal == std::chrono::steady_clock::time_point::min())
    { 
        assert(mStatsLastTimestampReal == std::chrono::steady_clock::time_point::min());
        assert(mStatsOriginTimestampGame == GameWallClock::time_point::min());

        std::chrono::steady_clock::time_point nowReal = std::chrono::steady_clock::now();
        mStatsOriginTimestampReal = nowReal;
        mStatsLastTimestampReal = nowReal;
        mStatsOriginTimestampGame = GameWallClock::GetInstance().Now();

        mTotalFrameCount = 0u;
        mLastFrameCount = 0u;
    }
    
    // Make the timer for the next step start now
    // TODO: see if should QueueEvent instead
    mGameTimer->Start(0, true);

    // Update SHIFT key state
    if (wxGetKeyState(WXK_SHIFT))
    {
        if (!mIsShiftKeyDown)
        {
            mIsShiftKeyDown = true;
            mToolController->OnShiftKeyDown();
        }
    }
    else
    {
        if (mIsShiftKeyDown)
        {
            mIsShiftKeyDown = false;
            mToolController->OnShiftKeyUp();
        }
    }

    // Run a game step
    DoGameStep();
    
    // Update stats
    ++mTotalFrameCount;
    ++mLastFrameCount;
}

void MainFrame::OnLowFrequencyTimerTrigger(wxTimerEvent & /*event*/)
{
    //
    // Update fps in title
    //

    SetFrameTitle();


    //
    // Update sound controller
    //

    assert(!!mSoundController);
    mSoundController->LowFrequencyUpdate();
}

//
// Main canvas event handlers
//

void MainFrame::OnMainGLCanvasResize(wxSizeEvent & event)
{
    if (!!mGameController)
    {
        mGameController->SetCanvasSize(
            event.GetSize().GetX(),
            event.GetSize().GetY());
    }
}

void MainFrame::OnMainGLCanvasLeftDown(wxMouseEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->OnLeftMouseDown();
}

void MainFrame::OnMainGLCanvasLeftUp(wxMouseEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->OnLeftMouseUp();
}

void MainFrame::OnMainGLCanvasRightDown(wxMouseEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->OnRightMouseDown();
}

void MainFrame::OnMainGLCanvasRightUp(wxMouseEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->OnRightMouseUp();
}

void MainFrame::OnMainGLCanvasMouseMove(wxMouseEvent& event)
{
    assert(!!mToolController);
    mToolController->OnMouseMove(event.GetX(), event.GetY());
}

void MainFrame::OnMainGLCanvasMouseWheel(wxMouseEvent& event)
{
    assert(!!mGameController);

    mGameController->AdjustZoom(powf(1.002f, event.GetWheelRotation()));
}


//
// Menu event handlers
//

void MainFrame::OnLoadShipMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mFileOpenDialog)
    {
        mFileOpenDialog = std::make_unique<wxFileDialog>(
            this, 
            L"Select Ship", 
            wxEmptyString, 
            wxEmptyString, 
            L"Ship files (*.shp; *.png)|*.shp; *.png",
            wxFD_OPEN | wxFD_FILE_MUST_EXIST, 
            wxDefaultPosition, 
            wxDefaultSize, 
            _T("File Open Dialog"));
    }

    assert(!!mFileOpenDialog);

    if (mFileOpenDialog->ShowModal() == wxID_OK)
    {
        std::string filename = mFileOpenDialog->GetPath().ToStdString();

        ResetState();

        assert(!!mGameController);
        try
        {
            mGameController->ResetAndLoadShip(filename);
        }
        catch (std::exception const & ex)
        { 
            Die(ex.what());
        }
    }
}

void MainFrame::OnReloadLastShipMenuItemSelected(wxCommandEvent & /*event*/)
{
    ResetState();

    assert(!!mGameController);
    try
    {
        mGameController->ReloadLastShip();
    }
    catch (std::exception const & ex)
    {
        Die(ex.what());
    }
}

void MainFrame::OnPauseMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (IsPaused())
    {
        GameWallClock::GetInstance().Pause();

        if (!!mSoundController)
            mSoundController->SetPaused(true);
    }
    else
    { 
        GameWallClock::GetInstance().Resume();

        if (!!mSoundController)
            mSoundController->SetPaused(false);
    }
}

void MainFrame::OnResetViewMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->ResetPan();
    mGameController->ResetZoom();
}

void MainFrame::OnZoomInMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);

    mGameController->AdjustZoom(1.05f);
}

void MainFrame::OnZoomOutMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);

    mGameController->AdjustZoom(0.95f);
}

void MainFrame::OnAmbientLightUpMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);

    float newAmbientLight = std::min(1.0f, mGameController->GetAmbientLightIntensity() * 1.05f);
    mGameController->SetAmbientLightIntensity(newAmbientLight);
}

void MainFrame::OnAmbientLightDownMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);

    float newAmbientLight = mGameController->GetAmbientLightIntensity() * 0.95f;
    mGameController->SetAmbientLightIntensity(newAmbientLight);
}

void MainFrame::OnSmashMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::Smash);
}

void MainFrame::OnSliceMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::Saw);
}

void MainFrame::OnGrabMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::Grab);
}

void MainFrame::OnSwirlMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::Swirl);
}

void MainFrame::OnPinMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::Pin);
}

void MainFrame::OnTimerBombMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::TimerBomb);
}

void MainFrame::OnRCBombMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::RCBomb);
}

void MainFrame::OnRCBombDetonateMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->DetonateRCBombs();
}

void MainFrame::OnOpenSettingsWindowMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mSettingsDialog)
    {
        mSettingsDialog = std::make_unique<SettingsDialog>(
            this,
            mGameController);
    }

    mSettingsDialog->Open();
}

void MainFrame::OnOpenLogWindowMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mLoggingDialog)
    {
        mLoggingDialog = std::make_unique<LoggingDialog>(this);
    }

    mLoggingDialog->Open();
}

void MainFrame::OnShowEventTickerMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mEventTickerPanel);
    if (mShowEventTickerMenuItem->IsChecked())
    {
        mMainFrameSizer->Show(mEventTickerPanel.get());
    }
    else
    {
        mMainFrameSizer->Hide(mEventTickerPanel.get());
    }

    mMainFrameSizer->Layout();
}

void MainFrame::OnMuteMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mSoundController);
    mSoundController->SetMute(mMuteMenuItem->IsChecked());
}

void MainFrame::OnAboutMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mAboutDialog)
    {
        mAboutDialog = std::make_unique<AboutDialog>(
            this,
            *mResourceLoader);
    }

    mAboutDialog->ShowModal();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void MainFrame::ResetState()
{
    assert(!!mSoundController);
    mSoundController->Reset();

    mRCBombsDetonateMenuItem->Enable(false);
}

void MainFrame::SetFrameTitle()
{
    //
    // Update fps and total (game) duration
    //

    std::chrono::steady_clock::time_point nowReal = std::chrono::steady_clock::now();

    auto totalElapsedReal = std::chrono::duration<float>(nowReal - mStatsOriginTimestampReal);
    auto lastElapsedReal = std::chrono::duration<float>(nowReal - mStatsLastTimestampReal);


    float totalFps = static_cast<float>(mTotalFrameCount) / totalElapsedReal.count();
    float lastFps = static_cast<float>(mLastFrameCount) / lastElapsedReal.count();

    auto totalElapsedGame = std::chrono::duration<float>(GameWallClock::GetInstance().Now() - mStatsOriginTimestampGame);
    int totalElapsedSecondsGame = static_cast<int>(roundf(totalElapsedGame.count()));
    int minutesGame = totalElapsedSecondsGame / 60;
    int secondsGame = totalElapsedSecondsGame - (minutesGame * 60);

    //
    // Build title
    //

    std::ostringstream ss;

    ss.fill('0');

    ss << GetVersionInfo(VersionFormat::Long)
        << "  FPS: " << std::fixed << std::setprecision(2) << totalFps << " (" << lastFps << ")"
        << " " << std::setw(2) << minutesGame << ":" << std::setw(2) << secondsGame;

    if (!mCurrentShipNames.empty())
    {
        ss << " - "
            << Utils::Join(mCurrentShipNames, " + ");
    }

    SetTitle(ss.str());

    mLastFrameCount = 0u;
    mStatsLastTimestampReal = nowReal;
}

bool MainFrame::IsPaused()
{
    return mPauseMenuItem->IsChecked();
}

void MainFrame::DoGameStep()
{
    // Update tool controller
    assert(!!mToolController);
    mToolController->Update();

    // Do a simulation step
    if (!IsPaused())
    {
        assert(!!mGameController);
        mGameController->DoStep();
    }

    // Render
    RenderGame();

    // Update event ticker
    assert(!!mEventTickerPanel);
    mEventTickerPanel->Update();

    // Update sound controller
    assert(!!mSoundController);
    mSoundController->HighFrequencyUpdate();
}

void MainFrame::RenderGame()
{
    if (!!mGameController)
    {
        // Render
        mGameController->Render();

        // Flush all the draw operations and flip the back buffer onto the screen.  
        mMainGLCanvas->SwapBuffers();
        mMainGLCanvas->Refresh();
    }
}

void MainFrame::Die(std::string const & message)
{
    //
    // Stop timers first
    //

    if (!!mGameTimer)
    {
        mGameTimer->Stop();
        mGameTimer.reset();
    }

    if (!!mLowFrequencyTimer)
    {
        mLowFrequencyTimer->Stop();
        mLowFrequencyTimer.reset();
    }


    //
    // Show message
    //

    wxMessageBox(message, wxT("Maritime Disaster"), wxICON_ERROR);


    //
    // Exit
    //

    this->Destroy();
}
