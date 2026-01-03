/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MainFrame.h"

#include "UI/AboutDialog.h"
#include "UI/BootSettingsDialog.h"
#include "UI/CheckForUpdatesDialog.h"
#include "UI/NewVersionDisplayDialog.h"
#include "UI/StartupTipDialog.h"

#include <UILib/StandardSystemPaths.h>
#include <UILib/ShipDescriptionDialog.h>
#include <UILib/WxHelpers.h>

#include <Game/GameVersion.h>

#include <Core/GameException.h>
#include <Core/Log.h>

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

ToolType constexpr InitialNonNpcToolType = ToolType::Smash;

long const ID_MAIN_CANVAS = wxNewId();

long const ID_LOAD_SHIP_MENUITEM = wxNewId();
long const ID_RELOAD_CURRENT_SHIP_MENUITEM = wxNewId();
long const ID_RELOAD_PREVIOUS_SHIP_MENUITEM = wxNewId();
long const ID_MORE_SHIPS_MENUITEM = wxNewId();
long const ID_SAVE_SCREENSHOT_MENUITEM = wxNewId();
long const ID_OPEN_SCREENSHOT_FOLDER_MENUITEM = wxNewId();
long const ID_QUIT_MENUITEM = wxNewId();

long const ID_ZOOM_IN_MENUITEM = wxNewId();
long const ID_ZOOM_OUT_MENUITEM = wxNewId();
long const ID_AUTO_FOCUS_AT_SHIP_LOAD_MENUITEM = wxNewId();
long const ID_AUTO_FOCUS_ON_SHIP_MENUITEM = wxNewId();
long const ID_AUTO_FOCUS_ON_SELECTED_NPC_MENUITEM = wxNewId();
long const ID_AUTO_FOCUS_ON_NOTHING_MENUITEM = wxNewId();
long const ID_RESET_VIEW_MENUITEM = wxNewId();
long const ID_CHANGE_SHIP_VIEW_EXTERIOR_MENUITEM = wxNewId();
long const ID_CHANGE_SHIP_VIEW_INTERIOR_MENUITEM = wxNewId();
long const ID_TIME_OF_DAY_UP_MENUITEM = wxNewId();
long const ID_TIME_OF_DAY_DOWN_MENUITEM = wxNewId();
long const ID_FULL_TIME_OF_DAY_MENUITEM = wxNewId();
long const ID_PAUSE_MENUITEM = wxNewId();
long const ID_STEP_MENUITEM = wxNewId();

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

long const ID_SHIPBUILDER_NEWSHIP_MENUITEM = wxNewId();
long const ID_SHIPBUILDER_EDITSHIP_MENUITEM = wxNewId();

long const ID_HELP_MENUITEM = wxNewId();
long const ID_ABOUT_MENUITEM = wxNewId();
long const ID_CHECK_FOR_UPDATES_MENUITEM = wxNewId();
long const ID_OPEN_HOME_PAGE_MENUITEM = wxNewId();
long const ID_OPEN_DOWNLOAD_PAGE_MENUITEM = wxNewId();

long const ID_POSTIINITIALIZE_TIMER = wxNewId();
long const ID_GAME_TIMER = wxNewId();
long const ID_LOW_FREQUENCY_TIMER = wxNewId();
long const ID_CHECK_UPDATES_TIMER = wxNewId();

