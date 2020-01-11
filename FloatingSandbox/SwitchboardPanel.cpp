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
    , mElementMap()
    , mGameController(gameController)
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

    //
    // Setup panel
    //

    mMainVSizer = new wxBoxSizer(wxVERTICAL);

    // Hint panel
    mHintPanel = new wxPanel(this);
    mHintPanel->Bind(wxEVT_ENTER_WINDOW, &SwitchboardPanel::OnEnterWindow, this);
    {
        wxBitmap dockCheckboxCheckedBitmap(resourceLoader.GetIconFilepath("docked_icon").string(), wxBITMAP_TYPE_PNG);
        wxBitmap dockCheckboxUncheckedBitmap(resourceLoader.GetIconFilepath("undocked_icon").string(), wxBITMAP_TYPE_PNG);

        wxPanel * fillerPanel = new wxPanel(mHintPanel, wxID_ANY, wxDefaultPosition, dockCheckboxCheckedBitmap.GetSize());

        wxStaticText * hintStaticText = new wxStaticText(mHintPanel, wxID_ANY, "Electrical Panel", wxDefaultPosition, wxDefaultSize, 0);
        hintStaticText->Bind(wxEVT_ENTER_WINDOW, &SwitchboardPanel::OnEnterWindow, this);

        mDockCheckbox = new BitmappedCheckbox(mHintPanel, wxID_ANY, dockCheckboxUncheckedBitmap, dockCheckboxCheckedBitmap, "Docks/Undocks the electrical panel.");
        mDockCheckbox->Bind(wxEVT_CHECKBOX, &SwitchboardPanel::OnDockCheckbox, this);

        mHintPanelSizer = new wxBoxSizer(wxHORIZONTAL);
        mHintPanelSizer->Add(fillerPanel, 0, wxALIGN_CENTER_HORIZONTAL);
        mHintPanelSizer->Add(hintStaticText, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_HORIZONTAL, 10);
        mHintPanelSizer->Add(mDockCheckbox, 0, wxALIGN_CENTER_HORIZONTAL | wxRESERVE_SPACE_EVEN_IF_HIDDEN);

        // Hide docking icon for now
        mDockCheckbox->Hide();

        mHintPanel->SetSizer(mHintPanelSizer);
    }
    mMainVSizer->Add(mHintPanel, 0, wxALIGN_CENTER_HORIZONTAL); // We want it as tall and as large as it is
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

    // Clear map
    mElementMap.clear();
}

void SwitchboardPanel::OnElectricalElementAnnouncementsBegin()
{
    LogMessage("TODOTEST:SwitchboardPanel::OnElectricalElementAnnouncementsBegin()");

    // Clear maps
    mElementMap.clear();
}

void SwitchboardPanel::OnSwitchCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex instanceIndex,
    SwitchType type,
    ElectricalState state,
    std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata)
{
    LogMessage("TODOTEST: SwitchboardPanel::OnSwitchCreated: ", instanceIndex);

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
        std::forward_as_tuple(ctrl, panelElementMetadata));
}

void SwitchboardPanel::OnPowerProbeCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex instanceIndex,
    PowerProbeType type,
    ElectricalState state,
    std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata)
{
    LogMessage("TODOTEST: SwitchboardPanel::OnPowerProbeCreated: ", instanceIndex);

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
            // TODO: use new gauge control
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
            // TODO: use new gauge control
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
    }

    assert(ctrl != nullptr);

    //
    // Add monitor to map
    //

    assert(mElementMap.find(electricalElementId) == mElementMap.end());
    mElementMap.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(electricalElementId),
        std::forward_as_tuple(ctrl, panelElementMetadata));
}

void SwitchboardPanel::OnElectricalElementAnnouncementsEnd()
{
    //
    // Layout
    //

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
        11, // Max these elements per row
        [this](int width, int height)
        {
            mSwitchPanelSizer->SetCols(width);
            mSwitchPanelSizer->SetRows(height);
        },
        [this](std::optional<ElectricalElementId> element, int x, int y)
        {
            if (!!element)
            {
                // Add control to sizer

                auto it = mElementMap.find(*element);
                assert(it != mElementMap.end());

                mSwitchPanelSizer->Add(
                    it->second.Control,
                    wxGBPosition(y, x + (mSwitchPanelSizer->GetCols() / 2)),
                    wxGBSpan(1, 1),
                    wxTOP | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL,
                    8);
            }
        });

    // Ask sizer to resize panel accordingly
    // TODOTEST: needed?
    mSwitchPanelSizer->SetSizeHints(mSwitchPanel);

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
    mMainVSizer->Add(mSwitchPanel, 0, wxALIGN_CENTER_HORIZONTAL); // We want it as wide and as tall as it is
}

void SwitchboardPanel::ShowDockCheckbox(bool doShow)
{
    assert(!!mHintPanelSizer);

    mDockCheckbox->Show(doShow);

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