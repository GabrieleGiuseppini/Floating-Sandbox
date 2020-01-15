/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-12-27
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SwitchboardPanel.h"

#include "WxHelpers.h"

#include <UIControls/LayoutHelper.h>

#include <wx/cursor.h>

#include <cassert>
#include <utility>

static constexpr int MaxElementsPerRow = 11;
static constexpr int MaxKeyboardShortcuts = 20;

std::unique_ptr<SwitchboardPanel> SwitchboardPanel::Create(
    wxWindow * parent,
    wxWindow * parentLayoutWindow,
    wxSizer * parentLayoutSizer,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    std::shared_ptr<UIPreferencesManager> uiPreferencesManager,
    ResourceLoader & resourceLoader)
{
    return std::unique_ptr<SwitchboardPanel>(
        new SwitchboardPanel(
            parent,
            parentLayoutWindow,
            parentLayoutSizer,
            std::move(gameController),
            std::move(soundController),
            std::move(uiPreferencesManager),
            resourceLoader));
}

SwitchboardPanel::SwitchboardPanel(
    wxWindow * parent,
    wxWindow * parentLayoutWindow,
    wxSizer * parentLayoutSizer,
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    std::shared_ptr<UIPreferencesManager> uiPreferencesManager,
    ResourceLoader & resourceLoader)
    : mShowingMode(ShowingMode::NotShowing)
    , mLeaveWindowTimer()
    //
    , mElementMap()
    , mKeyboardShortcutToElementId()
    , mGameController(std::move(gameController))
    , mSoundController(std::move(soundController))
    , mUiPreferencesManager(std::move(uiPreferencesManager))
    , mParentLayoutWindow(parentLayoutWindow)
    , mParentLayoutSizer(parentLayoutSizer)
    , mMinBitmapSize(std::numeric_limits<int>::max(), std::numeric_limits<int>::max())
{
    wxPanel::Create(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_SIMPLE);

    // Set background bitmap
    wxBitmap backgroundBitmap;
    backgroundBitmap.LoadFile(resourceLoader.GetIconFilepath("switchboard_background").string(), wxBITMAP_TYPE_PNG);
    SetBackgroundBitmap(backgroundBitmap);

    // Load cursor
    auto upCursor = WxHelpers::MakeCursor(
        resourceLoader.GetCursorFilepath("switch_cursor_up"),
        8,
        9);

    // Set cursor
    SetCursor(*upCursor);


    //
    // Load bitmaps
    //

    mAutomaticSwitchOnEnabledBitmap.LoadFile(resourceLoader.GetIconFilepath("automatic_switch_on_enabled").string(), wxBITMAP_TYPE_PNG);
    mAutomaticSwitchOffEnabledBitmap.LoadFile(resourceLoader.GetIconFilepath("automatic_switch_off_enabled").string(), wxBITMAP_TYPE_PNG);
    mAutomaticSwitchOnDisabledBitmap.LoadFile(resourceLoader.GetIconFilepath("automatic_switch_on_disabled").string(), wxBITMAP_TYPE_PNG);
    mAutomaticSwitchOffDisabledBitmap.LoadFile(resourceLoader.GetIconFilepath("automatic_switch_off_disabled").string(), wxBITMAP_TYPE_PNG);
    mMinBitmapSize.DecTo(mAutomaticSwitchOnEnabledBitmap.GetSize());

    mInteractivePushSwitchOnEnabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_push_switch_on_enabled").string(), wxBITMAP_TYPE_PNG);
    mInteractivePushSwitchOffEnabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_push_switch_off_enabled").string(), wxBITMAP_TYPE_PNG);
    mInteractivePushSwitchOnDisabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_push_switch_on_disabled").string(), wxBITMAP_TYPE_PNG);
    mInteractivePushSwitchOffDisabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_push_switch_off_disabled").string(), wxBITMAP_TYPE_PNG);
    mMinBitmapSize.DecTo(mInteractivePushSwitchOnEnabledBitmap.GetSize());

    mInteractiveToggleSwitchOnEnabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_toggle_switch_on_enabled").string(), wxBITMAP_TYPE_PNG);
    mInteractiveToggleSwitchOffEnabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_toggle_switch_off_enabled").string(), wxBITMAP_TYPE_PNG);
    mInteractiveToggleSwitchOnDisabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_toggle_switch_on_disabled").string(), wxBITMAP_TYPE_PNG);
    mInteractiveToggleSwitchOffDisabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_toggle_switch_off_disabled").string(), wxBITMAP_TYPE_PNG);
    mMinBitmapSize.DecTo(mInteractiveToggleSwitchOnEnabledBitmap.GetSize());

    mPowerMonitorOnBitmap.LoadFile(resourceLoader.GetIconFilepath("power_monitor_on").string(), wxBITMAP_TYPE_PNG);
    mPowerMonitorOffBitmap.LoadFile(resourceLoader.GetIconFilepath("power_monitor_off").string(), wxBITMAP_TYPE_PNG);
    mMinBitmapSize.DecTo(mPowerMonitorOnBitmap.GetSize());

    wxBitmap dockCheckboxCheckedBitmap(resourceLoader.GetIconFilepath("electrical_panel_dock_pin_down").string(), wxBITMAP_TYPE_PNG);
    wxBitmap dockCheckboxUncheckedBitmap(resourceLoader.GetIconFilepath("electrical_panel_dock_pin_up").string(), wxBITMAP_TYPE_PNG);

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
    // Setup enter/leave mouse events
    //

    Bind(wxEVT_ENTER_WINDOW, &SwitchboardPanel::OnEnterWindow, this);

    mLeaveWindowTimer = std::make_unique<wxTimer>(this, wxID_ANY);
    Connect(mLeaveWindowTimer->GetId(), wxEVT_TIMER, (wxObjectEventFunction)&SwitchboardPanel::OnLeaveWindowTimer);
}

