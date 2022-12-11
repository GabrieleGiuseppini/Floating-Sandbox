/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-12-27
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SwitchboardPanel.h"

#include <UILib/LayoutHelper.h>
#include <UILib/WxHelpers.h>

#include <Game/ImageFileTools.h>

#include <GameCore/ImageTools.h>

#include <wx/clntdata.h>
#include <wx/cursor.h>

#include <algorithm>
#include <cassert>
#include <sstream>
#include <utility>

static constexpr int MaxElementsPerRow = 11;
static constexpr int MaxKeyboardShortcuts = 20;

SwitchboardPanel * SwitchboardPanel::Create(
    wxWindow * parent,
    std::function<void()> onRelayout,
    IGameController & gameController,
    SoundController & soundController,
    UIPreferencesManager & uiPreferencesManager,
    ResourceLocator const & resourceLocator,
    ProgressCallback const & progressCallback)
{
    return new SwitchboardPanel(
        parent,
        std::move(onRelayout),
        gameController,
        soundController,
        uiPreferencesManager,
        resourceLocator,
        progressCallback);
}

SwitchboardPanel::SwitchboardPanel(
    wxWindow * parent,
    std::function<void()> onRelayout,
    IGameController & gameController,
    SoundController & soundController,
    UIPreferencesManager & uiPreferencesManager,
    ResourceLocator const & resourceLocator,
    ProgressCallback const & progressCallback)
    : mShowingMode(ShowingMode::NotShowing)
    , mLeaveWindowTimer()
    , mBackgroundBitmapComboBox(nullptr)
    , mBackgroundSelectorPopup()
    //
    , mElementMap()
    , mUpdateableElements()
    , mKeyboardShortcutToElementId()
    , mCurrentKeyDownElementId()
    //
    , mOnRelayout(std::move(onRelayout))
    , mGameController(gameController)
    , mSoundController(soundController)
    , mUIPreferencesManager(uiPreferencesManager)
    //
    , mMinBitmapSize(std::numeric_limits<int>::max(), std::numeric_limits<int>::max())
{
    float constexpr TotalProgressSteps = 12.0f;
    float progressSteps = 0.0;

    UnFocusablePanel::Create(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_SIMPLE);

    //
    // Setup background selector popup
    //

    progressCallback(progressSteps / TotalProgressSteps, ProgressMessageType::LoadingElectricalPanel);

    auto backgroundBitmapFilepaths = resourceLocator.GetBitmapFilePaths("switchboard_background_*");
    if (backgroundBitmapFilepaths.empty())
    {
        throw GameException("There are no switchboard background bitmaps available");
    }

    std::sort(
        backgroundBitmapFilepaths.begin(),
        backgroundBitmapFilepaths.end(),
        [](auto const & fp1, auto const & fp2)
        {
            return fp1.string() < fp2.string();
        });

    mBackgroundSelectorPopup = std::make_unique<wxPopupTransientWindow>(this, wxPU_CONTAINS_CONTROLS | wxBORDER_SIMPLE);
    {
        auto sizer = new wxBoxSizer(wxVERTICAL);
        {
            mBackgroundBitmapComboBox = new wxBitmapComboBox(mBackgroundSelectorPopup.get(), wxID_ANY, wxEmptyString,
                wxDefaultPosition, wxDefaultSize, wxArrayString(), wxCB_READONLY);

            for (auto const & backgroundBitmapFilepath : backgroundBitmapFilepaths)
            {
                auto backgroundBitmapThumb1 = ImageFileTools::LoadImageRgbaAndResize(
                    backgroundBitmapFilepath,
                    128);

                auto backgroundBitmapThumb2 = ImageTools::Truncate(std::move(backgroundBitmapThumb1), ImageSize(64, 32));

                mBackgroundBitmapComboBox->Append(
                    wxEmptyString,
                    WxHelpers::MakeBitmap(backgroundBitmapThumb2),
                    new wxStringClientData(backgroundBitmapFilepath.string()));
            }

            mBackgroundBitmapComboBox->Bind(wxEVT_COMBOBOX, &SwitchboardPanel::OnBackgroundSelectionChanged, this);

            sizer->Add(mBackgroundBitmapComboBox, 1, wxALL | wxEXPAND, 0);
        }

        mBackgroundSelectorPopup->SetSizerAndFit(sizer);
    }

    //
    // Set background bitmap
    //

    // Select background from preferences
    int backgroundBitmapIndex = std::min(
        mUIPreferencesManager.GetSwitchboardBackgroundBitmapIndex(),
        static_cast<int>(mBackgroundBitmapComboBox->GetCount()) - 1);
    mBackgroundBitmapComboBox->Select(backgroundBitmapIndex);

    // Set bitmap
    SetBackgroundBitmapFromCombo(mBackgroundBitmapComboBox->GetSelection());

    //
    // Load cursors
    //

    mInteractiveCursor = WxHelpers::LoadCursor(
        "switch_cursor_up",
        8,
        9,
        resourceLocator);

    mPassiveCursor = WxHelpers::LoadCursor(
        "question_mark_cursor_up",
        16,
        16,
        resourceLocator);

    //
    // Load bitmaps
    //

    wxBitmap dockCheckboxCheckedBitmap;
    wxBitmap dockCheckboxUncheckedBitmap;

    {
        progressSteps += 1.0f; // 1.0f
        progressCallback(progressSteps / TotalProgressSteps, ProgressMessageType::LoadingElectricalPanel);

        mAutomaticSwitchOnEnabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("automatic_switch_on_enabled").string(), wxBITMAP_TYPE_PNG);
        mAutomaticSwitchOffEnabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("automatic_switch_off_enabled").string(), wxBITMAP_TYPE_PNG);
        mAutomaticSwitchOnDisabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("automatic_switch_on_disabled").string(), wxBITMAP_TYPE_PNG);
        mAutomaticSwitchOffDisabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("automatic_switch_off_disabled").string(), wxBITMAP_TYPE_PNG);
        mMinBitmapSize.DecTo(mAutomaticSwitchOnEnabledBitmap.GetSize());

        progressSteps += 1.0f; // 2.0f
        progressCallback(progressSteps / TotalProgressSteps, ProgressMessageType::LoadingElectricalPanel);

        mInteractivePushSwitchOnEnabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("interactive_push_switch_on_enabled").string(), wxBITMAP_TYPE_PNG);
        mInteractivePushSwitchOffEnabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("interactive_push_switch_off_enabled").string(), wxBITMAP_TYPE_PNG);
        mInteractivePushSwitchOnDisabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("interactive_push_switch_on_disabled").string(), wxBITMAP_TYPE_PNG);
        mInteractivePushSwitchOffDisabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("interactive_push_switch_off_disabled").string(), wxBITMAP_TYPE_PNG);
        mMinBitmapSize.DecTo(mInteractivePushSwitchOnEnabledBitmap.GetSize());

        progressSteps += 1.0f; // 3.0f
        progressCallback(progressSteps / TotalProgressSteps, ProgressMessageType::LoadingElectricalPanel);

        mInteractiveToggleSwitchOnEnabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("interactive_toggle_switch_on_enabled").string(), wxBITMAP_TYPE_PNG);
        mInteractiveToggleSwitchOffEnabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("interactive_toggle_switch_off_enabled").string(), wxBITMAP_TYPE_PNG);
        mInteractiveToggleSwitchOnDisabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("interactive_toggle_switch_on_disabled").string(), wxBITMAP_TYPE_PNG);
        mInteractiveToggleSwitchOffDisabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("interactive_toggle_switch_off_disabled").string(), wxBITMAP_TYPE_PNG);
        mMinBitmapSize.DecTo(mInteractiveToggleSwitchOnEnabledBitmap.GetSize());

        progressSteps += 1.0f; // 4.0f
        progressCallback(progressSteps / TotalProgressSteps, ProgressMessageType::LoadingElectricalPanel);

        mShipSoundSwitchOnEnabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("ship_sound_switch_on_enabled").string(), wxBITMAP_TYPE_PNG);
        mShipSoundSwitchOffEnabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("ship_sound_switch_off_enabled").string(), wxBITMAP_TYPE_PNG);
        mShipSoundSwitchOnDisabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("ship_sound_switch_on_disabled").string(), wxBITMAP_TYPE_PNG);
        mShipSoundSwitchOffDisabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("ship_sound_switch_off_disabled").string(), wxBITMAP_TYPE_PNG);
        mMinBitmapSize.DecTo(mShipSoundSwitchOnEnabledBitmap.GetSize());

        progressSteps += 1.0f; // 5.0f
        progressCallback(progressSteps / TotalProgressSteps, ProgressMessageType::LoadingElectricalPanel);

        mPowerMonitorOnBitmap.LoadFile(resourceLocator.GetBitmapFilePath("power_monitor_on").string(), wxBITMAP_TYPE_PNG);
        mPowerMonitorOffBitmap.LoadFile(resourceLocator.GetBitmapFilePath("power_monitor_off").string(), wxBITMAP_TYPE_PNG);
        mMinBitmapSize.DecTo(mPowerMonitorOnBitmap.GetSize());

        progressSteps += 1.0f; // 6.0f
        progressCallback(progressSteps / TotalProgressSteps, ProgressMessageType::LoadingElectricalPanel);

        mWatertightDoorOpenEnabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("watertight_door_open_enabled").string(), wxBITMAP_TYPE_PNG);
        mWatertightDoorClosedEnabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("watertight_door_closed_enabled").string(), wxBITMAP_TYPE_PNG);
        mWatertightDoorOpenDisabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("watertight_door_open_disabled").string(), wxBITMAP_TYPE_PNG);
        mWatertightDoorClosedDisabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("watertight_door_closed_disabled").string(), wxBITMAP_TYPE_PNG);
        mMinBitmapSize.DecTo(mWatertightDoorOpenEnabledBitmap.GetSize());

        progressSteps += 1.0f; // 7.0f
        progressCallback(progressSteps / TotalProgressSteps, ProgressMessageType::LoadingElectricalPanel);

        mGauge0100Bitmap.LoadFile(resourceLocator.GetBitmapFilePath("gauge_0-100").string(), wxBITMAP_TYPE_PNG);
        mGaugeRpmBitmap.LoadFile(resourceLocator.GetBitmapFilePath("gauge_rpm").string(), wxBITMAP_TYPE_PNG);
        mGaugeVoltsBitmap.LoadFile(resourceLocator.GetBitmapFilePath("gauge_volts").string(), wxBITMAP_TYPE_PNG);
        mGaugeJetEngineBitmap.LoadFile(resourceLocator.GetBitmapFilePath("gauge_jet").string(), wxBITMAP_TYPE_PNG);
        mMinBitmapSize.DecTo(mGaugeRpmBitmap.GetSize());

        progressSteps += 1.0f; // 8.0f
        progressCallback(progressSteps / TotalProgressSteps, ProgressMessageType::LoadingElectricalPanel);

        mEngineControllerTelegraphBackgroundEnabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("telegraph_background_enabled").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerTelegraphBackgroundDisabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("telegraph_background_disabled").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerTelegraphHandBitmaps.emplace_back(resourceLocator.GetBitmapFilePath("telegraph_hand_0").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerTelegraphHandBitmaps.emplace_back(resourceLocator.GetBitmapFilePath("telegraph_hand_1").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerTelegraphHandBitmaps.emplace_back(resourceLocator.GetBitmapFilePath("telegraph_hand_2").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerTelegraphHandBitmaps.emplace_back(resourceLocator.GetBitmapFilePath("telegraph_hand_3").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerTelegraphHandBitmaps.emplace_back(resourceLocator.GetBitmapFilePath("telegraph_hand_4").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerTelegraphHandBitmaps.emplace_back(resourceLocator.GetBitmapFilePath("telegraph_hand_5").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerTelegraphHandBitmaps.emplace_back(resourceLocator.GetBitmapFilePath("telegraph_hand_6").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerTelegraphHandBitmaps.emplace_back(resourceLocator.GetBitmapFilePath("telegraph_hand_7").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerTelegraphHandBitmaps.emplace_back(resourceLocator.GetBitmapFilePath("telegraph_hand_8").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerTelegraphHandBitmaps.emplace_back(resourceLocator.GetBitmapFilePath("telegraph_hand_9").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerTelegraphHandBitmaps.emplace_back(resourceLocator.GetBitmapFilePath("telegraph_hand_10").string(), wxBITMAP_TYPE_PNG);

        progressSteps += 1.0f; // 9.0f
        progressCallback(progressSteps / TotalProgressSteps, ProgressMessageType::LoadingElectricalPanel);

        mEngineControllerJetThrottleBackgroundEnabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("jet_throttle_background_enabled").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerJetThrottleBackgroundDisabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("jet_throttle_background_disabled").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerJetThrottleHandleEnabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("jet_throttle_handle_enabled").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerJetThrottleHandleDisabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("jet_throttle_handle_disabled").string(), wxBITMAP_TYPE_PNG);

        progressSteps += 1.0f; // 10.0f
        progressCallback(progressSteps / TotalProgressSteps, ProgressMessageType::LoadingElectricalPanel);

        mEngineControllerJetThrustOnEnabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("thrust_button_on_enabled").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerJetThrustOffEnabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("thrust_button_off_enabled").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerJetThrustOnDisabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("thrust_button_on_disabled").string(), wxBITMAP_TYPE_PNG);
        mEngineControllerJetThrustOffDisabledBitmap.LoadFile(resourceLocator.GetBitmapFilePath("thrust_button_off_disabled").string(), wxBITMAP_TYPE_PNG);

        progressSteps += 1.0f; // 11.0f
        progressCallback(progressSteps / TotalProgressSteps, ProgressMessageType::LoadingElectricalPanel);

        dockCheckboxCheckedBitmap.LoadFile(resourceLocator.GetBitmapFilePath("electrical_panel_dock_pin_down").string(), wxBITMAP_TYPE_PNG);
        dockCheckboxUncheckedBitmap.LoadFile(resourceLocator.GetBitmapFilePath("electrical_panel_dock_pin_up").string(), wxBITMAP_TYPE_PNG);

        progressSteps += 1.0f; // 12.0f
        progressCallback(progressSteps / TotalProgressSteps, ProgressMessageType::LoadingElectricalPanel);
    }

    //
    // Setup panel
    //
    // HSizer1: |DockCheckbox(ShowToggable)| VSizer2 | Filler |
    //
    // VSizer2: ---------------
    //          |  HintPanel  |
    //          ---------------
    //          | SwitchPanel |
    //          ---------------

    mMainHSizer1 = new wxBoxSizer(wxHORIZONTAL);
    {
        // DockCheckbox
        {
            mDockCheckbox = new BitmappedCheckbox(this, wxID_ANY, dockCheckboxUncheckedBitmap, dockCheckboxCheckedBitmap, "Docks/Undocks the electrical panel.");
            mDockCheckbox->SetCursor(mInteractiveCursor);
            mDockCheckbox->Bind(wxEVT_CHECKBOX, &SwitchboardPanel::OnDockCheckbox, this);

            mMainHSizer1->Add(mDockCheckbox, 0, wxALIGN_TOP, 0);
        }

        // VSizer2
        {
            mMainVSizer2 = new wxBoxSizer(wxVERTICAL);

            // Hint panel
            {
                mHintPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 16), 0);
                mHintPanel->SetMinSize(wxSize(-1, 16)); // Determines height of hint panel
                mHintPanel->Bind(wxEVT_ENTER_WINDOW, &SwitchboardPanel::OnEnterWindow, this);

                mMainVSizer2->Add(mHintPanel, 0, wxALIGN_CENTER_HORIZONTAL, 0);
            }

            // Switch panel
            {
                MakeSwitchPanel();

                // The panel adds itself to VSizer2
            }

            mMainHSizer1->Add(mMainVSizer2, 1, wxEXPAND, 0);
        }

        // Filler
        {
            mMainHSizer1->Add(dockCheckboxUncheckedBitmap.GetSize().GetWidth(), 1, 0, wxALIGN_TOP, 0);
        }
    }

    // Hide dock checkbox and filler now
    mMainHSizer1->Hide(size_t(0));
    mMainHSizer1->Hide(size_t(2));

    // Hide hint panel now
    mMainVSizer2->Hide(mHintPanel);

    // Hide switch panel now
    mMainVSizer2->Hide(mSwitchPanel);

    //
    // Set main sizer
    //

    SetSizer(mMainHSizer1);

    //
    // Setup mouse events
    //

    Bind(wxEVT_ENTER_WINDOW, &SwitchboardPanel::OnEnterWindow, this);

    mLeaveWindowTimer = std::make_unique<wxTimer>(this, wxID_ANY);
    Connect(mLeaveWindowTimer->GetId(), wxEVT_TIMER, (wxObjectEventFunction)&SwitchboardPanel::OnLeaveWindowTimer);

    Bind(wxEVT_RIGHT_DOWN, &SwitchboardPanel::OnRightDown, this);
}

SwitchboardPanel::~SwitchboardPanel()
{
}

bool SwitchboardPanel::ProcessKeyDown(
    int keyCode,
    int keyModifiers)
{
    if (!!mCurrentKeyDownElementId)
    {
        // This is the subsequent in a sequence of key downs...
        // ...ignore it
        return false;
    }

    //
    // Translate key into index
    //

    size_t keyIndex;
    if (keyCode >= '1' && keyCode <= '9')
        keyIndex = static_cast<size_t>(keyCode - '1');
    else if (keyCode == '0')
        keyIndex = 9;
    else
        return false; // Not for us

    if ((keyModifiers & wxMOD_CONTROL) != 0)
    {
        // 0-9
    }
    else if ((keyModifiers & wxMOD_ALT) != 0)
    {
        // 10-19
        keyIndex += 10;
    }
    else
    {
        // Ignore
        return false;
    }

    //
    // Map and toggle
    //

    if (keyIndex < mKeyboardShortcutToElementId.size())
    {
        ElectricalElementId const elementId = mKeyboardShortcutToElementId[keyIndex];
        ElectricalElementInfo & elementInfo = mElementMap.at(elementId);
        if (nullptr == elementInfo.DisablableControl
            || elementInfo.DisablableControl->IsEnabled())
        {
            // Deliver event
            assert(nullptr != elementInfo.InteractiveControl);
            elementInfo.InteractiveControl->OnKeyboardShortcutDown((keyModifiers & wxMOD_SHIFT) != 0);

            // Remember this is the first keydown
            mCurrentKeyDownElementId = elementId;

            // Processed
            return true;
        }
    }

    // Ignore
    return false;
}

bool SwitchboardPanel::ProcessKeyUp(
    int keyCode,
    int /*keyModifiers*/)
{
    if (!mCurrentKeyDownElementId)
        return false; // This is the subsequent in a sequence of key ups...

    // Check if it's a panel key
    if (keyCode < '0' || keyCode > '9')
        return false; // Not for the panel

    //
    // Map and toggle
    //

    ElectricalElementInfo & elementInfo = mElementMap.at(*mCurrentKeyDownElementId);
    if (nullptr == elementInfo.DisablableControl
        || elementInfo.DisablableControl->IsEnabled())
    {
        // Deliver event
        assert(nullptr != elementInfo.InteractiveControl);
        elementInfo.InteractiveControl->OnKeyboardShortcutUp();
    }

    // Remember this is the first keyup
    mCurrentKeyDownElementId.reset();

    // Processed
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////

void SwitchboardPanel::OnElectricalElementAnnouncementsBegin()
{
    // Stop refreshing - we'll resume when announcements are over
    Freeze();

    // Reset all switch controls
    mSwitchPanel->Destroy();
    mSwitchPanel = nullptr;
    mSwitchPanelVSizer = nullptr;
    mSwitchPanelElementSizer = nullptr;
    if (mMainVSizer2->GetItemCount() > 1)
        mMainVSizer2->Remove(1);
    MakeSwitchPanel();

    // Clear maps
    mElementMap.clear();
    mUpdateableElements.clear();

    // Clear keyboard shortcuts map
    mKeyboardShortcutToElementId.clear();
    mCurrentKeyDownElementId.reset();
}

void SwitchboardPanel::OnSwitchCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex instanceIndex,
    SwitchType type,
    ElectricalState state,
    ElectricalMaterial const & electricalMaterial,
    std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata)
{
    //
    // Take element metadata
    //

    std::optional<std::string> label;
    bool isHidden = false; // Shown by default

    if (panelElementMetadata.has_value())
    {
        label = panelElementMetadata->Label;
        isHidden = panelElementMetadata->IsHidden;
    }

    if (!label.has_value())
    {
        label = electricalMaterial.MakeInstancedElementLabel(instanceIndex);
    }

    //
    // Make switch control
    //

    SwitchElectricalElementControl * swCtrl = nullptr;
    IInteractiveElectricalElementControl * intCtrl = nullptr;

    if (!isHidden)
    {
        switch (type)
        {
            case SwitchType::InteractivePushSwitch:
            {
                auto ctrl = new InteractivePushSwitchElectricalElementControl(
                    mSwitchPanel,
                    mInteractivePushSwitchOnEnabledBitmap,
                    mInteractivePushSwitchOffEnabledBitmap,
                    mInteractivePushSwitchOnDisabledBitmap,
                    mInteractivePushSwitchOffDisabledBitmap,
                    *label,
                    mInteractiveCursor,
                    [this, electricalElementId](ElectricalState newState)
                    {
                        mGameController.SetSwitchState(electricalElementId, newState);
                    },
                    state);

                swCtrl = ctrl;
                intCtrl = ctrl;

                break;
            }

            case SwitchType::InteractiveToggleSwitch:
            {
                auto ctrl = new InteractiveToggleSwitchElectricalElementControl(
                    mSwitchPanel,
                    mInteractiveToggleSwitchOnEnabledBitmap,
                    mInteractiveToggleSwitchOffEnabledBitmap,
                    mInteractiveToggleSwitchOnDisabledBitmap,
                    mInteractiveToggleSwitchOffDisabledBitmap,
                    *label,
                    mInteractiveCursor,
                    [this, electricalElementId](ElectricalState newState)
                    {
                        mGameController.SetSwitchState(electricalElementId, newState);
                    },
                    state);

                swCtrl = ctrl;
                intCtrl = ctrl;

                break;
            }

            case SwitchType::AutomaticSwitch:
            {
                auto ctrl = new AutomaticSwitchElectricalElementControl(
                    mSwitchPanel,
                    mAutomaticSwitchOnEnabledBitmap,
                    mAutomaticSwitchOffEnabledBitmap,
                    mAutomaticSwitchOnDisabledBitmap,
                    mAutomaticSwitchOffDisabledBitmap,
                    *label,
                    mPassiveCursor,
                    [this, electricalElementId]()
                    {
                        this->OnTick(electricalElementId);
                    },
                    state);

                swCtrl = ctrl;
                intCtrl = nullptr;

                break;
            }

            case SwitchType::ShipSoundSwitch:
            {
                auto ctrl = new InteractivePushSwitchElectricalElementControl(
                    mSwitchPanel,
                    mShipSoundSwitchOnEnabledBitmap,
                    mShipSoundSwitchOffEnabledBitmap,
                    mShipSoundSwitchOnDisabledBitmap,
                    mShipSoundSwitchOffDisabledBitmap,
                    *label,
                    mInteractiveCursor,
                    [this, electricalElementId](ElectricalState newState)
                    {
                        mGameController.SetSwitchState(electricalElementId, newState);
                    },
                    state);

                swCtrl = ctrl;
                intCtrl = ctrl;

                break;
            }
        }

        assert(swCtrl != nullptr);
    }

    //
    // Add switch to maps
    //

    assert(mElementMap.find(electricalElementId) == mElementMap.end());
    mElementMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(electricalElementId),
        std::forward_as_tuple(instanceIndex, swCtrl, swCtrl, intCtrl, panelElementMetadata));
}

void SwitchboardPanel::OnPowerProbeCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex instanceIndex,
    PowerProbeType type,
    ElectricalState state,
    ElectricalMaterial const & electricalMaterial,
    std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata)
{
    //
    // Create power monitor control
    //

    std::optional<std::string> label;
    bool isHidden = false;

    if (panelElementMetadata.has_value())
    {
        label = panelElementMetadata->Label;
        isHidden = panelElementMetadata->IsHidden;
    }

    ElectricalElementControl * ctrl = nullptr;

    if (!isHidden)
    {
        switch (type)
        {
            case PowerProbeType::Generator:
            {
                if (!label.has_value())
                {
                    label = electricalMaterial.MakeInstancedElementLabel(instanceIndex);
                }

                // Voltage Gauge
                auto ggCtrl = new GaugeElectricalElementControl(
                    mSwitchPanel,
                    mGaugeVoltsBitmap,
                    wxPoint(47, 47),
                    36.0f,
                    -Pi<float> / 4.0f,
                    Pi<float> * 5.0f / 4.0f,
                    *label,
                    mPassiveCursor,
                    [this, electricalElementId]()
                    {
                        this->OnTick(electricalElementId);
                    },
                    state == ElectricalState::On ? 0.0f : 1.0f);

                ctrl = ggCtrl;

                // Store as updateable element
                mUpdateableElements.emplace_back(ggCtrl);

                break;
            }

            case PowerProbeType::PowerMonitor:
            {
                if (!label.has_value())
                {
                    label = electricalMaterial.MakeInstancedElementLabel(instanceIndex);
                }

                ctrl = new PowerMonitorElectricalElementControl(
                    mSwitchPanel,
                    mPowerMonitorOnBitmap,
                    mPowerMonitorOffBitmap,
                    *label,
                    mPassiveCursor,
                    [this, electricalElementId]()
                    {
                        this->OnTick(electricalElementId);
                    },
                    state);

                break;
            }

            default:
            {
                assert(false);
                return;
            }
        }

        assert(ctrl != nullptr);
    }

    //
    // Add monitor to maps
    //

    assert(mElementMap.find(electricalElementId) == mElementMap.end());
    mElementMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(electricalElementId),
        std::forward_as_tuple(instanceIndex, ctrl, nullptr, nullptr, panelElementMetadata));
}

void SwitchboardPanel::OnEngineControllerCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex instanceIndex,
    ElectricalMaterial const & electricalMaterial,
    std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata)
{
    //
    // Take metadata
    //

    std::optional<std::string> label;
    bool isHidden = false;

    if (panelElementMetadata.has_value())
    {
        label = panelElementMetadata->Label;
        isHidden = panelElementMetadata->IsHidden;
    }

    if (!label.has_value())
    {
        label = electricalMaterial.MakeInstancedElementLabel(instanceIndex);
    }

    //
    // Create control
    //

    EngineControllerElectricalElementControl * ecCtrl = nullptr;

    if (!isHidden)
    {
        switch (electricalMaterial.EngineControllerType)
        {
            case ElectricalMaterial::EngineControllerElementType::JetThrottle:
            {
                ecCtrl = new EngineControllerJetEngineThrottleElectricalElementControl(
                    mSwitchPanel,
                    mEngineControllerJetThrottleBackgroundEnabledBitmap,
                    mEngineControllerJetThrottleBackgroundDisabledBitmap,
                    mEngineControllerJetThrottleHandleEnabledBitmap,
                    mEngineControllerJetThrottleHandleDisabledBitmap,
                    wxPoint(48, 79),
                    15,
                    *label,
                    mInteractiveCursor,
                    [this, electricalElementId](float controllerValue)
                    {
                        mGameController.SetEngineControllerState(
                            electricalElementId,
                            controllerValue);
                    },
                    0.0f); // Starting value

                break;
            }

            case ElectricalMaterial::EngineControllerElementType::JetThrust:
            {
                ecCtrl = new EngineControllerJetEngineThrustElectricalElementControl(
                    mSwitchPanel,
                    mEngineControllerJetThrustOnEnabledBitmap,
                    mEngineControllerJetThrustOffEnabledBitmap,
                    mEngineControllerJetThrustOnDisabledBitmap,
                    mEngineControllerJetThrustOffDisabledBitmap,
                    *label,
                    mInteractiveCursor,
                    [this, electricalElementId](float controllerValue)
                    {
                        mGameController.SetEngineControllerState(
                            electricalElementId,
                            controllerValue);
                    },
                    0.0f); // Starting value

                break;
            }

            case ElectricalMaterial::EngineControllerElementType::Telegraph:
            {
                ecCtrl = new EngineControllerTelegraphElectricalElementControl(
                    mSwitchPanel,
                    mEngineControllerTelegraphBackgroundEnabledBitmap,
                    mEngineControllerTelegraphBackgroundDisabledBitmap,
                    mEngineControllerTelegraphHandBitmaps,
                    wxPoint(47, 48),
                    3.90f,
                    -0.75f,
                    *label,
                    mInteractiveCursor,
                    [this, electricalElementId](float controllerValue)
                    {
                        mGameController.SetEngineControllerState(
                            electricalElementId,
                            controllerValue);
                    },
                    0.0f); // Starting value = center

                break;
            }
        }
    }

    //
    // Add to maps
    //

    assert(mElementMap.find(electricalElementId) == mElementMap.end());
    mElementMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(electricalElementId),
        std::forward_as_tuple(instanceIndex, ecCtrl, ecCtrl, ecCtrl, panelElementMetadata));
}