MainFrame::MainFrame(
    wxApp * mainApp,
    std::optional<std::filesystem::path> initialShipFilePath,
    BootSettings bootSettings,
    ThreadManager & threadManager,
    GameAssetManager const & gameAssetManager,
    LocalizationManager & localizationManager)
    : mMainApp(mainApp)
    , mShipBuilderMainFrame()
    , mBootSettings(bootSettings)
    , mThreadManager(threadManager)
    , mGameAssetManager(gameAssetManager)
    , mLocalizationManager(localizationManager)
    , mGameController()
    , mSoundController()
    , mMusicController()
    , mToolController()
    , mSettingsManager()
    , mUIPreferencesManager()
    , mUpdateChecker()
    , mMainPanel(nullptr)
    , mMainGLCanvas(nullptr)
    , mMainGLCanvasContext()
    , mCurrentOpenGLCanvas(nullptr)
    // UI members checked for existance
    , mProbePanel(nullptr)
    , mElectricalPanel(nullptr)
    // State
    , mInitialShipFilePath(initialShipFilePath)
    , mCurrentShipLoadSpecs()
    , mPreviousShipLoadSpecs()
    , mHasWindowBeenShown(false)
    , mHasStartupTipBeenChecked(false)
    , mIsGameFrozen(false)
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

    mMainGLCanvas = new GLCanvas(
        mMainPanel,
        ID_MAIN_CANVAS);

    mMainGLCanvas->Connect(wxEVT_PAINT, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasPaint, 0, this);
    mMainGLCanvas->Connect(wxEVT_SIZE, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasResize, 0, this);
    mMainGLCanvas->Connect(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasMouseLeftDown, 0, this);
    mMainGLCanvas->Connect(wxEVT_LEFT_UP, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasMouseLeftUp, 0, this);
    mMainGLCanvas->Connect(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasMouseRightDown, 0, this);
    mMainGLCanvas->Connect(wxEVT_RIGHT_UP, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasMouseRightUp, 0, this);
    mMainGLCanvas->Connect(wxEVT_MIDDLE_DOWN, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasMouseMiddleDown, 0, this);
    mMainGLCanvas->Connect(wxEVT_MOTION, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasMouseMove, 0, this);
    mMainGLCanvas->Connect(wxEVT_MOUSEWHEEL, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasMouseWheel, 0, this);
    mMainGLCanvas->Connect(wxEVT_MOUSE_CAPTURE_LOST, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasCaptureMouseLost, 0, this);

    mMainPanelSizer->Add(
        mMainGLCanvas,
        1,                  // Occupy all available vertical space
        wxEXPAND,           // Expand also horizontally
        0);                 // Border


    //
    // Build menu
    //

    {
        wxMenuBar * mainMenuBar = new wxMenuBar();

#define ADD_TOOL_MENUITEM(label, shortcut, bitmap_path, toolType, isNpc)\
        [&]()\
        {\
            wxMenu * const parentMenu = (isNpc) ? mNpcToolsMenu : mNonNpcToolsMenu;\
            auto const id = wxNewId();\
            wxMenuItem * toolMenuItem = new wxMenuItem(parentMenu, id, (label) + (shortcut), wxEmptyString, wxITEM_CHECK);\
            auto const [uncheckedBmp, checkedBmp] = MakeMenuBitmaps((bitmap_path));\
            SET_BITMAPS(toolMenuItem, uncheckedBmp, checkedBmp)\
            Bind(wxEVT_COMMAND_MENU_SELECTED,\
                [this](wxCommandEvent &)\
                {\
                    SelectTool((toolType), (isNpc));\
                },\
                id);\
            parentMenu->Append(toolMenuItem);\
            if (isNpc)\
            {\
                mNpcToolMenuItems.push_back({(toolType), toolMenuItem});\
            }\
            else\
            {\
                mNonNpcToolMenuItems.push_back({(toolType), toolMenuItem});\
            }\
            if ((toolType) == InitialNonNpcToolType)\
            {\
                toolMenuItem->Check();\
            }\
            return toolMenuItem;\
        }();

#ifdef __WXGTK__
        // Note: on GTK we build an accelerator table for plain keys because of https://trac.wxwidgets.org/ticket/17611
        std::vector<wxAcceleratorEntry> acceleratorEntries;
#define ADD_PLAIN_ACCELERATOR_KEY(key, menuItem) \
        acceleratorEntries.push_back(MakePlainAcceleratorKey(int((key)), (menuItem)));
#else
#define ADD_PLAIN_ACCELERATOR_KEY(key, menuItem) \
        (void)menuItem;
#endif

#ifdef __WXMSW__
#define SET_BITMAPS(toolMenuItem, uncheckedBmp, checkedBmp) \
        (toolMenuItem)->SetBitmaps(checkedBmp, uncheckedBmp);
#else
#define SET_BITMAPS(toolMenuItem, uncheckedBmp, checkedBmp) \
        (toolMenuItem)->SetBitmap(uncheckedBmp);
#endif

        // File

        {
            wxMenu * fileMenu = new wxMenu();

            wxMenuItem * loadShipMenuItem = new wxMenuItem(fileMenu, ID_LOAD_SHIP_MENUITEM, _("Load Ship...") + wxS("\tCtrl+O"), wxEmptyString, wxITEM_NORMAL);
            fileMenu->Append(loadShipMenuItem);
            Connect(ID_LOAD_SHIP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnLoadShipMenuItemSelected);

            wxMenuItem * reloadCurrentShipMenuItem = new wxMenuItem(fileMenu, ID_RELOAD_CURRENT_SHIP_MENUITEM, _("Reload Current Ship") + wxS("\tCtrl+R"), wxEmptyString, wxITEM_NORMAL);
            fileMenu->Append(reloadCurrentShipMenuItem);
            fileMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [this](wxCommandEvent &) { ReloadCurrentShip(); }, ID_RELOAD_CURRENT_SHIP_MENUITEM);

            mReloadPreviousShipMenuItem = new wxMenuItem(fileMenu, ID_RELOAD_PREVIOUS_SHIP_MENUITEM, _("Reload Previous Ship") + wxS("\tCtrl+V"), wxEmptyString, wxITEM_NORMAL);
            fileMenu->Append(mReloadPreviousShipMenuItem);
            Connect(ID_RELOAD_PREVIOUS_SHIP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnReloadPreviousShipMenuItemSelected);
            mReloadPreviousShipMenuItem->Enable(false);

            fileMenu->AppendSeparator();

            wxMenuItem * reloadShipsMenuItem = new wxMenuItem(fileMenu, ID_MORE_SHIPS_MENUITEM, _("Get More Ships..."));
            fileMenu->Append(reloadShipsMenuItem);
            fileMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [](wxCommandEvent &) { wxLaunchDefaultBrowser("https://floatingsandbox.com/ship-packs/"); }, ID_MORE_SHIPS_MENUITEM);

            fileMenu->AppendSeparator();

            wxMenuItem * saveScreenshotMenuItem = new wxMenuItem(fileMenu, ID_SAVE_SCREENSHOT_MENUITEM, _("Save Screenshot") + wxS("\tCtrl+C"), wxEmptyString, wxITEM_NORMAL);
            fileMenu->Append(saveScreenshotMenuItem);
            Connect(ID_SAVE_SCREENSHOT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSaveScreenshotMenuItemSelected);

            wxMenuItem * openScreenshotFolderMenuItem = new wxMenuItem(fileMenu, ID_OPEN_SCREENSHOT_FOLDER_MENUITEM, _("Open Screenshots Folder"));
            fileMenu->Append(openScreenshotFolderMenuItem);
            fileMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [this](wxCommandEvent &) { wxLaunchDefaultBrowser(mUIPreferencesManager->GetScreenshotsFolderPath().string()); }, ID_OPEN_SCREENSHOT_FOLDER_MENUITEM);

            fileMenu->AppendSeparator();

            wxMenuItem * quitMenuItem = new wxMenuItem(fileMenu, ID_QUIT_MENUITEM, _("Quit") + wxS("\tAlt+F4"), _("Quit the game"), wxITEM_NORMAL);
            fileMenu->Append(quitMenuItem);
            Connect(ID_QUIT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnQuit);

            mainMenuBar->Append(fileMenu, _("&File"));
        }


        // Controls

        {
            wxMenu * controlsMenu = new wxMenu();

            wxMenuItem * zoomInMenuItem = new wxMenuItem(controlsMenu, ID_ZOOM_IN_MENUITEM, _("Zoom In") + wxS("\t+"), wxEmptyString, wxITEM_NORMAL);
            controlsMenu->Append(zoomInMenuItem);
            Connect(ID_ZOOM_IN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnZoomInMenuItemSelected);
            ADD_PLAIN_ACCELERATOR_KEY('+', zoomInMenuItem);
            ADD_PLAIN_ACCELERATOR_KEY(WXK_NUMPAD_ADD, zoomInMenuItem);

            wxMenuItem * zoomOutMenuItem = new wxMenuItem(controlsMenu, ID_ZOOM_OUT_MENUITEM, _("Zoom Out") + wxS("\t-"), wxEmptyString, wxITEM_NORMAL);
            controlsMenu->Append(zoomOutMenuItem);
            Connect(ID_ZOOM_OUT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnZoomOutMenuItemSelected);
            ADD_PLAIN_ACCELERATOR_KEY('-', zoomOutMenuItem);
            ADD_PLAIN_ACCELERATOR_KEY(WXK_NUMPAD_SUBTRACT, zoomOutMenuItem);

            wxMenuItem * resetViewMenuItem = new wxMenuItem(controlsMenu, ID_RESET_VIEW_MENUITEM, _("Reset View") + wxS("\tHOME"), wxEmptyString, wxITEM_NORMAL);
            controlsMenu->Append(resetViewMenuItem);
            Connect(ID_RESET_VIEW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnResetViewMenuItemSelected);
            ADD_PLAIN_ACCELERATOR_KEY(WXK_HOME, resetViewMenuItem);

            mAutoFocusAtShipLoadMenuItem = new wxMenuItem(controlsMenu, ID_AUTO_FOCUS_AT_SHIP_LOAD_MENUITEM, _("Auto-Focus at Ship Load"), _("Enable or disable auto-focus when a ship is loaded."), wxITEM_CHECK);
            controlsMenu->Append(mAutoFocusAtShipLoadMenuItem);
            Connect(ID_AUTO_FOCUS_AT_SHIP_LOAD_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAutoFocusAtShipLoadMenuItemSelected);

            controlsMenu->AppendSeparator();

            mContinuousAutoFocusOnShipMenuItem = new wxMenuItem(controlsMenu, ID_AUTO_FOCUS_ON_SHIP_MENUITEM, _("Continuous Auto-Focus on Ship") + wxS("\tCtrl+HOME"), _("Enable continuous auto-focus on ships."), wxITEM_RADIO);
            controlsMenu->Append(mContinuousAutoFocusOnShipMenuItem);
            controlsMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [this](wxCommandEvent &)
                {
                    assert(!!mUIPreferencesManager);
                    mUIPreferencesManager->SetAutoFocusTarget(AutoFocusTargetKindType::Ship);
                },
                ID_AUTO_FOCUS_ON_SHIP_MENUITEM);

            mContinuousAutoFocusOnSelectedNpcMenuItem = new wxMenuItem(controlsMenu, ID_AUTO_FOCUS_ON_SELECTED_NPC_MENUITEM, _("Continuous Auto-Focus on NPC") + wxS("\tCtrl+PgUp"), _("Enable continuous auto-focus on selected NPCs."), wxITEM_RADIO);
            controlsMenu->Append(mContinuousAutoFocusOnSelectedNpcMenuItem);
            controlsMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [this](wxCommandEvent &)
                {
                    assert(!!mUIPreferencesManager);
                    mUIPreferencesManager->SetAutoFocusTarget(AutoFocusTargetKindType::SelectedNpc);
                },
                ID_AUTO_FOCUS_ON_SELECTED_NPC_MENUITEM);

            mContinuousAutoFocusOnNothingMenuItem = new wxMenuItem(controlsMenu, ID_AUTO_FOCUS_ON_NOTHING_MENUITEM, _("No Auto-Focus") + wxS("\tCtrl+End"), _("Disable continuous auto-focus."), wxITEM_RADIO);
            controlsMenu->Append(mContinuousAutoFocusOnNothingMenuItem);
            controlsMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [this](wxCommandEvent &)
                {
                    assert(!!mUIPreferencesManager);
                    mUIPreferencesManager->SetAutoFocusTarget(std::nullopt);
                },
                ID_AUTO_FOCUS_ON_NOTHING_MENUITEM);

            controlsMenu->AppendSeparator();

            mShipViewExteriorMenuItem = new wxMenuItem(controlsMenu, ID_CHANGE_SHIP_VIEW_EXTERIOR_MENUITEM, _("Exterior View"), wxEmptyString, wxITEM_RADIO);
            controlsMenu->Append(mShipViewExteriorMenuItem);
            Connect(ID_CHANGE_SHIP_VIEW_EXTERIOR_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShipViewMenuItemSelected);

            mShipViewInteriorMenuItem = new wxMenuItem(controlsMenu, ID_CHANGE_SHIP_VIEW_INTERIOR_MENUITEM, _("Interior View"), wxEmptyString, wxITEM_RADIO);
            controlsMenu->Append(mShipViewInteriorMenuItem);
            Connect(ID_CHANGE_SHIP_VIEW_INTERIOR_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShipViewMenuItemSelected);

            controlsMenu->AppendSeparator();

            wxMenuItem * timeOfDayUpMenuItem = new wxMenuItem(controlsMenu, ID_TIME_OF_DAY_UP_MENUITEM, _("Bright Ambient Light") + wxS("\tPgUp"), wxEmptyString, wxITEM_NORMAL);
            controlsMenu->Append(timeOfDayUpMenuItem);
            Connect(ID_TIME_OF_DAY_UP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnTimeOfDayUpMenuItemSelected);

            wxMenuItem * timeOfDayDownMenuItem = new wxMenuItem(controlsMenu, ID_TIME_OF_DAY_DOWN_MENUITEM, _("Dim Ambient Light") + wxS("\tPgDn"), wxEmptyString, wxITEM_NORMAL);
            controlsMenu->Append(timeOfDayDownMenuItem);
            Connect(ID_TIME_OF_DAY_DOWN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnTimeOfDayDownMenuItemSelected);

            wxMenuItem * fullTimeOfDayMenuItem = new wxMenuItem(controlsMenu, ID_FULL_TIME_OF_DAY_MENUITEM, _("Full Day/Night"), wxEmptyString, wxITEM_NORMAL);
            controlsMenu->Append(fullTimeOfDayMenuItem);
            Connect(ID_FULL_TIME_OF_DAY_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnFullTimeOfDayMenuItemSelected);

            controlsMenu->AppendSeparator();

            mPauseMenuItem = new wxMenuItem(controlsMenu, ID_PAUSE_MENUITEM, _("Pause") + wxS("\tSpace"), _("Pause the game"), wxITEM_CHECK);
            controlsMenu->Append(mPauseMenuItem);
            Connect(ID_PAUSE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnPauseMenuItemSelected);
            mPauseMenuItem->Check(false);
            ADD_PLAIN_ACCELERATOR_KEY(WXK_SPACE, mPauseMenuItem);

            mStepMenuItem = new wxMenuItem(controlsMenu, ID_STEP_MENUITEM, _("Step") + wxS("\tEnter"), _("Step one frame at a time"), wxITEM_NORMAL);
            controlsMenu->Append(mStepMenuItem);
            Connect(ID_STEP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnStepMenuItemSelected);
            mStepMenuItem->Enable(false);

            mainMenuBar->Append(controlsMenu, _("&Controls"));
        }


        // Non-NPC Tools

        {
            mNonNpcToolsMenu = new wxMenu();

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Move/Rotate"), wxS("\tM"), "move_cursor_up", ToolType::Move, false);
                ADD_PLAIN_ACCELERATOR_KEY('M', menuItem);
            }

            {
                ADD_TOOL_MENUITEM(_("Move All/Rotate All"), wxS("\tALT+M"), "move_all_cursor_up", ToolType::MoveAll, false);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Move/Rotate Gripped"), wxS("\t4"), "move_gripped_cursor_up", ToolType::MoveGripped, false);
                ADD_PLAIN_ACCELERATOR_KEY('4', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Pick-n-Pull"), wxS("\tK"), "pliers_cursor_up", ToolType::PickAndPull, false);
                ADD_PLAIN_ACCELERATOR_KEY('K', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Smash"), wxS("\tS"), "smash_cursor_up", ToolType::Smash, false);
                ADD_PLAIN_ACCELERATOR_KEY('S', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Slice"), wxS("\tL"), "chainsaw_cursor_up", ToolType::Saw, false);
                ADD_PLAIN_ACCELERATOR_KEY('L', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("HeatBlaster/CoolBlaster"), wxS("\tH"), "heat_blaster_heat_cursor_up", ToolType::HeatBlaster, false);
                ADD_PLAIN_ACCELERATOR_KEY('H', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Fire Extinguisher"), wxS("\tX"), "fire_extinguisher_cursor_up", ToolType::FireExtinguisher, false);
                ADD_PLAIN_ACCELERATOR_KEY('X', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Blast"), wxS("\t8"), "blast_cursor_up_1", ToolType::Blast, false);
                ADD_PLAIN_ACCELERATOR_KEY('8', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Electric Spark"), wxS("\t7"), "electric_spark_cursor_up", ToolType::ElectricSpark, false);
                ADD_PLAIN_ACCELERATOR_KEY('7', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Attract/Repel"), wxS("\tG"), "drag_cursor_up_plus", ToolType::Grab, false);
                ADD_PLAIN_ACCELERATOR_KEY('G', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Swirl/Counterswirl"), wxS("\tW"), "swirl_cursor_up_cw", ToolType::Swirl, false);
                ADD_PLAIN_ACCELERATOR_KEY('W', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Anti-Gravity Field"), wxS("\t0"), "anti_gravity_field_cursor", ToolType::AntiGravityField, false);
                ADD_PLAIN_ACCELERATOR_KEY('0', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Toggle Pin"), wxS("\tP"), "pin_cursor", ToolType::Pin, false);
                ADD_PLAIN_ACCELERATOR_KEY('P', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Inject/Remove Pressure"), wxS("\tB"), "air_tank_cursor_up", ToolType::InjectPressure, false);
                ADD_PLAIN_ACCELERATOR_KEY('B', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Flood/Dry"), wxS("\tF"), "flood_cursor_up", ToolType::FloodHose, false);
                ADD_PLAIN_ACCELERATOR_KEY('F', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Toggle Timer Bomb"), wxS("\tT"), "timer_bomb_cursor", ToolType::TimerBomb, false);
                ADD_PLAIN_ACCELERATOR_KEY('T', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Toggle RC Bomb"), wxS("\tR"), "rc_bomb_cursor", ToolType::RCBomb, false);
                ADD_PLAIN_ACCELERATOR_KEY('R', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Toggle Impact Bomb"), wxS("\tI"), "impact_bomb_cursor", ToolType::ImpactBomb, false);
                ADD_PLAIN_ACCELERATOR_KEY('I', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Toggle Anti-Matter Bomb"), wxS("\tA"), "am_bomb_cursor", ToolType::AntiMatterBomb, false);
                ADD_PLAIN_ACCELERATOR_KEY('A', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Toggle RC Fire-Extinguishing Bomb"), wxS("\t5"), "fire_extinguishing_bomb_cursor", ToolType::FireExtinguishingBomb, false);
                ADD_PLAIN_ACCELERATOR_KEY('5', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Thanos' Snap"), wxS("\tQ"), "thanos_snap_cursor_up", ToolType::ThanosSnap, false);
                ADD_PLAIN_ACCELERATOR_KEY('Q', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("WaveMaker"), wxS("\tV"), "wave_maker_cursor_up", ToolType::WaveMaker, false);
                ADD_PLAIN_ACCELERATOR_KEY('V', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("WindMaker"), wxS("\t9"), "wind_cursor_up", ToolType::WindMaker, false);
                ADD_PLAIN_ACCELERATOR_KEY('9', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("LaserCannon"), wxS("\t2"), "laser_cannon_icon", ToolType::LaserCannon, false);
                ADD_PLAIN_ACCELERATOR_KEY('2', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("LightningStrike"), wxS("\t6"), "lightning_cursor_and_icon", ToolType::LightningStrike, false);
                ADD_PLAIN_ACCELERATOR_KEY('6', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Adjust Terrain"), wxS("\tJ"), "terrain_adjust_cursor_up", ToolType::TerrainAdjust, false);
                ADD_PLAIN_ACCELERATOR_KEY('J', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Repair"), wxS("\tE"), "repair_structure_cursor_up", ToolType::RepairStructure, false);
                ADD_PLAIN_ACCELERATOR_KEY('E', menuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Scrub/Rot"), wxS("\tU"), "scrub_cursor_up", ToolType::Scrub, false);
                ADD_PLAIN_ACCELERATOR_KEY('U', menuItem);
            }

            {
                mScareFishMenuItem = ADD_TOOL_MENUITEM(_("Scare/Allure Fishes"), wxS("\tZ"), "megaphone_cursor_up", ToolType::ScareFish, false);
                ADD_PLAIN_ACCELERATOR_KEY('Z', mScareFishMenuItem);
            }

            {
                auto menuItem = ADD_TOOL_MENUITEM(_("Lamp"), wxS("\t1"), "lamp_cursor_up", ToolType::Lamp, false);
                ADD_PLAIN_ACCELERATOR_KEY('1', menuItem);
            }

            ADD_TOOL_MENUITEM(_("Toggle Physics Probe"), wxS(""), "physics_probe_cursor", ToolType::PhysicsProbe, false);

            mNonNpcToolsMenu->AppendSeparator();

            {
                mRCBombsDetonateMenuItem = new wxMenuItem(mNonNpcToolsMenu, ID_RCBOMBDETONATE_MENUITEM, _("Detonate RC Bombs") + wxS("\tD"), wxEmptyString, wxITEM_NORMAL);
                mNonNpcToolsMenu->Append(mRCBombsDetonateMenuItem);
                Connect(ID_RCBOMBDETONATE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnRCBombDetonateMenuItemSelected);
                mRCBombsDetonateMenuItem->Enable(false);
                ADD_PLAIN_ACCELERATOR_KEY('D', mRCBombsDetonateMenuItem);
            }

            {
                mAntiMatterBombsDetonateMenuItem = new wxMenuItem(mNonNpcToolsMenu, ID_ANTIMATTERBOMBDETONATE_MENUITEM, _("Detonate Anti-Matter Bombs") + wxS("\tN"), wxEmptyString, wxITEM_NORMAL);
                mNonNpcToolsMenu->Append(mAntiMatterBombsDetonateMenuItem);
                Connect(ID_ANTIMATTERBOMBDETONATE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAntiMatterBombDetonateMenuItemSelected);
                mAntiMatterBombsDetonateMenuItem->Enable(false);
                ADD_PLAIN_ACCELERATOR_KEY('N', mAntiMatterBombsDetonateMenuItem);
            }

            wxMenuItem * triggerTsunamiMenuItem = new wxMenuItem(mNonNpcToolsMenu, ID_TRIGGERTSUNAMI_MENUITEM, _("Trigger Tsunami"), wxEmptyString, wxITEM_NORMAL);
            mNonNpcToolsMenu->Append(triggerTsunamiMenuItem);
            Connect(ID_TRIGGERTSUNAMI_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnTriggerTsunamiMenuItemSelected);

            wxMenuItem * triggerRogueWaveMenuItem = new wxMenuItem(mNonNpcToolsMenu, ID_TRIGGERROGUEWAVE_MENUITEM, _("Trigger Rogue Wave"), wxEmptyString, wxITEM_NORMAL);
            mNonNpcToolsMenu->Append(triggerRogueWaveMenuItem);
            Connect(ID_TRIGGERROGUEWAVE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnTriggerRogueWaveMenuItemSelected);

            mTriggerStormMenuItem = new wxMenuItem(mNonNpcToolsMenu, ID_TRIGGERSTORM_MENUITEM, _("Trigger Storm"), wxEmptyString, wxITEM_NORMAL);
            mNonNpcToolsMenu->Append(mTriggerStormMenuItem);
            Connect(ID_TRIGGERSTORM_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnTriggerStormMenuItemSelected);
            mTriggerStormMenuItem->Enable(true);

            wxMenuItem * triggerLightningMenuItem = new wxMenuItem(mNonNpcToolsMenu, ID_TRIGGERLIGHTNING_MENUITEM, _("Trigger Lightning") + wxS("\tALT+L"), wxEmptyString, wxITEM_NORMAL);
            mNonNpcToolsMenu->Append(triggerLightningMenuItem);
            Connect(ID_TRIGGERLIGHTNING_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnTriggerLightningMenuItemSelected);

            mainMenuBar->Append(mNonNpcToolsMenu, _("&Tools"));
        }

        // NPC Tools

        {
            mNpcToolsMenu = new wxMenu();

            //
            // NPC
            //

            // Add human
            {
                mHumanNpcSubMenu = new wxMenu(_(""));

                // Create menu
                mAddHumanNpcMenuItem = new wxMenuItem(mNpcToolsMenu, wxID_ANY, _("Spawn Human NPC..."), wxEmptyString, wxITEM_NORMAL, mHumanNpcSubMenu);
                mNpcToolsMenu->Append(mAddHumanNpcMenuItem);
                mAddHumanNpcMenuItem->Enable(true); // Note: here we're assuming we _can_ add NPCs; unfortunately the NPCs class is created _before_ we register for events, hence have to guess here
                std::tie(mAddHumanNpcUncheckedIcon, mAddHumanNpcCheckedIcon) = MakeMenuBitmaps("add_human_npc_icon");
                SetMenuItemChecked(mAddHumanNpcMenuItem, mAddHumanNpcUncheckedIcon, mAddHumanNpcCheckedIcon, InitialNonNpcToolType == ToolType::PlaceHumanNpc);
                mNpcToolMenuItems.push_back({ ToolType::PlaceHumanNpc, mAddHumanNpcMenuItem });
            }

            // Add furniture
            {
                mFurnitureNpcSubMenu = new wxMenu(_(""));

                // Create menu
                mAddFurnitureNpcMenuItem = new wxMenuItem(mNpcToolsMenu, wxID_ANY, _("Spawn Furniture NPC..."), wxEmptyString, wxITEM_NORMAL, mFurnitureNpcSubMenu);
                mNpcToolsMenu->Append(mAddFurnitureNpcMenuItem);
                mAddFurnitureNpcMenuItem->Enable(true); // Note: here we're assuming we _can_ add NPCs; unfortunately the NPCs class is created _before_ we register for events, hence have to guess here
                std::tie(mAddFurnitureNpcUncheckedIcon, mAddFurnitureNpcCheckedIcon) = MakeMenuBitmaps("add_furniture_npc_icon");
                SetMenuItemChecked(mAddFurnitureNpcMenuItem, mAddFurnitureNpcUncheckedIcon, mAddFurnitureNpcCheckedIcon, InitialNonNpcToolType == ToolType::PlaceFurnitureNpc);
                mNpcToolMenuItems.push_back({ ToolType::PlaceFurnitureNpc, mAddFurnitureNpcMenuItem });
            }

            // Move
            {
                mMoveNpcMenuItem = ADD_TOOL_MENUITEM(_("Move NPC/NPCs"), wxS("\t3"), "move_npc_icon", ToolType::MoveNpc, true);
                ADD_PLAIN_ACCELERATOR_KEY('3', mMoveNpcMenuItem);
            }

            // Remove
            {
                mRemoveNpcMenuItem = ADD_TOOL_MENUITEM(_("Remove NPC/NPCs"), wxS("\tDEL"), "remove_npc_icon", ToolType::RemoveNpc, true);
            }

            // Turnaround
            {
                mTurnaroundNpcMenuItem = ADD_TOOL_MENUITEM(_("Turnaround NPC/NPCs"), wxS(""), "turnaround_npc_icon", ToolType::TurnaroundNpc, true);
            }

            // Follow
            {
                mFollowNpcMenuItem = ADD_TOOL_MENUITEM(_("Follow NPC"), wxS(""), "autofocus_on_npc_icon", ToolType::FollowNpc, true);
            }

            mNpcToolsMenu->AppendSeparator();

            // Add Human NPC Group
            {
                auto * menuItem = new wxMenuItem(mNpcToolsMenu, wxNewId(), _("Spawn Human NPC Group"), wxEmptyString, wxITEM_NORMAL);
                mNpcToolsMenu->Append(menuItem);
                Connect(menuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAddHumanNpcGroupMenuItemSelected);
            }

            // Add Furniture NPC Group
            {
                auto * menuItem = new wxMenuItem(mNpcToolsMenu, wxNewId(), _("Spawn Furniture NPC Group"), wxEmptyString, wxITEM_NORMAL);
                mNpcToolsMenu->Append(menuItem);
                Connect(menuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAddFurnitureNpcGroupMenuItemSelected);
            }

            // Select
            {
                mSelectNextNpcMenuItem = new wxMenuItem(mNpcToolsMenu, wxNewId(), _("Select Next NPC") + wxS("\t>"), wxEmptyString, wxITEM_NORMAL);
                mNpcToolsMenu->Append(mSelectNextNpcMenuItem);
                Connect(mSelectNextNpcMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSelectNextNpcMenuItemSelected);
            }

            // De-Select
            {
                mDeselectNpcMenuItem = new wxMenuItem(mNpcToolsMenu, wxNewId(), _("Deselect NPC"), wxEmptyString, wxITEM_NORMAL);
                mNpcToolsMenu->Append(mDeselectNpcMenuItem);
                Connect(mDeselectNpcMenuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnDeselectNpcMenuItemSelected);
            }

            mainMenuBar->Append(mNpcToolsMenu, _("&NPCs"));
        }

        // Options

        {
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

            optionsMenu->AppendSeparator();

            wxMenuItem * openLogWindowMenuItem = new wxMenuItem(optionsMenu, ID_OPEN_LOG_WINDOW_MENUITEM, _("Open Log Window") + wxS("\tCtrl+L"), wxEmptyString, wxITEM_NORMAL);
            optionsMenu->Append(openLogWindowMenuItem);
            Connect(ID_OPEN_LOG_WINDOW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnOpenLogWindowMenuItemSelected);

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

            optionsMenu->AppendSeparator();

            mFullScreenMenuItem = new wxMenuItem(optionsMenu, ID_FULL_SCREEN_MENUITEM, _("Full Screen") + wxS("\tF11"), wxEmptyString, wxITEM_NORMAL);
            optionsMenu->Append(mFullScreenMenuItem);
            Connect(ID_FULL_SCREEN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnFullScreenMenuItemSelected);

            mNormalScreenMenuItem = new wxMenuItem(optionsMenu, ID_NORMAL_SCREEN_MENUITEM, _("Normal Screen") + wxS("\tESC"), wxEmptyString, wxITEM_NORMAL);
            optionsMenu->Append(mNormalScreenMenuItem);
            Connect(ID_NORMAL_SCREEN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnNormalScreenMenuItemSelected);

            optionsMenu->AppendSeparator();

            mMuteMenuItem = new wxMenuItem(optionsMenu, ID_MUTE_MENUITEM, _("Mute") + wxS("\tCtrl+M"), wxEmptyString, wxITEM_CHECK);
            optionsMenu->Append(mMuteMenuItem);
            Connect(ID_MUTE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnMuteMenuItemSelected);
            mMuteMenuItem->Check(false);

            mainMenuBar->Append(optionsMenu, _("&Options"));
        }

        // ShipBuilder

        {
            wxMenu * shipBuilderMenu = new wxMenu();

            wxMenuItem * newShipMenuItem = new wxMenuItem(shipBuilderMenu, ID_SHIPBUILDER_NEWSHIP_MENUITEM, _("Create New Ship..."), _("Open the ShipBuilder to create a new ship"), wxITEM_NORMAL);
            shipBuilderMenu->Append(newShipMenuItem);
            Connect(ID_SHIPBUILDER_NEWSHIP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShipBuilderNewShipMenuItemSelected);

            wxMenuItem * editShipMenuItem = new wxMenuItem(shipBuilderMenu, ID_SHIPBUILDER_EDITSHIP_MENUITEM, _("Edit This Ship..."), _("Open the ShipBuilder to edit the currently-loaded ship"), wxITEM_NORMAL);
            shipBuilderMenu->Append(editShipMenuItem);
            Connect(ID_SHIPBUILDER_EDITSHIP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShipBuilderEditShipMenuItemSelected);

            mainMenuBar->Append(shipBuilderMenu, _("&ShipBuilder"));
        }

        // Help

        {
            wxMenu * helpMenu = new wxMenu();

            wxMenuItem * helpMenuItem = new wxMenuItem(helpMenu, ID_HELP_MENUITEM, _("Guide") + wxS("\tF1"), _("Get help about the simulator"), wxITEM_NORMAL);
            helpMenu->Append(helpMenuItem);
            Connect(ID_HELP_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnHelpMenuItemSelected);

            wxMenuItem * aboutMenuItem = new wxMenuItem(helpMenu, ID_ABOUT_MENUITEM, _("About and Credits") + wxS("\tF2"), _("Show credits and other I'vedunnit stuff"), wxITEM_NORMAL);
            helpMenu->Append(aboutMenuItem);
            Connect(ID_ABOUT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAboutMenuItemSelected);

            helpMenu->AppendSeparator();

            wxMenuItem * checkForUpdatesMenuItem = new wxMenuItem(helpMenu, ID_CHECK_FOR_UPDATES_MENUITEM, _("Check for Updates..."), wxEmptyString, wxITEM_NORMAL);
            helpMenu->Append(checkForUpdatesMenuItem);
            Connect(ID_CHECK_FOR_UPDATES_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnCheckForUpdatesMenuItemSelected);

            {
                wxMenuItem * menuItem = new wxMenuItem(helpMenu, wxID_ANY, _("Donate..."));
                helpMenu->Append(menuItem);
                helpMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [](wxCommandEvent &) { wxLaunchDefaultBrowser("https://floatingsandbox.com/donate/"); }, menuItem->GetId());
            }

            {
                wxMenuItem * menuItem = new wxMenuItem(helpMenu, wxID_ANY, _("Buy Merchandise..."));
                helpMenu->Append(menuItem);
                helpMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [](wxCommandEvent &) { wxLaunchDefaultBrowser("https://shop.floatingsandbox.com/"); }, menuItem->GetId());
            }

            helpMenu->AppendSeparator();

            wxMenuItem * openHomePageMenuItem = new wxMenuItem(helpMenu, ID_OPEN_HOME_PAGE_MENUITEM, _("Open Home Page"));
            helpMenu->Append(openHomePageMenuItem);
            helpMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [](wxCommandEvent &) { wxLaunchDefaultBrowser("https://floatingsandbox.com"); }, ID_OPEN_HOME_PAGE_MENUITEM);

            wxMenuItem * openDownloadPageMenuItem = new wxMenuItem(helpMenu, ID_OPEN_DOWNLOAD_PAGE_MENUITEM, _("Open Download Page"));
            helpMenu->Append(openDownloadPageMenuItem);
            helpMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [](wxCommandEvent &) { wxLaunchDefaultBrowser("https://gamejolt.com/games/floating-sandbox/353572"); }, ID_OPEN_DOWNLOAD_PAGE_MENUITEM);

            mainMenuBar->Append(helpMenu, _("&Help"));
        }

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

    mProbePanel = new ProbePanel(mMainPanel);

    mMainPanelSizer->Add(mProbePanel, 0, wxEXPAND); // Expand horizontally

    mMainPanelSizer->Hide(mProbePanel);



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
}

MainFrame::~MainFrame()
{
    LogMessage("MainFrame::~MainFrame()");
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
            mGameController->Pan(DisplayLogicalSize(-mUIPreferencesManager->GetPanIncrement(), 0));
            return true;
        }
    }
    else if (keyCode == WXK_UP)
    {
        if (!!mGameController && !!mUIPreferencesManager)
        {
            // Up
            mGameController->Pan(DisplayLogicalSize(0, -mUIPreferencesManager->GetPanIncrement()));
            return true;
        }
    }
    else if (keyCode == WXK_RIGHT)
    {
        if (!!mGameController && !!mUIPreferencesManager)
        {
            // Right
            mGameController->Pan(DisplayLogicalSize(mUIPreferencesManager->GetPanIncrement(), 0));
            return true;
        }
    }
    else if (keyCode == WXK_DOWN)
    {
        if (!!mGameController && !!mUIPreferencesManager)
        {
            // Down
            mGameController->Pan(DisplayLogicalSize(0, mUIPreferencesManager->GetPanIncrement()));
            return true;
        }
    }
    else if (keyCode == '/')
    {
        if (!!mGameController && !!mToolController)
        {
            // Query

            auto const screenCoords = mToolController->GetMouseScreenCoordinates();
            vec2f worldCoords = mGameController->ScreenToWorld(screenCoords);

            LogMessage("@ ", worldCoords.toString(), ":");

            if ((keyModifiers & wxMOD_ALT) == 0)
            {
                mGameController->QueryNearestPointAt(screenCoords);
            }
            else
            {
                mGameController->QueryNearestNpcAt(screenCoords);
            }

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

void MainFrame::OnSecretTypingBootSettings()
{
    BootSettingsDialog dlg(
        this,
        mGameAssetManager);

    dlg.ShowModal();
}

void MainFrame::OnSecretTypingDebug()
{
    if (!mDebugDialog)
    {
        mDebugDialog = std::make_unique<DebugDialog>(
            this,
            *mGameController,
            *mSoundController);
    }

    mDebugDialog->Open();
}

void MainFrame::OnSecretTypingLoadBuiltInShip(int ship)
{
    std::filesystem::path builtInShipFilePath;
    if (ship == 2)
        builtInShipFilePath = mGameAssetManager.GetApril1stShipDefinitionFilePath();
    else if (ship ==3)
        builtInShipFilePath = mGameAssetManager.GetHolidaysShipDefinitionFilePath();
    else
        builtInShipFilePath = mGameAssetManager.GetFallbackShipDefinitionFilePath();

    LoadShip(ShipLoadSpecifications(builtInShipFilePath), false);
}

void MainFrame::OnSecretTypingGoToWorldEnd(int side)
{
    mGameController->PanToWorldEnd(side);
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

    std::shared_ptr<SplashScreenDialog> splash;

    try
    {
        splash = std::make_unique<SplashScreenDialog>(mGameAssetManager);
    }
    catch (std::exception const & e)
    {
        OnError("Error during game initialization: " + std::string(e.what()), true);

        return;
    }

    //
    // Create OpenGL context, temporarily on the splash screen's canvas, as we need
    // the canvas to be visible at the moment the context is created
    //

    // Our current OpenGL canvas is the canvas of the splash screen
    mCurrentOpenGLCanvas.store(splash->GetOpenGLCanvas());

    // Create the main - and only - OpenGL context on the current (splash) canvas
    mMainGLCanvasContext = std::make_unique<wxGLContext>(mCurrentOpenGLCanvas.load());

#if defined(_DEBUG) && defined(_WIN32)
    LogMessage("MainFrame::OnPostInitializeTrigger: Hiding SplashScreenDialog");
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
            RenderDeviceProperties(
                DisplayLogicalSize(
                    mMainGLCanvas->GetSize().GetWidth(),
                    mMainGLCanvas->GetSize().GetHeight()),
                mMainGLCanvas->GetContentScaleFactor(),
                mBootSettings.DoForceNoGlFinish,
                mBootSettings.DoForceNoMultithreadedRendering,
                std::bind(&MainFrame::MakeOpenGLContextCurrent, this),
                [this]()
                {
                    //
                    // Invoked by a different thread, with asynchronous
                    // execution
                    //

                    //LogMessage("TODOTEST: Swapping buffers...");

                    assert(mCurrentOpenGLCanvas.load() != nullptr);
                    mCurrentOpenGLCanvas.load()->SwapBuffers();

                    //LogMessage("TODOTEST: ...buffers swapped.");
                }),
            mThreadManager,
            mGameAssetManager,
            ProgressCallback(
                [this, &splash](float progress, ProgressMessageType message)
                {
                    splash->UpdateProgress(progress, message);
                    this->mMainApp->Yield();
                    this->mMainApp->Yield();
                    this->mMainApp->Yield();
                },
                0.0f,
                0.3f));
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
        mSoundController = std::make_unique<SoundController>(
            mGameAssetManager,
            ProgressCallback(
                [&splash, this](float progress, ProgressMessageType message)
                {
                    splash->UpdateProgress(progress, message);
                    this->mMainApp->Yield();
                    this->mMainApp->Yield();
                    this->mMainApp->Yield();
                },
                0.3f,
                0.4f));
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
        mMusicController = std::make_unique<MusicController>(
            mGameAssetManager,
            ProgressCallback(
                [&splash, this](float progress, ProgressMessageType message)
                {
                    splash->UpdateProgress(progress, message);
                    this->mMainApp->Yield();
                    this->mMainApp->Yield();
                    this->mMainApp->Yield();
                },
                0.7f,
                0.05f));
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

    mSettingsManager = std::make_unique<SettingsManager>(
        *mGameController,
        *mSoundController,
        mGameAssetManager.GetThemeSettingsRootFilePath(),
        StandardSystemPaths::GetInstance().GetUserGameSettingsRootFolderPath());

    // Enable "Reload Last Modified Settings" menu if we have last-modified settings
    mReloadLastModifiedSettingsMenuItem->Enable(mSettingsManager->HasLastModifiedSettingsPersisted());


    //
    // Create UI Preferences Manager
    //

    mUIPreferencesManager = std::make_unique<UIPreferencesManager>(
        *mGameController,
        *mMusicController,
        mLocalizationManager);


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
        *mGameController,
        *mSoundController,
        *mUIPreferencesManager,
        mGameAssetManager,
        ProgressCallback(
            [&splash, this](float progress, ProgressMessageType message)
            {
                splash->UpdateProgress(progress, message);
                this->mMainApp->Yield();
                this->mMainApp->Yield();
                this->mMainApp->Yield();
            },
            0.75f,
            0.10f));

    mMainPanelSizer->Add(mElectricalPanel, 0, wxEXPAND); // Expand horizontally


    //
    // Create Tool Controller
    //

    try
    {
        mToolController = std::make_unique<ToolController>(
            InitialNonNpcToolType,
            mGameController->GetEffectiveAmbientLightIntensity(),
            mMainGLCanvas,
            *mGameController,
            *mSoundController,
            mGameAssetManager);
    }
    catch (std::exception const & e)
    {
        splash.reset();

        OnError("Error during initialization of tool controller: " + std::string(e.what()), true);

        return;
    }

    this->mMainApp->Yield();


    //
    // Reconciliate UI
    //

    ResetShipUIState();
    RebuildNpcMenus();
    ReconciliateUIWithUIPreferencesAndSettings();
    ReconciliateUIWithAutoFocusTarget(mGameController->GetAutoFocusTarget());


    //
    // Create ShipBuilder frame
    //

    {
        try
        {
            mShipBuilderMainFrame = std::make_unique<ShipBuilder::MainFrame>(
                mMainApp,
                GetIcon(),
                mGameAssetManager,
                mLocalizationManager,
                mGameController->GetMaterialDatabase(),
                mGameController->GetShipTexturizer(),
                mSoundController.get(),
                [this](std::optional<std::filesystem::path> shipFilePath)
                {
                    this->SwitchFromShipBuilder(shipFilePath);
                },
                ProgressCallback(
                    [&splash, this](float progress, ProgressMessageType message)
                    {
                        splash->UpdateProgress(progress, message);
                        this->mMainApp->Yield();
                        this->mMainApp->Yield();
                        this->mMainApp->Yield();
                    },
                    0.85f,
                    0.14f));
        }
        catch (std::exception const & e)
        {
            splash.reset();

            OnError("Error during initialization of ship builder: " + std::string(e.what()), true);

            return;
        }

        // Given that we might have made another OpenGL context current, make our context current again
        mGameController->RebindOpenGLContext();
    }

    //
    // Register game event handlers
    //

    // Tiny hack: synthetically fire first events that would have reached us
    // if we had already registered
    this->OnFishCountUpdated(mGameController->GetNumberOfFishes());
    this->OnNpcSelectionChanged(mGameController->GetCurrentlySelectedNpc());

    this->RegisterEventHandler(*mGameController);
    mProbePanel->RegisterEventHandler(*mGameController);
    mElectricalPanel->RegisterEventHandler(*mGameController);
    mSoundController->RegisterEventHandler(*mGameController);
    mMusicController->RegisterEventHandler(*mGameController);


    //
    // Load initial ship
    //

    splash->UpdateProgress(1.0f, ProgressMessageType::Ready);
    this->mMainApp->Yield();
    this->mMainApp->Yield();
    this->mMainApp->Yield();
    this->mMainApp->Yield();

    std::optional<ShipLoadSpecifications> startupShipLoadSpecs;

    if (mInitialShipFilePath.has_value())
    {
        // Use given
        startupShipLoadSpecs.emplace(*mInitialShipFilePath);
    }
    else
    {
        // See if the last loaded ship is applicable
        if (mUIPreferencesManager->GetReloadLastLoadedShipOnStartup())
        {
            startupShipLoadSpecs = mUIPreferencesManager->GetLastShipLoadedSpecifications();

            // Make sure it still exists
            if (startupShipLoadSpecs.has_value()
                && !std::filesystem::exists(startupShipLoadSpecs->DefinitionFilepath))
            {
                startupShipLoadSpecs.reset();
            }
        }
    }

    if (!startupShipLoadSpecs.has_value())
    {
        // Use default ship
        startupShipLoadSpecs.emplace(ChooseDefaultShip(mGameAssetManager));
    }

    assert(startupShipLoadSpecs.has_value());

    if (!std::filesystem::exists(startupShipLoadSpecs->DefinitionFilepath))
    {
        startupShipLoadSpecs.emplace(mGameAssetManager.GetFallbackShipDefinitionFilePath());
    }

    assert(startupShipLoadSpecs.has_value());

    try
    {
        mGameController->AddShip(*startupShipLoadSpecs, mGameAssetManager);

        // Succeeded
        OnShipLoaded(*startupShipLoadSpecs);
    }
    catch (UserGameException const & exc)
    {
        splash.reset();

        OnError(mLocalizationManager.MakeErrorMessage(exc), true);

        return;
    }
    catch (std::exception const & exc)
    {
        splash.reset();

        OnError("Error loading initial ship: " + std::string(exc.what()), true);

        return;
    }


    //
    // Start check update timer
    //

    if (mUIPreferencesManager->GetCheckUpdatesAtStartup())
    {
        // 10 seconds
        mCheckUpdatesTimer->Start(10000, true);
    }


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
    // Setup game timer
    //

    // Ensure 1 second of real time is (no less than) 1 second of simulation
    mGameTimerDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::duration<float>(mGameController->GetSimulationStepTimeDuration()));

    LogMessage("Game timer duration: ", mGameTimerDuration.count());

    mGameTimer = std::make_unique<wxTimer>(this, ID_GAME_TIMER);
    Connect(ID_GAME_TIMER, wxEVT_TIMER, (wxObjectEventFunction)&MainFrame::OnGameTimerTrigger);


    //
    // Start low-frequency timer
    //

    mLowFrequencyTimer = std::make_unique<wxTimer>(this, ID_LOW_FREQUENCY_TIMER);
    Connect(ID_LOW_FREQUENCY_TIMER, wxEVT_TIMER, (wxObjectEventFunction)&MainFrame::OnLowFrequencyTimerTrigger);

    StartLowFrequencyTimer();


    //
    // Signal the OnPaint event that it may transfer the canvas now
    //

    mSplashScreenDialog = std::move(splash);


    //
    // Run the first game iteration at the next idle event
    //

    Bind(wxEVT_IDLE, &MainFrame::OnPostInitializeIdle, this);

    wxWakeUpIdle(); // Make sure we run an Idle event right after this handler
}

void MainFrame::OnPostInitializeIdle(wxIdleEvent & /*event*/)
{
    LogMessage("MainFrame::OnPostInitializeIdle()");

    // Unbind this Idle handler
    bool unbindResult = Unbind(wxEVT_IDLE, &MainFrame::OnPostInitializeIdle, this);
    if (!unbindResult)
    {
        OnError("Cannot unbind OnIdle", true);
    }

    // Bind definitive Idle handler
    Bind(wxEVT_IDLE, &MainFrame::OnIdle, this);

    // Run the very first game iteration
    RunGameIteration();
}

void MainFrame::OnMainFrameClose(wxCloseEvent & /*event*/)
{
    LogMessage("MainFrame::OnMainFrameClose()");

    // Stop game
    FreezeGame();

    // Save last-modified settings, if enabled
    if (!!mUIPreferencesManager && mUIPreferencesManager->GetSaveSettingsOnExit())
    {
        if (!!mSettingsManager)
        {
            mSettingsManager->SaveLastModifiedSettings();
        }
    }

    // Destroy all our objects - before windows (and thus GL contextes, etc.)
    // get destroyed by wxWidgets
    mUpdateChecker.reset();
    mUIPreferencesManager.reset();
    mSettingsManager.reset();
    mToolController.reset();
    mMusicController.reset();
    mSoundController.reset();
    mGameController.reset();

    // Destroy the frame!
    Destroy();
}

void MainFrame::OnQuit(wxCommandEvent & /*event*/)
{
    LogMessage("MainFrame::OnQuit()");

    // Close frame
    Close(true);
}

void MainFrame::OnMainPanelKeyDown(wxKeyEvent & event)
{
    if (!ProcessKeyDown(event.GetKeyCode(), event.GetModifiers()))
        event.Skip();
}

void MainFrame::OnGameTimerTrigger(wxTimerEvent & /*event*/)
{
    ////{
    ////    static std::chrono::steady_clock::time_point lastGameTimerTriggerTimestamp = std::chrono::steady_clock::time_point::min();
    ////    auto const now = std::chrono::steady_clock::now();
    ////    if (lastGameTimerTriggerTimestamp != std::chrono::steady_clock::time_point::min())
    ////    {
    ////        LogMessage("Timer delay: ", std::chrono::duration_cast<std::chrono::microseconds>(now - lastGameTimerTriggerTimestamp).count());
    ////    }
    ////    lastGameTimerTriggerTimestamp = now;
    ////}

    RunGameIteration();
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
    if (mHasStartupTipBeenChecked
        && (mGameTimer != nullptr && !mGameTimer->IsRunning())
        && !mIsGameFrozen)
    {
        RunGameIteration();
    }
}

//
// Main canvas event handlers
//

void MainFrame::OnMainGLCanvasPaint(wxPaintEvent & event)
{
    if (mSplashScreenDialog)
    {
        //
        // Now that the glCanvas is visible, we may transfer the
        // OpenGL context to the canvas and close the splash screen
        //

        LogMessage("MainFrame::OnMainGLCanvasPaint(): rebinding OpenGLContext to main GL canvas, and hiding SplashScreen");

        // Move OpenGL context to *our* canvas
        assert(!!mMainGLCanvas);
        assert(!!mMainGLCanvasContext);
        assert(!!mGameController);
        mCurrentOpenGLCanvas.store(mMainGLCanvas);
        mGameController->RebindOpenGLContext();

        // Close splash screen
        mSplashScreenDialog->Close();
        mSplashScreenDialog->Destroy();
        mSplashScreenDialog.reset();
    }

    event.Skip();
}

void MainFrame::OnMainGLCanvasResize(wxSizeEvent & event)
{
    LogMessage("OnMainGLCanvasResize: ", event.GetSize().GetX(), "x", event.GetSize().GetY(),
        (mGameController) ? " (With GameController)" : " (Without GameController)");

    if (mGameController
        && event.GetSize().GetX() > 0
        && event.GetSize().GetY() > 0)
    {
        mGameController->SetCanvasSize(
            DisplayLogicalSize(
                event.GetSize().GetX(),
                event.GetSize().GetY()));
    }

    event.Skip();
}

void MainFrame::OnMainGLCanvasMouseLeftDown(wxMouseEvent & /*event*/)
{
    // First of all, set focus on the canvas if it has lost it - we want
    // it to receive all mouse events
    if (!mMainGLCanvas->HasFocus())
    {
        mMainGLCanvas->SetFocus();
    }

    // Tell tool controller
    if (mToolController)
    {
        mToolController->OnLeftMouseDown();
    }

    // Hang on to the mouse for as long as the button is pressed
    if (!mIsMouseCapturedByGLCanvas)
    {
        mMainGLCanvas->CaptureMouse();
        mIsMouseCapturedByGLCanvas = true;
    }
}

void MainFrame::OnMainGLCanvasMouseLeftUp(wxMouseEvent & /*event*/)
{
    // We can now release the mouse
    if (mIsMouseCapturedByGLCanvas)
    {
        mMainGLCanvas->ReleaseMouse();
        mIsMouseCapturedByGLCanvas = false;
    }

    if (mToolController)
    {
        mToolController->OnLeftMouseUp();
    }
}

void MainFrame::OnMainGLCanvasMouseRightDown(wxMouseEvent & /*event*/)
{
    if (mToolController)
    {
        mToolController->OnRightMouseDown();
    }

    // Hang on to the mouse for as long as the button is pressed
    if (!mIsMouseCapturedByGLCanvas)
    {
        mMainGLCanvas->CaptureMouse();
        mIsMouseCapturedByGLCanvas = true;
    }
}

void MainFrame::OnMainGLCanvasMouseRightUp(wxMouseEvent & /*event*/)
{
    // We can now release the mouse
    if (mIsMouseCapturedByGLCanvas)
    {
        mMainGLCanvas->ReleaseMouse();
        mIsMouseCapturedByGLCanvas = false;
    }

    if (mToolController)
    {
        mToolController->OnRightMouseUp();
    }
}

void MainFrame::OnMainGLCanvasMouseMiddleDown(wxMouseEvent & /*event*/)
{
    OnMidMouseButtonDown();
}

void MainFrame::OnMainGLCanvasMouseMove(wxMouseEvent & event)
{
    if (mToolController)
    {
        mToolController->OnMouseMove(
            DisplayLogicalCoordinates(
                event.GetX(),
                event.GetY()));
    }
}

void MainFrame::OnMainGLCanvasMouseWheel(wxMouseEvent & event)
{
    if (mGameController)
    {
        mGameController->AdjustZoom(powf(1.002f, event.GetWheelRotation()));
    }
}

void MainFrame::OnMainGLCanvasCaptureMouseLost(wxMouseCaptureLostEvent & /*event*/)
{
    if (mToolController)
    {
        mToolController->UnsetTool();
    }
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
        mShipLoadDialog = std::make_unique<ShipLoadDialog<ShipLoadDialogUsageType::ForGame>>(
            this,
            mGameAssetManager);
    }

    // Open dialog
    auto res = mShipLoadDialog->ShowModal(mUIPreferencesManager->GetShipLoadDirectories());

    // Process result
    if (res == wxID_OK)
    {
        //
        // Load ship
        //

        ShipLoadSpecifications const shipLoadSpecs = mShipLoadDialog->GetChosenShip();
        LoadShip(shipLoadSpecs, true);

        // Store directory in preferences
        mUIPreferencesManager->AddShipLoadDirectory(shipLoadSpecs.DefinitionFilepath.parent_path());
    }

    SetPaused(false);
}

void MainFrame::OnReloadPreviousShipMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(mPreviousShipLoadSpecs.has_value()); // Or else we wouldn't be here

    LoadShip(*mPreviousShipLoadSpecs, false);
}

void MainFrame::OnSaveScreenshotMenuItemSelected(wxCommandEvent & /*event*/)
{
    //
    // Fire snapshot sound
    //

    assert(!!mSoundController);
    mSoundController->PlaySnapshotSound();

    try
    {
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
            GameAssetManager::SavePngImage(
                screenshotImage,
                screenshotFilePath);
        }
        catch (std::filesystem::filesystem_error const & fex)
        {
            OnError(
                std::string("Could not save screenshot to file \"") + screenshotFilePath.string() + "\": " + fex.what(),
                false);
        }
    }
    catch (std::exception const & ex)
    {
        OnError(std::string("Could not take screenshot: ") + ex.what(), false);
        return;
    }
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

void MainFrame::OnAutoFocusAtShipLoadMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mUIPreferencesManager);
    mUIPreferencesManager->SetDoAutoFocusOnShipLoad(mAutoFocusAtShipLoadMenuItem->IsChecked());
}

void MainFrame::OnResetViewMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->ResetView();
}

void MainFrame::OnShipViewMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    if (mShipViewExteriorMenuItem->IsChecked())
    {
        assert(!mShipViewInteriorMenuItem->IsChecked());
        mGameController->SetShipViewMode(ShipViewModeType::Exterior);
    }
    else
    {
        assert(mShipViewInteriorMenuItem->IsChecked());
        mGameController->SetShipViewMode(ShipViewModeType::Interior);
    }
}

static float constexpr TimeOfDayStep = 0.01f;

void MainFrame::OnTimeOfDayUpMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    float const newTimeOfDay = std::min(1.0f, mGameController->GetTimeOfDay() + TimeOfDayStep);
    mGameController->SetTimeOfDay(newTimeOfDay);
}

void MainFrame::OnTimeOfDayDownMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    float const newTimeOfDay = std::max(0.0f, mGameController->GetTimeOfDay() - TimeOfDayStep);
    mGameController->SetTimeOfDay(newTimeOfDay);
}

void MainFrame::OnFullTimeOfDayMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->ToggleToFullDayOrNight();
}

void MainFrame::OnPauseMenuItemSelected(wxCommandEvent & /*event*/)
{
    SetPaused(mPauseMenuItem->IsChecked());
}

void MainFrame::OnStepMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);

    mGameController->PulseUpdateAtNextGameIteration();
}

void MainFrame::OnAddHumanNpcGroupMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->AddNpcGroup(NpcKindType::Human);

    // Switch to interior view
    assert(!!mGameController);
    if (mGameController->GetShipViewMode() != ShipViewModeType::Interior)
    {
        mGameController->SetShipViewMode(ShipViewModeType::Interior);
        mShipViewInteriorMenuItem->Check(true);
    }
}

void MainFrame::OnAddFurnitureNpcGroupMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->AddNpcGroup(NpcKindType::Furniture);

    // Switch to interior view
    assert(!!mGameController);
    if (mGameController->GetShipViewMode() != ShipViewModeType::Interior)
    {
        mGameController->SetShipViewMode(ShipViewModeType::Interior);
        mShipViewInteriorMenuItem->Check(true);
    }
}

