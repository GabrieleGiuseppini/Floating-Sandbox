/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-12-27
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SwitchboardPanel.h"

#include "WxHelpers.h"

#include <wx/cursor.h>

#include <cassert>
#include <utility>

std::unique_ptr<SwitchboardPanel> SwitchboardPanel::Create(
    wxWindow * parent,
    wxWindow * parentLayoutWindow,
    wxSizer * parentLayoutSizer,
    std::shared_ptr<IGameController> gameController,
    ResourceLoader & resourceLoader)
{
    return std::unique_ptr<SwitchboardPanel>(
        new SwitchboardPanel(
            parent,
            parentLayoutWindow,
            parentLayoutSizer,
            gameController,
            resourceLoader));
}

SwitchboardPanel::SwitchboardPanel(
    wxWindow * parent,
    wxWindow * parentLayoutWindow,
    wxSizer * parentLayoutSizer,
    std::shared_ptr<IGameController> gameController,
    ResourceLoader & resourceLoader)
    : mShowingMode(ShowingMode::NotShowing)
    , mLeaveWindowTimer()
    //
    , mSwitchMap()
    , mPowerMonitorMap()
    , mGameController(gameController)
    , mParentLayoutWindow(parentLayoutWindow)
    , mParentLayoutSizer(parentLayoutSizer)
{
    wxPanel::Create(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_SIMPLE);

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

    mInteractivePushSwitchOnEnabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_push_switch_on_enabled").string(), wxBITMAP_TYPE_PNG);
    mInteractivePushSwitchOffEnabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_push_switch_off_enabled").string(), wxBITMAP_TYPE_PNG);
    mInteractivePushSwitchOnDisabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_push_switch_on_disabled").string(), wxBITMAP_TYPE_PNG);
    mInteractivePushSwitchOffDisabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_push_switch_off_disabled").string(), wxBITMAP_TYPE_PNG);

    mInteractiveToggleSwitchOnEnabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_toggle_switch_on_enabled").string(), wxBITMAP_TYPE_PNG);
    mInteractiveToggleSwitchOffEnabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_toggle_switch_off_enabled").string(), wxBITMAP_TYPE_PNG);
    mInteractiveToggleSwitchOnDisabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_toggle_switch_on_disabled").string(), wxBITMAP_TYPE_PNG);
    mInteractiveToggleSwitchOffDisabledBitmap.LoadFile(resourceLoader.GetIconFilepath("interactive_toggle_switch_off_disabled").string(), wxBITMAP_TYPE_PNG);

    mPowerMonitorOnBitmap.LoadFile(resourceLoader.GetIconFilepath("power_monitor_on").string(), wxBITMAP_TYPE_PNG);
    mPowerMonitorOffBitmap.LoadFile(resourceLoader.GetIconFilepath("power_monitor_off").string(), wxBITMAP_TYPE_PNG);

    //
    // Setup panel
    //

    mMainVSizer = new wxBoxSizer(wxVERTICAL);

    // Hint panel
    mHintPanel = new wxPanel(this);
    // TODOTEST
    //mHintPanel->SetBackgroundColour(wxColour(128, 128, 128)); // Grey
    mHintPanel->Bind(wxEVT_ENTER_WINDOW, &SwitchboardPanel::OnEnterWindow, this);
    {
        wxBitmap dockCheckboxCheckedBitmap(resourceLoader.GetIconFilepath("docked_icon").string(), wxBITMAP_TYPE_PNG);
        wxBitmap dockCheckboxUncheckedBitmap(resourceLoader.GetIconFilepath("undocked_icon").string(), wxBITMAP_TYPE_PNG);

        wxPanel * fillerPanel = new wxPanel(mHintPanel, wxID_ANY, wxDefaultPosition, dockCheckboxCheckedBitmap.GetSize());

        wxStaticText * hintStaticText = new wxStaticText(mHintPanel, wxID_ANY, "Switches", wxDefaultPosition, wxDefaultSize, 0);
        hintStaticText->Bind(wxEVT_ENTER_WINDOW, &SwitchboardPanel::OnEnterWindow, this);

        mDockCheckbox = new BitmappedCheckbox(mHintPanel, wxID_ANY, dockCheckboxUncheckedBitmap, dockCheckboxCheckedBitmap, "Docks/Undocks the Switchboard.");
        mDockCheckbox->Bind(wxEVT_CHECKBOX, &SwitchboardPanel::OnDockCheckbox, this);

        mHintPanelSizer = new wxFlexGridSizer(3);
        mHintPanelSizer->AddGrowableCol(1, 1);
        mHintPanelSizer->Add(fillerPanel, 0, wxALIGN_CENTER_HORIZONTAL);
        mHintPanelSizer->Add(hintStaticText, 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL);
        mHintPanelSizer->Add(mDockCheckbox, 0, wxALIGN_CENTER_HORIZONTAL);

        // Hide L and R squares for now
        mHintPanelSizer->Hide(size_t(0));
        mHintPanelSizer->Hide(size_t(2));

        mHintPanel->SetSizer(mHintPanelSizer);
    }
    mMainVSizer->Add(mHintPanel, 0, wxEXPAND); // We want it as large as possible, but as tall as it is
    mMainVSizer->Hide(mHintPanel); // Hide it

    // Switch panel
    MakeSwitchPanel();
    mMainVSizer->Hide(mSwitchPanel); // Hide it

    SetSizer(mMainVSizer);


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
    LogMessage("TODOTEST:SwitchboardPanel::HideFully()");

    ShowDockCheckbox(false);
    InstallMouseTracking(false);

    // Hide hint panel
    mMainVSizer->Hide(mHintPanel);

    // Hide switch panel
    mMainVSizer->Hide(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::NotShowing;

    // Re-layout from parent
    LayoutParent();
}

void SwitchboardPanel::ShowPartially()
{
    LogMessage("TODOTEST:SwitchboardPanel::ShowPartially()");

    if (mShowingMode == ShowingMode::NotShowing)
    {
        InstallMouseTracking(true);
    }
    else if (mShowingMode == ShowingMode::ShowingFullyFloating)
    {
        ShowDockCheckbox(false);
    }

    // Show hint panel
    mMainVSizer->Show(mHintPanel);

    // Hide switch panel
    mMainVSizer->Hide(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::ShowingHint;

    // Re-layout from parent
    LayoutParent();
}

void SwitchboardPanel::ShowFullyFloating()
{
    LogMessage("TODOTEST:SwitchboardPanel::ShowFullyFloating()");

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
    mMainVSizer->Show(mHintPanel);

    // Show switch panel
    mMainVSizer->Show(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::ShowingFullyFloating;

    // Re-layout from parent
    LayoutParent();
}

void SwitchboardPanel::ShowFullyDocked()
{
    LogMessage("TODOTEST:SwitchboardPanel::ShowFullyDocked()");

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
    mMainVSizer->Show(mHintPanel);

    // Show switch panel
    mMainVSizer->Show(mSwitchPanel);

    // Transition state
    mShowingMode = ShowingMode::ShowingFullyDocked;

    // Re-layout from parent
    LayoutParent();
}

///////////////////////////////////////////////////////////////////////////////////////

void SwitchboardPanel::OnGameReset()
{
    LogMessage("TODOTEST:SwitchboardPanel::OnGameReset()");

    ShowDockCheckbox(false);
    InstallMouseTracking(false);

    // Reset all switch controls
    mSwitchPanel->Destroy();
    mSwitchPanel = nullptr;
    mSwitchPanelSizer = nullptr;
    MakeSwitchPanel();

    // Hide
    HideFully();

    // Clear maps
    mSwitchMap.clear();
    mPowerMonitorMap.clear();
}

void SwitchboardPanel::OnSwitchCreated(
    SwitchId switchId,
    std::string const & name,
    SwitchType type,
    ElectricalState state)
{
    LogMessage("TODOTEST: SwitchboardPanel::OnSwitchCreated: ", name, " T=", int(type));

    // TODO: handle overflow, add row eventually

    //
    // Create control
    //

    ElectricalElementControl * ctrl;
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
                name,
                [this, switchId](ElectricalState newState)
                {
                    this->mGameController->SetSwitchState(switchId, newState);
                },
                state);

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
                name,
                [this, switchId](ElectricalState newState)
                {
                    this->mGameController->SetSwitchState(switchId, newState);
                },
                state);

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
                name,
                state);

            break;
        }

        default:
        {
            assert(false);
            return;
        }
    }

    LogMessage("TODOTEST:SwitchboardPanel::OnSwitchCreated: BEFORE: SwitchPanelSize=",
        mSwitchPanel->GetSize().GetX(), "x", mSwitchPanel->GetSize().GetY(),
        " ctrl Size=", ctrl->GetSize().GetX(), "x", ctrl->GetSize().GetY());

    // Add to sizer
    mSwitchPanelSizer->Add(
        ctrl,
        0,
        wxTOP | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL,
        8);

    // Ask sizer to resize panel accordingly
    mSwitchPanelSizer->SetSizeHints(mSwitchPanel);

    mMainVSizer->Layout();

    LayoutParent();

    LogMessage("TODOTEST:SwitchboardPanel::OnSwitchCreated: AFTER: SwitchPanelSize=",
        mSwitchPanel->GetSize().GetX(), "x", mSwitchPanel->GetSize().GetY(),
        " visible=", mSwitchPanel->IsShown());


    //
    // Add switch to map
    //

    assert(mSwitchMap.find(switchId) == mSwitchMap.end());
    mSwitchMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(switchId),
        std::forward_as_tuple(ctrl));
}