void SwitchboardPanel::OnEngineMonitorCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex instanceIndex,
    float /*thrustMagnitude*/,
    float rpm,
    ElectricalMaterial const & electricalMaterial,
    std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata)
{
    //
    // Take metadata
    //

    std::optional<std::string> label;
    bool isHidden = false;

    if (panelElementMetadata.has_value())
    {
        label = panelElementMetadata->Label;
        isHidden = panelElementMetadata->IsHidden;
    }

    if (!label.has_value())
    {
        label = electricalMaterial.MakeInstancedElementLabel(instanceIndex);
    }

    //
    // Create control
    //

    GaugeElectricalElementControl * ggCtrl = nullptr;

    if (!isHidden)
    {
        switch (electricalMaterial.EngineType)
        {
            case ElectricalMaterial::EngineElementType::Jet:
            {
                ggCtrl = new GaugeElectricalElementControl(
                    mSwitchPanel,
                    mGaugeJetEngineBitmap,
                    wxPoint(49, 47),
                    36.0f,
                    Pi<float> / 4.0f - 0.061f,
                    2.0f * Pi<float> -Pi<float> / 4.0f - 0.20f,
                    *label,
                    mPassiveCursor,
                    [this, electricalElementId]()
                    {
                        this->OnTick(electricalElementId);
                    },
                    1.0f - rpm);

                break;
            }

            default:
            {
                ggCtrl = new GaugeElectricalElementControl(
                    mSwitchPanel,
                    mGaugeRpmBitmap,
                    wxPoint(47, 47),
                    36.0f,
                    Pi<float> / 4.0f - 0.06f,
                    2.0f * Pi<float> -Pi<float> / 4.0f,
                    *label,
                    mPassiveCursor,
                    [this, electricalElementId]()
                    {
                        this->OnTick(electricalElementId);
                    },
                    1.0f - rpm);

                break;
            }
        }

        // Store as updateable element
        mUpdateableElements.emplace_back(ggCtrl);
    }

    //
    // Add monitor to maps
    //

    assert(mElementMap.find(electricalElementId) == mElementMap.end());
    mElementMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(electricalElementId),
        std::forward_as_tuple(instanceIndex, ggCtrl, nullptr, nullptr, panelElementMetadata));
}