void MainFrame::OnSelectNextNpcMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->SelectNextNpc();
}

void MainFrame::OnDeselectNpcMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->SelectNpc(std::nullopt);
}

void MainFrame::OnTriggerLightningMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->TriggerLightning();
}

void MainFrame::OnRCBombDetonateMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->DetonateRCBombs();
}

void MainFrame::OnAntiMatterBombDetonateMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mGameController);
    mGameController->DetonateAntiMatterBombs();
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

//////////

void MainFrame::OnOpenSettingsWindowMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mSettingsDialog)
    {
        mSettingsDialog = std::make_unique<SettingsDialog>(
            this,
            *mSettingsManager,
            *mGameController,
            mGameAssetManager);
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
            *mUIPreferencesManager,
            [this]()
            {
                this->ReconciliateUIWithUIPreferencesAndSettings();
            },
            [this]()
            {
                this->ReloadCurrentShip();
            },
            mGameAssetManager);
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

void MainFrame::OnShowProbePanelMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mProbePanel);

    if (mShowProbePanelMenuItem->IsChecked())
    {
        mMainPanelSizer->Show(mProbePanel);
    }
    else
    {
        mMainPanelSizer->Hide(mProbePanel);
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
            mGameAssetManager,
            mLocalizationManager);
    }

    mHelpDialog->ShowModal();
}

