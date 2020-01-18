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

#include <wx/bitmap.h>
#include <wx/custombgwin.h>
#include <wx/gbsizer.h>
#include <wx/scrolwin.h>
#include <wx/timer.h>
#include <wx/wx.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

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
        ResourceLoader & resourceLoader);

    virtual ~SwitchboardPanel();

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

private:

    SwitchboardPanel(
        wxWindow * parent,
        wxWindow * parentLayoutWindow,
        wxSizer * parentLayoutSizer,
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        std::shared_ptr<UIPreferencesManager> uiPreferencesManager,
        ResourceLoader & resourceLoader);

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

    void OnLeaveWindowTimer(wxTimerEvent & event);

    void OnDockCheckbox(wxCommandEvent & event);

    void OnEnterWindow(wxMouseEvent & event);

    void OnLeaveWindow();

private:

    enum class ShowingMode
    {
        NotShowing,
        ShowingHint,
        ShowingFullyFloating,
        ShowingFullyDocked
    };

    ShowingMode mShowingMode;


    wxBoxSizer * mMainHSizer1;
    wxBoxSizer * mMainVSizer2;

    wxPanel * mHintPanel;
    wxBoxSizer * mHintPanelSizer;

    wxScrolled<wxPanel> * mSwitchPanel;
    wxGridBagSizer * mSwitchPanelSizer;

    BitmappedCheckbox * mDockCheckbox;

    std::unique_ptr<wxTimer> mLeaveWindowTimer;

private:

    struct ElectricalElementInfo
    {
        ElectricalElementControl * Control;
        std::optional<ElectricalPanelElementMetadata> PanelElementMetadata;
        bool IsEnabled;

        ElectricalElementInfo(
            ElectricalElementControl * control,
            std::optional<ElectricalPanelElementMetadata> panelElementMetadata)
            : Control(control)
            , PanelElementMetadata(panelElementMetadata)
            , IsEnabled(true)
        {}
    };

    std::unordered_map<ElectricalElementId, ElectricalElementInfo> mElementMap;

    // Keyboard shortcuts - indexed by key (Ctrl/Alt 1,...,0,-)
    std::vector<ElectricalElementId> mKeyboardShortcutToElementId;

    // The electrical element that we last delivered a KeyDown to,
    // so that we know whom to deliver KeyUp.
    // Note that we care only about the first key down in a sequence of key downs,
    // and only about the first key up in a sequence of key ups
    std::optional<ElectricalElementId> mCurrentKeyDownElectricalElementId;

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

    wxBitmap mPowerMonitorOnBitmap;
    wxBitmap mPowerMonitorOffBitmap;

    wxSize mMinBitmapSize;
};