void SwitchboardPanel::OnWaterPumpCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex instanceIndex,
    float normalizedForce,
    ElectricalMaterial const & electricalMaterial,
    std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata)
{
    //
    // Take metadata
    //

    std::optional<std::string> label;
    bool isHidden = false;

    if (panelElementMetadata.has_value())
    {
        label = panelElementMetadata->Label;
        isHidden = panelElementMetadata->IsHidden;
    }

    if (!label.has_value())
    {
        label = electricalMaterial.MakeInstancedElementLabel(instanceIndex);
    }

    //
    // Create control
    //

    GaugeElectricalElementControl * ggCtrl = nullptr;

    if (!isHidden)
    {
        ggCtrl = new GaugeElectricalElementControl(
            mSwitchPanel,
            mGauge0100Bitmap,
            wxPoint(48, 58),
            36.0f,
            Pi<float> / 4.0f - 0.02f,
            Pi<float> - Pi<float> / 4.0f + 0.01f,
            *label,
            mPassiveCursor,
            [this, electricalElementId]()
            {
                this->OnTick(electricalElementId);
            },
            1.0f - normalizedForce);

        // Store as updateable element
        mUpdateableElements.emplace_back(ggCtrl);
    }

    //
    // Add monitor to maps
    //

    assert(mElementMap.find(electricalElementId) == mElementMap.end());
    mElementMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(electricalElementId),
        std::forward_as_tuple(instanceIndex, ggCtrl, nullptr, nullptr, panelElementMetadata));
}

