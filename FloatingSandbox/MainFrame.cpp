/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MainFrame.h"

#include "CheckForUpdatesDialog.h"
#include "NewVersionDisplayDialog.h"
#include "ShipDescriptionDialog.h"
#include "SplashScreenDialog.h"
#include "StandardSystemPaths.h"
#include "StartupTipDialog.h"

#include <Game/ImageFileTools.h>

#include <GameOpenGL/GameOpenGL.h>

#include <GameCore/GameException.h>
#include <GameCore/ImageSize.h>
#include <GameCore/Log.h>
#include <GameCore/Utils.h>
#include <GameCore/Version.h>

#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/tooltip.h>

#include <cassert>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <map>
#include <sstream>
#include <thread>

#ifdef _MSC_VER
 // Nothing to do here - we use RC files
#else
#include "Resources/ShipBBB.xpm"
#endif

long const ID_MAIN_CANVAS = wxNewId();

long const ID_LOAD_SHIP_MENUITEM = wxNewId();
long const ID_RELOAD_LAST_SHIP_MENUITEM = wxNewId();
long const ID_MORE_SHIPS_MENUITEM = wxNewId();
long const ID_SAVE_SCREENSHOT_MENUITEM = wxNewId();
long const ID_QUIT_MENUITEM = wxNewId();

long const ID_ZOOM_IN_MENUITEM = wxNewId();
long const ID_ZOOM_OUT_MENUITEM = wxNewId();
long const ID_AMBIENT_LIGHT_UP_MENUITEM = wxNewId();
long const ID_AMBIENT_LIGHT_DOWN_MENUITEM = wxNewId();
long const ID_PAUSE_MENUITEM = wxNewId();
long const ID_STEP_MENUITEM = wxNewId();
long const ID_RESET_VIEW_MENUITEM = wxNewId();

long const ID_MOVE_MENUITEM = wxNewId();
long const ID_MOVE_ALL_MENUITEM = wxNewId();
long const ID_PICK_AND_PULL_MENUITEM = wxNewId();
long const ID_SMASH_MENUITEM = wxNewId();
long const ID_SLICE_MENUITEM = wxNewId();
long const ID_HEAT_BLASTER_MENUITEM = wxNewId();
long const ID_FIRE_EXTINGUISHER_MENUITEM = wxNewId();
long const ID_GRAB_MENUITEM = wxNewId();
long const ID_SWIRL_MENUITEM = wxNewId();
long const ID_PIN_MENUITEM = wxNewId();
long const ID_INJECT_AIR_BUBBLES_MENUITEM = wxNewId();
long const ID_FLOOD_HOSE_MENUITEM = wxNewId();
long const ID_TIMERBOMB_MENUITEM = wxNewId();
long const ID_RCBOMB_MENUITEM = wxNewId();
long const ID_IMPACTBOMB_MENUITEM = wxNewId();
long const ID_ANTIMATTERBOMB_MENUITEM = wxNewId();
long const ID_THANOSSNAP_MENUITEM = wxNewId();
long const ID_WAVEMAKER_MENUITEM = wxNewId();
long const ID_ADJUSTTERRAIN_MENUITEM = wxNewId();
long const ID_REPAIRSTRUCTURE_MENUITEM = wxNewId();
long const ID_SCRUB_MENUITEM = wxNewId();
long const ID_RCBOMBDETONATE_MENUITEM = wxNewId();
long const ID_ANTIMATTERBOMBDETONATE_MENUITEM = wxNewId();
long const ID_TRIGGERTSUNAMI_MENUITEM = wxNewId();
long const ID_TRIGGERROGUEWAVE_MENUITEM = wxNewId();
long const ID_TRIGGERSTORM_MENUITEM = wxNewId();
long const ID_TRIGGERLIGHTNING_MENUITEM = wxNewId();

long const ID_OPEN_SETTINGS_WINDOW_MENUITEM = wxNewId();
long const ID_RELOAD_LAST_MODIFIED_SETTINGS_MENUITEM = wxNewId();
long const ID_OPEN_PREFERENCES_WINDOW_MENUITEM = wxNewId();
long const ID_OPEN_LOG_WINDOW_MENUITEM = wxNewId();
long const ID_SHOW_EVENT_TICKER_MENUITEM = wxNewId();
long const ID_SHOW_PROBE_PANEL_MENUITEM = wxNewId();
long const ID_SHOW_STATUS_TEXT_MENUITEM = wxNewId();
long const ID_SHOW_EXTENDED_STATUS_TEXT_MENUITEM = wxNewId();
long const ID_FULL_SCREEN_MENUITEM = wxNewId();
long const ID_NORMAL_SCREEN_MENUITEM = wxNewId();
long const ID_MUTE_MENUITEM = wxNewId();

long const ID_HELP_MENUITEM = wxNewId();
long const ID_ABOUT_MENUITEM = wxNewId();
long const ID_CHECK_FOR_UPDATES_MENUITEM = wxNewId();
long const ID_DONATE_MENUITEM = wxNewId();
long const ID_OPEN_HOME_PAGE_MENUITEM = wxNewId();
long const ID_OPEN_DOWNLOAD_PAGE_MENUITEM = wxNewId();

long const ID_POSTIINITIALIZE_TIMER = wxNewId();
long const ID_GAME_TIMER = wxNewId();
long const ID_LOW_FREQUENCY_TIMER = wxNewId();
long const ID_CHECK_UPDATES_TIMER = wxNewId();