void MainFrame::OnShipBuilderNewShipMenuItemSelected(wxCommandEvent & /*event*/)
{
    SwitchToShipBuilderForNewShip();
}

void MainFrame::OnShipBuilderEditShipMenuItemSelected(wxCommandEvent & /*event*/)
{
    SwitchToShipBuilderForCurrentShip();
}

void MainFrame::OnAboutMenuItemSelected(wxCommandEvent & /*event*/)
{
    AboutDialog aboutDialog(this);

    aboutDialog.ShowModal();
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
            checkDlg.GetHasVersionOutcome()->HtmlFeatures,
            nullptr);

        newVersionDlg.ShowModal();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void MainFrame::RunGameIteration()
{
    assert(mUIPreferencesManager);

    if (mUpdateChecker && !IsPaused()) // A bit of a hack: if the LoadShip dialog is open while we ShowModal() the NewVersion dialog, the UI loop freezes
    {
        // We are checking for updates...
        // ...check whether the...check has completed
        auto const outcome = mUpdateChecker->GetOutcome();
        if (outcome.has_value())
        {
            // Forget about the update check
            // (immediately, to prevent OnIdle events firing while the dialog is open to re-enter here)
            mUpdateChecker.reset();

            // Check completed...
            // ...check if it's an interesting new version
            if (outcome->OutcomeType == UpdateChecker::UpdateCheckOutcomeType::HasVersion
                && *(outcome->LatestVersion) > CurrentGameVersion
                && !mUIPreferencesManager->IsUpdateBlacklisted(*(outcome->LatestVersion)))
            {
                //
                // Notify user of new version
                //

                NewVersionDisplayDialog dlg(
                    this,
                    *(outcome->LatestVersion),
                    outcome->HtmlFeatures,
                    mUIPreferencesManager.get());

                dlg.ShowModal();
            }
        }
    }

#if FS_IS_OS_WINDOWS()
    if (mHasStartupTipBeenChecked)
    {
        //
        // On Windows, timer events (appear to be) queued after GUI events, hence
        // even if the timer fires *during* a game iteration, its event will be
        // processed after outstanding GUI events.
        // The same does not appear to hold for GTK; if a timer fires during the
        // game iteration, its event will be processed immediately after the
        // current handler, and thus no GUI events will be processed, starving
        // (and freezing) the UI as a result (see also Issue#41 on GitHub).
        //
        // This, coupled with the fact that windows timers have a minimum granularity
        // matching our frame rate (1/64th of a second), makes it so that starting a
        // timer here is the best strategy to ensure a steady 64-FPS rate of game
        // iteration callbacks.
        //

        PostGameStepTimer(mGameTimerDuration);
    }
#else
    std::chrono::steady_clock::time_point const startTimestamp = std::chrono::steady_clock::now();
#endif

    assert(mGameController);
    assert(mToolController);

    // Update SHIFT key state
    OnShiftKeyObservation(wxGetKeyState(WXK_SHIFT));

    //
    // Run a game step
    //

    try
    {
        // Update tool controller
        mToolController->UpdateSimulation(mGameController->GetCurrentSimulationTime());

        // Update and render
        ////LogMessage("TODOTEST: MainFrame::OnGameTimerTrigger: Running game iteration; IsSplashShown=",
        ////    !!mSplashScreenDialog ? std::to_string(mSplashScreenDialog->IsShown()) : "<NoSplash>",
        ////    " IsMainGLCanvasShown=", !!mMainGLCanvas ? std::to_string(mMainGLCanvas->IsShown()) : "<NoCanvas>",
        ////    " IsFrameShown=", std::to_string(this->IsShown()));
        mGameController->RunGameIteration();

        // Update probe panel
        assert(!!mProbePanel);
        mProbePanel->UpdateSimulation();

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
            // Set canvas' background color to sky color
            {
                auto const skyColor = mGameController->GetFlatSkyColor();
                mMainGLCanvas->SetBackgroundColour(wxColor(skyColor.r, skyColor.g, skyColor.b));
                mMainGLCanvas->ClearBackground();
            }

            StartupTipDialog startupTipDialog(
                this,
                *mUIPreferencesManager,
                mGameAssetManager,
                mLocalizationManager);

            startupTipDialog.ShowModal();
        }

        // Don't check for startup tips anymore
        mHasStartupTipBeenChecked = true;

#if FS_IS_OS_WINDOWS()
        // Post next game step now
        PostGameStepTimer(mGameTimerDuration);
#endif
    }

