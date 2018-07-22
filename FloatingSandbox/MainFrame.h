/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "AboutDialog.h"
#include "EventTickerPanel.h"
#include "HelpDialog.h"
#include "LoggingDialog.h"
#include "SettingsDialog.h"
#include "SoundController.h"
#include "ToolController.h"

#include <GameLib/GameController.h>
#include <GameLib/GameWallClock.h>
#include <GameLib/IGameEventHandler.h>
#include <GameLib/ResourceLoader.h>

#include <wx/filedlg.h>
#include <wx/frame.h>
#include <wx/glcanvas.h>
#include <wx/menu.h>
#include <wx/timer.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

/*
 * The main window of the game's GUI.
 */
class MainFrame : public wxFrame, public IGameEventHandler
{
public:

    MainFrame(wxApp * mainApp);

    virtual ~MainFrame();

private:

    //
    // Canvas
    //

    std::unique_ptr<wxGLCanvas> mMainGLCanvas;
    std::unique_ptr<wxGLContext> mMainGLCanvasContext;

    //
    // Controls that we're interacting with
    //

    wxBoxSizer * mMainFrameSizer;
    wxMenuItem * mPauseMenuItem;    
    wxMenu * mToolsMenu;
    wxMenuItem * mRCBombsDetonateMenuItem;
    wxMenuItem * mShowEventTickerMenuItem;
    wxMenuItem * mMuteMenuItem;
    std::unique_ptr<EventTickerPanel> mEventTickerPanel;

    //
    // Dialogs
    //

    std::unique_ptr<wxFileDialog> mFileOpenDialog;
    std::unique_ptr<LoggingDialog> mLoggingDialog;
    std::unique_ptr<SettingsDialog> mSettingsDialog;
    std::unique_ptr<HelpDialog> mHelpDialog;
    std::unique_ptr<AboutDialog> mAboutDialog;

    //
    // Timers
    //

    std::unique_ptr<wxTimer> mPostInitializeTimer;
    std::unique_ptr<wxTimer> mGameTimer;
    std::unique_ptr<wxTimer> mLowFrequencyTimer;

private:

    //
    // Event handlers
    //

    // App
    void OnPostInitializeTrigger(wxTimerEvent& event);
    void OnMainFrameClose(wxCloseEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnPaint(wxPaintEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnGameTimerTrigger(wxTimerEvent& event);
    void OnLowFrequencyTimerTrigger(wxTimerEvent& event);

    // Main GL canvas
    void OnMainGLCanvasResize(wxSizeEvent& event);
    void OnMainGLCanvasLeftDown(wxMouseEvent& event);
    void OnMainGLCanvasLeftUp(wxMouseEvent& event);
    void OnMainGLCanvasRightDown(wxMouseEvent& event);
    void OnMainGLCanvasRightUp(wxMouseEvent& event);
    void OnMainGLCanvasMouseMove(wxMouseEvent& event);
    void OnMainGLCanvasMouseWheel(wxMouseEvent& event);

    // Menu
    void OnZoomInMenuItemSelected(wxCommandEvent& event);
    void OnZoomOutMenuItemSelected(wxCommandEvent& event);
    void OnAmbientLightUpMenuItemSelected(wxCommandEvent& event);
    void OnAmbientLightDownMenuItemSelected(wxCommandEvent& event);
    void OnPauseMenuItemSelected(wxCommandEvent& event);
    void OnResetViewMenuItemSelected(wxCommandEvent& event);    
    void OnLoadShipMenuItemSelected(wxCommandEvent& event);
    void OnReloadLastShipMenuItemSelected(wxCommandEvent& event);
    void OnSmashMenuItemSelected(wxCommandEvent& event);
    void OnSliceMenuItemSelected(wxCommandEvent& event);
    void OnGrabMenuItemSelected(wxCommandEvent& event);
    void OnSwirlMenuItemSelected(wxCommandEvent& event);    
    void OnPinMenuItemSelected(wxCommandEvent& event);
    void OnTimerBombMenuItemSelected(wxCommandEvent& event);
    void OnRCBombMenuItemSelected(wxCommandEvent& event);
    void OnRCBombDetonateMenuItemSelected(wxCommandEvent& event);
    void OnOpenSettingsWindowMenuItemSelected(wxCommandEvent& event);
    void OnOpenLogWindowMenuItemSelected(wxCommandEvent& event);
    void OnShowEventTickerMenuItemSelected(wxCommandEvent& event);
    void OnMuteMenuItemSelected(wxCommandEvent& event);
    void OnHelpMenuItemSelected(wxCommandEvent& event);
    void OnAboutMenuItemSelected(wxCommandEvent& event);

    //
    // Game event handler
    //

    virtual void OnGameReset() override
    {
        mCurrentShipNames.clear();
    }

    virtual void OnShipLoaded(
        unsigned int /*id*/,
        std::string const & name) override
    {
        mCurrentShipNames.push_back(name);
    }

    virtual void OnBombPlaced(
        ObjectId /*bombId*/,
        BombType bombType,
        bool /*isUnderwater*/) override
    {
        if (BombType::RCBomb == bombType)
        {
            ++mCurrentRCBombCount;
            mRCBombsDetonateMenuItem->Enable(mCurrentRCBombCount > 0);
        }
    }

    virtual void OnBombRemoved(
        ObjectId /*bombId*/,
        BombType bombType,
        std::optional<bool> /*isUnderwater*/) override
    {
        if (BombType::RCBomb == bombType)
        {
            assert(mCurrentRCBombCount > 0u);
            --mCurrentRCBombCount;
            mRCBombsDetonateMenuItem->Enable(mCurrentRCBombCount > 0);
        }
    }

private:

    void ResetState();
    void SetFrameTitle();
    bool IsPaused();
    void DoGameStep();
    void RenderGame();
    void Die(std::string const & message);

private:

    wxApp * const mMainApp;

    //
    // Helpers
    //

    std::shared_ptr<ResourceLoader> mResourceLoader;
    std::shared_ptr<GameController> mGameController;
    std::unique_ptr<SoundController> mSoundController;
    std::unique_ptr<ToolController> mToolController;


    //
    // State
    //

    std::vector<std::string> mCurrentShipNames;
    size_t mCurrentRCBombCount;
    bool mIsShiftKeyDown;


    //
    // Stats
    //

    uint64_t mTotalFrameCount;  
    uint64_t mLastFrameCount;
    std::chrono::steady_clock::time_point mStatsOriginTimestampReal;
    std::chrono::steady_clock::time_point mStatsLastTimestampReal;
    GameWallClock::time_point mStatsOriginTimestampGame;
};