void SwitchboardPanel::OnWatertightDoorCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex instanceIndex,
    bool isOpen,
    ElectricalMaterial const & electricalMaterial,
    std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata)
{
    //
    // Take metadata
    //

    std::optional<std::string> label;
    bool isHidden = false;

    if (panelElementMetadata.has_value())
    {
        label = panelElementMetadata->Label;
        isHidden = panelElementMetadata->IsHidden;
    }

    if (!label.has_value())
    {
        label = electricalMaterial.MakeInstancedElementLabel(instanceIndex);
    }

    //
    // Create control
    //

    SwitchElectricalElementControl * swCtrl = nullptr;

    if (!isHidden)
    {
        swCtrl = new AutomaticSwitchElectricalElementControl(
            mSwitchPanel,
            mWatertightDoorOpenEnabledBitmap,
            mWatertightDoorClosedEnabledBitmap,
            mWatertightDoorOpenDisabledBitmap,
            mWatertightDoorClosedDisabledBitmap,
            *label,
            mPassiveCursor,
            [this, electricalElementId]()
            {
                this->OnTick(electricalElementId);
            },
            isOpen ? ElectricalState::On : ElectricalState::Off);
    }

    //
    // Add indicator to maps
    //

    assert(mElementMap.find(electricalElementId) == mElementMap.end());
    mElementMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(electricalElementId),
        std::forward_as_tuple(instanceIndex, swCtrl, swCtrl, nullptr, panelElementMetadata));
}

