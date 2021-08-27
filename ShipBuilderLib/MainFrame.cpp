/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-08-27
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MainFrame.h"

#ifdef _MSC_VER
 // Nothing to do here - we use RC files
#else
#include "Resources/ShipBBB.xpm"
#endif

namespace ShipBuilder {

MainFrame::MainFrame(
    wxApp * mainApp,
    ResourceLocator const & resourceLocator,
    LocalizationManager & localizationManager)
    : mMainApp(mainApp)
    , mResourceLocator(resourceLocator)
    , mLocalizationManager(localizationManager)
{
    /*
    Create(
        nullptr,
        wxID_ANY,
        std::string(APPLICATION_NAME_WITH_SHORT_VERSION),
        wxDefaultPosition,
        wxDefaultSize,
        wxDEFAULT_FRAME_STYLE | wxMAXIMIZE,
        wxS("Main Frame"));

    SetIcon(wxICON(BBB_SHIP_ICON));
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    Maximize();
    Centre();

    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnMainFrameClose, this);

    // We hook chars to get arrow keys; can't do it with global event filter as that would intercept presses in dialogs,
    // while this one kicks off for leftovers of dialogs
    mMainPanel = new UnFocusablePanel(this, wxWANTS_CHARS);
    mMainPanel->Bind(wxEVT_CHAR_HOOK, &MainFrame::OnMainPanelKeyDown, this);

    mMainPanelSizer = new wxBoxSizer(wxVERTICAL);


    //
    // Build OpenGL canvas - this is where we render the game to
    //

    mMainGLCanvas = std::make_unique<GLCanvas>(
        mMainPanel,
        ID_MAIN_CANVAS);

    mMainGLCanvas->Connect(wxEVT_SHOW, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasShow, 0, this);
    mMainGLCanvas->Connect(wxEVT_PAINT, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasPaint, 0, this);
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


    //
    // Build menu
    //

    {
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

        wxMenu * fileMenu = new wxMenu();

        wxMenuItem * loadShipMenuItem = new wxMenuItem(fileMenu, ID_LOAD_SHIP_MENUITEM, _("Load Ship...") + wxS("\tCtrl+O"), wxEmptyString, wxITEM_NORMAL);
        fileMenu->Append(loadShipMenuItem);
        Connect(ID_LOAD_SHIP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnLoadShipMenuItemSelected);

        wxMenuItem * reloadCurrentShipMenuItem = new wxMenuItem(fileMenu, ID_RELOAD_CURRENT_SHIP_MENUITEM, _("Reload Current Ship") + wxS("\tCtrl+R"), wxEmptyString, wxITEM_NORMAL);
        fileMenu->Append(reloadCurrentShipMenuItem);
        Connect(ID_RELOAD_CURRENT_SHIP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnReloadCurrentShipMenuItemSelected);

        mReloadPreviousShipMenuItem = new wxMenuItem(fileMenu, ID_RELOAD_PREVIOUS_SHIP_MENUITEM, _("Reload Previous Ship") + wxS("\tCtrl+V"), wxEmptyString, wxITEM_NORMAL);
        fileMenu->Append(mReloadPreviousShipMenuItem);
        Connect(ID_RELOAD_PREVIOUS_SHIP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnReloadPreviousShipMenuItemSelected);
        mReloadPreviousShipMenuItem->Enable(false);

        fileMenu->Append(new wxMenuItem(fileMenu, wxID_SEPARATOR));

        wxMenuItem * reloadShipsMenuItem = new wxMenuItem(fileMenu, ID_MORE_SHIPS_MENUITEM, _("Get More Ships..."));
        fileMenu->Append(reloadShipsMenuItem);
        fileMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [](wxCommandEvent &) { wxLaunchDefaultBrowser("https://floatingsandbox.com/ship-packs/"); }, ID_MORE_SHIPS_MENUITEM);

        fileMenu->Append(new wxMenuItem(fileMenu, wxID_SEPARATOR));

        wxMenuItem * saveScreenshotMenuItem = new wxMenuItem(fileMenu, ID_SAVE_SCREENSHOT_MENUITEM, _("Save Screenshot") + wxS("\tCtrl+C"), wxEmptyString, wxITEM_NORMAL);
        fileMenu->Append(saveScreenshotMenuItem);
        Connect(ID_SAVE_SCREENSHOT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSaveScreenshotMenuItemSelected);

        fileMenu->Append(new wxMenuItem(fileMenu, wxID_SEPARATOR));

        wxMenuItem * quitMenuItem = new wxMenuItem(fileMenu, ID_QUIT_MENUITEM, _("Quit") + wxS("\tAlt-F4"), _("Quit the game"), wxITEM_NORMAL);
        fileMenu->Append(quitMenuItem);
        Connect(ID_QUIT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnQuit);

        mainMenuBar->Append(fileMenu, _("&File"));


        // Controls

        wxMenu * controlsMenu = new wxMenu();

        wxMenuItem * zoomInMenuItem = new wxMenuItem(controlsMenu, ID_ZOOM_IN_MENUITEM, _("Zoom In") + wxS("\t+"), wxEmptyString, wxITEM_NORMAL);
        controlsMenu->Append(zoomInMenuItem);
        Connect(ID_ZOOM_IN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnZoomInMenuItemSelected);
        ADD_PLAIN_ACCELERATOR_KEY('+', zoomInMenuItem)
        ADD_PLAIN_ACCELERATOR_KEY(WXK_NUMPAD_ADD, zoomInMenuItem)

        wxMenuItem * zoomOutMenuItem = new wxMenuItem(controlsMenu, ID_ZOOM_OUT_MENUITEM, _("Zoom Out") + wxS("\t-"), wxEmptyString, wxITEM_NORMAL);
        controlsMenu->Append(zoomOutMenuItem);
        Connect(ID_ZOOM_OUT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnZoomOutMenuItemSelected);
        ADD_PLAIN_ACCELERATOR_KEY('-', zoomOutMenuItem)
        ADD_PLAIN_ACCELERATOR_KEY(WXK_NUMPAD_SUBTRACT, zoomOutMenuItem)

        wxMenuItem * amblientLightUpMenuItem = new wxMenuItem(controlsMenu, ID_AMBIENT_LIGHT_UP_MENUITEM, _("Bright Ambient Light") + wxS("\tPgUp"), wxEmptyString, wxITEM_NORMAL);
        controlsMenu->Append(amblientLightUpMenuItem);
        Connect(ID_AMBIENT_LIGHT_UP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAmbientLightUpMenuItemSelected);

        wxMenuItem * ambientLightDownMenuItem = new wxMenuItem(controlsMenu, ID_AMBIENT_LIGHT_DOWN_MENUITEM, _("Dim Ambient Light") + wxS("\tPgDn"), wxEmptyString, wxITEM_NORMAL);
        controlsMenu->Append(ambientLightDownMenuItem);
        Connect(ID_AMBIENT_LIGHT_DOWN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAmbientLightDownMenuItemSelected);

        mPauseMenuItem = new wxMenuItem(controlsMenu, ID_PAUSE_MENUITEM, _("Pause") + wxS("\tSpace"), _("Pause the game"), wxITEM_CHECK);
        controlsMenu->Append(mPauseMenuItem);
        Connect(ID_PAUSE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnPauseMenuItemSelected);
        mPauseMenuItem->Check(false);
        ADD_PLAIN_ACCELERATOR_KEY(WXK_SPACE, mPauseMenuItem)

        mStepMenuItem = new wxMenuItem(controlsMenu, ID_STEP_MENUITEM, _("Step") + wxS("\tEnter"), _("Step one frame at a time"), wxITEM_NORMAL);
        controlsMenu->Append(mStepMenuItem);
        Connect(ID_STEP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnStepMenuItemSelected);
        mStepMenuItem->Enable(false);

        controlsMenu->Append(new wxMenuItem(controlsMenu, wxID_SEPARATOR));

        wxMenuItem * resetViewMenuItem = new wxMenuItem(controlsMenu, ID_RESET_VIEW_MENUITEM, _("Reset View") + wxS("\tHOME"), wxEmptyString, wxITEM_NORMAL);
        controlsMenu->Append(resetViewMenuItem);
        Connect(ID_RESET_VIEW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnResetViewMenuItemSelected);
        ADD_PLAIN_ACCELERATOR_KEY(WXK_HOME, resetViewMenuItem)

        mainMenuBar->Append(controlsMenu, _("&Controls"));


        // Tools

        mToolsMenu = new wxMenu();

        {

#ifdef __WXMSW__
#define SET_BITMAP(img) \
        auto img2 = WxHelpers::RetintCursorImage(img, rgbColor(0x00, 0x90, 0x00));\
        toolMenuItem->SetBitmaps(wxBitmap(img2), wxBitmap(img));
#else
#define SET_BITMAP(img) \
        toolMenuItem->SetBitmap(wxBitmap(img));
#endif

#define ADD_TOOL_MENUITEM(label, shortcut, bitmap_path, handler) \
        [&]()\
        {\
            auto const id = wxNewId();\
            wxMenuItem * toolMenuItem = new wxMenuItem(mToolsMenu, (id), (label) + (shortcut), wxEmptyString, wxITEM_RADIO);\
            auto img1 = wxImage(\
                resourceLocator.GetCursorFilePath((bitmap_path)).string(),\
                wxBITMAP_TYPE_PNG);\
            img1.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);\
            SET_BITMAP(img1)\
            mToolsMenu->Append(toolMenuItem);\
            Connect((id), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::handler);\
            return toolMenuItem;\
        }();

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Move/Rotate"), wxS("\tM"), "move_cursor_up", OnMoveMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('M', menuItem);
            }

            {
                ADD_TOOL_MENUITEM(_("Move All/Rotate All"), wxS("\tALT+M"), "move_all_cursor_up", OnMoveAllMenuItemSelected);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Pick-n-Pull"), wxS("\tK"), "pliers_cursor_up", OnPickAndPullMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('K', menuItem);
            }

            {
                mSmashMenuItem = ADD_TOOL_MENUITEM(_("Smash"), wxS("\tS"), "smash_cursor_up", OnSmashMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('S', mSmashMenuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Slice"), wxS("\tL"), "chainsaw_cursor_up", OnSliceMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('L', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("HeatBlaster/CoolBlaster"), wxS("\tH"), "heat_blaster_heat_cursor_up", OnHeatBlasterMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('H', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Fire Extinguisher"), wxS("\tX"), "fire_extinguisher_cursor_up", OnFireExtinguisherMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('X', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Blast"), wxS("\t8"), "blast_cursor_up_1", OnBlastToolMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('8', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Electric Spark"), wxS("\t7"), "electric_spark_cursor_up", OnElectricSparkToolMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('7', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Attract/Repel"), wxS("\tG"), "drag_cursor_up_plus", OnGrabMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('G', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Swirl/Counterswirl"), wxS("\tW"), "swirl_cursor_up_cw", OnSwirlMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('W', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Toggle Pin"), wxS("\tP"), "pin_cursor", OnPinMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('P', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Inject/Remove Pressure"), wxS("\tB"), "air_tank_cursor_up", OnInjectPressureMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('B', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Flood/Dry"), wxS("\tF"), "flood_cursor_up", OnFloodHoseMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('F', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Toggle Timer Bomb"), wxS("\tT"), "timer_bomb_cursor", OnTimerBombMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('T', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Toggle RC Bomb"), wxS("\tR"), "rc_bomb_cursor", OnRCBombMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('R', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Toggle Impact Bomb"), wxS("\tI"), "impact_bomb_cursor", OnImpactBombMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('I', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Toggle Anti-Matter Bomb"), wxS("\tA"), "am_bomb_cursor", OnAntiMatterBombMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('A', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Thanos' Snap"), wxS("\tQ"), "thanos_snap_cursor_up", OnThanosSnapMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('Q', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("WaveMaker"), wxS("\tV"), "wave_maker_cursor_up", OnWaveMakerMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('V', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Adjust Terrain"), wxS("\tJ"), "terrain_adjust_cursor_up", OnAdjustTerrainMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('J', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Repair"), wxS("\tE"), "repair_structure_cursor_up", OnRepairStructureMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('E', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Scrub/Rot"), wxS("\tU"), "scrub_cursor_up", OnScrubMenuItemSelected);
                ADD_PLAIN_ACCELERATOR_KEY('U', menuItem);
            }

            {
                mScareFishMenuItem = ADD_TOOL_MENUITEM(_("Scare/Allure Fishes"), wxS("\tZ"), "megaphone_cursor_up", OnScareFishMenuItemSelected);
                mScareFishMenuItem->Enable(false);
                ADD_PLAIN_ACCELERATOR_KEY('Z', mScareFishMenuItem);
            }

            ADD_TOOL_MENUITEM(_("Toggle Physics Probe"), wxS(""), "physics_probe_cursor", OnPhysicsProbeMenuItemSelected);
        }

        mToolsMenu->Append(new wxMenuItem(mToolsMenu, wxID_SEPARATOR));

        {
            mRCBombsDetonateMenuItem = new wxMenuItem(mToolsMenu, ID_RCBOMBDETONATE_MENUITEM, _("Detonate RC Bombs") + wxS("\tD"), wxEmptyString, wxITEM_NORMAL);
            mToolsMenu->Append(mRCBombsDetonateMenuItem);
            Connect(ID_RCBOMBDETONATE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnRCBombDetonateMenuItemSelected);
            mRCBombsDetonateMenuItem->Enable(false);
            ADD_PLAIN_ACCELERATOR_KEY('D', mRCBombsDetonateMenuItem)
        }

        {
            mAntiMatterBombsDetonateMenuItem = new wxMenuItem(mToolsMenu, ID_ANTIMATTERBOMBDETONATE_MENUITEM, _("Detonate Anti-Matter Bombs") + wxS("\tN"), wxEmptyString, wxITEM_NORMAL);
            mToolsMenu->Append(mAntiMatterBombsDetonateMenuItem);
            Connect(ID_ANTIMATTERBOMBDETONATE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAntiMatterBombDetonateMenuItemSelected);
            mAntiMatterBombsDetonateMenuItem->Enable(false);
            ADD_PLAIN_ACCELERATOR_KEY('N', mAntiMatterBombsDetonateMenuItem)
        }

        wxMenuItem * triggerTsunamiMenuItem = new wxMenuItem(mToolsMenu, ID_TRIGGERTSUNAMI_MENUITEM, _("Trigger Tsunami"), wxEmptyString, wxITEM_NORMAL);
        mToolsMenu->Append(triggerTsunamiMenuItem);
        Connect(ID_TRIGGERTSUNAMI_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnTriggerTsunamiMenuItemSelected);

        wxMenuItem * triggerRogueWaveMenuItem = new wxMenuItem(mToolsMenu, ID_TRIGGERROGUEWAVE_MENUITEM, _("Trigger Rogue Wave"), wxEmptyString, wxITEM_NORMAL);
        mToolsMenu->Append(triggerRogueWaveMenuItem);
        Connect(ID_TRIGGERROGUEWAVE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnTriggerRogueWaveMenuItemSelected);

        mTriggerStormMenuItem = new wxMenuItem(mToolsMenu, ID_TRIGGERSTORM_MENUITEM, _("Trigger Storm"), wxEmptyString, wxITEM_NORMAL);
        mToolsMenu->Append(mTriggerStormMenuItem);
        Connect(ID_TRIGGERSTORM_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnTriggerStormMenuItemSelected);
        mTriggerStormMenuItem->Enable(true);

        wxMenuItem * triggerLightningMenuItem = new wxMenuItem(mToolsMenu, ID_TRIGGERLIGHTNING_MENUITEM, _("Trigger Lightning") + wxS("\tALT+L"), wxEmptyString, wxITEM_NORMAL);
        mToolsMenu->Append(triggerLightningMenuItem);
        Connect(ID_TRIGGERLIGHTNING_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnTriggerLightningMenuItemSelected);


        mainMenuBar->Append(mToolsMenu, _("&Tools"));


        // Options

        wxMenu * optionsMenu = new wxMenu();

        wxMenuItem * openSettingsWindowMenuItem = new wxMenuItem(optionsMenu, ID_OPEN_SETTINGS_WINDOW_MENUITEM, _("Simulation Settings...") + wxS("\tCtrl+S"), wxEmptyString, wxITEM_NORMAL);
        optionsMenu->Append(openSettingsWindowMenuItem);
        Connect(ID_OPEN_SETTINGS_WINDOW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnOpenSettingsWindowMenuItemSelected);

        mReloadLastModifiedSettingsMenuItem = new wxMenuItem(optionsMenu, ID_RELOAD_LAST_MODIFIED_SETTINGS_MENUITEM, _("Reload Last-Modified Simulation Settings") + wxS("\tCtrl+D"), wxEmptyString, wxITEM_NORMAL);
        optionsMenu->Append(mReloadLastModifiedSettingsMenuItem);
        Connect(ID_RELOAD_LAST_MODIFIED_SETTINGS_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnReloadLastModifiedSettingsMenuItem);

        wxMenuItem * openPreferencesWindowMenuItem = new wxMenuItem(optionsMenu, ID_OPEN_PREFERENCES_WINDOW_MENUITEM, _("Game Preferences...") + wxS("\tCtrl+F"), wxEmptyString, wxITEM_NORMAL);
        optionsMenu->Append(openPreferencesWindowMenuItem);
        Connect(ID_OPEN_PREFERENCES_WINDOW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnOpenPreferencesWindowMenuItemSelected);

        optionsMenu->Append(new wxMenuItem(optionsMenu, wxID_SEPARATOR));

        wxMenuItem * openLogWindowMenuItem = new wxMenuItem(optionsMenu, ID_OPEN_LOG_WINDOW_MENUITEM, _("Open Log Window") + wxS("\tCtrl+L"), wxEmptyString, wxITEM_NORMAL);
        optionsMenu->Append(openLogWindowMenuItem);
        Connect(ID_OPEN_LOG_WINDOW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnOpenLogWindowMenuItemSelected);

        mShowEventTickerMenuItem = new wxMenuItem(optionsMenu, ID_SHOW_EVENT_TICKER_MENUITEM, _("Show Event Ticker") + wxS("\tCtrl+E"), wxEmptyString, wxITEM_CHECK);
        optionsMenu->Append(mShowEventTickerMenuItem);
        Connect(ID_SHOW_EVENT_TICKER_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShowEventTickerMenuItemSelected);
        mShowEventTickerMenuItem->Check(false);

        mShowProbePanelMenuItem = new wxMenuItem(optionsMenu, ID_SHOW_PROBE_PANEL_MENUITEM, _("Show Probe Panel") + wxS("\tCtrl+P"), wxEmptyString, wxITEM_CHECK);
        optionsMenu->Append(mShowProbePanelMenuItem);
        Connect(ID_SHOW_PROBE_PANEL_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShowProbePanelMenuItemSelected);
        mShowProbePanelMenuItem->Check(false);

        mShowStatusTextMenuItem = new wxMenuItem(optionsMenu, ID_SHOW_STATUS_TEXT_MENUITEM, _("Show Status Text") + wxS("\tCtrl+T"), wxEmptyString, wxITEM_CHECK);
        optionsMenu->Append(mShowStatusTextMenuItem);
        Connect(ID_SHOW_STATUS_TEXT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShowStatusTextMenuItemSelected);

        mShowExtendedStatusTextMenuItem = new wxMenuItem(optionsMenu, ID_SHOW_EXTENDED_STATUS_TEXT_MENUITEM, _("Show Extended Status Text") + wxS("\tCtrl+X"), wxEmptyString, wxITEM_CHECK);
        optionsMenu->Append(mShowExtendedStatusTextMenuItem);
        Connect(ID_SHOW_EXTENDED_STATUS_TEXT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShowExtendedStatusTextMenuItemSelected);

        optionsMenu->Append(new wxMenuItem(optionsMenu, wxID_SEPARATOR));

        mFullScreenMenuItem = new wxMenuItem(optionsMenu, ID_FULL_SCREEN_MENUITEM, _("Full Screen") + wxS("\tF11"), wxEmptyString, wxITEM_NORMAL);
        optionsMenu->Append(mFullScreenMenuItem);
        Connect(ID_FULL_SCREEN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnFullScreenMenuItemSelected);
        mFullScreenMenuItem->Enable(!StartInFullScreenMode);

        mNormalScreenMenuItem = new wxMenuItem(optionsMenu, ID_NORMAL_SCREEN_MENUITEM, _("Normal Screen") + wxS("\tESC"), wxEmptyString, wxITEM_NORMAL);
        optionsMenu->Append(mNormalScreenMenuItem);
        Connect(ID_NORMAL_SCREEN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnNormalScreenMenuItemSelected);
        mNormalScreenMenuItem->Enable(StartInFullScreenMode);

        optionsMenu->Append(new wxMenuItem(optionsMenu, wxID_SEPARATOR));

        mMuteMenuItem = new wxMenuItem(optionsMenu, ID_MUTE_MENUITEM, _("Mute") + wxS("\tCtrl+M"), wxEmptyString, wxITEM_CHECK);
        optionsMenu->Append(mMuteMenuItem);
        Connect(ID_MUTE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnMuteMenuItemSelected);
        mMuteMenuItem->Check(false);

        mainMenuBar->Append(optionsMenu, _("&Options"));


        // Help

        wxMenu * helpMenu = new wxMenu();

        wxMenuItem * helpMenuItem = new wxMenuItem(helpMenu, ID_HELP_MENUITEM, _("Guide") + wxS("\tF1"), _("Get help about the simulator"), wxITEM_NORMAL);
        helpMenu->Append(helpMenuItem);
        Connect(ID_HELP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnHelpMenuItemSelected);

        wxMenuItem * aboutMenuItem = new wxMenuItem(helpMenu, ID_ABOUT_MENUITEM, _("About and Credits") + wxS("\tF2"), _("Show credits and other I'vedunnit stuff"), wxITEM_NORMAL);
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

        mainMenuBar->Append(helpMenu, _("&Help"));

        SetMenuBar(mainMenuBar);

#ifdef __WXGTK__
        // Set accelerator
        wxAcceleratorTable accel(acceleratorEntries.size(), acceleratorEntries.data());
        SetAcceleratorTable(accel);
#endif
    }


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
    mPostInitializeTimer->Start(1, true);
    */
}

}