#if !FS_IS_OS_WINDOWS()
    //
    // Run next game step after the remaining time
    //

    auto const nextIterationDelay =
        mGameTimerDuration
        - std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTimestamp);

    if (nextIterationDelay.count() <= 0)
    {
        // It took us longer than the timer duration, hence run a game iteration
        // as soon as possible, but still giving the event loop some time to drain
        // UI events
        wxWakeUpIdle(); // Ensure an Idle event is produced even if we are...idle
    }
    else
    {
        // Schedule the next game iteration after the remaining time
        PostGameStepTimer(nextIterationDelay);
    }
#endif
}

void MainFrame::OnError(
    wxString const & message,
    bool die)
{
    // Stop game
    FreezeGame();

    // Show message
    wxMessageBox(message, _("Maritime Disaster"), wxICON_ERROR);

    // Restart game
    ThawGame();

    if (die)
    {
        // Exit
        Close(true);
    }
}

void MainFrame::FreezeGame()
{
    assert(!mIsGameFrozen);

    //
    // Prevent OnIdle, among other things, from running a game iteration
    //

    mIsGameFrozen = true;

    //
    // Stop timers
    //

    if (mGameTimer)
    {
        mGameTimer->Stop();
    }

    if (mLowFrequencyTimer)
    {
        mLowFrequencyTimer->Stop();
    }

    //
    // Freeze game controller
    //

    if (mGameController)
    {
        mGameController->Freeze();
    }

    //
    // Stop sounds
    //

    if (mSoundController)
    {
        mSoundController->Reset();
    }

    if (mMusicController)
    {
        mMusicController->Reset();
    }
}