void SwitchboardPanel::OnElectricalElementAnnouncementsEnd()
{
    //
    // Layout and assign keys
    //

    // Prepare elements for layout helper
    std::vector<LayoutHelper::LayoutElement<ElectricalElementId>> layoutElements;
    for (auto const & entry : mElementMap)
    {
        // Ignore if hidden
        if (nullptr != entry.second.Control)
        {
            if (entry.second.PanelElementMetadata.has_value())
            {
                layoutElements.emplace_back(
                    entry.first,
                    entry.second.PanelElementMetadata->PanelCoordinates);
            }
            else
            {
                layoutElements.emplace_back(
                    entry.first,
                    std::nullopt);
            }
        }
    }

    // Sort elements by instance ID
    std::sort(
        layoutElements.begin(),
        layoutElements.end(),
        [this](auto const & lhs, auto const & rhs)
        {
            return mElementMap.at(lhs.Element).InstanceIndex < mElementMap.at(rhs.Element).InstanceIndex;
        });

    // Layout
    LayoutHelper::Layout<ElectricalElementId>(
        layoutElements,
        MaxElementsPerRow,
        [this](int nCols, int nRows)
        {
            mSwitchPanelElementSizer->SetCols(nCols);
            mSwitchPanelElementSizer->SetRows(nRows);
        },
        [this](std::optional<ElectricalElementId> elementId, IntegralCoordinates const & coords)
        {
            if (!!elementId)
            {
                // Get this element
                auto it = mElementMap.find(*elementId);
                assert(it != mElementMap.end());

                // Add control to sizer
                mSwitchPanelElementSizer->Add(
                    it->second.Control,
                    wxGBPosition(coords.y, coords.x + (mSwitchPanelElementSizer->GetCols() / 2)),
                    wxGBSpan(1, 1),
                    wxTOP | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL | wxALIGN_BOTTOM,
                    8);

                // If interactive, make keyboard shortcut
                if (nullptr != it->second.InteractiveControl
                    && mKeyboardShortcutToElementId.size() < MaxKeyboardShortcuts)
                {
                    int keyIndex = static_cast<int>(mKeyboardShortcutToElementId.size());

                    // Store key mapping
                    mKeyboardShortcutToElementId.emplace_back(*elementId);

                    // Create shortcut label

                    std::stringstream ss;
                    if (keyIndex < 10)
                    {
                        ss << "Ctrl-";
                    }
                    else
                    {
                        assert(keyIndex < 20);
                        keyIndex -= 10;
                        ss << "Alt-";
                    }

                    assert(keyIndex >= 0 && keyIndex <= 9);
                    if (keyIndex == 9)
                        ss << "0";
                    else
                        ss << char('1' + keyIndex);

                    // Assign label
                    it->second.InteractiveControl->SetKeyboardShortcutLabel(ss.str());
                }
            }
        });

    // Ask sizer to resize panel accordingly
    mSwitchPanelVSizer->SetSizeHints(mSwitchPanel);

    //
    // Decide panel visibility
    //

    if (layoutElements.empty())
    {
        // No elements

        // Hide
        HideFully();
    }
    else
    {
        // We have elements

        if (mUIPreferencesManager.GetAutoShowSwitchboard())
        {
            ShowFullyDocked();
        }
        else
        {
            ShowPartially();
        }
    }

    // Resume refresh
    Thaw();

    // Re-layout from parent
    mOnRelayout();

    // If scrollbar appears, add spacing
    if (mSwitchPanel->HasScrollbar(wxHORIZONTAL))
    {
        mSwitchPanelVSizer->AddSpacer(10);
        mSwitchPanelVSizer->SetSizeHints(mSwitchPanel);
        mOnRelayout();
    }
}