void SwitchboardPanel::OnSwitchEnabled(
    SwitchId switchId,
    bool isEnabled)
{
    // Enable/disable switch control
    auto it = mSwitchMap.find(switchId);
    assert(it != mSwitchMap.end());
    it->second.Control->SetEnabled(isEnabled);

    // Remember enable state
    it->second.IsEnabled = isEnabled;
}

void SwitchboardPanel::OnSwitchToggled(
    SwitchId switchId,
    ElectricalState newState)
{
    // Toggle switch control
    auto it = mSwitchMap.find(switchId);
    assert(it != mSwitchMap.end());
    it->second.Control->SetState(newState);
}

void SwitchboardPanel::OnPowerMonitorCreated(
    PowerMonitorId powerMonitorId,
    std::string const & name,
    ElectricalState state)
{
    LogMessage("TODOTEST: SwitchboardPanel::OnPowerMonitorCreated: ", name);

    // TODO: handle overflow, add row eventually

    //
    // Create control
    //

    ElectricalElementControl * ctrl = new PowerMonitorElectricalElementControl(
        mSwitchPanel,
        mPowerMonitorOnBitmap,
        mPowerMonitorOffBitmap,
        name,
        state);

    // Add to sizer
    mSwitchPanelSizer->Add(
        ctrl,
        0,
        wxTOP | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL,
        8);

    // Ask sizer to resize panel accordingly
    mSwitchPanelSizer->SetSizeHints(mSwitchPanel);

    mMainVSizer->Layout();

    LayoutParent();


    //
    // Add monitor to map
    //

    assert(mPowerMonitorMap.find(powerMonitorId) == mPowerMonitorMap.end());
    mPowerMonitorMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(powerMonitorId),
        std::forward_as_tuple(ctrl));
}