MainFrame::MainFrame(wxApp * mainApp)
    : mMainApp(mainApp)
    , mResourceLocator(new ResourceLocator())
    , mGameController()
    , mSoundController()
    , mMusicController()
    , mToolController()
    , mSettingsManager()
    , mUIPreferencesManager()
    , mHasWindowBeenShown(false)
    , mHasStartupTipBeenChecked(false)
    , mPauseCount(0)
    , mCurrentShipTitles()
    , mCurrentRCBombCount(0u)
    , mCurrentAntiMatterBombCount(0u)
    , mIsShiftKeyDown(false)
    , mIsMouseCapturedByGLCanvas(false)
{
    Create(
        nullptr,
        wxID_ANY,
        std::string(APPLICATION_NAME_WITH_SHORT_VERSION),
        wxDefaultPosition,
        wxDefaultSize,
        wxDEFAULT_FRAME_STYLE | wxMAXIMIZE,
        _T("Main Frame"));

    SetIcon(wxICON(BBB_SHIP_ICON));
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    Maximize();
    Centre();

    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnMainFrameClose, this);

    mMainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);
    mMainPanel->Bind(wxEVT_CHAR_HOOK, &MainFrame::OnMainPanelKeyDown, this); // Just for arrow keys
    mMainPanelSizer = new wxBoxSizer(wxVERTICAL);



    //
    // Build main GL canvas and activate GL context
    //

    // Note: Using the wxWidgets 3.1 style does not work on OpenGL 4 drivers; it forces a 1.1.0 context

    int mainGLCanvasAttributes[] =
    {
        WX_GL_RGBA,
        WX_GL_DOUBLEBUFFER,
        WX_GL_DEPTH_SIZE,      16,

        // Cannot specify CORE_PROFILE or else wx tries OpenGL 3.0 and fails if it's not supported
        //WX_GL_CORE_PROFILE,

        // Useless to specify version as Glad will always take the max
        //WX_GL_MAJOR_VERSION,    GameOpenGL::MinOpenGLVersionMaj,
        //WX_GL_MINOR_VERSION,    GameOpenGL::MinOpenGLVersionMin,

        0, 0
    };

    mMainGLCanvas = std::make_unique<wxGLCanvas>(
        mMainPanel,
        ID_MAIN_CANVAS,
        mainGLCanvasAttributes,
        wxDefaultPosition,
        wxDefaultSize,
        0L,
        _T("Main GL Canvas"));

    mMainGLCanvas->Connect(wxEVT_SIZE, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasResize, 0, this);
    mMainGLCanvas->Connect(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasLeftDown, 0, this);
    mMainGLCanvas->Connect(wxEVT_LEFT_UP, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasLeftUp, 0, this);
    mMainGLCanvas->Connect(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasRightDown, 0, this);
    mMainGLCanvas->Connect(wxEVT_RIGHT_UP, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasRightUp, 0, this);
    mMainGLCanvas->Connect(wxEVT_MOTION, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasMouseMove, 0, this);
    mMainGLCanvas->Connect(wxEVT_MOUSEWHEEL, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasMouseWheel, 0, this);
    mMainGLCanvas->Connect(wxEVT_MOUSE_CAPTURE_LOST, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasCaptureMouseLost, 0, this);

    mMainPanelSizer->Add(
        mMainGLCanvas.get(),
        1,                  // Occupy all available vertical space
        wxEXPAND,           // Expand also horizontally
        0);                 // Border

    // Take context for this canvas
    mMainGLCanvasContext = std::make_unique<wxGLContext>(mMainGLCanvas.get());

    // Activate context
    mMainGLCanvasContext->SetCurrent(*mMainGLCanvas);


    //
    // Initialize OpenGL
    //

    try
    {
        GameOpenGL::InitOpenGL();
    }
    catch (std::exception const & e)
    {
        throw std::runtime_error("Error during OpenGL initialization: " + std::string(e.what()));
    }


    //
    // Build menu
    //

    wxMenuBar * mainMenuBar = new wxMenuBar();


    // File

    wxMenu * fileMenu = new wxMenu();

    wxMenuItem * loadShipMenuItem = new wxMenuItem(fileMenu, ID_LOAD_SHIP_MENUITEM, _("Load Ship\tCtrl+O"), wxEmptyString, wxITEM_NORMAL);
    fileMenu->Append(loadShipMenuItem);
    Connect(ID_LOAD_SHIP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnLoadShipMenuItemSelected);

    wxMenuItem * reloadLastShipMenuItem = new wxMenuItem(fileMenu, ID_RELOAD_LAST_SHIP_MENUITEM, _("Reload Ship\tCtrl+R"), wxEmptyString, wxITEM_NORMAL);
    fileMenu->Append(reloadLastShipMenuItem);
    Connect(ID_RELOAD_LAST_SHIP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnReloadLastShipMenuItemSelected);

    fileMenu->Append(new wxMenuItem(fileMenu, wxID_SEPARATOR));

    wxMenuItem * reloadShipsMenuItem = new wxMenuItem(fileMenu, ID_MORE_SHIPS_MENUITEM, _("Get More Ships..."));
    fileMenu->Append(reloadShipsMenuItem);
    fileMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [](wxCommandEvent &) { wxLaunchDefaultBrowser("https://floatingsandbox.com/ship-packs/"); }, ID_MORE_SHIPS_MENUITEM);

    fileMenu->Append(new wxMenuItem(fileMenu, wxID_SEPARATOR));

    wxMenuItem * saveScreenshotMenuItem = new wxMenuItem(fileMenu, ID_SAVE_SCREENSHOT_MENUITEM, _("Save Screenshot\tCtrl+C"), wxEmptyString, wxITEM_NORMAL);
    fileMenu->Append(saveScreenshotMenuItem);
    Connect(ID_SAVE_SCREENSHOT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSaveScreenshotMenuItemSelected);

    fileMenu->Append(new wxMenuItem(fileMenu, wxID_SEPARATOR));

    wxMenuItem* quitMenuItem = new wxMenuItem(fileMenu, ID_QUIT_MENUITEM, _("Quit\tAlt-F4"), _("Quit the application"), wxITEM_NORMAL);
    fileMenu->Append(quitMenuItem);
    Connect(ID_QUIT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnQuit);

    mainMenuBar->Append(fileMenu, _("&File"));


    // Controls

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

    mStepMenuItem = new wxMenuItem(controlsMenu, ID_STEP_MENUITEM, _("Step\tEnter"), _("Step one frame at a time"), wxITEM_NORMAL);
    mStepMenuItem->Enable(false);
    controlsMenu->Append(mStepMenuItem);
    Connect(ID_STEP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnStepMenuItemSelected);

    controlsMenu->Append(new wxMenuItem(controlsMenu, wxID_SEPARATOR));

    wxMenuItem * resetViewMenuItem = new wxMenuItem(controlsMenu, ID_RESET_VIEW_MENUITEM, _("Reset View\tHOME"), wxEmptyString, wxITEM_NORMAL);
    controlsMenu->Append(resetViewMenuItem);
    Connect(ID_RESET_VIEW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnResetViewMenuItemSelected);

    mainMenuBar->Append(controlsMenu, _("Controls"));


    // Tools

    mToolsMenu = new wxMenu();

    wxMenuItem * moveMenuItem = new wxMenuItem(mToolsMenu, ID_MOVE_MENUITEM, _("Move/Rotate\tM"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(moveMenuItem);
    Connect(ID_MOVE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnMoveMenuItemSelected);

    wxMenuItem * moveAllMenuItem = new wxMenuItem(mToolsMenu, ID_MOVE_ALL_MENUITEM, _("Move All/Rotate All\tALT+M"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(moveAllMenuItem);
    Connect(ID_MOVE_ALL_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnMoveAllMenuItemSelected);

    wxMenuItem * pickAndPullMenuItem = new wxMenuItem(mToolsMenu, ID_PICK_AND_PULL_MENUITEM, _("Pick-n-Pull\tK"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(pickAndPullMenuItem);
    Connect(ID_PICK_AND_PULL_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnPickAndPullMenuItemSelected);

    wxMenuItem * smashMenuItem = new wxMenuItem(mToolsMenu, ID_SMASH_MENUITEM, _("Smash\tS"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(smashMenuItem);
    Connect(ID_SMASH_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSmashMenuItemSelected);

    wxMenuItem * sliceMenuItem = new wxMenuItem(mToolsMenu, ID_SLICE_MENUITEM, _("Slice\tL"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(sliceMenuItem);
    Connect(ID_SLICE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSliceMenuItemSelected);

    wxMenuItem * heatBlasterMenuItem = new wxMenuItem(mToolsMenu, ID_HEAT_BLASTER_MENUITEM, _("HeatBlaster/CoolBlaster\tH"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(heatBlasterMenuItem);
    Connect(ID_HEAT_BLASTER_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnHeatBlasterMenuItemSelected);

    wxMenuItem * fireExtinguisherMenuItem = new wxMenuItem(mToolsMenu, ID_FIRE_EXTINGUISHER_MENUITEM, _("Fire Extinguisher\tX"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(fireExtinguisherMenuItem);
    Connect(ID_FIRE_EXTINGUISHER_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnFireExtinguisherMenuItemSelected);

    wxMenuItem * grabMenuItem = new wxMenuItem(mToolsMenu, ID_GRAB_MENUITEM, _("Attract/Repel\tG"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(grabMenuItem);
    Connect(ID_GRAB_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnGrabMenuItemSelected);

    wxMenuItem * swirlMenuItem = new wxMenuItem(mToolsMenu, ID_SWIRL_MENUITEM, _("Swirl/Counterswirl\tW"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(swirlMenuItem);
    Connect(ID_SWIRL_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSwirlMenuItemSelected);

    wxMenuItem * pinMenuItem = new wxMenuItem(mToolsMenu, ID_PIN_MENUITEM, _("Toggle Pin\tP"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(pinMenuItem);
    Connect(ID_PIN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnPinMenuItemSelected);

    /* Easter Egg
    wxMenuItem * injectAirBubblesMenuItem = new wxMenuItem(mToolsMenu, ID_INJECT_AIR_BUBBLES_MENUITEM, _("Inject Air Bubbles\tB"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(injectAirBubblesMenuItem);
    Connect(ID_INJECT_AIR_BUBBLES_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnInjectAirBubblesMenuItemSelected);
    */

    wxMenuItem * floodHoseMenuItem = new wxMenuItem(mToolsMenu, ID_FLOOD_HOSE_MENUITEM, _("Flood/Dry\tF"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(floodHoseMenuItem);
    Connect(ID_FLOOD_HOSE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnFloodHoseMenuItemSelected);

    wxMenuItem * timerBombMenuItem = new wxMenuItem(mToolsMenu, ID_TIMERBOMB_MENUITEM, _("Toggle Timer Bomb\tT"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(timerBombMenuItem);
    Connect(ID_TIMERBOMB_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnTimerBombMenuItemSelected);

    wxMenuItem * rcBombMenuItem = new wxMenuItem(mToolsMenu, ID_RCBOMB_MENUITEM, _("Toggle RC Bomb\tR"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(rcBombMenuItem);
    Connect(ID_RCBOMB_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnRCBombMenuItemSelected);

    wxMenuItem * impactBombMenuItem = new wxMenuItem(mToolsMenu, ID_IMPACTBOMB_MENUITEM, _("Toggle Impact Bomb\tI"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(impactBombMenuItem);
    Connect(ID_IMPACTBOMB_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnImpactBombMenuItemSelected);

    wxMenuItem * antiMatterBombMenuItem = new wxMenuItem(mToolsMenu, ID_ANTIMATTERBOMB_MENUITEM, _("Toggle Anti-Matter Bomb\tA"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(antiMatterBombMenuItem);
    Connect(ID_ANTIMATTERBOMB_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAntiMatterBombMenuItemSelected);

    wxMenuItem * thanosSnapMenuItem = new wxMenuItem(mToolsMenu, ID_THANOSSNAP_MENUITEM, _("Thanos' Snap\tQ"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(thanosSnapMenuItem);
    Connect(ID_THANOSSNAP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnThanosSnapMenuItemSelected);

    wxMenuItem * waveMakerMenuItem = new wxMenuItem(mToolsMenu, ID_WAVEMAKER_MENUITEM, _("WaveMaker\tV"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(waveMakerMenuItem);
    Connect(ID_WAVEMAKER_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnWaveMakerMenuItemSelected);

    wxMenuItem * adjustTerrainMenuItem = new wxMenuItem(mToolsMenu, ID_ADJUSTTERRAIN_MENUITEM, _("Adjust Terrain\tJ"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(adjustTerrainMenuItem);
    Connect(ID_ADJUSTTERRAIN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAdjustTerrainMenuItemSelected);

    wxMenuItem * repairStructureMenuItem = new wxMenuItem(mToolsMenu, ID_REPAIRSTRUCTURE_MENUITEM, _("Repair Structure\tE"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(repairStructureMenuItem);
    Connect(ID_REPAIRSTRUCTURE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnRepairStructureMenuItemSelected);

    wxMenuItem * scrubMenuItem = new wxMenuItem(mToolsMenu, ID_SCRUB_MENUITEM, _("Scrub\tU"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(scrubMenuItem);
    Connect(ID_SCRUB_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnScrubMenuItemSelected);

    mToolsMenu->Append(new wxMenuItem(mToolsMenu, wxID_SEPARATOR));

    mRCBombsDetonateMenuItem = new wxMenuItem(mToolsMenu, ID_RCBOMBDETONATE_MENUITEM, _("Detonate RC Bombs\tD"), wxEmptyString, wxITEM_NORMAL);
    mToolsMenu->Append(mRCBombsDetonateMenuItem);
    Connect(ID_RCBOMBDETONATE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnRCBombDetonateMenuItemSelected);
    mRCBombsDetonateMenuItem->Enable(false);

    mAntiMatterBombsDetonateMenuItem = new wxMenuItem(mToolsMenu, ID_ANTIMATTERBOMBDETONATE_MENUITEM, _("Detonate Anti-Matter Bombs\tN"), wxEmptyString, wxITEM_NORMAL);
    mToolsMenu->Append(mAntiMatterBombsDetonateMenuItem);
    Connect(ID_ANTIMATTERBOMBDETONATE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAntiMatterBombDetonateMenuItemSelected);
    mAntiMatterBombsDetonateMenuItem->Enable(false);

    wxMenuItem * triggerTsunamiMenuItem = new wxMenuItem(mToolsMenu, ID_TRIGGERTSUNAMI_MENUITEM, _("Trigger Tsunami"), wxEmptyString, wxITEM_NORMAL);
    mToolsMenu->Append(triggerTsunamiMenuItem);
    Connect(ID_TRIGGERTSUNAMI_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnTriggerTsunamiMenuItemSelected);

    wxMenuItem * triggerRogueWaveMenuItem = new wxMenuItem(mToolsMenu, ID_TRIGGERROGUEWAVE_MENUITEM, _("Trigger Rogue Wave"), wxEmptyString, wxITEM_NORMAL);
    mToolsMenu->Append(triggerRogueWaveMenuItem);
    Connect(ID_TRIGGERROGUEWAVE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnTriggerRogueWaveMenuItemSelected);

    mTriggerStormMenuItem = new wxMenuItem(mToolsMenu, ID_TRIGGERSTORM_MENUITEM, _("Trigger Storm"), wxEmptyString, wxITEM_NORMAL);
    mToolsMenu->Append(mTriggerStormMenuItem);
    Connect(ID_TRIGGERSTORM_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)& MainFrame::OnTriggerStormMenuItemSelected);
    mTriggerStormMenuItem->Enable(true);

    wxMenuItem * triggerLightningMenuItem = new wxMenuItem(mToolsMenu, ID_TRIGGERLIGHTNING_MENUITEM, _("Trigger Lightning\tALT+L"), wxEmptyString, wxITEM_NORMAL);
    mToolsMenu->Append(triggerLightningMenuItem);
    Connect(ID_TRIGGERLIGHTNING_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnTriggerLightningMenuItemSelected);

    mainMenuBar->Append(mToolsMenu, _("Tools"));


    // Options

    wxMenu * optionsMenu = new wxMenu();

    wxMenuItem * openSettingsWindowMenuItem = new wxMenuItem(optionsMenu, ID_OPEN_SETTINGS_WINDOW_MENUITEM, _("Simulation Settings...\tCtrl+S"), wxEmptyString, wxITEM_NORMAL);
    optionsMenu->Append(openSettingsWindowMenuItem);
    Connect(ID_OPEN_SETTINGS_WINDOW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnOpenSettingsWindowMenuItemSelected);

    mReloadLastModifiedSettingsMenuItem = new wxMenuItem(optionsMenu, ID_RELOAD_LAST_MODIFIED_SETTINGS_MENUITEM, _("Reload Last-Modified Simulation Settings\tCtrl+D"), wxEmptyString, wxITEM_NORMAL);
    optionsMenu->Append(mReloadLastModifiedSettingsMenuItem);
    Connect(ID_RELOAD_LAST_MODIFIED_SETTINGS_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)& MainFrame::OnReloadLastModifiedSettingsMenuItem);

    wxMenuItem * openPreferencesWindowMenuItem = new wxMenuItem(optionsMenu, ID_OPEN_PREFERENCES_WINDOW_MENUITEM, _("Game Preferences...\tCtrl+F"), wxEmptyString, wxITEM_NORMAL);
    optionsMenu->Append(openPreferencesWindowMenuItem);
    Connect(ID_OPEN_PREFERENCES_WINDOW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnOpenPreferencesWindowMenuItemSelected);

    optionsMenu->Append(new wxMenuItem(optionsMenu, wxID_SEPARATOR));

    wxMenuItem * openLogWindowMenuItem = new wxMenuItem(optionsMenu, ID_OPEN_LOG_WINDOW_MENUITEM, _("Open Log Window\tCtrl+L"), wxEmptyString, wxITEM_NORMAL);
    optionsMenu->Append(openLogWindowMenuItem);
    Connect(ID_OPEN_LOG_WINDOW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnOpenLogWindowMenuItemSelected);

    mShowEventTickerMenuItem = new wxMenuItem(optionsMenu, ID_SHOW_EVENT_TICKER_MENUITEM, _("Show Event Ticker\tCtrl+E"), wxEmptyString, wxITEM_CHECK);
    optionsMenu->Append(mShowEventTickerMenuItem);
    mShowEventTickerMenuItem->Check(false);
    Connect(ID_SHOW_EVENT_TICKER_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShowEventTickerMenuItemSelected);

    mShowProbePanelMenuItem = new wxMenuItem(optionsMenu, ID_SHOW_PROBE_PANEL_MENUITEM, _("Show Probe Panel\tCtrl+P"), wxEmptyString, wxITEM_CHECK);
    optionsMenu->Append(mShowProbePanelMenuItem);
    mShowProbePanelMenuItem->Check(false);
    Connect(ID_SHOW_PROBE_PANEL_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShowProbePanelMenuItemSelected);

    mShowStatusTextMenuItem = new wxMenuItem(optionsMenu, ID_SHOW_STATUS_TEXT_MENUITEM, _("Show Status Text\tCtrl+T"), wxEmptyString, wxITEM_CHECK);
    optionsMenu->Append(mShowStatusTextMenuItem);
    Connect(ID_SHOW_STATUS_TEXT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShowStatusTextMenuItemSelected);

    mShowExtendedStatusTextMenuItem = new wxMenuItem(optionsMenu, ID_SHOW_EXTENDED_STATUS_TEXT_MENUITEM, _("Show Extended Status Text\tCtrl+X"), wxEmptyString, wxITEM_CHECK);
    optionsMenu->Append(mShowExtendedStatusTextMenuItem);
    Connect(ID_SHOW_EXTENDED_STATUS_TEXT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShowExtendedStatusTextMenuItemSelected);

    optionsMenu->Append(new wxMenuItem(optionsMenu, wxID_SEPARATOR));

    mFullScreenMenuItem = new wxMenuItem(optionsMenu, ID_FULL_SCREEN_MENUITEM, _("Full Screen\tF11"), wxEmptyString, wxITEM_NORMAL);
    optionsMenu->Append(mFullScreenMenuItem);
    Connect(ID_FULL_SCREEN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnFullScreenMenuItemSelected);
    mFullScreenMenuItem->Enable(!StartInFullScreenMode);

    mNormalScreenMenuItem = new wxMenuItem(optionsMenu, ID_NORMAL_SCREEN_MENUITEM, _("Normal Screen\tESC"), wxEmptyString, wxITEM_NORMAL);
    optionsMenu->Append(mNormalScreenMenuItem);
    Connect(ID_NORMAL_SCREEN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnNormalScreenMenuItemSelected);
    mNormalScreenMenuItem->Enable(StartInFullScreenMode);

    optionsMenu->Append(new wxMenuItem(optionsMenu, wxID_SEPARATOR));

    mMuteMenuItem = new wxMenuItem(optionsMenu, ID_MUTE_MENUITEM, _("Mute\tCtrl+M"), wxEmptyString, wxITEM_CHECK);
    optionsMenu->Append(mMuteMenuItem);
    mMuteMenuItem->Check(false);
    Connect(ID_MUTE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnMuteMenuItemSelected);

    mainMenuBar->Append(optionsMenu, _("Options"));


    // Help

    wxMenu * helpMenu = new wxMenu();

    wxMenuItem * helpMenuItem = new wxMenuItem(helpMenu, ID_HELP_MENUITEM, _("Help\tF1"), _("Get help about the simulator"), wxITEM_NORMAL);
    helpMenu->Append(helpMenuItem);
    Connect(ID_HELP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnHelpMenuItemSelected);

    wxMenuItem * aboutMenuItem = new wxMenuItem(helpMenu, ID_ABOUT_MENUITEM, _("About\tF2"), _("Show credits and other I'vedunnit stuff"), wxITEM_NORMAL);
    helpMenu->Append(aboutMenuItem);
    Connect(ID_ABOUT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAboutMenuItemSelected);

    helpMenu->Append(new wxMenuItem(helpMenu, wxID_SEPARATOR));

    wxMenuItem * checkForUpdatesMenuItem = new wxMenuItem(helpMenu, ID_CHECK_FOR_UPDATES_MENUITEM, _("Check for Updates..."), wxEmptyString, wxITEM_NORMAL);
    helpMenu->Append(checkForUpdatesMenuItem);
    Connect(ID_CHECK_FOR_UPDATES_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnCheckForUpdatesMenuItemSelected);

    wxMenuItem * donateMenuItem = new wxMenuItem(helpMenu, ID_DONATE_MENUITEM, _("Donate..."));
    helpMenu->Append(donateMenuItem);
    helpMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [](wxCommandEvent &) { wxLaunchDefaultBrowser("https://floatingsandbox.com/donate/"); }, ID_DONATE_MENUITEM);

    helpMenu->Append(new wxMenuItem(helpMenu, wxID_SEPARATOR));

    wxMenuItem * openHomePageMenuItem = new wxMenuItem(helpMenu, ID_OPEN_HOME_PAGE_MENUITEM, _("Open Home Page"));
    helpMenu->Append(openHomePageMenuItem);
    helpMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [](wxCommandEvent &) { wxLaunchDefaultBrowser("https://floatingsandbox.com"); }, ID_OPEN_HOME_PAGE_MENUITEM);

    wxMenuItem * openDownloadPageMenuItem = new wxMenuItem(helpMenu, ID_OPEN_DOWNLOAD_PAGE_MENUITEM, _("Open Download Page"));
    helpMenu->Append(openDownloadPageMenuItem);
    helpMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [](wxCommandEvent &) { wxLaunchDefaultBrowser("https://gamejolt.com/games/floating-sandbox/353572"); }, ID_OPEN_DOWNLOAD_PAGE_MENUITEM);

    mainMenuBar->Append(helpMenu, _("Help"));

    SetMenuBar(mainMenuBar);



    //
    // Probe panel
    //

    mProbePanel = std::make_unique<ProbePanel>(mMainPanel);

    mMainPanelSizer->Add(mProbePanel.get(), 0, wxEXPAND); // Expand horizontally

    mMainPanelSizer->Hide(mProbePanel.get());



    //
    // Event ticker panel
    //

    mEventTickerPanel = std::make_unique<EventTickerPanel>(mMainPanel);

    mMainPanelSizer->Add(mEventTickerPanel.get(), 0, wxEXPAND); // Expand horizontally

    mMainPanelSizer->Hide(mEventTickerPanel.get());



    //
    // Finalize frame
    //

    mMainPanel->SetSizer(mMainPanelSizer);
    mMainPanel->Layout();



    //
    // Initialize tooltips
    //

    wxToolTip::Enable(true);
    wxToolTip::SetDelay(200);



    //
    // Initialize timers
    //

    mCheckUpdatesTimer = std::make_unique<wxTimer>(this, ID_CHECK_UPDATES_TIMER);
    Connect(ID_CHECK_UPDATES_TIMER, wxEVT_TIMER, (wxObjectEventFunction)&MainFrame::OnCheckUpdatesTimerTrigger);


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

bool MainFrame::ProcessKeyDown(
    int keyCode,
    int keyModifiers)
{
    if (keyCode == WXK_LEFT)
    {
        if (!!mGameController && !!mUIPreferencesManager)
        {
            // Left
            mGameController->Pan(vec2f(-mUIPreferencesManager->GetPanIncrement(), 0.0f));
            return true;
        }
    }
    else if (keyCode == WXK_UP)
    {
        if (!!mGameController && !!mUIPreferencesManager)
        {
            // Up
            mGameController->Pan(vec2f(00.0f, -mUIPreferencesManager->GetPanIncrement()));
            return true;
        }
    }
    else if (keyCode == WXK_RIGHT)
    {
        if (!!mGameController && !!mUIPreferencesManager)
        {
            // Right
            mGameController->Pan(vec2f(mUIPreferencesManager->GetPanIncrement(), 0.0f));
            return true;
        }
    }
    else if (keyCode == WXK_DOWN)
    {
        if (!!mGameController && !!mUIPreferencesManager)
        {
            // Down
            mGameController->Pan(vec2f(0.0f, mUIPreferencesManager->GetPanIncrement()));
            return true;
        }
    }
    else if (keyCode == 'B')
    {
        // Air Bubbles tool

        assert(!!mToolController);
        mToolController->SetTool(ToolType::InjectAirBubbles);

        // Note: at this moment the current menu item is still selected, so re-selecting it has no effect; there's no way
        // around this, but this is an Easter Egg after all....

        return true;
    }
    else if (keyCode == '/')
    {
        if (!!mGameController && !!mToolController)
        {
            // Query

            vec2f screenCoords = mToolController->GetMouseScreenCoordinates();
            vec2f worldCoords = mGameController->ScreenToWorld(screenCoords);

            LogMessage("@ ", worldCoords.toString(), ":");

            mGameController->QueryNearestPointAt(screenCoords);

            return true;
        }
    }
    else
    {
        // Deliver to electric panel
        if (!!mElectricalPanel)
        {
            if (mElectricalPanel->ProcessKeyDown(
                keyCode,
                keyModifiers))
            {
                return true;
            }
        }
    }

    // Allow it to be handled
    return false;
}

bool MainFrame::ProcessKeyUp(
    int keyCode,
    int keyModifiers)
{
    // Deliver to electric panel
    if (!!mElectricalPanel)
    {
        if (mElectricalPanel->ProcessKeyUp(
            keyCode,
            keyModifiers))
        {
            return true;
        }
    }

    // Allow it to be handled
    return false;
}

void MainFrame::OnSecretTypingOpenDebugWindow()
{
    LogMessage("MainFrame::OnSecretTypingOpenDebugWindow");
}

void MainFrame::OnSecretTypingLoadFallbackShip()
{
    ResetState();
    mGameController->ResetAndLoadFallbackShip(*mResourceLocator);
}


//
// App event handlers
//

void MainFrame::OnPostInitializeTrigger(wxTimerEvent & /*event*/)
{
    auto const postInitializeStartTimestamp = std::chrono::steady_clock::now();

    //
    // Create splash screen
    //

    std::unique_ptr<SplashScreenDialog> splash = std::make_unique<SplashScreenDialog>(*mResourceLocator);

#ifdef _DEBUG
    // The guy is pesky while debugging
    splash->Hide();
#endif

    mMainApp->Yield();


    //
    // Create Game Controller
    //

    try
    {
        mGameController = GameController::Create(
            ImageSize(
                mMainGLCanvas->GetSize().GetWidth(),
                mMainGLCanvas->GetSize().GetHeight()),
            [this]()
            {
                assert(!!mMainGLCanvas);
                mMainGLCanvas->SwapBuffers();
            },
            *mResourceLocator,
            [&splash, this](float progress, std::string const & message)
            {
                // 0.0 -> 0.5
                splash->UpdateProgress(progress / 2.0f, message);
                this->mMainApp->Yield();
                this->mMainApp->Yield();
                this->mMainApp->Yield();
            });
    }
    catch (std::exception const & e)
    {
        splash.reset();

        OnError("Error during initialization of game controller: " + std::string(e.what()), true);

        return;
    }

    this->mMainApp->Yield();


    //
    // Create Sound Controller
    //

    try
    {
        mSoundController = std::make_shared<SoundController>(
            *mResourceLocator,
            [&splash, this](float progress, std::string const & message)
            {
                // 0.5 -> 0.66
                splash->UpdateProgress(0.5f + progress / 6.0f, message);
                this->mMainApp->Yield();
                this->mMainApp->Yield();
                this->mMainApp->Yield();
            });
    }
    catch (std::exception const & e)
    {
        splash.reset();

        OnError("Error during initialization of sound controller: " + std::string(e.what()), true);

        return;
    }

    this->mMainApp->Yield();


    //
    // Create Music Controller
    //

    try
    {
        mMusicController = std::make_shared<MusicController>(
            *mResourceLocator,
            [&splash, this](float progress, std::string const & message)
            {
                // 0.66 -> 0.83
                splash->UpdateProgress(0.666f + progress / 6.0f, message);
                this->mMainApp->Yield();
                this->mMainApp->Yield();
                this->mMainApp->Yield();
            });
    }
    catch (std::exception const & e)
    {
        splash.reset();

        OnError("Error during initialization of music controller: " + std::string(e.what()), true);

        return;
    }

    this->mMainApp->Yield();


    //
    // Create Settings Manager
    //

    mSettingsManager = std::make_shared<SettingsManager>(
        mGameController,
        mSoundController,
        mResourceLocator->GetThemeSettingsRootFilepath(),
        StandardSystemPaths::GetInstance().GetUserGameSettingsRootFolderPath());

    // Enable "Reload Last Modified Settings" menu if we have last-modified settings
    mReloadLastModifiedSettingsMenuItem->Enable(mSettingsManager->HasLastModifiedSettingsPersisted());


    //
    // Create UI Preferences Manager
    //

    mUIPreferencesManager = std::make_shared<UIPreferencesManager>(
        mGameController,
        mMusicController,
        *mResourceLocator);

    ReconcileWithUIPreferences();


    //
    // Create Electrical Panel
    //

    mElectricalPanel = SwitchboardPanel::Create(
        mMainPanel,
        [this]()
        {
            // Layout
            mMainPanel->Layout();
        },
        mGameController,
        mSoundController,
        mUIPreferencesManager,
        *mResourceLocator,
        [&splash, this](float progress, std::string const & message)
        {
            // 0.83 -> 1.0
            splash->UpdateProgress(0.83f + progress / 6.0f, message);
            this->mMainApp->Yield();
            this->mMainApp->Yield();
            this->mMainApp->Yield();
        });

    mMainPanelSizer->Add(mElectricalPanel.get(), 0, wxEXPAND); // Expand horizontally


    //
    // Create Tool Controller
    //

    // Set initial tool
    ToolType initialToolType = ToolType::Smash;
    mToolsMenu->Check(ID_SMASH_MENUITEM, true);

    try
    {
        mToolController = std::make_unique<ToolController>(
            initialToolType,
            mGameController->GetEffectiveAmbientLightIntensity(),
            mMainGLCanvas.get(),
            mGameController,
            mSoundController,
            *mResourceLocator);
    }
    catch (std::exception const & e)
    {
        splash.reset();

        OnError("Error during initialization of tool controller: " + std::string(e.what()), true);

        return;
    }

    this->mMainApp->Yield();


    //
    // Register game event handlers
    //

    this->RegisterEventHandler(*mGameController);
    mProbePanel->RegisterEventHandler(*mGameController);
    mEventTickerPanel->RegisterEventHandler(*mGameController);
    mElectricalPanel->RegisterEventHandler(*mGameController);
    mSoundController->RegisterEventHandler(*mGameController);
    mMusicController->RegisterEventHandler(*mGameController);
    mToolController->RegisterEventHandler(*mGameController);


    //
    // Load initial ship
    //

    try
    {
        // Try default ship first
        mGameController->AddDefaultShip(*mResourceLocator);
    }
    catch (std::exception const & exc)
    {
        LogMessage("Error locating default ship: ", exc.what());

        // Try fallback ship now
        mGameController->AddFallbackShip(*mResourceLocator);
    }

    splash->UpdateProgress(1.0f, "Ready!");

    this->mMainApp->Yield();

    //
    // Start check update timer
    //

    if (mUIPreferencesManager->GetCheckUpdatesAtStartup())
    {
        // 10 seconds
        mCheckUpdatesTimer->Start(10000, true);
    }


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
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        end = std::chrono::steady_clock::now();
    }

#endif

    splash->Close();


    //
    // Finalize frame
    //

    UpdateFrameTitle();

    // Set focus on canvas, so it starts getting mouse events
    mMainGLCanvas->SetFocus();

    // Log post-initialize duration
    auto const postInitializeEndTimestamp = std::chrono::steady_clock::now();
    auto const elapsed = std::chrono::duration_cast<std::chrono::duration<float>>(postInitializeEndTimestamp - postInitializeStartTimestamp);
    LogMessage("Post-Initialize took ", elapsed.count(), "s");


    //
    // Start timers
    //


    //
    // Start game timer
    //

    mGameTimer = std::make_unique<wxTimer>(this, ID_GAME_TIMER);
    Connect(ID_GAME_TIMER, wxEVT_TIMER, (wxObjectEventFunction)&MainFrame::OnGameTimerTrigger);

    PostGameStepTimer();


    //
    // Start low-frequency timer
    //

    mLowFrequencyTimer = std::make_unique<wxTimer>(this, ID_LOW_FREQUENCY_TIMER);
    Connect(ID_LOW_FREQUENCY_TIMER, wxEVT_TIMER, (wxObjectEventFunction)&MainFrame::OnLowFrequencyTimerTrigger);

    StartLowFrequencyTimer();
}

void MainFrame::OnMainFrameClose(wxCloseEvent & /*event*/)
{
    if (!!mGameTimer)
        mGameTimer->Stop();

    if (!!mLowFrequencyTimer)
        mLowFrequencyTimer->Stop();

    // Save last-modified settings, if enabled
    if (!!mUIPreferencesManager && mUIPreferencesManager->GetSaveSettingsOnExit())
        if (!!mSettingsManager)
            mSettingsManager->SaveLastModifiedSettings();

    Destroy();
}

void MainFrame::OnQuit(wxCommandEvent & /*event*/)
{
    // Flush log
    try
    {
        std::filesystem::path const diagnosticsFolderPath = StandardSystemPaths::GetInstance().GetDiagnosticsFolderPath(true);
        Logger::Instance.FlushToFile(diagnosticsFolderPath, "last_run");
    }
    catch (...)
    { /* ignore */ }

    // Close frame
    Close();
}

void MainFrame::OnMainPanelKeyDown(wxKeyEvent & event)
{
    if (!ProcessKeyDown(event.GetKeyCode(), event.GetModifiers()))
        event.Skip();
}

void MainFrame::OnGameTimerTrigger(wxTimerEvent & /*event*/)
{
    if (!!mUpdateChecker)
    {
        // We are checking for updates...
        // ...check whether the check has completed
        auto outcome = mUpdateChecker->GetOutcome();
        if (!!outcome)
        {
            // Check completed...
            // ...check if it's an interesting new version
            if (outcome->OutcomeType == UpdateChecker::UpdateCheckOutcomeType::HasVersion
                && *(outcome->LatestVersion) > Version::CurrentVersion()
                && !mUIPreferencesManager->IsUpdateBlacklisted(*(outcome->LatestVersion)))
            {
                //
                // Notify user of new version
                //

                NewVersionDisplayDialog dlg(
                    this,
                    *(outcome->LatestVersion),
                    outcome->Features,
                    mUIPreferencesManager.get());

                dlg.ShowModal();
            }

            // Forget about the update check
            mUpdateChecker.reset();
        }
    }

    if (mHasStartupTipBeenChecked)
    {
        // We've already checked the startup tip...
        // ...so we can safely post the timer event for the next
        // frame - the earlier the better (mind the 15ms minimum on MSW)
        PostGameStepTimer();
    }

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


    //
    // Run a game step
    //

    try
    {
        // Update tool controller
        assert(!!mToolController);
        mToolController->UpdateSimulation();

        // Update game - will also render
        assert(!!mGameController);
        mGameController->RunGameIteration();

        // Update probe panel
        assert(!!mProbePanel);
        mProbePanel->UpdateSimulation();

        // Update event ticker
        assert(!!mEventTickerPanel);
        mEventTickerPanel->UpdateSimulation();

        // Update electrical panel
        assert(!!mElectricalPanel);
        mElectricalPanel->UpdateSimulation();

        // Update sound controller
        assert(!!mSoundController);
        mSoundController->UpdateSimulation();

        // Update music controller
        assert(!!mMusicController);
        mMusicController->UpdateSimulation();

        // Do after-render chores
        AfterGameRender();
    }
    catch (std::exception const & e)
    {
        OnError("Error during game step: " + std::string(e.what()), true);

        return;
    }

    if (!mHasStartupTipBeenChecked)
    {
        // Show startup tip - unless user has decided not to
        if (mUIPreferencesManager->GetShowStartupTip())
        {
            StartupTipDialog startupTipDialog(
                this,
                mUIPreferencesManager,
                *mResourceLocator);

            startupTipDialog.ShowModal();
        }

        // Post next game step now
        PostGameStepTimer();

        // Don't check for startup tips anymore
        mHasStartupTipBeenChecked = true;
    }
}

void MainFrame::OnLowFrequencyTimerTrigger(wxTimerEvent & /*event*/)
{
    //
    // Update game controller
    //

    assert(!!mGameController);
    mGameController->LowFrequencyUpdate();


    //
    // Update sound controller
    //

    assert(!!mSoundController);
    mSoundController->LowFrequencyUpdateSimulation();


    //
    // Update music controller
    //

    assert(!!mMusicController);
    mMusicController->LowFrequencyUpdateSimulation();
}

void MainFrame::OnCheckUpdatesTimerTrigger(wxTimerEvent & /*event*/)
{
    mUpdateChecker = std::make_unique<UpdateChecker>();
}

void MainFrame::OnIdle(wxIdleEvent & /*event*/)
{
}

//
// Main canvas event handlers
//

void MainFrame::OnMainGLCanvasResize(wxSizeEvent & event)
{
    LogMessage("OnMainGLCanvasResize: ", event.GetSize().GetX(), "x", event.GetSize().GetY());

    if (!!mGameController
        && event.GetSize().GetX() > 0
        && event.GetSize().GetY() > 0)
    {
        mGameController->SetCanvasSize(
            event.GetSize().GetX(),
            event.GetSize().GetY());
    }

	event.Skip();
}

void MainFrame::OnMainGLCanvasLeftDown(wxMouseEvent & /*event*/)
{
    // First of all, set focus on the canvas if it has lost it - we want
    // it to receive all mouse events
    if (!mMainGLCanvas->HasFocus())
    {
        mMainGLCanvas->SetFocus();
    }

    // Tell tool controller
    assert(!!mToolController);
    mToolController->OnLeftMouseDown();

    // Hang on to the mouse for as long as the button is pressed
    if (!mIsMouseCapturedByGLCanvas)
    {
        mMainGLCanvas->CaptureMouse();
        mIsMouseCapturedByGLCanvas = true;
    }
}

void MainFrame::OnMainGLCanvasLeftUp(wxMouseEvent & /*event*/)
{
    // We can now release the mouse
    if (mIsMouseCapturedByGLCanvas)
    {
        mMainGLCanvas->ReleaseMouse();
        mIsMouseCapturedByGLCanvas = false;
    }

    assert(!!mToolController);
    mToolController->OnLeftMouseUp();
}

void MainFrame::OnMainGLCanvasRightDown(wxMouseEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->OnRightMouseDown();

    // Hang on to the mouse for as long as the button is pressed
    if (!mIsMouseCapturedByGLCanvas)
    {
        mMainGLCanvas->CaptureMouse();
        mIsMouseCapturedByGLCanvas = true;
    }
}

void MainFrame::OnMainGLCanvasRightUp(wxMouseEvent & /*event*/)
{
    // We can now release the mouse
    if (mIsMouseCapturedByGLCanvas)
    {
        mMainGLCanvas->ReleaseMouse();
        mIsMouseCapturedByGLCanvas = false;
    }

    assert(!!mToolController);
    mToolController->OnRightMouseUp();
}

void MainFrame::OnMainGLCanvasMouseMove(wxMouseEvent & event)
{
    assert(!!mToolController);
    mToolController->OnMouseMove(event.GetX(), event.GetY());
}

void MainFrame::OnMainGLCanvasMouseWheel(wxMouseEvent & event)
{
    assert(!!mGameController);
    mGameController->AdjustZoom(powf(1.002f, event.GetWheelRotation()));
}

void MainFrame::OnMainGLCanvasCaptureMouseLost(wxMouseCaptureLostEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->UnsetTool();
}

//
// Menu event handlers
//

void MainFrame::OnLoadShipMenuItemSelected(wxCommandEvent & /*event*/)
{
    LogMessage("MainFrame::OnLoadShipMenuItemSelected: Enter");

    SetPaused(true);

    // See if we need to create the ShipLoad dialog
    if (!mShipLoadDialog)
    {
        mShipLoadDialog = std::make_unique<ShipLoadDialog>(
            this,
            mUIPreferencesManager,
            *mResourceLocator);
    }

    // Open dialog
    auto res = mShipLoadDialog->ShowModal();

    // Process result
    if (res == wxID_OK)
    {
        //
        // Load ship
        //

        ResetState();

        assert(!!mGameController);
        try
        {
            auto shipMetadata = mGameController->ResetAndLoadShip(mShipLoadDialog->GetChosenShipFilepath());

            // Open description, if a description exists and the user allows
            if (!!shipMetadata.Description
                && mUIPreferencesManager->GetShowShipDescriptionsAtShipLoad())
            {
                ShipDescriptionDialog shipDescriptionDialog(
                    this,
                    shipMetadata,
                    true,
                    mUIPreferencesManager);

                shipDescriptionDialog.ShowModal();
            }
        }
        catch (std::exception const & ex)
        {
            OnError(ex.what(), false);
        }
    }

    SetPaused(false);
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
        OnError(ex.what(), false);
    }
}

void MainFrame::OnSaveScreenshotMenuItemSelected(wxCommandEvent & /*event*/)
{
    //
    // Fire snapshot sound
    //

    assert(!!mSoundController);
    mSoundController->PlaySnapshotSound();


    //
    // Take screenshot
    //

    assert(!!mGameController);
    auto screenshotImage = mGameController->TakeScreenshot();


    //
    // Ensure pictures folder exists
    //

    assert(!!mUIPreferencesManager);
    auto const folderPath = mUIPreferencesManager->GetScreenshotsFolderPath();

    if (!std::filesystem::exists(folderPath))
    {
        try
        {
            std::filesystem::create_directories(folderPath);
        }
        catch (std::filesystem::filesystem_error const & fex)
        {
            OnError(
                std::string("Could not save screenshot to path \"") + folderPath.string() + "\": " + fex.what(),
                false);

            return;
        }
    }

    //
    // Choose filename
    //

    std::filesystem::path screenshotFilePath;

    std::string shipName = mCurrentShipTitles.empty()
        ? "NoShip"
        : mCurrentShipTitles.back();

    do
    {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto const tm = std::localtime(&now_time_t);

        std::stringstream ssFilename;
        ssFilename.fill('0');
        ssFilename
            << std::setw(4) << (1900 + tm->tm_year) << std::setw(2) << (1 + tm->tm_mon) << std::setw(2) << tm->tm_mday
            << "_"
            << std::setw(2) << tm->tm_hour << std::setw(2) << tm->tm_min << std::setw(2) << tm->tm_sec
            << "_"
            << std::setw(3) << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch() % std::chrono::seconds(1)).count()
            << "_"
            << shipName
            << ".png";

        screenshotFilePath = folderPath / ssFilename.str();

    } while (std::filesystem::exists(screenshotFilePath));


    //
    // Save screenshot
    //

    try
    {
        ImageFileTools::SaveImage(
            screenshotFilePath,
            screenshotImage);
    }
    catch (std::filesystem::filesystem_error const & fex)
    {
        OnError(
            std::string("Could not save screenshot to file \"") + screenshotFilePath.string() + "\": " + fex.what(),
            false);
    }
}

void MainFrame::OnPauseMenuItemSelected(wxCommandEvent & /*event*/)
{
    SetPaused(mPauseMenuItem->IsChecked());
}

void MainFrame::OnStepMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);

    mGameController->PulseUpdate();
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
    assert(!!mUIPreferencesManager);
    mGameController->AdjustZoom(mUIPreferencesManager->GetZoomIncrement());
}

void MainFrame::OnZoomOutMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    assert(!!mUIPreferencesManager);
    assert(mUIPreferencesManager->GetZoomIncrement() > 0.0f);
    mGameController->AdjustZoom(1.0f / mUIPreferencesManager->GetZoomIncrement());
}

void MainFrame::OnAmbientLightUpMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    float newAmbientLight = std::min(1.0f, mGameController->GetAmbientLightIntensity() * 1.02f);
    mGameController->SetAmbientLightIntensity(newAmbientLight);
}

void MainFrame::OnAmbientLightDownMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    float newAmbientLight = mGameController->GetAmbientLightIntensity() / 1.02f;
    mGameController->SetAmbientLightIntensity(newAmbientLight);
}

void MainFrame::OnMoveMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::Move);
}

void MainFrame::OnMoveAllMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::MoveAll);
}

void MainFrame::OnPickAndPullMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::PickAndPull);
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

void MainFrame::OnHeatBlasterMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::HeatBlaster);
}

void MainFrame::OnFireExtinguisherMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::FireExtinguisher);
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

void MainFrame::OnInjectAirBubblesMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::InjectAirBubbles);
}

void MainFrame::OnFloodHoseMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::FloodHose);
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

void MainFrame::OnImpactBombMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::ImpactBomb);
}

void MainFrame::OnAntiMatterBombDetonateMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->DetonateAntiMatterBombs();
}

void MainFrame::OnWaveMakerMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::WaveMaker);
}

void MainFrame::OnAdjustTerrainMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::TerrainAdjust);
}

void MainFrame::OnRepairStructureMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::RepairStructure);
}

void MainFrame::OnScrubMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::Scrub);
}

void MainFrame::OnRCBombDetonateMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->DetonateRCBombs();
}

void MainFrame::OnAntiMatterBombMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::AntiMatterBomb);
}

void MainFrame::OnThanosSnapMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::ThanosSnap);
}