SwitchboardPanel::~SwitchboardPanel()
{
}

void SwitchboardPanel::HideFully()
{
    ShowDockCheckbox(false);
    InstallMouseTracking(false);

    // Hide hint panel
    mMainVSizer2->Hide(mHintPanel);

    // Hide switch panel
    mMainVSizer2->Hide(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::NotShowing;

    // Re-layout from parent
    LayoutParent();
}

void SwitchboardPanel::ShowPartially()
{
    if (mShowingMode == ShowingMode::NotShowing)
    {
        InstallMouseTracking(true);
    }
    else if (mShowingMode == ShowingMode::ShowingFullyFloating)
    {
        ShowDockCheckbox(false);
    }

    // Show hint panel
    mMainVSizer2->Show(mHintPanel);

    // Hide switch panel
    mMainVSizer2->Hide(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::ShowingHint;

    // Re-layout from parent
    LayoutParent();
}

void SwitchboardPanel::ShowFullyFloating()
{
    if (mShowingMode == ShowingMode::ShowingHint)
    {
        mDockCheckbox->SetChecked(false);
        ShowDockCheckbox(true);
    }
    else if (mShowingMode == ShowingMode::ShowingFullyDocked)
    {
        InstallMouseTracking(true);
    }

    // Show hint panel
    mMainVSizer2->Show(mHintPanel);

    // Show switch panel
    mMainVSizer2->Show(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::ShowingFullyFloating;

    // Re-layout from parent
    LayoutParent();
}

void SwitchboardPanel::ShowFullyDocked()
{
    if (mShowingMode == ShowingMode::ShowingFullyFloating)
    {
        InstallMouseTracking(false);
    }
    else if (mShowingMode == ShowingMode::NotShowing)
    {
        mDockCheckbox->SetChecked(true);
        ShowDockCheckbox(true);
    }

    // Show hint panel
    mMainVSizer2->Show(mHintPanel);

    // Show switch panel
    mMainVSizer2->Show(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::ShowingFullyDocked;

    // Re-layout from parent
    LayoutParent();
}

bool SwitchboardPanel::OnKeyboardShortcut(
    int keyCode,
    int keyModifier)
{
    //
    // Translate key into index
    //

    int keyIndex;
    if (keyCode >= '1' && keyCode <= '9')
        keyIndex = keyCode - '1';
    else if (keyCode == '0')
        keyIndex = 9;
    else
        return false;

    if (keyModifier == wxMOD_CONTROL)
    {
        // 0-9
    }
    else if (keyModifier == wxMOD_ALT)
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
        auto const elementId = mKeyboardShortcutToElementId[keyIndex];
        auto & elementInfo = mElementMap.at(elementId);
        assert(elementInfo.IsInteractive);

        if (elementInfo.IsEnabled)
        {
            elementInfo.Control->ToggleState();
            mGameController->SetSwitchState(elementId, elementInfo.Control->GetState());
            return true;
        }
    }

    // Ignore
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////

void SwitchboardPanel::OnGameReset()
{
    // Hide
    HideFully();

    // Clear keyboard shortcuts map
    mKeyboardShortcutToElementId.clear();
}

void SwitchboardPanel::OnElectricalElementAnnouncementsBegin()
{
    // Stop refreshing - we'll resume when announcements are over
    Freeze();

    // Reset all switch controls
    mSwitchPanel->Destroy();
    mSwitchPanel = nullptr;
    mSwitchPanelSizer = nullptr;
    MakeSwitchPanel();

    // Clear map
    mElementMap.clear();

    // Clear keyboard shortcuts map
    mKeyboardShortcutToElementId.clear();
}

void SwitchboardPanel::OnSwitchCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex instanceIndex,
    SwitchType type,
    ElectricalState state,
    std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata)
{
    LogMessage("SwitchboardPanel::OnSwitchCreated: ", int(instanceIndex), " state=", static_cast<bool>(state));

    //
    // Make label, if needed
    //

    std::string label;
    if (!!panelElementMetadata)
    {
        label = panelElementMetadata->Label;
    }
    else
    {
        // Make label
        std::stringstream ss;
        ss << "Switch " << " #" << static_cast<int>(instanceIndex);
        label = ss.str();
    }

    //
    // Make control
    //

    ElectricalElementControl * ctrl;
    bool isInteractive;

    switch (type)
    {
        case SwitchType::InteractivePushSwitch:
        {
            ctrl = new InteractivePushSwitchElectricalElementControl(
                mSwitchPanel,
                mInteractivePushSwitchOnEnabledBitmap,
                mInteractivePushSwitchOffEnabledBitmap,
                mInteractivePushSwitchOnDisabledBitmap,
                mInteractivePushSwitchOffDisabledBitmap,
                label,
                [this, electricalElementId](ElectricalState newState)
                {
                    this->mGameController->SetSwitchState(electricalElementId, newState);
                },
                state);

            isInteractive = true;

            break;
        }

        case SwitchType::InteractiveToggleSwitch:
        {
            ctrl = new InteractiveToggleSwitchElectricalElementControl(
                mSwitchPanel,
                mInteractiveToggleSwitchOnEnabledBitmap,
                mInteractiveToggleSwitchOffEnabledBitmap,
                mInteractiveToggleSwitchOnDisabledBitmap,
                mInteractiveToggleSwitchOffDisabledBitmap,
                label,
                [this, electricalElementId](ElectricalState newState)
                {
                    this->mGameController->SetSwitchState(electricalElementId, newState);
                },
                state);

            isInteractive = true;

            break;
        }

        case SwitchType::AutomaticSwitch:
        {
            ctrl = new AutomaticSwitchElectricalElementControl(
                mSwitchPanel,
                mAutomaticSwitchOnEnabledBitmap,
                mAutomaticSwitchOffEnabledBitmap,
                mAutomaticSwitchOnDisabledBitmap,
                mAutomaticSwitchOffDisabledBitmap,
                label,
                state);

            isInteractive = false;

            break;
        }

        default:
        {
            assert(false);
            return;
        }
    }

    //
    // Add switch to map
    //

    assert(mElementMap.find(electricalElementId) == mElementMap.end());
    mElementMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(electricalElementId),
        std::forward_as_tuple(ctrl, panelElementMetadata, isInteractive));
}

void SwitchboardPanel::OnPowerProbeCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex instanceIndex,
    PowerProbeType type,
    ElectricalState state,
    std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata)
{
    LogMessage("SwitchboardPanel::OnPowerProbeCreated: ", int(instanceIndex), " state=", static_cast<bool>(state));

    //
    // Create control
    //

    std::string label;
    ElectricalElementControl * ctrl;

    if (!!panelElementMetadata)
    {
        label = panelElementMetadata->Label;
    }

    switch (type)
    {
        case PowerProbeType::Engine:
        {
            // TODO: use new gauge control w/RPM bitmaps
            ctrl = new PowerMonitorElectricalElementControl(
                mSwitchPanel,
                mPowerMonitorOnBitmap,
                mPowerMonitorOffBitmap,
                label,
                state);

            if (!panelElementMetadata)
            {
                // Make label
                std::stringstream ss;
                ss << "Engine #" << static_cast<int>(instanceIndex);
                label = ss.str();
            }

            break;
        }

        case PowerProbeType::Generator:
        {
            // TODO: use new gauge control w/Voltage bitmaps
            ctrl = new PowerMonitorElectricalElementControl(
                mSwitchPanel,
                mPowerMonitorOnBitmap,
                mPowerMonitorOffBitmap,
                label,
                state);

            if (!panelElementMetadata)
            {
                // Make label
                std::stringstream ss;
                ss << "Generator #" << static_cast<int>(instanceIndex);
                label = ss.str();
            }

            break;
        }

        case PowerProbeType::PowerMonitor:
        {
            ctrl = new PowerMonitorElectricalElementControl(
                mSwitchPanel,
                mPowerMonitorOnBitmap,
                mPowerMonitorOffBitmap,
                label,
                state);

            if (!panelElementMetadata)
            {
                // Make label
                std::stringstream ss;
                ss << "Monitor #" << static_cast<int>(instanceIndex);
                label = ss.str();
            }

            break;
        }

        default:
        {
            assert(false);
            return;
        }
    }

    assert(ctrl != nullptr);

    //
    // Add monitor to map
    //

    assert(mElementMap.find(electricalElementId) == mElementMap.end());
    mElementMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(electricalElementId),
        std::forward_as_tuple(ctrl, panelElementMetadata, false));
}