void SwitchboardPanel::OnPowerMonitorToggled(
    PowerMonitorId powerMonitorId,
    ElectricalState newState)
{
    // Toggle power monitor control
    auto it = mPowerMonitorMap.find(powerMonitorId);
    assert(it != mPowerMonitorMap.end());
    it->second.Control->SetState(newState);
}

///////////////////////////////////////////////////////////////////

void SwitchboardPanel::MakeSwitchPanel()
{
    // Create grid sizer for switch panel
    mSwitchPanelSizer = new wxFlexGridSizer(1, 0, 0, 15);
    mSwitchPanelSizer->SetFlexibleDirection(wxHORIZONTAL);

    // Create panel for switches
    mSwitchPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    // TODOTEST
    //mSwitchPanel->SetBackgroundColour(wxColor(200, 200, 200));
    mSwitchPanel->SetSizerAndFit(mSwitchPanelSizer);

    // Add switch panel to v-sizer
    mMainVSizer->Add(mSwitchPanel, 0, wxALIGN_CENTER_HORIZONTAL); // We want it as wide and as tall as it is
}

void SwitchboardPanel::ShowDockCheckbox(bool doShow)
{
    assert(!!mHintPanelSizer);

    mHintPanelSizer->Show(size_t(0), doShow);
    mHintPanelSizer->Show(size_t(2), doShow);

    mHintPanelSizer->Layout();
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
        ShowFullyDocked();
    }
    else if (!event.IsChecked() && mShowingMode == ShowingMode::ShowingFullyDocked)
    {
        ShowFullyFloating();
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