void MainFrame::ThawGame()
{
    assert(mIsGameFrozen);

    //
    // Thaw game controller
    //

    if (mGameController)
    {
        mGameController->Thaw();
    }

    //
    // Restart timers
    //

    if (mGameTimer)
    {
        PostGameStepTimer(mGameTimerDuration);
    }

    if (mLowFrequencyTimer)
    {
        StartLowFrequencyTimer();
    }

    //
    // Re-allow OnIdle, among other things, to run a game iteration
    //

    mIsGameFrozen = false;
}

void MainFrame::SetPaused(bool isPaused)
{
    if (isPaused)
    {
        if (0 == mPauseCount)
        {
            // Set pause

            if (mGameController)
                mGameController->SetPaused(true);

            if (mSoundController)
                mSoundController->SetPaused(true);

            if (mMusicController)
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

            if (mGameController)
                mGameController->SetPaused(false);

            if (mSoundController)
                mSoundController->SetPaused(false);

            if (mMusicController)
                mMusicController->SetPaused(false);

            mStepMenuItem->Enable(false);
        }
    }
}

bool MainFrame::IsPaused() const
{
    return mPauseCount > 0;
}

void MainFrame::PostGameStepTimer(std::chrono::milliseconds duration)
{
    assert(mGameTimer);

    mGameTimer->Start(
        duration.count(),
        true); // One-shot
}

