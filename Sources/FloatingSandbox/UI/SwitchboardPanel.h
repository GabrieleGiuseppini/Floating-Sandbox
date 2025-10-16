/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-12-27
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../SoundController.h"
#include "../UIPreferencesManager.h"

#include <UILib/BitmappedCheckbox.h>
#include <UILib/ElectricalElementControl.h>
#include <UILib/UnFocusablePanel.h>
#include <UILib/UnFocusableScrollablePanel.h>

#include <Game/IGameController.h>
#include <Game/IGameEventHandlers.h>
#include <Game/GameAssetManager.h>

#include <Core/ProgressCallback.h>

#include <wx/bitmap.h>
#include <wx/bmpcbox.h>
#include <wx/custombgwin.h>
#include <wx/gbsizer.h>
#include <wx/popupwin.h>
#include <wx/timer.h>
#include <wx/wx.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class SwitchPanel;

class SwitchboardPanel final
    : public wxCustomBackgroundWindow<UnFocusablePanel>
    , public IElectricalElementEventHandler
{
public:

    static SwitchboardPanel * Create(
        wxWindow * parent,
        std::function<void()> onRelayout,
        IGameController & gameController,
        SoundController & soundController,
        UIPreferencesManager & uiPreferencesManager,
        GameAssetManager const & gameAssetManager,
        ProgressCallback const & progressCallback);

    ~SwitchboardPanel();

    void UpdateSimulation()
    {
        for (auto ctrl : mUpdateableElements)
        {
            ctrl->UpdateSimulation();
        }
    }

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
        gameController.RegisterElectricalElementEventHandler(this);
    }

    virtual void OnElectricalElementAnnouncementsBegin() override;

    virtual void OnSwitchCreated(
        GlobalElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        SwitchType type,
        ElectricalState state,
        ElectricalMaterial const & electricalMaterial,
        std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override;

    virtual void OnPowerProbeCreated(
        GlobalElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        PowerProbeType type,
        ElectricalState state,
        ElectricalMaterial const & electricalMaterial,
        std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override;

    virtual void OnEngineControllerCreated(
        GlobalElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        ElectricalMaterial const & electricalMaterial,
        std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override;

    virtual void OnEngineMonitorCreated(
        GlobalElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        float thrustMagnitude,
        float rpm,
        ElectricalMaterial const & electricalMaterial,
        std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override;

    virtual void OnWaterPumpCreated(
        GlobalElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        float normalizedForce,
        ElectricalMaterial const & electricalMaterial,
        std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override;

    virtual void OnWatertightDoorCreated(
        GlobalElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        bool isOpen,
        ElectricalMaterial const & electricalMaterial,
        std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override;

    virtual void OnElectricalElementAnnouncementsEnd() override;

    virtual void OnSwitchEnabled(
        GlobalElectricalElementId electricalElementId,
        bool isEnabled) override;

    virtual void OnSwitchToggled(
        GlobalElectricalElementId electricalElementId,
        ElectricalState newState) override;

    virtual void OnPowerProbeToggled(
        GlobalElectricalElementId electricalElementId,
        ElectricalState newState) override;

    virtual void OnEngineControllerEnabled(
        GlobalElectricalElementId electricalElementId,
        bool isEnabled) override;

    virtual void OnEngineControllerUpdated(
        GlobalElectricalElementId electricalElementId,
        ElectricalMaterial const & electricalMaterial,
        float oldControllerValue,
        float newControllerValue) override;

    virtual void OnEngineMonitorUpdated(
        GlobalElectricalElementId electricalElementId,
        float thrustMagnitude,
        float rpm) override;

    virtual void OnWaterPumpEnabled(
        GlobalElectricalElementId electricalElementId,
        bool isEnabled) override;

    virtual void OnWaterPumpUpdated(
        GlobalElectricalElementId electricalElementId,
        float normalizedForce) override;

    virtual void OnWatertightDoorEnabled(
        GlobalElectricalElementId electricalElementId,
        bool isEnabled) override;

    virtual void OnWatertightDoorUpdated(
        GlobalElectricalElementId electricalElementId,
        bool isOpen) override;

private:

    SwitchboardPanel(
        wxWindow * parent,
        std::function<void()> onRelayout,
        IGameController & gameController,
        SoundController & soundController,
        UIPreferencesManager & uiPreferencesManager,
        GameAssetManager const & gameAssetManager,
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

    void SetBackgroundBitmapFromCombo(int selection);

private:

    void OnLeaveWindowTimer(wxTimerEvent & event);

    void OnDockCheckbox(wxCommandEvent & event);

    void OnEnterWindow(wxMouseEvent & event);

    void OnLeaveWindow();

    void OnRightDown(wxMouseEvent & event);

    void OnBackgroundSelectionChanged(wxCommandEvent & event);

    void OnTick(GlobalElectricalElementId electricalElementId);

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

    UnFocusableScrollablePanel * mSwitchPanel;
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
        std::optional<ElectricalPanel::ElementMetadata> PanelElementMetadata;

        ElectricalElementInfo(
            ElectricalElementInstanceIndex instanceIndex,
            ElectricalElementControl * control,
            IDisablableElectricalElementControl * disablableControl,
            IInteractiveElectricalElementControl * interactiveControl,
            std::optional<ElectricalPanel::ElementMetadata> panelElementMetadata)
            : InstanceIndex(instanceIndex)
            , Control(control)
            , DisablableControl(disablableControl)
            , InteractiveControl(interactiveControl)
            , PanelElementMetadata(std::move(panelElementMetadata))
        {}
    };

    std::unordered_map<GlobalElectricalElementId, ElectricalElementInfo> mElementMap;

    // The electrical elements that need to be updated
    std::vector<IUpdateableElectricalElementControl *> mUpdateableElements;

    // Keyboard shortcuts - indexed by key (Ctrl/Alt 1,...,0,-)
    std::vector<GlobalElectricalElementId> mKeyboardShortcutToElementId;

    // The electrical element that we last delivered a KeyDown to,
    // so that we know whom to deliver KeyUp.
    // Note that we care only about the first key down in a sequence of key downs,
    // and only about the first key up in a sequence of key ups
    std::optional<GlobalElectricalElementId> mCurrentKeyDownElementId;

private:

    std::function<void()> const mOnRelayout;

    IGameController & mGameController;
    SoundController & mSoundController;
    UIPreferencesManager & mUIPreferencesManager;

    //
    // Bitmaps
    //

    wxBitmap mAutomaticSwitchOnWaterEnabledBitmap;
    wxBitmap mAutomaticSwitchOnThermalEnabledBitmap;
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

    wxBitmap mWatertightDoorOpenEnabledBitmap;
    wxBitmap mWatertightDoorClosedEnabledBitmap;
    wxBitmap mWatertightDoorOpenDisabledBitmap;
    wxBitmap mWatertightDoorClosedDisabledBitmap;

    wxBitmap mGauge0100Bitmap;
    wxBitmap mGaugeRpmBitmap;
    wxBitmap mGaugeVoltsBitmap;
    wxBitmap mGaugeJetEngineBitmap;

    wxBitmap mEngineControllerTelegraphBackgroundEnabledBitmap;
    wxBitmap mEngineControllerTelegraphBackgroundDisabledBitmap;
    std::vector<wxBitmap> mEngineControllerTelegraphHandBitmaps;

    wxBitmap mEngineControllerJetThrottleBackgroundEnabledBitmap;
    wxBitmap mEngineControllerJetThrottleBackgroundDisabledBitmap;
    wxBitmap mEngineControllerJetThrottleHandleEnabledBitmap;
    wxBitmap mEngineControllerJetThrottleHandleDisabledBitmap;

    wxBitmap mEngineControllerJetThrustOnEnabledBitmap;
    wxBitmap mEngineControllerJetThrustOffEnabledBitmap;
    wxBitmap mEngineControllerJetThrustOnDisabledBitmap;
    wxBitmap mEngineControllerJetThrustOffDisabledBitmap;

    wxSize mMinBitmapSize;
};
