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
#include <Game/ShipLoadSpecifications.h>

#include "SplashScreenDialog.h" // Need to include this (which includes wxGLCanvas) *after* our glad.h has been included,
 // so that wxGLCanvas ends up *not* including the system's OpenGL header but glad's instead

#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/timer.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
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
    , public INpcGameEventHandler
    , public IGenericGameEventHandler
    , public IControlGameEventHandler
{
public:

    MainFrame(
        wxApp * mainApp,
        std::optional<std::filesystem::path> initialShipFilePath,
        ResourceLocator const & resourceLocator,
        LocalizationManager & localizationManager);

    ~MainFrame();

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

    wxApp* const mMainApp;

    std::unique_ptr<ShipBuilder::MainFrame> mShipBuilderMainFrame;

    //
    // Helpers
    //

    ResourceLocator const& mResourceLocator;
    LocalizationManager& mLocalizationManager;
    std::unique_ptr<GameController> mGameController;
    std::unique_ptr<SoundController> mSoundController;
    std::unique_ptr<MusicController> mMusicController;
    std::unique_ptr<ToolController> mToolController;
    std::unique_ptr<SettingsManager> mSettingsManager;
    std::unique_ptr<UIPreferencesManager> mUIPreferencesManager;
    std::unique_ptr<UpdateChecker> mUpdateChecker;

    UnFocusablePanel * mMainPanel;

    //
    // OpenGL Canvas and Context
    //

    GLCanvas * mMainGLCanvas;
    std::unique_ptr<wxGLContext> mMainGLCanvasContext;

    // Pointer to the canvas that the OpenGL context may be made current on
    std::atomic<wxGLCanvas *> mCurrentOpenGLCanvas;

    //
    // Controls that we're interacting with
    //

    wxBoxSizer * mMainPanelSizer;
    wxMenuItem * mReloadPreviousShipMenuItem;
    wxMenuItem * mAutoFocusAtShipLoadMenuItem;
    wxMenuItem * mContinuousAutoFocusOnShipMenuItem;
    wxMenuItem * mContinuousAutoFocusOnSelectedNpcMenuItem;
    wxMenuItem * mContinuousAutoFocusOnNothingMenuItem;
    wxMenuItem * mShipViewExteriorMenuItem;
    wxMenuItem * mShipViewInteriorMenuItem;
    wxMenuItem * mPauseMenuItem;
    wxMenuItem * mStepMenuItem;

    wxMenu * mNonNpcToolsMenu;
    wxMenuItem * mScareFishMenuItem;
    wxMenuItem * mRCBombsDetonateMenuItem;
    wxMenuItem * mAntiMatterBombsDetonateMenuItem;
    wxMenuItem * mTriggerStormMenuItem;
    wxMenu * mNpcToolsMenu;
    wxMenu * mHumanNpcSubMenu;
    wxMenu * mFurnitureNpcSubMenu;
    wxBitmap mAddHumanNpcUncheckedIcon;
    wxBitmap mAddHumanNpcCheckedIcon;
    wxBitmap mAddFurnitureNpcUncheckedIcon;
    wxBitmap mAddFurnitureNpcCheckedIcon;
    wxMenuItem * mAddHumanNpcMenuItem;
    wxMenuItem * mAddFurnitureNpcMenuItem;
    wxMenuItem * mMoveNpcMenuItem;
    wxMenuItem * mRemoveNpcMenuItem;
    wxMenuItem * mTurnaroundNpcMenuItem;
    wxMenuItem * mFollowNpcMenuItem;
    wxMenuItem * mSelectNextNpcMenuItem;
    wxMenuItem * mDeselectNpcMenuItem;

    wxMenuItem * mReloadLastModifiedSettingsMenuItem;
    wxMenuItem * mShowEventTickerMenuItem;
    wxMenuItem * mShowProbePanelMenuItem;
    wxMenuItem * mShowStatusTextMenuItem;
    wxMenuItem * mShowExtendedStatusTextMenuItem;
    wxMenuItem * mFullScreenMenuItem;
    wxMenuItem * mNormalScreenMenuItem;
    wxMenuItem * mMuteMenuItem;
    ProbePanel * mProbePanel;
    EventTickerPanel * mEventTickerPanel;
    SwitchboardPanel * mElectricalPanel;

    std::vector<std::tuple<ToolType, wxMenuItem *>> mNonNpcToolMenuItems;
    std::vector<std::tuple<ToolType, wxMenuItem *>> mNpcToolMenuItems;
    std::vector<std::tuple<std::optional<NpcSubKindIdType>, wxMenuItem *>> mAddHumanNpcSubMenuItems;
    std::vector<std::tuple<std::optional<NpcSubKindIdType>, wxMenuItem *>> mAddFurnitureNpcSubMenuItems;

    //
    // Dialogs
    //

    std::shared_ptr<SplashScreenDialog> mSplashScreenDialog;
    std::unique_ptr<DebugDialog> mDebugDialog;
    std::unique_ptr<HelpDialog> mHelpDialog;
    std::unique_ptr<LoggingDialog> mLoggingDialog;
    std::unique_ptr<PreferencesDialog> mPreferencesDialog;
    std::unique_ptr<SettingsDialog> mSettingsDialog;
    std::unique_ptr<ShipLoadDialog<ShipLoadDialogUsageType::ForGame>> mShipLoadDialog;

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
    void OnMainGLCanvasPaint(wxPaintEvent & event);
    void OnMainGLCanvasResize(wxSizeEvent & event);
    void OnMainGLCanvasMouseLeftDown(wxMouseEvent & event);
    void OnMainGLCanvasMouseLeftUp(wxMouseEvent & event);
    void OnMainGLCanvasMouseRightDown(wxMouseEvent & event);
    void OnMainGLCanvasMouseRightUp(wxMouseEvent & event);
    void OnMainGLCanvasMouseMiddleDown(wxMouseEvent & event);
    void OnMainGLCanvasMouseMove(wxMouseEvent & event);
    void OnMainGLCanvasMouseWheel(wxMouseEvent & event);
    void OnMainGLCanvasCaptureMouseLost(wxMouseCaptureLostEvent & event);

    // Menu
    void OnZoomInMenuItemSelected(wxCommandEvent & event);
    void OnZoomOutMenuItemSelected(wxCommandEvent & event);
    void OnAutoFocusAtShipLoadMenuItemSelected(wxCommandEvent & event);
    void OnResetViewMenuItemSelected(wxCommandEvent & event);
    void OnShipViewMenuItemSelected(wxCommandEvent & event);
    void OnTimeOfDayUpMenuItemSelected(wxCommandEvent & event);
    void OnTimeOfDayDownMenuItemSelected(wxCommandEvent & event);
    void OnFullTimeOfDayMenuItemSelected(wxCommandEvent & event);
    void OnPauseMenuItemSelected(wxCommandEvent & event);
    void OnStepMenuItemSelected(wxCommandEvent & event);
    void OnLoadShipMenuItemSelected(wxCommandEvent & event);
    void OnReloadPreviousShipMenuItemSelected(wxCommandEvent & event);
    void OnSaveScreenshotMenuItemSelected(wxCommandEvent & event);

    void OnTriggerLightningMenuItemSelected(wxCommandEvent & event);
    void OnRCBombDetonateMenuItemSelected(wxCommandEvent & event);
    void OnAntiMatterBombDetonateMenuItemSelected(wxCommandEvent & event);
    void OnTriggerTsunamiMenuItemSelected(wxCommandEvent & event);
    void OnTriggerRogueWaveMenuItemSelected(wxCommandEvent & event);
    void OnTriggerStormMenuItemSelected(wxCommandEvent & event);
    void OnAddHumanNpcGroupMenuItemSelected(wxCommandEvent & event);
    void OnAddFurnitureNpcGroupMenuItemSelected(wxCommandEvent & event);
    void OnSelectNextNpcMenuItemSelected(wxCommandEvent & event);
    void OnDeselectNpcMenuItemSelected(wxCommandEvent & event);

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
        gameController.RegisterNpcEventHandler(this);
        gameController.RegisterGenericEventHandler(this);
        gameController.RegisterControlEventHandler(this);
    }

    void OnGameReset() override
    {
        // Refresh title bar
        mCurrentShipTitles.clear();
        UpdateFrameTitle();
    }

    void OnShipLoaded(
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

    void OnStormBegin() override
    {
        mTriggerStormMenuItem->Enable(false);
    }

    void OnStormEnd() override
    {
        mTriggerStormMenuItem->Enable(true);
    }

    void OnGadgetPlaced(
        GlobalGadgetId /*gadgetId*/,
        GadgetType gadgetType,
        bool /*isUnderwater*/) override
    {
        if (GadgetType::RCBomb == gadgetType
            || GadgetType::FireExtinguishingBomb == gadgetType)
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

    void OnGadgetRemoved(
        GlobalGadgetId /*gadgetId*/,
        GadgetType gadgetType,
        std::optional<bool> /*isUnderwater*/) override
    {
        if (GadgetType::RCBomb == gadgetType
            || GadgetType::FireExtinguishingBomb == gadgetType)
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

    void OnFishCountUpdated(size_t count) override
    {
        ReconciliateUIWithFishPresence(count > 0);
    }

    void OnNpcCountsUpdated(size_t totalNpcCount) override
    {
        ReconciliateUIWithNpcPresence(totalNpcCount > 0);
    }

    void OnNpcSelectionChanged(std::optional<NpcId> selectedNpc) override
    {
        ReconciliateUIWithNpcSelection(selectedNpc.has_value());
    }

    void OnAutoFocusTargetChanged(std::optional<AutoFocusTargetKindType> target) override
    {
        ReconciliateUIWithAutoFocusTarget(target);
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

            if (!mUIPreferencesManager || mUIPreferencesManager->GetStartInFullScreen())
            {
                this->ShowFullScreen(true, wxFULLSCREEN_NOBORDER);
            }

#ifdef __WXGTK__
            // Make sure the main frame is correctly laid out afterwards
            this->Layout();
            mMainApp->Yield();
            mMainApp->Yield();
#endif

            mHasWindowBeenShown = true;
        }
    }

    void OnError(
        wxString const & message,
        bool die);

    void FreezeGame();

    void ThawGame();

    void SetPaused(bool isPaused);

    bool IsPaused() const;

    void PostGameStepTimer(std::chrono::milliseconds duration);

    void StartLowFrequencyTimer();

    void ResetShipUIState();

    void UpdateFrameTitle();

    void SelectTool(ToolType toolType, bool isNpcTool);
    void OnNonNpcToolSelected(ToolType toolType);
    void OnNpcToolSelected(ToolType toolType);

    void ReconciliateUIWithUIPreferencesAndSettings();
    void ReconciliateUIWithFishPresence(bool areFishPresent);
    void ReconciliateUIWithNpcPresence(bool areNpcsPresent);
    void ReconciliateUIWithNpcSelection(bool areNpcsSelected);
    void ReconciliateAddNpcSubItems(ToolType toolType);
    void ReconciliateUIWithAutoFocusTarget(std::optional<AutoFocusTargetKindType> target);
    void ReconciliateShipViewModeWithCurrentTool();

    void RebuildNpcMenus();

    static std::filesystem::path ChooseDefaultShip(ResourceLocator const & resourceLocator);

    void LoadShip(
        ShipLoadSpecifications const & loadSpecs,
        bool isFromUser);

    void ReloadCurrentShip();

    void OnShipLoaded(ShipLoadSpecifications loadSpecs); // By val to have own copy vs current/prev

    wxAcceleratorEntry MakePlainAcceleratorKey(int key, wxMenuItem * menuItem);

    void SwitchToShipBuilderForNewShip();

    void SwitchToShipBuilderForCurrentShip();

    void SwitchFromShipBuilder(std::optional<std::filesystem::path> shipFilePath);

    std::tuple<wxBitmap, wxBitmap> MakeMenuBitmaps(std::string const & iconName) const;

    void SetMenuItemChecked(wxMenuItem * menuItem, wxBitmap & uncheckedBitmap, wxBitmap & checkedBitmap, bool isChecked);

    void OnShiftKeyObservation(bool isDown);
    void OnMidMouseButtonDown();

private:

    //
    // State
    //

    std::optional<std::filesystem::path> const mInitialShipFilePath;
    std::optional<ShipLoadSpecifications> mCurrentShipLoadSpecs;
    std::optional<ShipLoadSpecifications> mPreviousShipLoadSpecs;

    bool mHasWindowBeenShown;
    bool mHasStartupTipBeenChecked;
    bool mIsGameFrozen;
    int mPauseCount;
    std::vector<std::string> mCurrentShipTitles;
    size_t mCurrentRCBombCount;
    size_t mCurrentAntiMatterBombCount;
    bool mIsShiftKeyDown; // Implements SHIFT state machine; mutex with GameController::IsShiftOn
    bool mIsMouseCapturedByGLCanvas;
};