void SwitchboardPanel::OnSwitchEnabled(
    ElectricalElementId electricalElementId,
    bool isEnabled)
{
    //
    // Enable/disable switch
    //

    auto & elementInfo = mElementMap.at(electricalElementId);
    assert(elementInfo.Control == nullptr || elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::Switch);

    SwitchElectricalElementControl * swCtrl = dynamic_cast<SwitchElectricalElementControl *>(elementInfo.Control);
    if (swCtrl != nullptr)
    {
        swCtrl->SetEnabled(isEnabled);
    }
}

void SwitchboardPanel::OnSwitchToggled(
    ElectricalElementId electricalElementId,
    ElectricalState newState)
{
    //
    // Toggle switch
    //

    auto & elementInfo = mElementMap.at(electricalElementId);
    assert(elementInfo.Control == nullptr || elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::Switch);

    SwitchElectricalElementControl * swCtrl = dynamic_cast<SwitchElectricalElementControl *>(elementInfo.Control);
    if (swCtrl != nullptr)
    {
        swCtrl->SetState(newState);
    }
}

void SwitchboardPanel::OnPowerProbeToggled(
    ElectricalElementId electricalElementId,
    ElectricalState newState)
{
    //
    // Toggle control
    //

    auto & elementInfo = mElementMap.at(electricalElementId);
    if (elementInfo.Control != nullptr)
    {
        if (elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::PowerMonitor)
        {
            PowerMonitorElectricalElementControl * pmCtrl = dynamic_cast<PowerMonitorElectricalElementControl *>(elementInfo.Control);
            assert(pmCtrl != nullptr);

            pmCtrl->SetState(newState);
        }
        else
        {
            assert(elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::Gauge);

            GaugeElectricalElementControl * ggCtrl = dynamic_cast<GaugeElectricalElementControl *>(elementInfo.Control);
            assert(ggCtrl != nullptr);

            ggCtrl->SetValue(newState == ElectricalState::On ? 0.0f : 1.0f);
        }
    }
}

