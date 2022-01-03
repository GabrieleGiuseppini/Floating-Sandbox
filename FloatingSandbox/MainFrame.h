/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "DebugDialog.h"
#include "EventTickerPanel.h"
#include "HelpDialog.h"
#include "MusicController.h"
#include "PreferencesDialog.h"
#include "ProbePanel.h"
#include "SettingsDialog.h"
#include "SettingsManager.h"
#include "SoundController.h"
#include "SwitchboardPanel.h"
#include "ToolController.h"
#include "UIPreferencesManager.h"
#include "UpdateChecker.h"

#include <ShipBuilderLib/MainFrame.h>

#include <UILib/LocalizationManager.h>
#include <UILib/LoggingDialog.h>
#include <UILib/ShipLoadDialog.h>
#include <UILib/UnFocusablePanel.h>

#include <Game/GameController.h>
#include <Game/IGameEventHandlers.h>
#include <Game/ResourceLocator.h>

#include "SplashScreenDialog.h" // Need to include this (which includes wxGLCanvas) *after* our glad.h has been included,
 // so that wxGLCanvas ends up *not* including the system's OpenGL header but glad's instead

#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/timer.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

/*
 * The main window of the game's GUI.
 */
class MainFrame final
    : public wxFrame
    , public ILifecycleGameEventHandler
    , public IAtmosphereGameEventHandler
    , public IGenericGameEventHandler
{
private:

    static constexpr bool StartInFullScreenMode = true;

public:

    MainFrame(
        wxApp * mainApp,
        ResourceLocator const & resourceLocator,
        LocalizationManager & localizationManager);

    bool ProcessKeyDown(
        int keyCode,
        int keyModifiers);

    bool ProcessKeyUp(
        int keyCode,
        int keyModifiers);

    void OnSecretTypingBootSettings();
    void OnSecretTypingDebug();
    void OnSecretTypingLoadBuiltInShip(int ship);
    void OnSecretTypingGoToWorldEnd(int side);

private:

    UnFocusablePanel * mMainPanel;

    //
    // OpenGL Canvas and Context
    //

    std::unique_ptr<GLCanvas> mMainGLCanvas;
    std::unique_ptr<wxGLContext> mMainGLCanvasContext;

    // Pointer to the canvas that the OpenGL context may be made current on
    std::atomic<wxGLCanvas *> mCurrentOpenGLCanvas;

    //
    // Controls that we're interacting with
    //

    wxBoxSizer * mMainPanelSizer;
    wxMenuItem * mReloadPreviousShipMenuItem;
    wxMenuItem * mPauseMenuItem;
    wxMenuItem * mStepMenuItem;
    wxMenu * mToolsMenu;
    wxMenuItem * mSmashMenuItem;
    wxMenuItem * mScareFishMenuItem;
    wxMenuItem * mRCBombsDetonateMenuItem;
    wxMenuItem * mAntiMatterBombsDetonateMenuItem;
    wxMenuItem * mTriggerStormMenuItem;
    wxMenuItem * mReloadLastModifiedSettingsMenuItem;
    wxMenuItem * mShowEventTickerMenuItem;
    wxMenuItem * mShowProbePanelMenuItem;
    wxMenuItem * mShowStatusTextMenuItem;
    wxMenuItem * mShowExtendedStatusTextMenuItem;
    wxMenuItem * mFullScreenMenuItem;
    wxMenuItem * mNormalScreenMenuItem;
    wxMenuItem * mMuteMenuItem;
    std::unique_ptr<ProbePanel> mProbePanel;
    std::unique_ptr<EventTickerPanel> mEventTickerPanel;
    std::unique_ptr<SwitchboardPanel> mElectricalPanel;

    //
    // Dialogs
    //

    std::shared_ptr<SplashScreenDialog> mSplashScreenDialog;
    std::unique_ptr<DebugDialog> mDebugDialog;
    std::unique_ptr<HelpDialog> mHelpDialog;
    std::unique_ptr<LoggingDialog> mLoggingDialog;
    std::unique_ptr<PreferencesDialog> mPreferencesDialog;
    std::unique_ptr<SettingsDialog> mSettingsDialog;
    std::unique_ptr<ShipLoadDialog> mShipLoadDialog;

    //
    // Timers
    //

    std::unique_ptr<wxTimer> mPostInitializeTimer;
    std::chrono::milliseconds mGameTimerDuration;
    std::unique_ptr<wxTimer> mGameTimer;
    std::unique_ptr<wxTimer> mLowFrequencyTimer;
    std::unique_ptr<wxTimer> mCheckUpdatesTimer;

private:

    //
    // Event handlers
    //

    // App
    void OnPostInitializeTrigger(wxTimerEvent & event);
    void OnPostInitializeIdle(wxIdleEvent & event);
    void OnMainFrameClose(wxCloseEvent & event);
    void OnQuit(wxCommandEvent & event);
    void OnMainPanelKeyDown(wxKeyEvent & event);
    void OnGameTimerTrigger(wxTimerEvent & event);
    void OnLowFrequencyTimerTrigger(wxTimerEvent & event);
    void OnCheckUpdatesTimerTrigger(wxTimerEvent & event);
    void OnIdle(wxIdleEvent & event);

    // Main GL canvas
    void OnMainGLCanvasShow(wxShowEvent & event);
    void OnMainGLCanvasPaint(wxPaintEvent & event);
    void OnMainGLCanvasResize(wxSizeEvent & event);
    void OnMainGLCanvasLeftDown(wxMouseEvent & event);
    void OnMainGLCanvasLeftUp(wxMouseEvent & event);
    void OnMainGLCanvasRightDown(wxMouseEvent & event);
    void OnMainGLCanvasRightUp(wxMouseEvent & event);
    void OnMainGLCanvasMouseMove(wxMouseEvent & event);
    void OnMainGLCanvasMouseWheel(wxMouseEvent & event);
    void OnMainGLCanvasCaptureMouseLost(wxMouseCaptureLostEvent & event);

    // Menu
    void OnZoomInMenuItemSelected(wxCommandEvent & event);
    void OnZoomOutMenuItemSelected(wxCommandEvent & event);
    void OnAmbientLightUpMenuItemSelected(wxCommandEvent & event);
    void OnAmbientLightDownMenuItemSelected(wxCommandEvent & event);
    void OnPauseMenuItemSelected(wxCommandEvent & event);
    void OnStepMenuItemSelected(wxCommandEvent & event);
    void OnResetViewMenuItemSelected(wxCommandEvent & event);
    void OnLoadShipMenuItemSelected(wxCommandEvent & event);
    void OnReloadCurrentShipMenuItemSelected(wxCommandEvent & event);
    void OnReloadPreviousShipMenuItemSelected(wxCommandEvent & event);
    void OnSaveScreenshotMenuItemSelected(wxCommandEvent & event);

    void OnMoveMenuItemSelected(wxCommandEvent & event);
    void OnMoveAllMenuItemSelected(wxCommandEvent & event);
    void OnPickAndPullMenuItemSelected(wxCommandEvent & event);
    void OnSmashMenuItemSelected(wxCommandEvent & event);
    void OnSliceMenuItemSelected(wxCommandEvent & event);
    void OnHeatBlasterMenuItemSelected(wxCommandEvent & event);
    void OnFireExtinguisherMenuItemSelected(wxCommandEvent & event);
    void OnBlastToolMenuItemSelected(wxCommandEvent & event);
    void OnElectricSparkToolMenuItemSelected(wxCommandEvent & event);
    void OnGrabMenuItemSelected(wxCommandEvent & event);
    void OnSwirlMenuItemSelected(wxCommandEvent & event);
    void OnPinMenuItemSelected(wxCommandEvent & event);
    void OnInjectPressureMenuItemSelected(wxCommandEvent & event);
    void OnFloodHoseMenuItemSelected(wxCommandEvent & event);
    void OnTimerBombMenuItemSelected(wxCommandEvent & event);
    void OnRCBombMenuItemSelected(wxCommandEvent & event);
    void OnImpactBombMenuItemSelected(wxCommandEvent & event);
    void OnAntiMatterBombMenuItemSelected(wxCommandEvent & event);
    void OnThanosSnapMenuItemSelected(wxCommandEvent & event);
    void OnWaveMakerMenuItemSelected(wxCommandEvent & event);
    void OnAdjustTerrainMenuItemSelected(wxCommandEvent & event);
    void OnRepairStructureMenuItemSelected(wxCommandEvent & event);
    void OnScrubMenuItemSelected(wxCommandEvent & event);
    void OnScareFishMenuItemSelected(wxCommandEvent & event);
    void OnTriggerLightningMenuItemSelected(wxCommandEvent & event);
    void OnRCBombDetonateMenuItemSelected(wxCommandEvent & event);
    void OnAntiMatterBombDetonateMenuItemSelected(wxCommandEvent & event);
    void OnTriggerTsunamiMenuItemSelected(wxCommandEvent & event);
    void OnTriggerRogueWaveMenuItemSelected(wxCommandEvent & event);
    void OnTriggerStormMenuItemSelected(wxCommandEvent & event);
    void OnPhysicsProbeMenuItemSelected(wxCommandEvent & event);

    void OnOpenSettingsWindowMenuItemSelected(wxCommandEvent & event);
    void OnReloadLastModifiedSettingsMenuItem(wxCommandEvent & event);
    void OnOpenPreferencesWindowMenuItemSelected(wxCommandEvent & event);
    void OnOpenLogWindowMenuItemSelected(wxCommandEvent & event);
    void OnShowEventTickerMenuItemSelected(wxCommandEvent & event);
    void OnShowProbePanelMenuItemSelected(wxCommandEvent & event);
    void OnShowStatusTextMenuItemSelected(wxCommandEvent & event);
    void OnShowExtendedStatusTextMenuItemSelected(wxCommandEvent & event);
    void OnFullScreenMenuItemSelected(wxCommandEvent & event);
    void OnNormalScreenMenuItemSelected(wxCommandEvent & event);
    void OnMuteMenuItemSelected(wxCommandEvent & event);
    void OnHelpMenuItemSelected(wxCommandEvent & event);
    void OnShipBuilderNewShipMenuItemSelected(wxCommandEvent & event);
    void OnShipBuilderEditShipMenuItemSelected(wxCommandEvent & event);
    void OnAboutMenuItemSelected(wxCommandEvent & event);
    void OnCheckForUpdatesMenuItemSelected(wxCommandEvent & event);

    //
    // Game event handler
    //

    void RegisterEventHandler(IGameController & gameController)
    {
        gameController.RegisterLifecycleEventHandler(this);
        gameController.RegisterAtmosphereEventHandler(this);
        gameController.RegisterGenericEventHandler(this);
    }

    virtual void OnGameReset() override
    {
        // Refresh title bar
        mCurrentShipTitles.clear();
        UpdateFrameTitle();
    }

    virtual void OnShipLoaded(
        unsigned int /*id*/,
        ShipMetadata const & shipMetadata) override
    {
        std::string shipTitle = shipMetadata.ShipName;
        if (shipMetadata.Author.has_value())
            shipTitle += " - by " + *shipMetadata.Author;
        if (shipMetadata.ArtCredits.has_value())
            shipTitle += "; art by " + *shipMetadata.ArtCredits;

        mCurrentShipTitles.push_back(shipTitle);

        UpdateFrameTitle();
    }

    virtual void OnStormBegin() override
    {
        mTriggerStormMenuItem->Enable(false);
    }

    virtual void OnStormEnd() override
    {
        mTriggerStormMenuItem->Enable(true);
    }

    virtual void OnGadgetPlaced(
        GadgetId /*gadgetId*/,
        GadgetType gadgetType,
        bool /*isUnderwater*/) override
    {
        if (GadgetType::RCBomb == gadgetType)
        {
            ++mCurrentRCBombCount;
            mRCBombsDetonateMenuItem->Enable(mCurrentRCBombCount > 0);
        }
        else if (GadgetType::AntiMatterBomb == gadgetType)
        {
            ++mCurrentAntiMatterBombCount;
            mAntiMatterBombsDetonateMenuItem->Enable(mCurrentAntiMatterBombCount > 0);
        }
    }

    virtual void OnGadgetRemoved(
        GadgetId /*gadgetId*/,
        GadgetType gadgetType,
        std::optional<bool> /*isUnderwater*/) override
    {
        if (GadgetType::RCBomb == gadgetType)
        {
            assert(mCurrentRCBombCount > 0u);
            --mCurrentRCBombCount;
            mRCBombsDetonateMenuItem->Enable(mCurrentRCBombCount > 0);
        }
        else if (GadgetType::AntiMatterBomb == gadgetType)
        {
            assert(mCurrentAntiMatterBombCount > 0u);
            --mCurrentAntiMatterBombCount;
            mAntiMatterBombsDetonateMenuItem->Enable(mCurrentAntiMatterBombCount > 0);
        }
    }

    virtual void OnFishCountUpdated(size_t count) override
    {
        mScareFishMenuItem->Enable(count > 0);
    }

private:

    void RunGameIteration();

    void MakeOpenGLContextCurrent()
    {
        LogMessage("MainFrame::MakeOpenGLContextCurrent()");

        assert(mCurrentOpenGLCanvas.load() != nullptr);
        mMainGLCanvasContext->SetCurrent(*(mCurrentOpenGLCanvas.load()));
    }

    inline void AfterGameRender()
    {
        if (!mHasWindowBeenShown)
        {
            LogMessage("MainFrame::AfterGameRender: showing frame");

            this->Show(true);

            if (StartInFullScreenMode)
                this->ShowFullScreen(true, wxFULLSCREEN_NOBORDER);

#ifdef __WXGTK__
            // Make sure the main frame is correctly laid out afterwards
            this->Layout();
            mMainApp->Yield();
            mMainApp->Yield();
#endif

            mHasWindowBeenShown = true;
        }
    }

    void ResetShipUIState();

    void UpdateFrameTitle();

    void OnError(
        wxString const & message,
        bool die);

    void FreezeGame();

    void ThawGame();

    void SetPaused(bool isPaused);

    void PostGameStepTimer(std::chrono::milliseconds duration);

    void StartLowFrequencyTimer();

    void ReconcileWithUIPreferences();

    static std::filesystem::path ChooseDefaultShip(ResourceLocator const & resourceLocator);

    void LoadShip(
        std::filesystem::path const & shipFilePath,
        bool isFromUser);

    void OnShipLoaded(std::filesystem::path shipFilePath);

    wxAcceleratorEntry MakePlainAcceleratorKey(int key, wxMenuItem * menuItem);

    void SwitchToShipBuilderForNewShip();

    void SwitchToShipBuilderForCurrentShip();

    void SwitchFromShipBuilder(std::optional<std::filesystem::path> shipFilePath);

private:

    wxApp * const mMainApp;

    std::unique_ptr<ShipBuilder::MainFrame> mShipBuilderMainFrame;

    //
    // Helpers
    //

    ResourceLocator const & mResourceLocator;
    LocalizationManager & mLocalizationManager;
    std::shared_ptr<GameController> mGameController;
    std::shared_ptr<SoundController> mSoundController;
    std::shared_ptr<MusicController> mMusicController;
    std::unique_ptr<ToolController> mToolController;
    std::shared_ptr<SettingsManager> mSettingsManager;
    std::shared_ptr<UIPreferencesManager> mUIPreferencesManager;
    std::unique_ptr<UpdateChecker> mUpdateChecker;

    //
    // State
    //

    std::filesystem::path mCurrentShipFilePath;
    std::filesystem::path mPreviousShipFilePath;

    bool mHasWindowBeenShown;
    bool mHasStartupTipBeenChecked;
    bool mIsGameFrozen;
    int mPauseCount;
    std::vector<std::string> mCurrentShipTitles;
    size_t mCurrentRCBombCount;
    size_t mCurrentAntiMatterBombCount;
    bool mIsShiftKeyDown;
    bool mIsMouseCapturedByGLCanvas;
};