void SwitchboardPanel::OnElectricalElementAnnouncementsEnd()
{
    //
    // Layout and assign keys
    //

    // Prepare elements for layout helper
    std::vector<LayoutHelper::LayoutElement<ElectricalElementId>> layoutElements;
    for (auto const it : mElementMap)
    {
        if (!!(it.second.PanelElementMetadata))
            layoutElements.emplace_back(
                it.first,
                std::make_pair(it.second.PanelElementMetadata->X, it.second.PanelElementMetadata->Y));
        else
            layoutElements.emplace_back(
                it.first,
                std::nullopt);
    }

    LayoutHelper::Layout<ElectricalElementId>(
        layoutElements,
        MaxElementsPerRow,
        [this](int width, int height)
        {
            mSwitchPanelSizer->SetCols(width);
            mSwitchPanelSizer->SetRows(height);
        },
        [this](std::optional<ElectricalElementId> element, int x, int y)
        {
            if (!!element)
            {
                // Get this element
                auto it = mElementMap.find(*element);
                assert(it != mElementMap.end());

                // Add control to sizer
                mSwitchPanelSizer->Add(
                    it->second.Control,
                    wxGBPosition(y, x + (mSwitchPanelSizer->GetCols() / 2)),
                    wxGBSpan(1, 1),
                    wxTOP | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL | wxALIGN_BOTTOM,
                    8);

                // If interactive, make keyboard shortcut
                if (it->second.IsInteractive
                    && mKeyboardShortcutToElementId.size() < MaxKeyboardShortcuts)
                {
                    int keyIndex = static_cast<int>(mKeyboardShortcutToElementId.size());

                    // Store key mapping
                    mKeyboardShortcutToElementId.emplace_back(*element);

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
                    it->second.Control->SetKeyboardShortcutLabel(ss.str());
                }
            }
        });

    // Ask sizer to resize panel accordingly
    mSwitchPanelSizer->SetSizeHints(mSwitchPanel);

    // Resume refresh
    Thaw();

    // Re-layout from parent
    LayoutParent();
}