void SwitchboardPanel::OnEngineControllerEnabled(
    ElectricalElementId electricalElementId,
    bool isEnabled)
{
    //
    // Enable/disable controller
    //

    auto & elementInfo = mElementMap.at(electricalElementId);
    assert(elementInfo.Control == nullptr || elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::EngineController);

    EngineControllerElectricalElementControl * ecCtrl = dynamic_cast<EngineControllerElectricalElementControl *>(elementInfo.Control);
    if (ecCtrl != nullptr)
    {
        ecCtrl->SetEnabled(isEnabled);
    }
}

void SwitchboardPanel::OnEngineControllerUpdated(
    ElectricalElementId electricalElementId,
    ElectricalMaterial const & /*electricalMaterial*/,
    float /*oldControllerValue*/,
    float newControllerValue)
{
    //
    // Toggle controller
    //

    auto & elementInfo = mElementMap.at(electricalElementId);
    assert(elementInfo.Control == nullptr || elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::EngineController);

    EngineControllerElectricalElementControl * ecCtrl = dynamic_cast<EngineControllerElectricalElementControl *>(elementInfo.Control);
    if (ecCtrl != nullptr)
    {
        ecCtrl->SetValue(newControllerValue);
    }
}

void SwitchboardPanel::OnEngineMonitorUpdated(
    ElectricalElementId electricalElementId,
    float /*thrustMagnitude*/,
    float rpm)
{
    //
    // Change RPM
    //

    auto & elementInfo = mElementMap.at(electricalElementId);
    assert(elementInfo.Control == nullptr || elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::Gauge);

    GaugeElectricalElementControl * ggCtrl = dynamic_cast<GaugeElectricalElementControl *>(elementInfo.Control);
    if (ggCtrl != nullptr)
    {
        ggCtrl->SetValue(1.0f - rpm);
    }
}

void SwitchboardPanel::OnWaterPumpEnabled(
    ElectricalElementId /*electricalElementId*/,
    bool /*isEnabled*/)
{
    // Ignored, simply because at the moment we don't have
    // disabled images for gauges
}

void SwitchboardPanel::OnWaterPumpUpdated(
    ElectricalElementId electricalElementId,
    float normalizedForce)
{
    //
    // Change gauge
    //

    auto & elementInfo = mElementMap.at(electricalElementId);
    assert(elementInfo.Control == nullptr || elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::Gauge);

    GaugeElectricalElementControl * ggCtrl = dynamic_cast<GaugeElectricalElementControl *>(elementInfo.Control);
    if (ggCtrl != nullptr)
    {
        ggCtrl->SetValue(1.0f - normalizedForce);
    }
}

void SwitchboardPanel::OnWatertightDoorEnabled(
    ElectricalElementId electricalElementId,
    bool isEnabled)
{
    //
    // Enable/disable indicator
    //

    auto & elementInfo = mElementMap.at(electricalElementId);
    assert(elementInfo.Control == nullptr || elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::Switch);

    SwitchElectricalElementControl * swCtrl = dynamic_cast<SwitchElectricalElementControl *>(elementInfo.Control);
    if (swCtrl != nullptr)
    {
        swCtrl->SetEnabled(isEnabled);
    }
}

void SwitchboardPanel::OnWatertightDoorUpdated(
    ElectricalElementId electricalElementId,
    bool isOpen)
{
    //
    // Toggle indicator
    //

    auto & elementInfo = mElementMap.at(electricalElementId);
    assert(elementInfo.Control == nullptr || elementInfo.Control->GetControlType() == ElectricalElementControl::ControlType::Switch);

    SwitchElectricalElementControl * swCtrl = dynamic_cast<SwitchElectricalElementControl *>(elementInfo.Control);
    if (swCtrl != nullptr)
    {
        swCtrl->SetState(isOpen ? ElectricalState::On : ElectricalState::Off);
    }
}

///////////////////////////////////////////////////////////////////