void MainFrame::StartLowFrequencyTimer()
{
    assert(mLowFrequencyTimer);

    mLowFrequencyTimer->Start(
        1000,
        false); // Continuous
}

void MainFrame::ResetShipUIState()
{
    // Reset conditional state, waiting for (eventual) communication
    mScareFishMenuItem->Enable(false);
    mRCBombsDetonateMenuItem->Enable(false);
    mAntiMatterBombsDetonateMenuItem->Enable(false);
    mTriggerStormMenuItem->Enable(true);

    ReconciliateShipViewModeWithCurrentTool(); // We might be on Interior view without having an NPC tool (e.g. because of a recent AddGroup)
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

void MainFrame::SelectTool(ToolType toolType, bool isNpcTool)
{
    assert(!!mToolController);
    mToolController->SetTool((toolType));

    if (isNpcTool)
    {
        OnNpcToolSelected(toolType);
    }
    else
    {
        OnNonNpcToolSelected(toolType);
    }
}

void MainFrame::OnNonNpcToolSelected(ToolType toolType)
{
    // Reconciliate menu items
    for (auto & entry : mNonNpcToolMenuItems)
    {
        std::get<1>(entry)->Check(std::get<0>(entry) == toolType);
    }
    for (auto & entry : mNpcToolMenuItems)
    {
        auto const menuToolType = std::get<0>(entry);
        if (menuToolType == ToolType::PlaceHumanNpc)
        {
            SetMenuItemChecked(mAddHumanNpcMenuItem, mAddHumanNpcUncheckedIcon, mAddHumanNpcCheckedIcon, false);
        }
        else if (menuToolType == ToolType::PlaceFurnitureNpc)
        {
            SetMenuItemChecked(mAddFurnitureNpcMenuItem, mAddFurnitureNpcUncheckedIcon, mAddFurnitureNpcCheckedIcon, false);
        }
        else
        {
            std::get<1>(entry)->Check(false);
        }
    }

    // Reconciliate AddNpc sub-menu items
    ReconciliateAddNpcSubItems(toolType);

    // Reconciliate ship view mode
    ReconciliateShipViewModeWithCurrentTool();
}

void MainFrame::OnNpcToolSelected(ToolType toolType)
{
    // Reconciliate menu items
    for (auto & entry : mNonNpcToolMenuItems)
    {
        std::get<1>(entry)->Check(false);
    }
    for (auto & entry : mNpcToolMenuItems)
    {
        auto const menuToolType = std::get<0>(entry);
        if (menuToolType == ToolType::PlaceHumanNpc)
        {
            SetMenuItemChecked(mAddHumanNpcMenuItem, mAddHumanNpcUncheckedIcon, mAddHumanNpcCheckedIcon, menuToolType == toolType);
        }
        else if (menuToolType == ToolType::PlaceFurnitureNpc)
        {
            SetMenuItemChecked(mAddFurnitureNpcMenuItem, mAddFurnitureNpcUncheckedIcon, mAddFurnitureNpcCheckedIcon, menuToolType == toolType);
        }
        else
        {
            std::get<1>(entry)->Check(menuToolType == toolType);
        }
    }

    // Reconciliate AddNpc sub-menu items
    ReconciliateAddNpcSubItems(toolType);

    // Reconciliate ship view mode
    ReconciliateShipViewModeWithCurrentTool();
}

void MainFrame::ReconciliateUIWithUIPreferencesAndSettings()
{
    assert(mGameController);
    assert(mUIPreferencesManager);

    mAutoFocusAtShipLoadMenuItem->Check(mUIPreferencesManager->GetDoAutoFocusOnShipLoad());

    mContinuousAutoFocusOnShipMenuItem->Check(mUIPreferencesManager->GetAutoFocusTarget() == AutoFocusTargetKindType::Ship);
    mContinuousAutoFocusOnSelectedNpcMenuItem->Check(mUIPreferencesManager->GetAutoFocusTarget() == AutoFocusTargetKindType::SelectedNpc);
    mContinuousAutoFocusOnNothingMenuItem->Check(mUIPreferencesManager->GetAutoFocusTarget() == std::nullopt);

    mShipViewExteriorMenuItem->Check(mGameController->GetShipViewMode() == ShipViewModeType::Exterior);
    mShipViewInteriorMenuItem->Check(mGameController->GetShipViewMode() == ShipViewModeType::Interior);

    mFullScreenMenuItem->Enable(!mUIPreferencesManager->GetStartInFullScreen());
    mNormalScreenMenuItem->Enable(mUIPreferencesManager->GetStartInFullScreen());

    mPreviousShipLoadSpecs = mUIPreferencesManager->GetLastShipLoadedSpecifications();
    mReloadPreviousShipMenuItem->Enable(mPreviousShipLoadSpecs.has_value());

    mShowStatusTextMenuItem->Check(mUIPreferencesManager->GetShowStatusText());
    mShowExtendedStatusTextMenuItem->Check(mUIPreferencesManager->GetShowExtendedStatusText());
    mMuteMenuItem->Check(mUIPreferencesManager->GetGlobalMute());
}

void MainFrame::ReconciliateUIWithFishPresence(bool areFishPresent)
{
    if (areFishPresent)
    {
        if (!mScareFishMenuItem->IsEnabled())
        {
            mScareFishMenuItem->Enable(true);
        }
    }
    else
    {
        if (mScareFishMenuItem->IsEnabled())
        {
            mScareFishMenuItem->Enable(false);
        }

        assert(!!mToolController);
        if (mToolController->GetCurrentTool() == ToolType::ScareFish)
        {
            SelectTool(InitialNonNpcToolType, false);
        }
    }
}

void MainFrame::ReconciliateUIWithNpcPresence(bool areNpcsPresent)
{
    if (areNpcsPresent)
    {
        // Enable NPC menu items

        if (!mContinuousAutoFocusOnSelectedNpcMenuItem->IsEnabled())
        {
            mContinuousAutoFocusOnSelectedNpcMenuItem->Enable(true);
        }

        if (!mMoveNpcMenuItem->IsEnabled())
        {
            mMoveNpcMenuItem->Enable(true);
        }

        if (!mRemoveNpcMenuItem->IsEnabled())
        {
            mRemoveNpcMenuItem->Enable(true);
        }

        if (!mTurnaroundNpcMenuItem->IsEnabled())
        {
            mTurnaroundNpcMenuItem->Enable(true);
        }

        if (!mFollowNpcMenuItem->IsEnabled())
        {
            mFollowNpcMenuItem->Enable(true);
        }

        if (!mSelectNextNpcMenuItem->IsEnabled())
        {
            mSelectNextNpcMenuItem->Enable(true);
        }

        if (!mDeselectNpcMenuItem->IsEnabled())
        {
            mDeselectNpcMenuItem->Enable(mGameController->GetCurrentlySelectedNpc().has_value());
        }
    }
    else
    {
        // Disable NPC menu items, resetting tool in case it's not allowed anymore

        if (mContinuousAutoFocusOnSelectedNpcMenuItem->IsEnabled())
        {
            mContinuousAutoFocusOnSelectedNpcMenuItem->Enable(false);
        }

        if (mMoveNpcMenuItem->IsEnabled())
        {
            mMoveNpcMenuItem->Enable(false);
        }

        if (mRemoveNpcMenuItem->IsEnabled())
        {
            mRemoveNpcMenuItem->Enable(false);
        }

        if (mTurnaroundNpcMenuItem->IsEnabled())
        {
            mTurnaroundNpcMenuItem->Enable(false);
        }

        if (mFollowNpcMenuItem->IsEnabled())
        {
            mFollowNpcMenuItem->Enable(false);
        }

        if (mSelectNextNpcMenuItem->IsEnabled())
        {
            mSelectNextNpcMenuItem->Enable(false);
        }

        if (mDeselectNpcMenuItem->IsEnabled())
        {
            mDeselectNpcMenuItem->Enable(false);
        }

        // If current tool is a tool that requires NPCs, toggle it out
        assert(!!mToolController);
        auto const currentTool = mToolController->GetCurrentTool();
        if (currentTool == ToolType::MoveNpc || currentTool == ToolType::TurnaroundNpc || currentTool == ToolType::FollowNpc)
        {
            SelectTool(InitialNonNpcToolType, false);
        }
    }
}

void MainFrame::ReconciliateUIWithNpcSelection(bool areNpcsSelected)
{
    mDeselectNpcMenuItem->Enable(areNpcsSelected);
}

void MainFrame::ReconciliateAddNpcSubItems(ToolType toolType)
{
    assert(!!mToolController);

    auto const selectedHumanSubKind = mToolController->GetCurrentAddHumanNpcSubKind();
    for (auto & entry : mAddHumanNpcSubMenuItems)
    {
        std::get<1>(entry)->Check(toolType == ToolType::PlaceHumanNpc && std::get<0>(entry) == selectedHumanSubKind);
    }

    auto const selectedFurnitureSubKind = mToolController->GetCurrentAddFurnitureNpcSubKind();
    for (auto & entry : mAddFurnitureNpcSubMenuItems)
    {
        std::get<1>(entry)->Check(toolType == ToolType::PlaceFurnitureNpc && std::get<0>(entry) == selectedFurnitureSubKind);
    }
}

void MainFrame::ReconciliateUIWithAutoFocusTarget(std::optional<AutoFocusTargetKindType> target)
{
    if (target == AutoFocusTargetKindType::Ship)
    {
        if (!mContinuousAutoFocusOnShipMenuItem->IsChecked())
        {
            mContinuousAutoFocusOnShipMenuItem->Check(true);
        }
    }
    else if (target == AutoFocusTargetKindType::SelectedNpc)
    {
        if (!mContinuousAutoFocusOnSelectedNpcMenuItem->IsChecked())
        {
            mContinuousAutoFocusOnSelectedNpcMenuItem->Check(true);
        }
    }
    else
    {
        assert(!target.has_value());

        if (!mContinuousAutoFocusOnNothingMenuItem->IsChecked())
        {
            mContinuousAutoFocusOnNothingMenuItem->Check(true);
        }
    }
}

void MainFrame::ReconciliateShipViewModeWithCurrentTool()
{
    assert(!!mToolController);
    auto const currentTool = mToolController->GetCurrentTool();
    if (currentTool.has_value())
    {
        bool const isNpcTool = static_cast<std::uint32_t>(*currentTool) >= static_cast<std::uint32_t>(ToolType::_FirstNpcTool);

        assert(!!mGameController);
        if (isNpcTool)
        {
            // Switch to interior view
            if (mGameController->GetShipViewMode() != ShipViewModeType::Interior)
            {
                mGameController->SetShipViewMode(ShipViewModeType::Interior);
                mShipViewInteriorMenuItem->Check(true);
            }
        }
        else
        {
            // Switch to exterior view, if enabled
            assert(!!mUIPreferencesManager);
            if (mGameController->GetShipViewMode() != ShipViewModeType::Exterior
                && mUIPreferencesManager->GetDoAutoToggleToExteriorViewWhenNonNpcToolIsSelected())
            {
                mGameController->SetShipViewMode(ShipViewModeType::Exterior);
                mShipViewExteriorMenuItem->Check(true);
            }
        }
    }
}

void MainFrame::RebuildNpcMenus()
{
    assert(mGameController);

    auto const language = mLocalizationManager.GetEnforcedLanguageIdentifier();

    // Humans
    {
        // Clear first

        auto const menuItems = mHumanNpcSubMenu->GetMenuItems();
        for (auto const & item : menuItems)
        {
            wxMenuItem * removedItem = mHumanNpcSubMenu->Remove(item);
            delete removedItem;
        }

        mAddHumanNpcSubMenuItems.clear();

        // Add all

        auto addMenu = [this](std::optional<NpcSubKindIdType> subKindId, wxString const & label)
            {
                auto const commandId = wxNewId();
                auto * subMenuItem = new wxMenuItem(nullptr, commandId, label, wxEmptyString, wxITEM_CHECK);
                mHumanNpcSubMenu->Append(subMenuItem);
                mHumanNpcSubMenu->Bind(
                    wxEVT_COMMAND_MENU_SELECTED,
                    [this, subKindId](wxCommandEvent &)
                    {
                        // Set tool
                        assert(!!mToolController);
                        mToolController->SetPlaceHumanNpcTool(subKindId);

                        // Reconciliate
                        OnNpcToolSelected(ToolType::PlaceHumanNpc);
                    },
                    commandId);

                mAddHumanNpcSubMenuItems.push_back({ subKindId , subMenuItem });
            };

        addMenu(std::nullopt, _("Random"));

        for (std::uint32_t iRole = 0; iRole <= static_cast<std::uint32_t>(NpcHumanRoleType::_Last); ++iRole)
        {
            mHumanNpcSubMenu->AppendSeparator();

            for (auto const & subKindInfo : mGameController->GetHumanNpcSubKinds(static_cast<NpcHumanRoleType>(iRole), language))
            {
                addMenu(std::get<0>(subKindInfo), wxString::FromUTF8(std::get<1>(subKindInfo)));
            }
        }
    }

    // Furniture
    {
        // Clear first

        auto const menuItems = mFurnitureNpcSubMenu->GetMenuItems();
        for (auto const & item : menuItems)
        {
            wxMenuItem * removedItem = mFurnitureNpcSubMenu->Remove(item);
            delete removedItem;
        }

        mAddFurnitureNpcSubMenuItems.clear();

        // Add all

        auto addMenu = [this](std::optional<NpcSubKindIdType> subKindId, wxString const & label)
            {
                auto const commandId = wxNewId();
                auto * subMenuItem = new wxMenuItem(nullptr, commandId, label, wxEmptyString, wxITEM_CHECK);
                mFurnitureNpcSubMenu->Append(subMenuItem);
                mFurnitureNpcSubMenu->Bind(
                    wxEVT_COMMAND_MENU_SELECTED,
                    [this, subKindId](wxCommandEvent &)
                    {
                        // Set tool
                        assert(!!mToolController);
                        mToolController->SetPlaceFurnitureNpcTool(subKindId);

                        // Reconciliate
                        OnNpcToolSelected(ToolType::PlaceFurnitureNpc);
                    },
                    commandId);

                mAddFurnitureNpcSubMenuItems.push_back({ subKindId , subMenuItem });
            };

        addMenu(std::nullopt, _("Random"));

        for (std::uint32_t iRole = 0; iRole <= static_cast<std::uint32_t>(NpcFurnitureRoleType::_Last); ++iRole)
        {
            mFurnitureNpcSubMenu->AppendSeparator();

            for (auto const & subKindInfo : mGameController->GetFurnitureNpcSubKinds(static_cast<NpcFurnitureRoleType>(iRole), language))
            {
                addMenu(std::get<0>(subKindInfo), wxString::FromUTF8(std::get<1>(subKindInfo)));
            }
        }
    }
}

std::filesystem::path MainFrame::ChooseDefaultShip(GameAssetManager const & gameAssetManager)
{
    //
    // Decide default ship based on day
    //

    std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto const tm = std::localtime(&now_c);

    if (tm->tm_mon == 0 && tm->tm_mday == 17)  // Jan 17: Floating Sandbox's birthday
        return gameAssetManager.GetFallbackShipDefinitionFilePath();
    else if (tm->tm_mon == 3 && tm->tm_mday == 1)  // April 1
        return gameAssetManager.GetApril1stShipDefinitionFilePath();
    else if (tm->tm_mon == 11 && tm->tm_mday >= 24) // Winter holidays
        return gameAssetManager.GetHolidaysShipDefinitionFilePath();
    else
        return gameAssetManager.GetDefaultShipDefinitionFilePath(); // Just default
}

void MainFrame::LoadShip(
    ShipLoadSpecifications const & loadSpecs,
    bool isFromUser)
{
    //
    // Reset
    //

    assert(mToolController);
    mToolController->ResetStart();

    assert(mSoundController);
    mSoundController->Reset();

    assert(mMusicController);
    mMusicController->Reset();

    ResetShipUIState(); // Important: now, before ship load generates events

    //
    // Load ship
    //

    try
    {
        // Load
        assert(mGameController);
        auto const shipMetadata = mGameController->ResetAndLoadShip(loadSpecs, mGameAssetManager);

        // Succeeded
        OnShipLoaded(loadSpecs);

        // Open description, if a description exists and the user allows
        if (isFromUser
            && shipMetadata.Description.has_value()
            && mUIPreferencesManager->GetShowShipDescriptionsAtShipLoad())
        {
            ShipDescriptionDialog shipDescriptionDialog(
                this,
                shipMetadata,
                true,
                mGameAssetManager);

            shipDescriptionDialog.ShowModal();

            // Store user preference, in case they made a choice
            auto const showDescriptionsUserPreference = shipDescriptionDialog.GetShowDescriptionsUserPreference();
            if (showDescriptionsUserPreference.has_value())
            {
                mUIPreferencesManager->SetShowShipDescriptionsAtShipLoad(*showDescriptionsUserPreference);
            }
        }
    }
    catch (UserGameException const & exc)
    {
        OnError(mLocalizationManager.MakeErrorMessage(exc), false);
    }
    catch (std::exception const & ex)
    {
        OnError(ex.what(), false);
    }

    assert(mToolController);
    mToolController->ResetEnd();
}

void MainFrame::ReloadCurrentShip()
{
    assert(mCurrentShipLoadSpecs.has_value());

    LoadShip(*mCurrentShipLoadSpecs, false);
}

void MainFrame::OnShipLoaded(ShipLoadSpecifications loadSpecs)
{
    //
    // Check whether the current ship may become the "previous" ship
    //

    if (mCurrentShipLoadSpecs.has_value()
        && loadSpecs.DefinitionFilepath != mCurrentShipLoadSpecs->DefinitionFilepath)
    {
        mPreviousShipLoadSpecs = mCurrentShipLoadSpecs;

        mReloadPreviousShipMenuItem->Enable(true);
    }

    //
    // Remember the current ship load specs
    //

    mCurrentShipLoadSpecs = loadSpecs;

    assert(mUIPreferencesManager);
    mUIPreferencesManager->SetLastShipLoadedSpecifications(loadSpecs);
}

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

void MainFrame::SwitchToShipBuilderForNewShip()
{
    // Freeze game
    FreezeGame();

    // Hide us
    Show(false);

    // Open ShipBuilder frame for new ship
    assert(mShipBuilderMainFrame);
    mShipBuilderMainFrame->OpenForNewShip(mUIPreferencesManager->GetDisplayUnitsSystem());
}

void MainFrame::SwitchToShipBuilderForCurrentShip()
{
    // Freeze game
    FreezeGame();

    // Hide us
    Show(false);

    // Open ShipBuilder frame for editing current ship
    assert(mShipBuilderMainFrame);
    assert(mCurrentShipLoadSpecs.has_value());
    mShipBuilderMainFrame->OpenForLoadShip(mCurrentShipLoadSpecs->DefinitionFilepath, mUIPreferencesManager->GetDisplayUnitsSystem());
}

void MainFrame::SwitchFromShipBuilder(std::optional<std::filesystem::path> shipFilePath)
{
    // Show us
    Show(true);

    // Make ourselves the topmost frame
    mMainApp->SetTopWindow(this);
    mMainApp->Yield();

    // Switch back to our OpenGL context, now that we've left the builder's one
    mGameController->RebindOpenGLContext();

    if (shipFilePath.has_value())
    {
        // Load the ship
        LoadShip(ShipLoadSpecifications(*shipFilePath), false);
    }
    else
    {
        // Reload current ship - if still exists (might have been renamed)
        assert(mCurrentShipLoadSpecs.has_value());
        if (std::filesystem::exists(mCurrentShipLoadSpecs->DefinitionFilepath))
        {
            LoadShip(*mCurrentShipLoadSpecs, false);
        }
        else
        {
            ShipLoadSpecifications loadSpecs(ChooseDefaultShip(mGameAssetManager));
            LoadShip(loadSpecs, false);
        }
    }

    // Restart
    ThawGame();
}

std::tuple<wxBitmap, wxBitmap> MainFrame::MakeMenuBitmaps(std::string const & iconName) const
{
    auto img = wxImage(
        mGameAssetManager.GetIconFilePath(iconName).string(),
        wxBITMAP_TYPE_PNG);
    img.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
    auto img2 = WxHelpers::RetintCursorImage(img, rgbColor(0x00, 0x90, 0x00));

    return { wxBitmap(img), wxBitmap(img2) };
}

void MainFrame::SetMenuItemChecked(
    wxMenuItem * menuItem,
    wxBitmap & uncheckedBitmap,
    wxBitmap & checkedBitmap,
    bool isChecked)
{
    menuItem->SetBitmap(isChecked ? checkedBitmap : uncheckedBitmap);
}

void MainFrame::OnShiftKeyObservation(bool isDown)
{
    if (mGameController)
    {
        if (!mGameController->IsShiftOn()) // We do not care about SHIFT if mid-mouse button is down
        {
            if (isDown)
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
        }
    }
}

void MainFrame::OnMidMouseButtonDown()
{
    if (mGameController)
    {
        if (!mIsShiftKeyDown) // We do not care about mid button if SHIFT is down
        {
            // Toggle sticky shift ("shift lock")

            if (!mGameController->IsShiftOn())
            {
                mGameController->SetShiftOn(true);
                mToolController->OnShiftKeyDown();
            }
            else
            {
                mGameController->SetShiftOn(false);
                mToolController->OnShiftKeyUp();
            }
        }
    }
}