void SwitchboardPanel::OnSwitchEnabled(
    ElectricalElementId electricalElementId,
    bool isEnabled)
{
    // Enable/disable control
    auto it = mElementMap.find(electricalElementId);
    assert(it != mElementMap.end());
    it->second.Control->SetEnabled(isEnabled);

    // Remember enable state
    it->second.IsEnabled = isEnabled;
}

void SwitchboardPanel::OnSwitchToggled(
    ElectricalElementId electricalElementId,
    ElectricalState newState)
{
    // Toggle control
    auto it = mElementMap.find(electricalElementId);
    assert(it != mElementMap.end());
    it->second.Control->SetState(newState);
}

void SwitchboardPanel::OnPowerProbeToggled(
    ElectricalElementId electricalElementId,
    ElectricalState newState)
{
    // Toggle power monitor control
    auto it = mElementMap.find(electricalElementId);
    assert(it != mElementMap.end());
    it->second.Control->SetState(newState);
}

///////////////////////////////////////////////////////////////////

void SwitchboardPanel::MakeSwitchPanel()
{
    // Create grid sizer for switch panel
    mSwitchPanelSizer = new wxGridBagSizer(0, 15);
    mSwitchPanelSizer->SetEmptyCellSize(mMinBitmapSize);

    // Create (scrollable) panel for switches
    mSwitchPanel = new wxScrolled<wxPanel>(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL);
    mSwitchPanel->SetScrollRate(5, 0);
    mSwitchPanel->FitInside();
    mSwitchPanel->SetSizerAndFit(mSwitchPanelSizer);

    // Add switch panel to v-sizer
    assert(mMainVSizer2->GetItemCount() == 1);
    mMainVSizer2->Add(mSwitchPanel, 0, wxALIGN_CENTER_HORIZONTAL); // We want it as wide and as tall as it is
}