void MainFrame::OnTriggerTsunamiMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->TriggerTsunami();
}

void MainFrame::OnTriggerRogueWaveMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->TriggerRogueWave();
}

void MainFrame::OnTriggerStormMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->TriggerStorm();
}

void MainFrame::OnTriggerLightningMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->TriggerLightning();
}

//////////

void MainFrame::OnOpenSettingsWindowMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mSettingsDialog)
    {
        mSettingsDialog = std::make_unique<SettingsDialog>(
            this,
            mSettingsManager,
            mGameController,
            *mResourceLocator);
    }

    mSettingsDialog->Open();
}

void MainFrame::OnReloadLastModifiedSettingsMenuItem(wxCommandEvent & /*event*/)
{
    // Load last-modified settings
    bool hasLoadedSettings = false;
    try
    {
        assert(!!mSettingsManager);
        hasLoadedSettings = mSettingsManager->EnforceDefaultsAndLastModifiedSettings();
    }
    catch (std::exception const & exc)
    {
        OnError("Could not load last-modified settings: " + std::string(exc.what()), false);

        // Disable menu item
        mReloadLastModifiedSettingsMenuItem->Enable(false);
    }

    // Display notification
    if (hasLoadedSettings)
    {
        assert(!!mGameController);
        mGameController->DisplaySettingsLoadedNotification();
    }
}

void MainFrame::OnOpenPreferencesWindowMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mPreferencesDialog)
    {
        mPreferencesDialog = std::make_unique<PreferencesDialog>(
            this,
            mUIPreferencesManager,
            [this]()
            {
                this->ReconcileWithUIPreferences();
            });
    }

    mPreferencesDialog->Open();
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
        mMainPanelSizer->Show(mEventTickerPanel.get());
    }
    else
    {
        mMainPanelSizer->Hide(mEventTickerPanel.get());
    }

    mMainPanelSizer->Layout();
}

void MainFrame::OnShowProbePanelMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mProbePanel);

    if (mShowProbePanelMenuItem->IsChecked())
    {
        mMainPanelSizer->Show(mProbePanel.get());
    }
    else
    {
        mMainPanelSizer->Hide(mProbePanel.get());
    }

    mMainPanelSizer->Layout();
}

void MainFrame::OnShowStatusTextMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetShowStatusText(mShowStatusTextMenuItem->IsChecked());
}

void MainFrame::OnShowExtendedStatusTextMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetShowExtendedStatusText(mShowExtendedStatusTextMenuItem->IsChecked());
}

void MainFrame::OnFullScreenMenuItemSelected(wxCommandEvent & /*event*/)
{
    mFullScreenMenuItem->Enable(false);
    mNormalScreenMenuItem->Enable(true);

    this->ShowFullScreen(true, wxFULLSCREEN_NOBORDER);
}