void SwitchboardPanel::MakeSwitchPanel()
{
    // Create grid sizer for elements
    mSwitchPanelElementSizer = new wxGridBagSizer(0, 15);
    mSwitchPanelElementSizer->SetEmptyCellSize(mMinBitmapSize);

    // Create V sizer for switch panel
    mSwitchPanelVSizer = new wxBoxSizer(wxVERTICAL);

    // Add grid sizer to V sizer
    mSwitchPanelVSizer->Add(mSwitchPanelElementSizer, 0, wxALIGN_TOP, 0);

    // Create (scrollable) panel for switches
    mSwitchPanel = new UnFocusableScrollablePanel(this);
    mSwitchPanel->SetScrollRate(5, 0);
    mSwitchPanel->FitInside();
    mSwitchPanel->SetSizerAndFit(mSwitchPanelVSizer);
    mSwitchPanel->Bind(wxEVT_RIGHT_DOWN, &SwitchboardPanel::OnRightDown, this);

    // Add switch panel to v-sizer
    assert(mMainVSizer2->GetItemCount() == 1);
    mMainVSizer2->Add(mSwitchPanel, 0, wxALIGN_CENTER_HORIZONTAL); // We want it as wide and as tall as it is
}

void SwitchboardPanel::HideFully()
{
    // Hide hint panel
    mMainVSizer2->Hide(mHintPanel);
    ShowDockCheckbox(false);
    InstallMouseTracking(false);

    // Hide switch panel
    mMainVSizer2->Hide(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::NotShowing;
}

void SwitchboardPanel::ShowPartially()
{
    // Show hint panel
    InstallMouseTracking(true);
    ShowDockCheckbox(false);
    mMainVSizer2->Show(mHintPanel);

    // Hide switch panel
    mMainVSizer2->Hide(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::ShowingHint;
}

void SwitchboardPanel::ShowFullyFloating()
{
    // Show hint panel
    if (mDockCheckbox->IsChecked())
        mDockCheckbox->SetChecked(false);
    ShowDockCheckbox(true);
    InstallMouseTracking(true);
    mMainVSizer2->Show(mHintPanel);

    // Show switch panel
    mMainVSizer2->Show(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::ShowingFullyFloating;
}

void SwitchboardPanel::ShowFullyDocked()
{
    // Show hint panel
    if (!mDockCheckbox->IsChecked())
        mDockCheckbox->SetChecked(true);
    ShowDockCheckbox(true);
    InstallMouseTracking(false);
    mMainVSizer2->Show(mHintPanel);

    // Show switch panel
    mMainVSizer2->Show(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::ShowingFullyDocked;
}

void SwitchboardPanel::ShowDockCheckbox(bool doShow)
{
    assert(!!mMainHSizer1);
    assert(mMainHSizer1->GetItemCount() == 3);

    if (doShow)
    {
        if (!mMainHSizer1->IsShown(size_t(0)))
            mMainHSizer1->Show(size_t(0), true);
        if (!mMainHSizer1->IsShown(size_t(2)))
            mMainHSizer1->Show(size_t(2), true);
    }
    else
    {
        if (mMainHSizer1->IsShown(size_t(0)))
            mMainHSizer1->Show(size_t(0), false);
        if (mMainHSizer1->IsShown(size_t(2)))
            mMainHSizer1->Show(size_t(2), false);
    }
}

void SwitchboardPanel::InstallMouseTracking(bool isActive)
{
    assert(!!mLeaveWindowTimer);

    if (isActive && !mLeaveWindowTimer->IsRunning())
    {
        mLeaveWindowTimer->Start(750, false);
    }
    else if (!isActive && mLeaveWindowTimer->IsRunning())
    {
        mLeaveWindowTimer->Stop();
    }
}

void SwitchboardPanel::SetBackgroundBitmapFromCombo(int selection)
{
    assert(static_cast<unsigned int>(selection) < mBackgroundBitmapComboBox->GetCount());

    wxStringClientData * bitmapFilePath = dynamic_cast<wxStringClientData *>(mBackgroundBitmapComboBox->GetClientObject(selection));
    assert(nullptr != bitmapFilePath);

    wxBitmap backgroundBitmap;
    backgroundBitmap.LoadFile(bitmapFilePath->GetData(), wxBITMAP_TYPE_PNG);
    SetBackgroundBitmap(backgroundBitmap);

    Refresh();
}

void SwitchboardPanel::OnLeaveWindowTimer(wxTimerEvent & /*event*/)
{
    wxPoint const clientCoords = ScreenToClient(wxGetMousePosition());
    if (clientCoords.y < 0)
    {
        OnLeaveWindow();
    }
}

void SwitchboardPanel::OnDockCheckbox(wxCommandEvent & event)
{
    if (event.IsChecked())
    {
        //
        // Dock
        //

        ShowFullyDocked();

        // Re-layout from parent
        mOnRelayout();

        // Play sound
        mSoundController.PlayElectricalPanelDockSound(false);
    }
    else
    {
        //
        // Undock
        //

        ShowFullyFloating();

        // Re-layout from parent
        mOnRelayout();

        // Play sound
        mSoundController.PlayElectricalPanelDockSound(true);
    }
}

void SwitchboardPanel::OnEnterWindow(wxMouseEvent & /*event*/)
{
    if (mShowingMode == ShowingMode::ShowingHint)
    {
        //
        // Open the panel
        //

        ShowFullyFloating();

        // Re-layout from parent
        mOnRelayout();

        // Play sound
        mSoundController.PlayElectricalPanelOpenSound(false);
    }
}

void SwitchboardPanel::OnLeaveWindow()
{
    if (mShowingMode == ShowingMode::ShowingFullyFloating)
    {
        //
        // Lower the panel
        //

        ShowPartially();

        // Re-layout from parent
        mOnRelayout();

        // Play sound
        mSoundController.PlayElectricalPanelOpenSound(true);
    }
}

void SwitchboardPanel::OnRightDown(wxMouseEvent & event)
{
    assert(!!mBackgroundSelectorPopup);

    wxWindow * window = dynamic_cast<wxWindow *>(event.GetEventObject());
    if (nullptr == window)
        return;

    mBackgroundSelectorPopup->SetPosition(window->ClientToScreen(event.GetPosition()));
    mBackgroundSelectorPopup->Popup();
}

void SwitchboardPanel::OnBackgroundSelectionChanged(wxCommandEvent & /*event*/)
{
    int selection = mBackgroundBitmapComboBox->GetSelection();

    // Set bitmap
    SetBackgroundBitmapFromCombo(selection);

    // Remember preferences
    mUIPreferencesManager.SetSwitchboardBackgroundBitmapIndex(selection);
}

void SwitchboardPanel::OnTick(ElectricalElementId electricalElementId)
{
    mGameController.HighlightElectricalElement(electricalElementId);
    mSoundController.PlayTickSound();
}