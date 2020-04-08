/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-12-27
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SoundController.h"
#include "UIPreferencesManager.h"

#include <UIControls/BitmappedCheckbox.h>
#include <UIControls/ElectricalElementControl.h>

#include <Game/IGameController.h>
#include <Game/IGameEventHandlers.h>
#include <Game/ResourceLoader.h>

#include <GameCore/ProgressCallback.h>

#include <wx/bitmap.h>
#include <wx/bmpcbox.h>
#include <wx/custombgwin.h>
#include <wx/gbsizer.h>
#include <wx/popupwin.h>
#include <wx/scrolwin.h>
#include <wx/timer.h>
#include <wx/wx.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class SwitchboardPanel
    : public wxCustomBackgroundWindow<wxPanel>
    , public ILifecycleGameEventHandler
    , public IElectricalElementGameEventHandler
{
public:

    static std::unique_ptr<SwitchboardPanel> Create(
        wxWindow * parent,
        wxWindow * parentLayoutWindow,
        wxSizer * parentLayoutSizer,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        std::shared_ptr<UIPreferencesManager> uiPreferencesManager,
        ResourceLoader & resourceLoader,
        ProgressCallback const & progressCallback);

    virtual ~SwitchboardPanel();

    void Update();

    bool ProcessKeyDown(
        int keyCode,
        int keyModifiers);

    bool ProcessKeyUp(
        int keyCode,
        int keyModifiers);

public:

    //
    // Game event handlers
    //

    void RegisterEventHandler(IGameController & gameController)
    {
        gameController.RegisterLifecycleEventHandler(this);
        gameController.RegisterElectricalElementEventHandler(this);
    }

    virtual void OnElectricalElementAnnouncementsBegin() override;

    virtual void OnSwitchCreated(
        ElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        SwitchType type,
        ElectricalState state,
        std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata) override;

    virtual void OnPowerProbeCreated(
        ElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        PowerProbeType type,
        ElectricalState state,
        std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata) override;

    virtual void OnEngineControllerCreated(
        ElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata) override;

    virtual void OnEngineMonitorCreated(
        ElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        ElectricalMaterial const & electricalMaterial,
        float thrustMagnitude,
        float rpm,
        std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata) override;

    virtual void OnElectricalElementAnnouncementsEnd() override;

    virtual void OnSwitchEnabled(
        ElectricalElementId electricalElementId,
        bool isEnabled) override;

    virtual void OnSwitchToggled(
        ElectricalElementId electricalElementId,
        ElectricalState newState) override;

    virtual void OnPowerProbeToggled(
        ElectricalElementId electricalElementId,
        ElectricalState newState) override;

    virtual void OnEngineControllerEnabled(
        ElectricalElementId electricalElementId,
        bool isEnabled) override;

    virtual void OnEngineControllerUpdated(
        ElectricalElementId electricalElementId,
        int telegraphValue) override;

    virtual void OnEngineMonitorUpdated(
        ElectricalElementId electricalElementId,
        float thrustMagnitude,
        float rpm) override;

private:

    SwitchboardPanel(
        wxWindow * parent,
        wxWindow * parentLayoutWindow,
        wxSizer * parentLayoutSizer,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        std::shared_ptr<UIPreferencesManager> uiPreferencesManager,
        ResourceLoader & resourceLoader,
        ProgressCallback const & progressCallback);

    void MakeSwitchPanel();

    bool IsShowing() const
    {
        return mShowingMode != ShowingMode::NotShowing;
    }

    void HideFully();

    void ShowPartially();

    void ShowFullyFloating();

    void ShowFullyDocked();

    void ShowDockCheckbox(bool doShow);

    void InstallMouseTracking(bool isActive);

    void LayoutParent();

    void SetBackgroundBitmapFromCombo(int selection);

private:

    void OnLeaveWindowTimer(wxTimerEvent & event);

    void OnDockCheckbox(wxCommandEvent & event);

    void OnEnterWindow(wxMouseEvent & event);

    void OnLeaveWindow();

    void OnRightDown(wxMouseEvent & event);

    void OnBackgroundSelectionChanged(wxCommandEvent & event);

    void OnTick(ElectricalElementId electricalElementId);

private:

    enum class ShowingMode
    {
        NotShowing,
        ShowingHint,
        ShowingFullyFloating,
        ShowingFullyDocked
    };

    ShowingMode mShowingMode;

    //

    wxBoxSizer * mMainHSizer1;
    wxBoxSizer * mMainVSizer2;

    wxPanel * mHintPanel;
    wxBoxSizer * mHintPanelSizer;

    wxScrolled<wxPanel> * mSwitchPanel;
    wxBoxSizer * mSwitchPanelVSizer;
    wxGridBagSizer * mSwitchPanelElementSizer;

    BitmappedCheckbox * mDockCheckbox;

    std::unique_ptr<wxTimer> mLeaveWindowTimer;

    wxBitmapComboBox * mBackgroundBitmapComboBox;
    std::unique_ptr<wxPopupTransientWindow> mBackgroundSelectorPopup;

    wxCursor mInteractiveCursor;
    wxCursor mPassiveCursor;

private:

    struct ElectricalElementInfo
    {
        ElectricalElementInstanceIndex InstanceIndex;
        ElectricalElementControl * Control;
        IDisablableElectricalElementControl * DisablableControl;
        IInteractiveElectricalElementControl * InteractiveControl;
        std::optional<ElectricalPanelElementMetadata> PanelElementMetadata;

        ElectricalElementInfo(
            ElectricalElementInstanceIndex instanceIndex,
            ElectricalElementControl * control,
            IDisablableElectricalElementControl * disablableControl,
            IInteractiveElectricalElementControl * interactiveControl,
            std::optional<ElectricalPanelElementMetadata> panelElementMetadata)
            : InstanceIndex(instanceIndex)
            , Control(control)
            , DisablableControl(disablableControl)
            , InteractiveControl(interactiveControl)
            , PanelElementMetadata(panelElementMetadata)
        {}
    };

    std::unordered_map<ElectricalElementId, ElectricalElementInfo> mElementMap;

    // The electrical elements that need to be updated
    std::vector<IUpdateableElectricalElementControl *> mUpdateableElements;

    // Keyboard shortcuts - indexed by key (Ctrl/Alt 1,...,0,-)
    std::vector<ElectricalElementId> mKeyboardShortcutToElementId;

    // The electrical element that we last delivered a KeyDown to,
    // so that we know whom to deliver KeyUp.
    // Note that we care only about the first key down in a sequence of key downs,
    // and only about the first key up in a sequence of key ups
    std::optional<ElectricalElementId> mCurrentKeyDownElementId;

private:

    std::shared_ptr<IGameController> const mGameController;
    std::shared_ptr<SoundController> const mSoundController;
    std::shared_ptr<UIPreferencesManager> const mUIPreferencesManager;

    wxWindow * const mParentLayoutWindow;
    wxSizer * const mParentLayoutSizer;

    //
    // Bitmaps
    //

    wxBitmap mAutomaticSwitchOnEnabledBitmap;
    wxBitmap mAutomaticSwitchOffEnabledBitmap;
    wxBitmap mAutomaticSwitchOnDisabledBitmap;
    wxBitmap mAutomaticSwitchOffDisabledBitmap;

    wxBitmap mInteractivePushSwitchOnEnabledBitmap;
    wxBitmap mInteractivePushSwitchOffEnabledBitmap;
    wxBitmap mInteractivePushSwitchOnDisabledBitmap;
    wxBitmap mInteractivePushSwitchOffDisabledBitmap;

    wxBitmap mInteractiveToggleSwitchOnEnabledBitmap;
    wxBitmap mInteractiveToggleSwitchOffEnabledBitmap;
    wxBitmap mInteractiveToggleSwitchOnDisabledBitmap;
    wxBitmap mInteractiveToggleSwitchOffDisabledBitmap;

    wxBitmap mShipSoundSwitchOnEnabledBitmap;
    wxBitmap mShipSoundSwitchOffEnabledBitmap;
    wxBitmap mShipSoundSwitchOnDisabledBitmap;
    wxBitmap mShipSoundSwitchOffDisabledBitmap;

    wxBitmap mPowerMonitorOnBitmap;
    wxBitmap mPowerMonitorOffBitmap;

    wxBitmap mGaugeRpmBitmap;
    wxBitmap mGaugeVoltsBitmap;

    wxBitmap mEngineControllerBackgroundEnabledBitmap;
    wxBitmap mEngineControllerBackgroundDisabledBitmap;
    std::vector<wxBitmap> mEngineControllerHandBitmaps;

    wxSize mMinBitmapSize;
};