void SwitchboardPanel::ShowDockCheckbox(bool doShow)
{
    assert(!!mMainHSizer1);
    assert(mMainHSizer1->GetItemCount() == 3);
    mMainHSizer1->Show(size_t(0), doShow);
    mMainHSizer1->Show(size_t(2), doShow);
}

void SwitchboardPanel::InstallMouseTracking(bool isActive)
{
    LogMessage("SwitchboardPanel::InstallMouseTracking: ", isActive);

    assert(!!mLeaveWindowTimer);

    if (isActive)
    {
        mLeaveWindowTimer->Start(500, false);
    }
    else
    {
        mLeaveWindowTimer->Stop();
    }
}

void SwitchboardPanel::LayoutParent()
{
    mParentLayoutWindow->Layout();
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
    if (event.IsChecked() && mShowingMode == ShowingMode::ShowingFullyFloating)
    {
        //
        // Dock
        //

        ShowFullyDocked();

        mSoundController->PlayElectricalPanelDockSound(false);
    }
    else if (!event.IsChecked() && mShowingMode == ShowingMode::ShowingFullyDocked)
    {
        //
        // Undock
        //

        ShowFullyFloating();

        mSoundController->PlayElectricalPanelDockSound(true);
    }
}

void SwitchboardPanel::OnEnterWindow(wxMouseEvent & /*event*/)
{
    if (mShowingMode == ShowingMode::ShowingHint)
    {
        ShowFullyFloating();
    }
}

void SwitchboardPanel::OnLeaveWindow()
{
    if (mShowingMode == ShowingMode::ShowingFullyFloating)
    {
        ShowPartially();
    }
}