void MainFrame::OnNormalScreenMenuItemSelected(wxCommandEvent & /*event*/)
{
    mFullScreenMenuItem->Enable(true);
    mNormalScreenMenuItem->Enable(false);

    this->ShowFullScreen(false);
}

void MainFrame::OnMuteMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetGlobalMute(mMuteMenuItem->IsChecked());
}

void MainFrame::OnHelpMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mHelpDialog)
    {
        mHelpDialog = std::make_unique<HelpDialog>(
            this,
            *mResourceLocator);
    }

    mHelpDialog->ShowModal();
}

void MainFrame::OnAboutMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mAboutDialog)
    {
        mAboutDialog = std::make_unique<AboutDialog>(
            this,
            *mResourceLocator);
    }

    mAboutDialog->Open();
}

void MainFrame::OnCheckForUpdatesMenuItemSelected(wxCommandEvent & /*event*/)
{
    CheckForUpdatesDialog checkDlg(this);
    auto ret = checkDlg.ShowModal();
    if (ret == wxID_OK)
    {
        assert(!!checkDlg.GetHasVersionOutcome());

        //
        // Notify user of new version
        //

        NewVersionDisplayDialog newVersionDlg(
            this,
            *(checkDlg.GetHasVersionOutcome()->LatestVersion),
            checkDlg.GetHasVersionOutcome()->Features,
            nullptr);

        newVersionDlg.ShowModal();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void MainFrame::ResetState()
{
    assert(!!mSoundController);
    mSoundController->Reset();

    assert(!!mMusicController);
    mMusicController->Reset();

    mRCBombsDetonateMenuItem->Enable(false);
    mAntiMatterBombsDetonateMenuItem->Enable(false);
    mTriggerStormMenuItem->Enable(true);
}

void MainFrame::UpdateFrameTitle()
{
    //
    // Build title
    //

    std::ostringstream ss;

    ss.fill('0');

    ss << std::string(APPLICATION_NAME_WITH_SHORT_VERSION);

    if (!mCurrentShipTitles.empty())
    {
        ss << " - "
            << Utils::Join(mCurrentShipTitles, " + ");
    }

    SetTitle(ss.str());
}

void MainFrame::OnError(
    std::string const & message,
    bool die)
{
    //
    // Stop timers first
    //

    if (!!mGameTimer)
    {
        mGameTimer->Stop();
    }

    if (!!mLowFrequencyTimer)
    {
        mLowFrequencyTimer->Stop();
    }


    //
    // Show message
    //

    wxMessageBox(message, wxT("Maritime Disaster"), wxICON_ERROR);

    if (die)
    {
        //
        // Exit
        //

        this->Destroy();
    }
    else
    {
        // Restart game

        if (!!mGameTimer)
        {
            PostGameStepTimer();
        }

        if (!!mLowFrequencyTimer)
        {
            StartLowFrequencyTimer();
        }
    }
}

void MainFrame::PostGameStepTimer()
{
    assert(!!mGameTimer);

    // On Windows the timer resolution is 15.something ms,
    // so we use a delay of zero to shoot for a maximum of ~64 frames/sec
    mGameTimer->Start(0, true);
}

void MainFrame::StartLowFrequencyTimer()
{
    assert(!!mLowFrequencyTimer);

    mLowFrequencyTimer->Start(1000, false);
}

void MainFrame::SetPaused(bool isPaused)
{
    if (isPaused)
    {
        if (0 == mPauseCount)
        {
            // Set pause

            if (!!mGameController)
                mGameController->SetPaused(true);

            if (!!mSoundController)
                mSoundController->SetPaused(true);

            if (!!mMusicController)
                mMusicController->SetPaused(true);

            mStepMenuItem->Enable(true);
        }

        ++mPauseCount;
    }
    else
    {
        assert(mPauseCount > 0);
        --mPauseCount;

        if (0 == mPauseCount)
        {
            // Resume

            if (!!mGameController)
                mGameController->SetPaused(false);

            if (!!mSoundController)
                mSoundController->SetPaused(false);

            if (!!mMusicController)
                mMusicController->SetPaused(false);

            mStepMenuItem->Enable(false);
        }
    }
}

void MainFrame::ReconcileWithUIPreferences()
{
    mShowStatusTextMenuItem->Check(mUIPreferencesManager->GetShowStatusText());
    mShowExtendedStatusTextMenuItem->Check(mUIPreferencesManager->GetShowExtendedStatusText());
    mMuteMenuItem->Check(mUIPreferencesManager->GetGlobalMute());
}